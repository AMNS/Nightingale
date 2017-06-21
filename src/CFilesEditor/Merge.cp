/* Merge.c for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* This file frequently uses the following strange-looking construction:
			if (VOICE_MAYBE_USED(clipboard,v) && VOICE_MAYBE_USED(doc,vMap[v]))
				if (ClipVInUse(v) && DocVInUse(doc,vMap[v])) {
			...
			}

The VOICE_MAYBE_USED macro checks the given score's <voiceTab[v].partn> to see
whether the voice is in use anywhere in the score: it's very fast. ClipVInUse and
DocVInUse check only a single system: this is relevant because merging is only allowed
within one system, but they're fairly time-consuming. So doing a rough check first
with VOICE_MAYBE_USED can save a lot of time. */

static PTIME			*pDurArray, *qDurArray;
static LINK				startMeas, endMeas;
static SPACETIMEINFO	*spTimeInfo;
static long 			*stfTimeDiff;

static PTIME			*pClDurArray, *qClDurArray;
static LINK				startClMeas, endClMeas;
static SPACETIMEINFO	*clSpTimeInfo;
static long 			*clStfTimeDiff;
static short			voiceMap[MAXVOICES+1];

static enum {
	MergeCancel=1,
	MergeTrunc,
	MergePin,
	MergeOK
} E_MergeItems;

static enum {
	Mrg_TupletInV = -100,
	Mrg_RhythmConfl,
	Mrg_AllocErr,
	Mrg_OK=0
} E_MergeErrItems;

typedef struct {
	LINK link;
	long lTime;
	DDIST xd;
} MERGEARRAY;

typedef struct {
	LINK 	srcL;					/* Clipboard measure */
	LINK 	dstL;					/* Doc measure */
	long	startTime;				/* Zero for all but first */
	long	measTime;				/* Start time of doc's measure */
	short 	clipEmpty:1,			/* No objs in this clipboard measure */
			clipNoSyncs:1,			/* No Syncs in this clipboard measure */
			docEmpty:1,				/* No objs in this doc measure	*/
			docNoSyncs:1,			/* No Syncs in this doc measure	*/
			docLast:1,				/* Last doc measure; copy any remaining clipboard */
			unused:11;
} MeasInfo;

/* -------------------------------------------------------------------------------------- */
/* Prototypes for internal functions */

static LINK GetSysEndL(LINK pL,Boolean goLeft);
static LINK VSyncSearch(LINK pL, LINK *subL, short v, Boolean goLeft, Boolean needSel);
static LINK VoiceSearch(LINK pL, VInfo *vInfo, short v, Boolean goLeft);
static Boolean ObjInStfRange(LINK pL, short firstStf, short lastStf);
static LINK VoiceRangeSearch(LINK pL, VInfo *vInfo, short v, Boolean goLeft);
static LINK ClipVRangeSearch(LINK pL,short v,short firstStf,short lastStf,Boolean goLeft);
static void GetVoiceStf(Document *doc, VInfo *vInfo, short v);
static void GetClipVStf(short v, short *singleStf, short *firstStf, short *lastStf);
static long GetStartTime(Document *doc,VInfo *vInfo,short v,short vStfDiff);
static short Obj2Stf(LINK link,short v);
static long GetClipEndTime(VInfo *vInfo,MeasInfo *mInfo,short v);
static Boolean CheckStartTime(VInfo *vInfo,MeasInfo *mInfo,short v);
static Boolean ObjFillsV(LINK pL,short v);
static Boolean ClipVInUse(short v);
static Boolean DocVInUse(Document *doc,short v);
static LINK ClipTupletInV(short v);
static Boolean RhythmConflictInV(Document *doc,VInfo *vInfo,short *vMap);
static Boolean CheckMerge(Document *,short stfDiff,short *vMap,VInfo *vInfo,short *mergeErr);

static LINK MCopyClipRange(Document *,LINK,LINK,LINK,COPYMAP *,short,short *);
static void FixMergeLinks(Document *, Document *, LINK, LINK, COPYMAP *, short);

static void MeasGetSysRelLTimes(Document *, MERGEARRAY *, LINK);
static void InitMergeArr(Document *, LINK, LINK, MERGEARRAY *);
static void SetupMergeArr(LINK, LINK, MERGEARRAY **, short *);

static void SetMergedFlags(LINK startL, LINK endL, Boolean value);
static COPYMAP *MergeSetupClip(Document *doc, short *numObjs);
static MeasInfo	*SetupMeasInfo1(Document *doc, short *nClipMeas);
static void SetupMeasInfo2(Document *doc, MeasInfo *measInfo, short nClipMeas);
static MeasInfo *BuildMeasInfo(Document *doc);

static void MDisposeArrays(void);
static void MDisposeClArrays(void);
static void MDisposeAllArrays(void);

static void SortPTimes(PTIME *durArray,short n,short nvoices);

static short CountSyncs(LINK startL,LINK endL);
static LINK InsNewObjInto(LINK newObjL,LINK prevL,LINK insertL);
static PTIME *MPrepareSelRange(Document *doc,LINK measL,short *nInRange);
static PTIME *MPrepareClSelRange(Document *doc,LINK measL,short *nInRange);

static Boolean MComputePlayTimes(Document *doc, short);
static Boolean MComputeClipPlayTimes(Document *doc, short);

static void XLateDurArray(PTIME *durArray,long startTime,short nInMeas);
static void SetSyncSize(Document *doc,LINK pL,short oldRastral,short newRastral);
static void SetNoteSize(Document *doc,LINK aNoteL,short oldRastral,short newRastral);

static LINK MCreateSync(Document *,PTIME *,PTIME **,short *,short *);

static LINK CopyClSubObjs(Document *doc,LINK newObjL,PTIME *pTime);
static LINK CopyClSync(Document *doc,short subCount,PTIME *pTime);
static LINK MCreateClSync(Document *,PTIME *,PTIME **,short *,short *);

static void MergeClSubObjs(Document *doc,LINK newObjL,LINK newSubL,PTIME *pTime,short,short *);
static LINK MMergeSync(Document *,PTIME *,PTIME **,PTIME *,PTIME **,short *,short *,short *,
						short *,short,short *);

static void GetMergeRange(LINK startMeas, LINK endMeas,LINK *prevL,LINK *lastL,Boolean first);
static void ThinNonSyncObjs(Document *doc, LINK prevMeasL, LINK lastL);
static Boolean MRearrangeAll(Document *doc, short, short, LINK, LINK *, LINK *, Boolean, short,
								COPYMAP *, short *, Boolean);

static Boolean Merge1Measure(Document *doc,MeasInfo *measInfo,short clipMeasNum,LINK insertL,
								LINK *prevL,LINK *lastL,short stfDiff,COPYMAP *mergeMap,
								short *vMap,Boolean thin);
static LINK MergeFromClip(Document *doc, LINK insertL, short *vMap, VInfo *vInfo,
								Boolean thin);
static void CleanupMerge(Document *doc, LINK prevMeasL, LINK lastL);

static void SetPastedAsCue(Document *doc, LINK prevMeasL, LINK lastL, short velocity);

/* ------------------------------------------------------------------------ CheckMerge -- */
/* Utilities for CheckMerge. */

static LINK GetSysEndL(LINK pL, Boolean goLeft)
{
	LINK sysL,endSysL;

	sysL = LSSearch(pL,SYSTEMtype,ANYONE,goLeft,false);
	if (!sysL) return NILINK;

	if (goLeft) return sysL;

	endSysL = LinkRSYS(sysL);
	if (endSysL && FirstSysInPage(endSysL))
		endSysL = SysPAGE(endSysL);
		
	return endSysL;
}

/* When called from GetClipVStf, pL is the clipFirstMeas, so calling GetSysEndL from
LeftLINK(pL) will provide correct results. But when called from GetVoiceStf, pL is
the selEndL parameter, so calling GetSysEndL from pL will give incorrect results if
the insertion point is at the end of a system (will give the sysEndL of a following
system, if one exists). Calling GetSysEndL from LeftLINK(pL) is correct in this case.

Cannot pass in the left LINK of pL, because must search from pL itself. */

static LINK VSyncSearch(LINK pL, LINK *subL, short v, Boolean goLeft, Boolean needSel)
{
	LINK qL,aNoteL,aGRNoteL,endSysL;
	
	endSysL = GetSysEndL(LeftLINK(pL),goLeft);

	for (qL=pL; qL && qL!=endSysL; qL=GetNext(qL, goLeft)) {
		if (SyncTYPE(qL)) {
			aNoteL = NoteInVoice(qL,v,needSel);
			if (aNoteL)
				{ *subL = aNoteL; return qL; }
		}
		if (GRSyncTYPE(qL)) {
			aGRNoteL = GRNoteInVoice(qL,v,needSel);
			if (aGRNoteL)
				{ *subL = aGRNoteL; return qL; }
		}
	}

	*subL = NILINK;
	return NILINK;
}

/* Search inSystem in direction goLeft for the next obj in a singleStf voice v. For a
single stf voice, constraining L_Search by setting pbSearch.id to the stf of that
voice will prevent L_Search from returning objects without voices on irrelevant
staves. Ignore measures, which are on all staves, and are transparent for the sake of
allowing multi-measure merges. We may have to extend this test to other or even all
staff-spanning objects (RptEnds, pseudomeasures). */

static LINK VoiceSearch(LINK pL, VInfo *vInfo, short v, Boolean goLeft)
{
	LINK qL;
	SearchParam pbSearch;

	InitSearchParam(&pbSearch);
	pbSearch.id = vInfo[v].firstStf;
	pbSearch.voice = v;
	pbSearch.needSelected = false;
	pbSearch.inSystem = true;
	pbSearch.optimize = true;

	qL = L_Search(pL, ANYTYPE, goLeft, &pbSearch);
	while (qL && MeasureTYPE(qL))
		qL = L_Search(GetNext(qL, goLeft), ANYTYPE, goLeft, &pbSearch);

	return qL;
}

/* Test if pL or one of its subObjs is in range [firstStf,lastStf] */

static Boolean ObjInStfRange(LINK pL, short firstStf, short lastStf)
{
	short i;

	for (i=firstStf; i<=lastStf; i++)
		if (ObjOnStaff(pL,i,false)) return true;

	return false;
}

/* Search inSystem in direction goLeft for the next obj in a !singleStf voice v. For a
single stf voice, constraining L_Search by setting pbSearch.id to the stf of that
voice will prevent L_Search from returning objects without voices on irrelevant
staves. We cannot do this here; must retrieve all objects without v and test if they
are in the range of staves. Ignore measures, which are on all staves, and are
transparent for the sake of allowing multi-measure merges. We may have to extend this
test to other or even all staff-spanning objects (RptEnds, pseudomeasures). */

static LINK VoiceRangeSearch(LINK pL, VInfo *vInfo, short v, Boolean goLeft)
{
	LINK qL;
	SearchParam pbSearch;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = v;
	pbSearch.needSelected = false;
	pbSearch.inSystem = true;
	pbSearch.optimize = true;

	qL = L_Search(pL, ANYTYPE, goLeft, &pbSearch);
	while (qL && (MeasureTYPE(qL) || !ObjInStfRange(qL,vInfo[v].firstStf,vInfo[v].lastStf)))
		qL = L_Search(GetNext(qL, goLeft), ANYTYPE, goLeft, &pbSearch);
	
	return qL;
}

static LINK ClipVRangeSearch(LINK pL, short v, short firstStf, short lastStf, Boolean goLeft)
{
	LINK qL;
	SearchParam pbSearch;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = v;
	pbSearch.needSelected = false;
	pbSearch.inSystem = true;
	pbSearch.optimize = true;

	qL = L_Search(pL, ANYTYPE, goLeft, &pbSearch);
	while (qL && (MeasureTYPE(qL) || !ObjInStfRange(qL,firstStf,lastStf)))
		qL = L_Search(GetNext(qL, goLeft), ANYTYPE, goLeft, &pbSearch);
	
	return qL;
}

/* Merge maps from voice v in the clipboard to voice v+stfDiff in the doc. The
document voice v being filled in here is actually at v+stfDiff in the doc;
vInfo[v+stfDiff] being filled in here holds info regarding singleStf,firstStf and
lastStf for the destination voice in the doc. If it is singleStf, then firstStf =
lastStf and is the staff of the voice.

It's only necessary to check Syncs and GRSyncs to get the stfRange of the voice, since
any other objects are either objs without voices on staves or J_D objs with voices
whose owning objs are Syncs or GRSyncs or objs without voices which will be in the
default voice of their staff. */

static void GetVoiceStf(Document *doc, VInfo *vInfo, short v)
{
	LINK startL,sysL,endSysL,firstL,subL,pL,aNoteL,aGRNoteL;

	InstallDoc(doc);

	startL = doc->selEndL;
	sysL = LSSearch(LeftLINK(startL),SYSTEMtype,ANYONE,GO_LEFT,false);

	endSysL = GetSysEndL(LeftLINK(startL),GO_RIGHT);

	firstL = VSyncSearch(startL,&subL,v,GO_RIGHT,false);
	if (!firstL) {
		vInfo[v].singleStf = true; return;
	}

	if (SyncTYPE(firstL)) {
		vInfo[v].firstStf = NoteSTAFF(subL);
		vInfo[v].lastStf = NoteSTAFF(subL);
	}
	if (GRSyncTYPE(firstL)) {
		vInfo[v].firstStf = GRNoteSTAFF(subL);
		vInfo[v].lastStf = GRNoteSTAFF(subL);
	}
	
	for (pL=firstL; pL && pL!=endSysL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL=NoteInVoice(pL,v,false);
			if (aNoteL) {
				if (vInfo[v].firstStf > NoteSTAFF(aNoteL))
					vInfo[v].firstStf = NoteSTAFF(aNoteL);
				if (vInfo[v].lastStf < NoteSTAFF(aNoteL))
					vInfo[v].lastStf = NoteSTAFF(aNoteL);
			}
		}
		if (GRSyncTYPE(pL)) {
			aGRNoteL=GRNoteInVoice(pL,v,false);
			if (aGRNoteL) {
				if (vInfo[v].firstStf > GRNoteSTAFF(aGRNoteL))
					vInfo[v].firstStf = GRNoteSTAFF(aGRNoteL);
				if (vInfo[v].lastStf < GRNoteSTAFF(aGRNoteL))
					vInfo[v].lastStf = GRNoteSTAFF(aGRNoteL);
			}
		}
	}

	vInfo[v].singleStf = (vInfo[v].firstStf==vInfo[v].lastStf);
}

/* Get information singleStf,firstStf & lastStf for voice v in the clipboard. This is
the src voice which is mapped to v+stfDiff in the doc. */

static void GetClipVStf(short v, short *singleStf, short *firstStf, short *lastStf)
{
	LINK startL,firstL,subL,pL,aNoteL,aGRNoteL;

	startL = LSSearch(clipboard->headL,MEASUREtype,ANYONE,GO_RIGHT,false);
	
	firstL = VSyncSearch(startL,&subL,v,GO_RIGHT,false);
	if (!firstL) {
		*singleStf = true; return;
	}

	if (SyncTYPE(firstL)) {
		*firstStf = NoteSTAFF(subL);
		*lastStf = NoteSTAFF(subL);
	}
	if (GRSyncTYPE(firstL)) {
		*firstStf = GRNoteSTAFF(subL);
		*lastStf = GRNoteSTAFF(subL);
	}
	
	for (pL=firstL; pL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL=NoteInVoice(pL,v,false);
			if (aNoteL) {
				if (*firstStf > NoteSTAFF(aNoteL))
					*firstStf = NoteSTAFF(aNoteL);
				if (*lastStf < NoteSTAFF(aNoteL))
					*lastStf = NoteSTAFF(aNoteL);
			}
		}
		if (GRSyncTYPE(pL)) {
			aGRNoteL=GRNoteInVoice(pL,v,false);
			if (aGRNoteL) {
				if (*firstStf > GRNoteSTAFF(aGRNoteL))
					*firstStf = GRNoteSTAFF(aGRNoteL);
				if (*lastStf < GRNoteSTAFF(aGRNoteL))
					*lastStf = GRNoteSTAFF(aGRNoteL);
			}
		}
	}

	*singleStf = (*firstStf==*lastStf);
}

/* Return the total amount of time after doc->selEndL that voice vStfDiff in Document
doc starts; fills in vInfo[v].startTime with that time.

The vInfo struct handles the mapping by merge from src v to dst v+stfDiff by storing
the startTime of voice v+stfDiff (=vStfDiff) at vInfo[v], since the endTime of
clipboard voice v is compared to this startTime.

Leaves doc installed. */

static long GetStartTime(Document *doc, VInfo *vInfo, short v, short vStfDiff)
{
	LINK startL,startMeasL,lastMeasL,measL,firstInVL,prevInVL;
	long startTime=0L,prevTime=0L;

	InstallDoc(doc);

	vInfo[v].startTime = -1L;
	startL = doc->selEndL;
	startMeasL = LSSearch(startL,MEASUREtype,ANYONE,GO_LEFT,false);

	if (vInfo[vStfDiff].singleStf) {
		firstInVL = VoiceSearch(startL,vInfo,vStfDiff,GO_RIGHT);
	}
	else {
		firstInVL = VoiceRangeSearch(startL, vInfo, vStfDiff, GO_RIGHT);
	}

	if (!firstInVL) return -1L;

	/* All merging is done on a single system; if the first obj in the voice is on a
		following system, return -1 to indicate no further checking is needed.
		
		If the insertion point is at the end of the system, doc->selEndL will be the
		succeeding page or system object; use LeftLINK(startL) to guarantee actually
		testing the system containing the insertion point. This use of LeftLINK
		depends on startL not equalling doc->headL or actually not being before the
		system object of the system to be merged into; this is caught by testing
		beforeFirst in FixEditMenu. */

	if (!SamePage(LeftLINK(startL), firstInVL)) return -1L;

	if (!SameSystem(LeftLINK(startL), firstInVL)) return -1L;

	lastMeasL = LSSearch(firstInVL, MEASUREtype, ANYONE, GO_LEFT, false);

	if (lastMeasL!=startMeasL)
		for (measL=startMeasL; measL!=lastMeasL; measL = LinkRMEAS(measL))
			startTime += GetMeasDur(doc, LinkRMEAS(measL), ANYONE);

	/* If firstInVL is neither J_IT nor J_IP, GetLTime will return the lTime of the
		immediately preceding Sync. We want the J_IT or J_IP object of firstInVL's
		slot, not the Sync immediately preceding that slot.
		
		Note that an analogous search is not required to get the clipEndTime, since
		there we search to the right from clipboard->tailL and we're guaranteed to get
		the J_IT or J_IP object establishing the slot before any J_D obj in the slot. */

	if (J_DTYPE(firstInVL))
		firstInVL = FirstValidxd(firstInVL, GO_RIGHT);
		
	/* If lastMeasL is not equal to startMeasL, there is an empty slot for merging
		spanning multiple measures, and firstInVL will be the first obj in the voice
		in the last measure. If lastMeasL is equal to startMeasL, firstInVL is in the
		firstMeasure and the insertion point may not be at the first object in the
		measure; in this case we need its lTime relative to the object before the
		insertion point. */

	if (lastMeasL!=startMeasL)
		startTime += GetLTime(doc, firstInVL);
	else {
		startTime = GetLTime(doc, firstInVL);

		prevInVL = FirstValidxd(LeftLINK(startL), GO_LEFT);
		prevTime = MeasureTYPE(prevInVL) ? 0L : GetLTime(doc, prevInVL) + 
						GetLDur(doc, prevInVL, Obj2Stf(prevInVL, v));

		startTime -= prevTime;
	}

	vInfo[v].startTime = startTime;
	return startTime;
}

/* Call this to get a staffn to pass to GetLDur. Given an obj with subObj in voice
v, return the staff of that subObj. We have nothing reasonable to return for
PAGEtype through CONNECTtype and MEASUREtype through ENDINGtype; but SYNCs are the
only meaningful objects to pass to GetLDur, so it doesn't make any difference. If we
wish to call this for any other reason, will have to review this. */
 
static short Obj2Stf(LINK link, short v)
{
	LINK aNoteL,aGRNoteL;

	switch (ObjLType(link)) {
		case PAGEtype:
		case SYSTEMtype:
		case STAFFtype:
		case CONNECTtype:
			return 1;

		case SYNCtype:
			aNoteL = FirstSubLINK(link);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteVOICE(aNoteL)==v)
					return NoteSTAFF(aNoteL);
			return v;										/* Have no reasonable default */
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(link);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
				if (GRNoteVOICE(aGRNoteL)==v)
					return GRNoteSTAFF(aGRNoteL);
			return 1;

		case MEASUREtype:
		case PSMEAStype:
		case CLEFtype:
		case TIMESIGtype:		
		case KEYSIGtype:
		case DYNAMtype:
		case RPTENDtype:
		case ENDINGtype:
			return 1;

		case SLURtype:
			return SlurSTAFF(link);
		case GRAPHICtype:
			return GraphicSTAFF(link);
		case TEMPOtype:
			return TempoSTAFF(link);
		case BEAMSETtype:
			return BeamSTAFF(link);
		case TUPLETtype:
			return TupletSTAFF(link);
		case OTTAVAtype:
			return OttavaSTAFF(link);
		case SPACERtype:
			return SpacerSTAFF(link);
		default:
			return 1;
	}
}


/* Get the total amount of time that voice v occupies in Document doc. Leaves
clipboard installed. */

static long GetClipEndTime(VInfo *vInfo, MeasInfo *mInfo, short v)
{
	LINK firstMeasL,lastMeasL,lastInVL,measL;
	long endTime=0L;
	Document *doc=clipboard;
	short singleStf,firstStf,lastStf,i;
	
	InstallDoc(doc);

	firstMeasL = LSSearch(doc->headL,MEASUREtype,ANYONE,GO_RIGHT,false);
	
	/* Get information about the src voice in the clipboard. We no longer care
		whether the dst voice in doc is singleStf, or about its stfRange; here we
		are only concerned with the last obj in voice v in the clipboard, and its
		endTime. Need to get voice info here: whether singleStf or not, and
		stfRange, to handle non-Sync objects properly. */
	
	GetClipVStf(v,&singleStf,&firstStf,&lastStf);
	if (singleStf) {
		lastInVL = VoiceSearch(doc->tailL,vInfo,v,GO_LEFT);
	}
	else {
		lastInVL = ClipVRangeSearch(doc->tailL,v,firstStf,lastStf,GO_LEFT);
	}

	/* If there are no objects at all in voice v, return endTime of 0. */
	if (!lastInVL) return 0L;

	lastMeasL = LSSearch(lastInVL,MEASUREtype,ANYONE,GO_LEFT,false);
	
	for (i=0,measL=firstMeasL; measL && measL!=lastMeasL; measL=LinkRMEAS(measL))
		i++;

	/* Get time of corresponding measure in score. */
	endTime = mInfo[i].measTime;

	endTime += GetLTime(doc,lastInVL) + GetLDur(doc,lastInVL,Obj2Stf(lastInVL,v));
	
	return endTime;
}


/* Check the start time of voice v. Get the total duration of the voice v in the
clipboard, and return true if it is less than startTime. */

static Boolean CheckStartTime(VInfo *vInfo, MeasInfo *mInfo, short v)
{
	long clipEndTime;

	clipEndTime = GetClipEndTime(vInfo,mInfo,v);
	vInfo[v].overlap = (clipEndTime > vInfo[v].startTime);
	return (!vInfo[v].overlap);
}

/* If pL is in the voice v, return true, with the following exceptions ??NOT REALLY: 
if pL is a Measure, keep traversing: can paste into range with Measures.
if pL is a Graphic, ObjHasVoice returns true, but L_Search doesn't check
	its voice field, so treat it like an obj without a voice.
if pL doesn't have a voice, keep traversing: we can't check it adequately.
	e.g. if pL is a dynamic on stf 1, can't get which v (if > 1) on stf
			1 it belongs to.
??Could do more refined checking, as follows: ignore J_D objects, since
	their relObjs should have voices and be checked; consider J_IP objs,
	since if they are on the staff of the voice, they should be considered.
	But to accomplish this checking, we need a mapping from voice to staff,
	which doesn't exist. */

static Boolean ObjFillsV(LINK pL, short v)
{
	if (ObjHasVoice(pL)) return true;
	if (ObjOnStaff(pL,v,false)) {
		if (J_IPTYPE(pL)) return true;
		
		if (J_ITTYPE(pL)) return (!MeasureTYPE(pL));
	}
	return false;
}

/* Determines if voice v is in use in the first system on the clipboard: iff so,
returns true. Intended for use in merging, which as of now is on one system only.
For determination of what is considered to use a voice, cf. ObjFillsV. Leaves
clipboard installed. */

static Boolean ClipVInUse(short v)
{
	LINK measL,pL;
	SearchParam pbSearch;

	InstallDoc(clipboard);

	measL = LSSearch(clipboard->headL,MEASUREtype,ANYONE,GO_RIGHT,false);

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = v;
	pbSearch.needSelected = false;
	pbSearch.inSystem = true;
	pbSearch.optimize = true;

	pL = L_Search(RightLINK(measL), ANYTYPE, GO_RIGHT, &pbSearch);
	while (pL && !ObjFillsV(pL,v))
		pL = L_Search(RightLINK(pL), ANYTYPE, GO_RIGHT, &pbSearch);

	return (pL!=NILINK);
}

/* Determines if voice v is in use in doc in the system containing doc->selEndL: iff
so, returns true. Intended for use in merging, which as of now is on one system only.
For determination of what is considered to use a voice, cf. ObjFillsV. Leaves doc
installed. */

static Boolean DocVInUse(Document *doc, short v)
{
	LINK pL;
	SearchParam pbSearch;

	InstallDoc(doc);

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = v;
	pbSearch.needSelected = false;
	pbSearch.inSystem = true;
	pbSearch.optimize = true;

	pL = L_Search(doc->selEndL, ANYTYPE, GO_RIGHT, &pbSearch);
	while (pL && !ObjFillsV(pL,v))
		pL = L_Search(RightLINK(pL), ANYTYPE, GO_RIGHT, &pbSearch);

	return (pL!=NILINK);
}

/* Returns the first tuplet in v if there is a tuplet in voice v in the clipboard,
else NILINK. Leaves doc installed ??IT DOESN'T LOOK LIKE IT DOES. */

static LINK ClipTupletInV(short v)
{
	LINK tupletL;

	InstallDoc(clipboard);

	tupletL = LVSearch(clipboard->headL,TUPLETtype,v,GO_RIGHT,false);
	return tupletL;
}

/* Returns true if there is a rhythm conflict between clipV and docV, where
conflicting rhythm means: there are Syncs in clip that map to times where
there are not Syncs in score in that voice. Also returns true if it can't
allocate memory it needs. */

static Boolean RhythmConflictInV(Document *doc, VInfo *vInfo, short *vMap)
{
	LINK startL,measL,clMeasL,pL,cL,startMeasL,clFirstMeasL,firstSyncL,measEndL;
	long startTime=0L,clipTime,docTime;
	short nInClipMeas,nInDocMeas,i,j,v;
	SPACETIMEINFO *docSpTimeInfo,*clSpTimeInfo;
	Boolean needCheck,mergeOK=true,syncOK,rhythmConflict,first;

	clSpTimeInfo = AllocSpTimeInfo();
	if (!clSpTimeInfo) return true;
	
	docSpTimeInfo = AllocSpTimeInfo();
	if (!docSpTimeInfo) return true;
	
	InstallDoc(clipboard);
	clFirstMeasL = LSSearch(clipboard->headL,MEASUREtype,ANYONE,GO_RIGHT,false);
	
	InstallDoc(doc);
	startL = doc->selEndL;
	startMeasL = LSSearch(startL,MEASUREtype,ANYONE,GO_LEFT,false);

	/*
	 * Get the first sync at or after the insertion point. It might not be in the
	 * measure, or might not exist. If either case holds, return true to indicate
	 * rhythm conflict.
	 */
	firstSyncL = LSSearch(startL,SYNCtype,ANYONE,GO_RIGHT,false);
	if (!firstSyncL)
		return true; 
	
	measEndL = EndMeasSearch(doc, startMeasL);
	if (IsAfter(measEndL,firstSyncL))
		return true;

	first = true;
	measL = startMeasL;
	clMeasL = clFirstMeasL;
	for ( ; measL && clMeasL; measL=LinkRMEAS(measL),
										clMeasL=DLinkRMEAS(clipboard,clMeasL)) {

		for (v=1; v<=MAXVOICES; v++) {
			if (VOICE_MAYBE_USED(clipboard,v) && VOICE_MAYBE_USED(doc,vMap[v]))
				if (ClipVInUse(v) && DocVInUse(doc,vMap[v])) {
					
					/* Determine if this voice actually needs to be checked. It's in use
						in both the clipboard and the system we're merging into, but if
						there are no syncs in this voice in the measure, then no checking
						needs to be done. */
					
					InstallDoc(doc);
					needCheck = false;
					measEndL = EndMeasSearch(doc, measL);
					pL=first ? firstSyncL : measL;
					for ( ; pL!=measEndL; pL=RightLINK(pL))
						if (SyncTYPE(pL))
							if (SyncInVoice(pL,vMap[v])) {
								needCheck = true; break;
							}
					
					if (needCheck) {
						InstallDoc(clipboard);
						measEndL = EndMeasSearch(clipboard, clMeasL);
						nInClipMeas = GetSpTimeInfo(clipboard,RightLINK(clMeasL),measEndL,
																clSpTimeInfo,false);
				
						InstallDoc(doc);
						measEndL = EndMeasSearch(doc, measL);
						nInDocMeas = GetSpTimeInfo(doc,RightLINK(measL),measEndL,docSpTimeInfo,false);
				
						/* Check syncs in corresponding voices in rhythmic spines. To set
							vBad flags for all voices which are bad, remove the goto done
							statements. */
							
						if (first) {
							for (j=0; j<nInDocMeas; j++) {
								if (docSpTimeInfo[j].link == firstSyncL) {
									startTime = docSpTimeInfo[j].startTime; break;
								}
							}
						}
					
						InstallDoc(clipboard);
						for (i = 0; i<nInClipMeas; i++) {
							cL = clSpTimeInfo[i].link;
							if (SyncInVoice(cL,v)) {
								clipTime = clSpTimeInfo[i].startTime;
								if (first)
									clipTime += startTime;
								syncOK = false;
								for (j=0; j<nInDocMeas; j++) {
									docTime = docSpTimeInfo[j].startTime;
									if (docTime==clipTime) {
										syncOK = true; break;
									}
									else if (docTime>clipTime) {
										vInfo[v].vBad = true;
										mergeOK = false; goto done;
									}
								}
								if (!syncOK) {
									vInfo[v].vBad = true;
									mergeOK = false; goto done;
								}
							}
						}
						
						first = false;
					}
		
				}
		}
		
		InstallDoc(doc);
	}

done:
	DisposePtr((Ptr)clSpTimeInfo);
	DisposePtr((Ptr)docSpTimeInfo);
	
	rhythmConflict = !mergeOK;
	return(rhythmConflict);
}


/* ------------------------------------------------------------------------ CheckMerge -- */
/* Check to see that the voices being pasted aren't already in use in the destination
area. NB: also initializes the vInfo array! */

static Boolean CheckMerge(Document *doc, short /*stfDiff*/, short *vMap, VInfo *vInfo,
									short *mergeErr)
{
	Boolean mergeOK=true;
	short i,v,nClipMeas;
	LINK docMeasL;
	MeasInfo *measInfo;

	*mergeErr = noErr;

	measInfo = SetupMeasInfo1(doc,&nClipMeas);
	if (!measInfo) {
		*mergeErr = Mrg_AllocErr;
		return false;
	}
	SetupMeasInfo2(doc, measInfo, nClipMeas);
	
	for (v=1; v<=MAXVOICES; v++) {
		vInfo[v].startTime = -1L;
		vInfo[v].firstStf = -1;
		vInfo[v].lastStf = -1;
		vInfo[v].singleStf = true;
		vInfo[v].hasV = false;
		vInfo[v].vOK = true;
		vInfo[v].vBad = false;
		vInfo[v].overlap = false;
	}
	
	/* Initialize the vInfo array */
	for (v=1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(clipboard, v) && vMap[v]>=0 && VOICE_MAYBE_USED(doc, vMap[v]))
			GetVoiceStf(doc, vInfo, vMap[v]);

	measInfo[0].measTime = 0L;
	for (i=1; i<nClipMeas; i++) {
		docMeasL = measInfo[i].dstL;
		measInfo[i].measTime = measInfo[i-1].measTime;
		measInfo[i].measTime += GetMeasDur(doc, docMeasL, ANYONE);
	}

	/* Get the start times of all voices potentially involved in the merge. */
	for (v=1; v<=MAXVOICES; v++) {
		if (VOICE_MAYBE_USED(clipboard, v) && VOICE_MAYBE_USED(doc, vMap[v]))
			if (ClipVInUse(v) && DocVInUse(doc, vMap[v])) {
				GetStartTime(doc, vInfo, v, vMap[v]);
			}
	}

	/* Check those start times against the end times of all corresponding
		voices in the clipboard. */
	for (v=1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(clipboard, v) && VOICE_MAYBE_USED(doc, vMap[v])) {
			if (ClipVInUse(v) && DocVInUse(doc, vMap[v])) {
				if (vInfo[v].startTime>=0)
					vInfo[v].vOK = CheckStartTime(vInfo, measInfo, v);
			}
		}

	/*
	 * If all voices are OK to merge, mergeOK stays true through the loop
	 *	and CheckMerge returns true. Otherwise, need to continue checking.
	 */
	for (v=1; v<=MAXVOICES && mergeOK; v++)
		if (VOICE_MAYBE_USED(clipboard,v) && VOICE_MAYBE_USED(doc,vMap[v])) {
			if (ClipVInUse(v) && DocVInUse(doc,vMap[v])) {
				mergeOK = vInfo[v].vOK;
			}
		}

	if (mergeOK) {
		InstallDoc(doc); return mergeOK;
	}

	/*
	 * One or more voices don't have enough temporal space to merge. Continue
	 * checking to see if it is still possible to merge. Only need to check
	 * voices for which vInfo[v].vOK is false; vInfo[v].vOK will be true for
	 * unused voices and voices which are OK. Init mergeOK to true again and
	 * set it false if any voices are bad.
	 *
	 * Set the vBad flag true for all clipboard voices which have temporal
	 * overlap and which also contain tuplets.
	 */
	for (v=1,mergeOK=true; v<=MAXVOICES; v++)
		if (!vInfo[v].vOK) {
			if (ClipTupletInV(v)) {
				*mergeErr = Mrg_TupletInV;
				vInfo[v].vBad = true;
				mergeOK = false;
			}
		}
	if (!mergeOK) goto done;

	/*
	 * Determine if there is a rhythm conflict in any of the voices which have
	 * temporal overlap.
	 */
	if (RhythmConflictInV(doc,vInfo,vMap)) {
		*mergeErr = Mrg_RhythmConfl;
		mergeOK = false;
	}	

done:
	InstallDoc(doc);
	return mergeOK;
}


/* -------------------------------------------------------------------------------------- */
/* For prototypes for CheckMerge1 and for clipVInfo, see comments in CheckMerge.c. */

short CheckMerge1(Document *doc, ClipVInfo *clipVInfo)
{
	short v,stfDiff,partDiff,stfOff;
	VInfo vInfo[MAXVOICES+1];
	MeasInfo *mInfo;

	mInfo = BuildMeasInfo(doc);

	for (v=1; v<=MAXVOICES; v++) {
		vInfo[v].startTime = -1L;
		vInfo[v].firstStf = -1;
		vInfo[v].lastStf = -1;
		vInfo[v].singleStf = true;
		vInfo[v].hasV = false;
	}
	stfDiff = GetStfDiff(doc,&partDiff,&stfOff);
	for (v=1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(clipboard,v) && VOICE_MAYBE_USED(doc,v+stfDiff))
			GetVoiceStf(doc,vInfo,v+stfDiff);

	for (v=1; v<=MAXVOICES; v++) {
		if (VOICE_MAYBE_USED(clipboard,v) && VOICE_MAYBE_USED(doc,v+stfDiff))
			if (ClipVInUse(v) && DocVInUse(doc,v+stfDiff)) {
				GetStartTime(doc,vInfo,v,v+stfDiff);
			}
	}

	for (v=1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(clipboard,v) && VOICE_MAYBE_USED(doc,v+stfDiff)) {
			if (ClipVInUse(v) && DocVInUse(doc,v+stfDiff)) {
				if (vInfo[v].startTime>=0) {
					long clipEndTime;
				
					clipEndTime = GetClipEndTime(vInfo,mInfo,v);
					clipVInfo[v].startTime = vInfo[v].startTime;
					clipVInfo[v].clipEndTime = clipEndTime;
					clipVInfo[v].vBad = (clipEndTime > vInfo[v].startTime);
				}
			}
		}

	InstallDoc(doc);
	return stfDiff;
}


/* -------------------------------------------------------------------- MCopyClipRange -- */
/* Copy the range (startL,endL) from the clipboard into the score; fill in mergeMap so
that MergeFromClip can fix crosslinks for NBJD objects. Update cross links for BJD
objects, and do not update them for NBJD objects, since MergeFromClip expects them to
have their old values. Fix staff size for all objects copied in; if staff rastral for
clipboard is different from score, update staff rastral of copied in objects. Leaves
doc installed.

It looks like this routine is designed for cases where the range -- normally a measure
-- contains no Syncs in either the clipboard or the score; in other cases, MRearrangeAll
is the function used.  --DAB, June 2017
*/

static LINK MCopyClipRange(Document *doc, LINK startL, LINK endL, LINK insertL,
									COPYMAP *mergeMap, short stfDiff, short *vMap)
{
	LINK pL,copyL,firstMeasL,initL,firstL=NILINK;
	short numObjs;
	
	numObjs = GetNumClObjs(doc);
	initL = LeftLINK(insertL);
	
	InstallDoc(clipboard);
	
	for (pL=RightLINK(startL); pL!=endL; pL=RightLINK(pL)) {
		copyL = DuplicateObject(ObjLType(pL),pL,false,clipboard,doc,false);
		if (!copyL) continue;

		SetCopyMap(pL,copyL,numObjs,mergeMap);
		if (!firstL) firstL = copyL;						/* Return first node copied	*/

		InstallDoc(doc);

		RightLINK(LeftLINK(insertL)) = copyL;				/* Link to previous object */
		LeftLINK(copyL) = LeftLINK(insertL);
		
		RightLINK(copyL) = insertL;							/* Link to following object	*/
		LeftLINK(insertL) = copyL;

		DeselectNode(copyL);
		LinkVALID(copyL) = false;

		FixStfAndVoice(copyL,stfDiff,vMap);
		if (GraphicTYPE(copyL)) FixGraphicFont(doc, copyL);

		InstallDoc(clipboard);
	}

	InstallDoc(doc);

	firstMeasL = SSearch(doc->headL,MEASUREtype,GO_RIGHT);

	FixAllBeamLinks(doc, doc, firstMeasL, insertL);
	FixOttavaLinks(doc, doc, firstMeasL, insertL);
	FixTupletLinks(doc, doc, firstMeasL, insertL);
	
	PasteFixStfSize(doc, RightLINK(initL), insertL);
	
	return firstL;
}


/* --------------------------------------------------------------- FixMergeLinks et al -- */
/* Initialize the copy map with the LINKs in the clipboard. The purpose is to fix
up cross links for non-Beamset like J_D objects. Logic is: while there may end up
being more LINKs in the main object list in the merged range than in the clipboard
object of merged LINKs, every non-Beamset like J_D object merged in from the
clipboard will have LINKs in the clipboard corresponding to its crosslink LINKs
(e.g. firstSyncL field), so a map containing all clipboard LINKs and only all
clipboard LINKs will necessarily contain all needed info.

Cf. Init/Set/GetCopyMap, GetNumClObjs.

/* --------------------------------------------------------------------- FixMergeLinks -- */
/* Fix up cross links for non-Beamset-like J_D objects merged in. */

static void FixMergeLinks(Document */*srcDoc*/, Document *dstDoc,
									LINK startL, LINK endL,
									COPYMAP *copyMap, short numObjs)
{
	short i;  LINK pL;
	
	InstallDoc(dstDoc);

	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (LinkSPAREFLAG(pL)) {
			switch (ObjLType(pL)) {
				case SLURtype:
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL==SlurFIRSTSYNC(pL)) {
								SlurFIRSTSYNC(pL) = copyMap[i].dstL;
								break;
							}
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL == SlurLASTSYNC(pL)) {
								SlurLASTSYNC(pL) = copyMap[i].dstL;
								break;
							}
					break;
				case DYNAMtype:
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL==DynamFIRSTSYNC(pL)) {
								DynamFIRSTSYNC(pL) = copyMap[i].dstL;
								break;
							}
					if (IsHairpin(pL))
						for (i = 0; i<numObjs; i++)
							if (copyMap[i].srcL)
								if (copyMap[i].srcL==DynamLASTSYNC(pL)) {
									DynamLASTSYNC(pL) = copyMap[i].dstL;
									break;
								}
					break;
				case GRAPHICtype:
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL==GraphicFIRSTOBJ(pL)) {
								GraphicFIRSTOBJ(pL) = copyMap[i].dstL;
								break;
							}
					if (GraphicSubType(pL)==GRDraw)
						for (i = 0; i<numObjs; i++)
							if (copyMap[i].srcL)
								if (copyMap[i].srcL==GraphicLASTOBJ(pL)) {
									GraphicLASTOBJ(pL) = copyMap[i].dstL;
									break;
								}
					
					break;
				case TEMPOtype:
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL==TempoFIRSTOBJ(pL)) {
								TempoFIRSTOBJ(pL) = copyMap[i].dstL;
								break;
							}
					break;
					
				case ENDINGtype:
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL==EndingFIRSTOBJ(pL)) {
								EndingFIRSTOBJ(pL) = copyMap[i].dstL;
								break;
							}				
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL==EndingLASTOBJ(pL)) {
								EndingLASTOBJ(pL) = copyMap[i].dstL;
								break;
							}				
					break;
	
				default:
					;
			}
		}
}


/* --------------------------------------------------------------- MeasGetSysRelLTimes -- */

static void MeasGetSysRelLTimes(Document *doc, MERGEARRAY *mergeArr, LINK startL)
{
	LINK measEndL,pL;
	short i,ninMeasure;
	SPACETIMEINFO *spTimeInfo;

	measEndL = EndMeasSearch(doc, startL);

	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) return;
	
	ninMeasure = GetSpTimeInfo(doc, startL, measEndL, spTimeInfo, false);

	for (i = 0; i<ninMeasure; i++) {
		pL = spTimeInfo[i].link;
		if (pL!=mergeArr[i].link)
			MayErrMsg("MeasGetSysRelLTimes: pL=%ld but mergeArr[%ld].link=%ld",
							(long)pL, (long)(i), (long)mergeArr[i].link);
		mergeArr[i].lTime = spTimeInfo[i].startTime;
	}

	DisposePtr((Ptr)spTimeInfo);
}


/* --------------------------------------------------------------------- InitMergeArr -- */
/* Fill <mergeArr> with sysRel xds and lTimes for all valid-xd LINKs starting with
startL in startL's system. */

static void InitMergeArr(Document *doc, LINK startL, LINK endL, MERGEARRAY *mergeArr)
{
	short i;  LINK pL;

	pL = FirstValidxd(startL,GO_RIGHT);
	for (i=0; pL!=endL; i++,pL=FirstValidxd(RightLINK(pL),GO_RIGHT)) {
		mergeArr[i].link = pL;
		mergeArr[i].xd = SysRelxd(pL);
	}
	
	MeasGetSysRelLTimes(doc, mergeArr, startL);
}


/* --------------------------------------------------------------------- SetupMergeArr -- */
/* Allocate an array of LINKs, lTimes and xds for all valid-xd objects in the range
[startL,endL). */

static void SetupMergeArr(LINK startL, LINK endL, MERGEARRAY **mergeArr, short *objCount)
{
	LINK pL;
	short i, numObjs=0;

	for (pL=startL; pL!=endL; pL = FirstValidxd(RightLINK(pL),GO_RIGHT))
			numObjs++;						
	*mergeArr = (MERGEARRAY *)NewPtr((Size)(numObjs+10)*sizeof(MERGEARRAY));

	if (GoodNewPtr((Ptr)mergeArr))
		for (i=0; i<numObjs; i++) {
			(*mergeArr)[i].lTime = -1L;
			(*mergeArr)[i].link = NILINK;
			(*mergeArr)[i].xd = 0;
		}
	else
		NoMoreMemory();

	*objCount = numObjs;
}

static void SetMergedFlags(LINK startL, LINK endL, Boolean value)
{
	LINK pL,aNoteL;
	
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (SyncTYPE(pL)) {
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				NoteMERGED(aNoteL) = value;
		}
}


/* Prepare for merging. Set <spareFlag>s of every object in the clipboard, set <merged>
flags of every note/rest in the clipboard, and create and return a COPYMAP. */

static COPYMAP *MergeSetupClip(Document *doc, short *numObjs)
{
	COPYMAP *mergeMap;
	LINK startL, endL;
 
	InstallDoc(clipboard);
	SetSpareFlags(clipboard->headL,clipboard->tailL,true);
	SetMergedFlags(clipboard->headL,clipboard->tailL,true);

	startL = RightLINK(clipFirstMeas);
	endL = clipboard->tailL;

	SetupCopyMap(startL, endL, &mergeMap, numObjs);
	InitCopyMap(startL,endL,mergeMap);

	InstallDoc(doc);
	return mergeMap;
}


/* ------------------------------------------------------------ Set up MeasInfo arrays -- */

#define EXTRAMEAS	4

/* Create <measInfo> array and start filling it in. Return the number of Measure objects
in the clipboard in <nClipMeas>. */

static MeasInfo *SetupMeasInfo1(Document *doc, short *nClipMeas)
{
	LINK pL, rightL, syncL, nextL;
	short i, nMeas=0;
	MeasInfo *measInfo, *pMeasInfo;

	InstallDoc(clipboard);
	for (pL=clipFirstMeas; pL; pL=LinkRMEAS(pL)) nMeas++;
	
	measInfo = (MeasInfo *)NewPtr((Size)(nMeas+EXTRAMEAS)*sizeof(MeasInfo));

	if (!GoodNewPtr((Ptr)measInfo)) {
		NoMoreMemory();
		*nClipMeas = 0;
		InstallDoc(doc);
		return NULL;
	}

	for (i=0,pMeasInfo=measInfo; i<nMeas; i++,pMeasInfo++) {
		pMeasInfo->srcL = pMeasInfo->dstL = NILINK;
		pMeasInfo->startTime = 0L;
		pMeasInfo->clipEmpty = false;
		pMeasInfo->clipNoSyncs = false;
		pMeasInfo->docEmpty = false;
		pMeasInfo->docNoSyncs = false;
		pMeasInfo->docLast = false;
		pMeasInfo->unused = 0;
	}

	pMeasInfo = measInfo;
	for (pL=clipFirstMeas; pL; pL=LinkRMEAS(pL),pMeasInfo++) {
			pMeasInfo->srcL = pL;
			
			rightL = RightLINK(pL);
			if (MeasureTYPE(rightL) || rightL==clipboard->tailL)
				pMeasInfo->clipEmpty = true;

			/* Cf. comment in SetupMeasInfo2. */

			pMeasInfo->clipNoSyncs = true;
			nextL = LinkRMEAS(pL);
			for (syncL=rightL; syncL!=nextL; syncL=RightLINK(syncL)) {
				if (SyncTYPE(syncL))
					{ pMeasInfo->clipNoSyncs = false; break; }
			}
	}
	
	*nClipMeas = nMeas;					/* Return number of Measure objects in range. */
	InstallDoc(doc);
	return measInfo;
}

/* Finish filling in the <measInfo> array; also clear the <spareFlag> of every object
and the <merged> flag of every note/rest in <doc>'s main object list. */

static void SetupMeasInfo2(Document *doc, MeasInfo *measInfo, short nClipMeas)
{
	MeasInfo *pMeasInfo;
	short i;
	LINK measL,firstMeasL,rightL,syncL,nextL;

	SetSpareFlags(doc->headL,doc->tailL,false);
	SetMergedFlags(doc->headL,doc->tailL,false);

	firstMeasL = measL = LSSearch(LeftLINK(doc->selStartL),MEASUREtype,ANYONE,GO_LEFT,false);
	pMeasInfo = measInfo;

	for (i=0; i<nClipMeas; i++,measL=LinkRMEAS(measL),pMeasInfo++) {
		pMeasInfo->dstL = measL;
		
		rightL = RightLINK(measL);
		if (MeasureTYPE(rightL) || SystemTYPE(rightL) || PageTYPE(rightL) || rightL==doc->tailL)
			pMeasInfo->docEmpty = true;

		/* If this is the last measure of a system before the last system, we don't
			expect any syncs before the first measure of the following system, so the
			traversal to LinkRMEAS should work okay. If it is the last measure of the
			score, LinkRMEAS will be NILINK, and the traversal will stop correctly at
			RightLINK of the tail. */

		pMeasInfo->docNoSyncs = true;
		nextL = LinkRMEAS(measL);
		for (syncL=rightL; syncL!=nextL; syncL=RightLINK(syncL)) {
			if (SyncTYPE(syncL))
				{ pMeasInfo->docNoSyncs = false; break; }
		}

		if (!LinkRMEAS(measL) || !SameSystem(firstMeasL,LinkRMEAS(measL)))
			{ pMeasInfo->docLast = true; break; }
	}
}

static MeasInfo *BuildMeasInfo(Document *doc)
{
	MeasInfo *measInfo;
	LINK docMeasL;
	short i,nClipMeas;

	measInfo = SetupMeasInfo1(doc,&nClipMeas);
	if (!measInfo) return NULL;

	SetupMeasInfo2(doc,measInfo,nClipMeas);
	
	measInfo[0].measTime = 0L;
	for (i=1; i<nClipMeas; i++) {
		docMeasL = measInfo[i].dstL;
		measInfo[i].measTime = measInfo[i-1].measTime;
		measInfo[i].measTime += GetMeasDur(doc, docMeasL, ANYONE);
	}
	
	return measInfo;
}


/* ----------------------------------------------------------- Dispose of Merge arrays -- */
/* Dispose merge arrays for doc. pDurArray is allocated by MPrepareSelRange; qDurArray
is allocated by MRearrangeAll; the others by MComputePlayTimes. */

static void MDisposeArrays()
{
	if (pDurArray)
		{ DisposePtr((Ptr)pDurArray); pDurArray = NULL; }
	if (qDurArray)
		{ DisposePtr((Ptr)qDurArray); qDurArray = NULL; }
	if (spTimeInfo)
		{ DisposePtr((Ptr)spTimeInfo); spTimeInfo = NULL; }
	if (stfTimeDiff)
		{ DisposePtr((Ptr)stfTimeDiff); stfTimeDiff = NULL; }
}

/*
 * Dispose merge arrays for clipboard.
 */

static void MDisposeClArrays()
{
	if (pClDurArray)
		{ DisposePtr((Ptr)pClDurArray); pClDurArray = NULL; }
	if (qClDurArray)
		{ DisposePtr((Ptr)qClDurArray); qClDurArray = NULL; }
	if (clSpTimeInfo)
		{ DisposePtr((Ptr)clSpTimeInfo); clSpTimeInfo = NULL; }
	if (clStfTimeDiff)
		{ DisposePtr((Ptr)clStfTimeDiff); clStfTimeDiff = NULL; }
}

/*
 * Dispose merge arrays for doc and clipboard.
 */

static void MDisposeAllArrays()
{
	MDisposeArrays();
	MDisposeClArrays();
}


/* ------------------------------------------------------------------------ SortPTimes -- */
/* Sort durArray via the Shell algorithm. */

static void SortPTimes(PTIME *durArray, short n, short nvoices)
{
	short gap, i, j, arrLen;
	PTIME t;

	arrLen = n*nvoices;
	for (gap=arrLen/2; gap>0; gap /= 2)
		for (i=gap; i<arrLen; i++)
			for (j=i-gap; 
				  j>=0 && durArray[j].pTime>durArray[j+gap].pTime; 
				  j-=gap) {
				t = durArray[j];
				durArray[j] = durArray[j+gap];
				durArray[j+gap] = t;
			}
}


/* ------------------------------------------------------------------------ CountSyncs -- */
/* ??Should cut this and replace call with a call to CountObjects.*/

static short CountSyncs(LINK startL, LINK endL)
{
	LINK pL; short numSyncs=0;
	
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (SyncTYPE(pL)) numSyncs++;
	
	return numSyncs;
}


/* --------------------------------------------------------------------- InsNewObjInto -- */

static LINK InsNewObjInto(LINK newObjL, LINK prevL, LINK insertL)
{
	RightLINK(newObjL) = insertL;
	LeftLINK(insertL) = newObjL;
	RightLINK(prevL) = newObjL;
	LeftLINK(newObjL) = prevL;
	
	return newObjL;
}
		
/* ------------------------------------------------------------------ MPrepareSelRange -- */
/* Prepare pDurArray for use by Merge. */

static PTIME *MPrepareSelRange(Document *doc, LINK measL, short *nInRange)
{
	short nInMeas=0, numNotes;
	
	startMeas = measL;
	endMeas = EndMeasSearch(doc, startMeas);

	nInMeas = CountSyncs(startMeas, endMeas);

	/* Allocate pDurArray[numNotes]. numNotes provides one note per voice for all
		syncs in selection range. */

	numNotes = nInMeas*(MAXVOICES+1);
	pDurArray = (PTIME *)NewPtr((Size)numNotes*sizeof(PTIME));
	if (!GoodNewPtr((Ptr)pDurArray)) { NoMoreMemory(); return NULL; }

	InitDurArray(pDurArray, nInMeas);
	*nInRange = nInMeas;

	return pDurArray;
}

/* --------------------------------------------------------------- MPrepareClSelRange -- */
/* Prepare pClDurArray for use by Merge. */

static PTIME *MPrepareClSelRange(Document *doc, LINK measL, short *nInRange)
{
	short nInMeas=0, numNotes;
	
	InstallDoc(clipboard);

	startClMeas = measL;
	endClMeas = EndMeasSearch(clipboard, startClMeas);

	nInMeas = CountSyncs(startClMeas, endClMeas);

	/*  Allocate pClDurArray[numNotes]. numNotes provides one note per voice for all
		syncs in selection range. */ 

	numNotes = nInMeas*(MAXVOICES+1);
	pClDurArray = (PTIME *)NewPtr((Size)numNotes*sizeof(PTIME));
	if (!GoodNewPtr((Ptr)pClDurArray)) { InstallDoc(doc); NoMoreMemory(); return NULL; }

	InitDurArray(pClDurArray, nInMeas);
	*nInRange = nInMeas;
	
	InstallDoc(doc);
	return pClDurArray;
}


/* ----------------------------------------------------------------- MComputePlayTimes -- */
/* Version of ComputePlayTimes for Merge */

static Boolean MComputePlayTimes(Document *doc, short nInMeas)
{
	/* Allocate objects. */

	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) goto broken;
	stfTimeDiff = (long *)NewPtr((Size)(MAXVOICES+1) * sizeof(long));
	if (!GoodNewPtr((Ptr)stfTimeDiff)) goto broken;
	
	/* Set playDur values and pTime values for the pDurArray, and set link values
		for owning beams, ottavas and tuplets. */

	SetPlayDurs(doc,pDurArray,nInMeas,startMeas,endMeas);
	SetPTimes(doc,pDurArray,nInMeas,spTimeInfo,startMeas,endMeas);
	SetLinkOwners(pDurArray,nInMeas,startMeas,endMeas);
	SortPTimes(pDurArray, nInMeas, MAXVOICES+1);

	return true;
	
broken:
	NoMoreMemory();
	return false;
}

/* Compute playTimes for notes in the clipboard. */

static Boolean MComputeClipPlayTimes(Document *doc, short nInMeas)
{
	InstallDoc(clipboard);

	/* Allocate objects. */

	clSpTimeInfo = AllocSpTimeInfo();
	if (!clSpTimeInfo) goto broken;
	clStfTimeDiff = (long *)NewPtr((Size)(MAXVOICES+1) * sizeof(long));
	if (!GoodNewPtr((Ptr)clStfTimeDiff)) goto broken;
	
	/* Set playDur values and pTime values for the pDurArray, and set link values
		for owning beams, ottavas and tuplets. */

	SetPlayDurs(doc,pClDurArray,nInMeas,startClMeas,endClMeas);
	SetPTimes(clipboard,pClDurArray,nInMeas,clSpTimeInfo,startClMeas,endClMeas);
	SetLinkOwners(pClDurArray,nInMeas,startClMeas,endClMeas);
	SortPTimes(pClDurArray, nInMeas, MAXVOICES+1);

	InstallDoc(doc);
	return true;
	
broken:

	InstallDoc(doc);
	NoMoreMemory();
	return false;
}


/* ------------------------------------------------------------------- XLateDurArray -- */
/* Translate the time of durArray elements forward in time by startTime, in order
to translate the range of notes indicated by those elements forward to start at
the time of a note (the insertion pt) after the beginning of the measure. */

static void XLateDurArray(PTIME *durArray, long startTime, short /*nInMeas*/)
{
	PTIME *pTime;
	
	pTime = durArray;
	
	for ( ; pTime->pTime<BIGNUM; pTime++)
		pTime->pTime += startTime;
}


#define SCALE(v)	((v) = sizeRatio*(v))

/* ---------------------------------------------------------- SetSyncSize, SetNoteSize -- */
/* Scale sync's horizontal and vertical coordinates to reflect change in staff rastral
size of its environment. */

static void SetSyncSize(Document */*doc*/, LINK pL, short oldRastral, short newRastral)
{
	FASTFLOAT sizeRatio;
	PMEVENT p;  PANOTE aNote;
	LINK aNoteL;
	
	if (oldRastral==newRastral) return;

	sizeRatio = (FASTFLOAT)drSize[newRastral]/drSize[oldRastral];

	p = GetPMEVENT(pL);
	SCALE(p->xd);

	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		SCALE(aNote->xd);
		SCALE(aNote->yd);
		SCALE(aNote->ystem);
	}
}

/* Scale note's horizontal and vertical coordinates to reflect change in staff rastral
size of its environment. */

static void SetNoteSize(Document */*doc*/, LINK aNoteL, short oldRastral, short newRastral)
{
	FASTFLOAT sizeRatio;  PANOTE aNote;
	
	if (oldRastral==newRastral) return;

	sizeRatio = (FASTFLOAT)drSize[newRastral]/drSize[oldRastral];

	aNote = GetPANOTE(aNoteL);
	SCALE(aNote->xd);
	SCALE(aNote->yd);
	SCALE(aNote->ystem);
}


/* ----------------------------------------------------------------------- MCreateSync -- */
/* Return a sync created out of all pTimes with the same pTime. If sync is created
out of pTimes from the clipboard, need to fill in newObjL fields in the clipboard's
durArray. 
Need version specialized for creating syncs from the clipboard.
CopySync and CopySubObjs need to be re-written to correct errors in managing
document heaps. CopySubObjs => MergeClSubObjs (already written) and CopySync =>
CopyClSync (not already written). */

static LINK MCreateSync(Document *doc, PTIME *pTime, PTIME **newPTime,
								short *nEntries, short *nItems)
{
	short subCount,itemCount;  PTIME *qTime;  LINK newObjL;

	/* Get the number of notes in this sync. */

	InstallDoc(doc);

	qTime = pTime;
	subCount = itemCount = 0;
	while (qTime->pTime==pTime->pTime) {
		subCount += qTime->mult;						/* Handle chords */
		itemCount++;									/* Count # of pTime structs to */
		qTime++;										/*	 allow caller to index array */
	}
	
	/* Create the sync object, and copy the first note's object information
		into it. */

	if (subCount) {
		newObjL = CopySync(doc,subCount,pTime);
		if (newObjL) CopySubObjs(doc,newObjL,pTime);
	
		*nEntries = subCount;
		*nItems = itemCount;
		*newPTime = qTime;
		
		return newObjL;
	}
	return NILINK;
}


/* ------------------------------------------------------------------- CopyClSubObjs -- */
/* Copy the subObject information from each note at this time into the
subObjects of the newly created object. Sets qTime->newObjL and qTime->newSubL,
which provide the mapping from syncs in the clipboard to their counterparts
in the score. Returns the subObj LINK in the list following the last one
initialized, or NILINK if every LINK is initialized. Clipboard Heaps are
expected to be installed. */

static LINK CopyClSubObjs(Document *doc, LINK newObjL, PTIME *pTime)
{
	LINK subL,newSubL,tempSubL;  PTIME *qTime;
	Boolean objSel=false;  short v;

	qTime = pTime;
	subL = qTime->subL;
	newSubL = DFirstSubLINK(doc,newObjL);

	while (qTime->pTime==pTime->pTime) {
		if (NoteSEL(subL)) objSel = true;

		if (qTime->mult > 1) {
			v = NoteVOICE(subL);
			subL = FirstSubLINK(qTime->objL);
			for ( ; subL; subL = NextNOTEL(subL))
				if (NoteVOICE(subL)==v)
					newSubL = CopyClipNotes(doc, subL, newSubL, qTime->pTime);

			/* Set the newObjL & newSubL fields. Use tempSubL here, since the
				else block of this if requires newSubL to be set correctly. */

			qTime->newObjL = newObjL;
			tempSubL = DFirstSubLINK(doc,newObjL);
			for ( ; tempSubL; tempSubL = DNextNOTEL(doc,tempSubL))
				if (DMainNote(doc,tempSubL) && DNoteVOICE(doc,tempSubL)==v) 
					{ qTime->newSubL = tempSubL; break; }

		}
		else {
			qTime->newObjL = newObjL;
			qTime->newSubL = newSubL;
			newSubL = CopyClipNotes(doc, subL, newSubL, qTime->pTime);
		}
		qTime++;
		subL = qTime->subL;
	}

	if (objSel) DLinkSEL(doc,newObjL) = true;
	return newSubL;
}


/* ---------------------------------------------------------------------- CopyClSync -- */
/* Create a new Sync with subCount nEntries and copy object level information from
pTime into it. Sync is created in the document and information is copied from the
clipboard. clipboard Heaps are expected to be installed. If we can't create the Sync,
return NILINK. */

static LINK CopyClSync(Document *doc, short subCount, PTIME *pTime)
{
	PMEVENT pObj,pNewObj;
	LINK firstSubObjL,newObjL;

	InstallDoc(doc);
	newObjL = NewNode(doc, SYNCtype, subCount);
	if (newObjL == NILINK)
		{ InstallDoc(clipboard); return NILINK; }

	pObj = (PMEVENT)LinkToPtr(clipboard->Heap+OBJtype,pTime->objL);
	pNewObj = (PMEVENT)LinkToPtr(doc->Heap+OBJtype,newObjL);
	firstSubObjL = FirstSubLINK(newObjL);					/* Save it before it gets wiped out */
	BlockMove(pObj, pNewObj, sizeof(SUPEROBJECT));
	FirstSubLINK(newObjL) = firstSubObjL;					/* Restore it. */
	LinkNENTRIES(newObjL) = subCount;						/* Set new nEntries. */
	
	InstallDoc(clipboard);
	return newObjL;
}


/* --------------------------------------------------------------------- MCreateClSync -- */
/* Return a sync created out of all clipboard pTimes with the same pTime.
#1. spareFlags in the object list are set for two reasons: 
	a. To show range of nodes merged in so MergeFromClip can determine start
		and end of range for sake of MergeFixContext (done by GetMergeRange).
	b. To show which nodes come from the clipboard, so that FixMergeLinks can
		know how to update non-Beamset-type J_D objects.
	This spareFlag is set for reason a.
*/

static LINK MCreateClSync(Document *doc, PTIME *pTime, PTIME **newPTime, short *nEntries,
							short *nItems)
{
	short subCount,itemCount;
	PTIME *qTime; 
	LINK newObjL;

	InstallDoc(clipboard);

	/* Get the number of notes in this sync. */

	qTime = pTime;
	subCount = itemCount = 0;
	while (qTime->pTime==pTime->pTime) {
		subCount += qTime->mult;							/* Handle chords */
		itemCount++;
		qTime++;
	}
	
	/* Create the sync object, and copy the first note's object information into
		it. CopyClSubObjs sets qTime->newObjL and qTime->newSubL. */

	if (subCount) {
		newObjL = CopyClSync(doc, subCount, pTime);
		if (newObjL==NILINK) goto broken;
		CopyClSubObjs(doc, newObjL, pTime);
	
		DLinkSPAREFLAG(doc,newObjL) = true; 				/* #1. */

		*nEntries = subCount;
		*nItems = itemCount;
		*newPTime = qTime;
		
		InstallDoc(doc);
		SetSyncSize(doc, newObjL, clipboard->srastral, doc->srastral);
		return newObjL;
	}

broken:
	InstallDoc(doc);
	return NILINK;
}


/* -------------------------------------------------------------------- MergeClSubObjs -- */
/* Copy the subObject information from each note at this time from a sync in the
clipboard into the subObjects of an already created object generated to hold the
notes of a pair of syncs merged together, one from the doc and one from the
clipboard. pTime is the PTIME array of notes from the clipboard. newObjL is the
already generated sync object, and newSubL is the first subObj in the segment of the
list into which the subObjs from the clipboard will be copied.

#1. If the subObj is selected, make sure that the selection status of
object and subObj are consistent, so e.g. DeselRange will work correctly.
#2. newSubL is the variable used to traverse newObjL's list of notes. Get
the LINK before subL is copied into it, since CopyClipNotes sets it to
NextNOTEL(newSubL), and we need to increment staff and voice by stfDiff. */

static void MergeClSubObjs(Document *doc, LINK newObjL, LINK newSubL, PTIME *pTime,
				short stfDiff, short *vMap)
{
	LINK subL,tempSubL,aNoteL;  PTIME *qTime;
	Boolean objSel=false;  short v;

	InstallDoc(clipboard);

	qTime = pTime;
	subL = qTime->subL;
	if (!newSubL) newSubL = DFirstSubLINK(doc,newObjL);

	while (qTime->pTime==pTime->pTime) {
		if (NoteSEL(subL)) objSel = true;									/* #1. */

		if (qTime->mult > 1) {
			v = NoteVOICE(subL);
			subL = FirstSubLINK(qTime->objL);
			for ( ; subL; subL = NextNOTEL(subL))
				if (NoteVOICE(subL)==v) {
					aNoteL = newSubL;										/* #2. */
					newSubL = CopyClipNotes(doc, subL, newSubL, qTime->pTime);
					
					InstallDoc(doc);
					SetNoteSize(doc,aNoteL,clipboard->srastral,doc->srastral);
					InstallDoc(clipboard);
					
					DNoteSTAFF(doc,aNoteL) += stfDiff;
					
					/* #3. DNoteVOICE(doc,aNoteL) is the note's voice as copied from the
						clipboard. The vMap indexed on clipboard internal voices maps
						the voice from the clipboard to the new voice in the doc. */

					DNoteVOICE(doc,aNoteL) = vMap[DNoteVOICE(doc,aNoteL)];
				}

			/* Set the newObjL & newSubL fields. Use tempSubL here, since the
				else block of this if requires newSubL to be set correctly. */

			qTime->newObjL = newObjL;
			tempSubL = DFirstSubLINK(doc,newObjL);
			for ( ; tempSubL; tempSubL = DNextNOTEL(doc,tempSubL))
				if (DMainNote(doc,tempSubL) && DNoteVOICE(doc,tempSubL)==vMap[v]) 
					{ qTime->newSubL = tempSubL; break; }

		}
		else {
			qTime->newObjL = newObjL;
			qTime->newSubL = newSubL;
			aNoteL = newSubL;												/* #2. */
			newSubL = CopyClipNotes(doc, subL, newSubL, qTime->pTime);

			InstallDoc(doc);
			SetNoteSize(doc,aNoteL,clipboard->srastral,doc->srastral);
			InstallDoc(clipboard);

			DNoteSTAFF(doc,aNoteL) += stfDiff;
			DNoteVOICE(doc,aNoteL) = vMap[DNoteVOICE(doc,aNoteL)];			/* #3. */
		}
		qTime++;
		subL = qTime->subL;
	}
	
	if (objSel) DLinkSEL(doc,newObjL) = true;

	InstallDoc(doc);
}

/* ------------------------------------------------------------------------ MMergeSync -- */
/* Merge together two syncs, one from score, one from clipboard, occurring at the
same time, into one sync.

#1. SpareFlags in the object list are set for two reasons: 
	a. To show range of nodes merged in so MergeFromClip can determine start
		and end of range for sake of MergeFixContext (done by GetMergeRange).
	b. To show which nodes come from the clipboard, so that FixMergeLinks can
		know how to update non-Beamset-type J_D objects.
	This spareFlag is set for reason a.
*/

static LINK MMergeSync(
						Document *doc,
						PTIME *pTime, PTIME **newPTime,
						PTIME *pClTime, PTIME **newClPTime,
						short *nEntries, short *nClEntries,
						short *nItems, short *nClItems,
						short stfDiff,
						short *vMap
						)
{
	short subCount=0,clSubCount=0,itemCount=0,clItemCount=0;
	PTIME *qTime,*qClTime;
	LINK newObjL,newSubL;

	/* Get the number of notes in this sync from the main object list */

	qTime = pTime;
	while (qTime->pTime==pTime->pTime) {
		subCount += qTime->mult;								/* Handle chords */
		itemCount++;
		qTime++;
	}
	*nEntries = subCount;
	*nItems = itemCount;

	/* Get the number of notes in this sync from the clipboard. */

	qClTime = pClTime;
	while (qClTime->pTime==pTime->pTime) {
		clSubCount += qClTime->mult;							/* Handle chords */
		clItemCount++;
		qClTime++;
	}
	*nClEntries = clSubCount;
	subCount += clSubCount;

	*nClItems = clItemCount;
	
	if (subCount) {
		newObjL = CopySync(doc,subCount,pTime);
		if (newObjL) {
			newSubL = CopySubObjs(doc,newObjL,pTime);
			
			MergeClSubObjs(doc,newObjL,newSubL,pClTime,stfDiff,vMap);
		}

		DLinkSPAREFLAG(doc,newObjL) = true;						/* #1. */

		*newPTime = qTime;
		*newClPTime = qClTime;
		
		InstallDoc(doc);
		return newObjL;
	}
	
	InstallDoc(doc);
	return NILINK;
}


/* --------------------------------------------------------------------- GetMergeRange -- */
/* Get actual range of nodes merged in so that an exact range of merged nodes
can be returned to MergeFixContext and other routines which need one. Merged in
nodes are distinguished from those in the score by setting their spareFlags true
as they are merged in; nodes which are combinations of subObjs from clip and score
are considered to be merged in. If <first> is true, set prevL; set lastL for every
measure, since we will drop out of loop merging each measure with lastL set
correctly. */

static void GetMergeRange(LINK startMeas, LINK endMeas, LINK *prevL, LINK *lastL,
									Boolean first)
{
	LINK pL;
	
	if (first) {
		*prevL = startMeas;
		for (pL=startMeas; pL && pL!=endMeas; pL=RightLINK(pL))
			if (LinkSPAREFLAG(pL))
				{ *prevL = LeftLINK(pL); break; }
	}
	
	*lastL = endMeas;
	for (pL=endMeas; pL && pL!=startMeas; pL=LeftLINK(pL))
		if (LinkSPAREFLAG(pL))
			{ *lastL = RightLINK(pL); break; }
}


/* --------------------------------------------------------------------- MRearrangeAll -- */
/* This is the heart of the merging process; it handles one measure at a time. We need
to pass mergeMap to RelocateClObjs, RelocateClGenlJDObjs, use to init copy map for
updating of Slurs, Graphics, Tempos, Endings and Dynamics. FixCrossPtrs should update
those types (NBJD) which originated in the score. Logic of FixCrossPtrs: all endpoint
links and cross links for Slur-type objs originating in score should be fixed up in
this way, because if one of their endpoint syncs is rearranged, the rearrangement will
be mapped by some merged measure's durArray. If one of their endpoint non-syncs is
rearranged, it will just be re-linked into the object list at the same link value. 
E.g., if a Graphic's firstObj is a Clef, it will be re-inserted with the same LINK
value, so the firstObj field won't need to be updated.

Remaining question: Does this process run the danger of over-writing previously updated
links? No, since srcL maps from score, where pre-rearrangement link values are unique,
to score, where post-rearrangement link values are unique.

The above comments are basically those of many years ago. It looks like this routine is
designed for cases where the one-measure range contains Syncs in either the clipboard or
the score or both, which is certainly the normal case; in other cases, MCopyClipRange is
the function used. FIXME: when MCopyClipRange copies a Graphic, it calls FixGraphicFont;
shouldn't we do that here too?  --DAB, June 2017 */

static Boolean MRearrangeAll(
						Document *doc,
						short nNotes, short nClNotes,
						LINK insertL,
						LINK *prevL1, LINK *lastL1,
						Boolean firstMeas,
						short stfDiff,
						COPYMAP *mergeMap,
						short *vMap,
						Boolean thin
						)
{
	short i,j,numNotes,numClNotes,arrBound,clArrBound,nEntries,nClEntries,
		numObjs,nItems,nClItems;
	PTIME *pTime, *qTime, *pClTime, *qClTime;
	LINK newObjL,headL,tailL,initL,prevL,mInsertL,firstL,lastL;
	MERGEARRAY *mergeA;
		
	/* Compact the pDurArray: allocate qDurArray, copy into it only those pTimes s.t.
		pTime->subL!= NILINK, and then fill in the rest of the entries with default
		values. Likewise for pClDurArray. */

	numNotes = nNotes*(MAXVOICES+1);
	qDurArray = (PTIME *)NewPtr((Size)numNotes*sizeof(PTIME));
	if (!GoodNewPtr((Ptr)qDurArray)) { NoMoreMemory(); return false; }

	arrBound = CompactDurArray(pDurArray,qDurArray,numNotes);

#ifdef DEBUG_QDURARRAY
	DebugDurArray(arrBound,qDurArray);
#endif

	numClNotes = nClNotes*(MAXVOICES+1);
	qClDurArray = (PTIME *)NewPtr((Size)numClNotes*sizeof(PTIME));
	if (!GoodNewPtr((Ptr)qClDurArray)) { NoMoreMemory(); return false; }

	clArrBound = CompactDurArray(pClDurArray,qClDurArray,numClNotes);

#ifdef DEBUG_QCLDURARRAY
	DebugDurArray(clArrBound,qClDurArray);
#endif

	/* If this is the first measure of the range of measures to be merged, the merged
		nodes may not start at the temporal beginning of the measure; get the time at
		which to start merging, and translate the pTime->pTime fields of the merged
		range to start at this time. */

	if (firstMeas) {
		short nDocObjs; long thisTime, startTime;
		LINK validFirstL;

		SetupMergeArr(RightLINK(startMeas), endMeas, &mergeA, &nDocObjs);
		if (!mergeA) return false;

		if (nDocObjs > 0) {
			InitMergeArr(doc, RightLINK(startMeas), endMeas, mergeA);
	
			startTime = mergeA[nDocObjs-1].lTime;
			validFirstL = FirstValidxd(insertL, GO_RIGHT);
	
			for (i=0; i<nDocObjs; i++) {
				thisTime = mergeA[i].lTime;
				if (mergeA[i].link == validFirstL)
					{ startTime = mergeA[i].lTime; break; }
			}
		
			/* insertion point at end of measure: translate merged range to time of
				last object in measure + logical dur of last object. */
	
			if (MeasureTYPE(insertL) || TailTYPE(insertL) || J_STRUCTYPE(insertL))
				startTime = thisTime + GetLDur(doc,mergeA[nDocObjs-1].link,0);
				
			XLateDurArray(qClDurArray,startTime,numClNotes);
		}

		DisposePtr((Ptr)mergeA);
	}

	numObjs = GetNumClObjs(doc);

	SetCopyMap(startClMeas,startMeas,numObjs,mergeMap);
	SetCopyMap(endClMeas,endMeas,numObjs,mergeMap);

	initL = prevL = startMeas;								/* Get insertion point for inserting */
	mInsertL = endMeas;										/* 	 newly created syncs */

	firstL = RightLINK(startMeas);							/* Get boundary markers of preexisting */
	lastL = LeftLINK(endMeas);								/* 	 list between measure bounds */

	/* The object list now looks like this, where "(|)" means the object is a Measure:
		startMeas(|)	firstL	...		lastL	endMeas(I)
	*/
		
	/* Link the selection range into a temporary object list. This is simply to store
		the non-Sync nodes until they can be returned to the object list, instead of
		calling DeleteRange(doc, startMeas, endMeas). */

	headL = NewNode(doc, HEADERtype, 2);					/* Create new empty list */
	if (!headL) goto broken1;
	tailL = NewNode(doc, TAILtype, 0);
	if (!tailL) { DeleteNode(doc, headL); goto broken1; }
	LeftLINK(headL) = RightLINK(tailL) = NILINK;

	RightLINK(startMeas) = endMeas;							/* Detach list between measure bounds */
	LeftLINK(endMeas) = startMeas;

	RightLINK(headL) = firstL;								/* Reattach list between measure bounds */
	LeftLINK(tailL) = lastL;								/*   to the temporary list */
	LeftLINK(firstL) = headL;
	RightLINK(lastL) = tailL;

LogPrintf(LOG_DEBUG, "MRearrangeAll: thin=%d firstL=%u endMeas=%d\n", thin, firstL, endMeas);
Browser(doc, firstL, endMeas);
	if (thin) ThinNonSyncObjs(doc, firstL, endMeas);	// ?????TESTING!!!!!!!!!!!

	/* Loop through the qDurArray and pClDurArray, creating syncs out of notes that
		have the same pTimes from each array, merging syncs from both arrays into the
		main object list. */
		
	pTime = qTime = qDurArray;
	pClTime = qClTime = qClDurArray;

	for (i=0,j=0; i<arrBound && j<clArrBound; ) {
		if (pClTime->pTime > pTime->pTime) {
			newObjL = MCreateSync(doc,pTime,&qTime,&nEntries,&nItems);
			pTime = qTime;
			i += nItems;
		}
		else if (pClTime->pTime < pTime->pTime) {
			newObjL = MCreateClSync(doc,pClTime,&qClTime,&nClEntries,&nClItems);
			FixStfAndVoice(newObjL,stfDiff,vMap);
			SetCopyMap(pClTime->objL,newObjL,numObjs,mergeMap);
			pClTime = qClTime;
			j += nClItems;
		}
		else {
			newObjL = MMergeSync(doc,pTime,&qTime,pClTime,&qClTime,&nEntries,&nClEntries,
														&nItems,&nClItems,stfDiff,vMap);
			SetCopyMap(pClTime->objL,newObjL,numObjs,mergeMap);
			pTime = qTime;
			pClTime = qClTime;
			i += nItems;
			j += nClItems;
		}
		if (newObjL) prevL = InsNewObjInto(newObjL,prevL,mInsertL);
	}
	
	if (i<arrBound)
		for ( ; i<arrBound; ) {
			newObjL = MCreateSync(doc,pTime,&qTime,&nEntries,&nItems);
			pTime = qTime;
			i += nItems;

			if (newObjL) prevL = InsNewObjInto(newObjL,prevL,mInsertL);
		}
	else if (j<clArrBound)
		for ( ; j<clArrBound; ) {
			newObjL = MCreateClSync(doc,pClTime,&qClTime,&nClEntries,&nClItems);
			FixStfAndVoice(newObjL,stfDiff,vMap);
			SetCopyMap(pClTime->objL,newObjL,numObjs,mergeMap);
			pClTime = qClTime;
			j += nClItems;

			if (newObjL) prevL = InsNewObjInto(newObjL,prevL,mInsertL);
		}

	InstallDoc(doc);

#ifdef DEBUG_QCLDURARRAY
	DebugDurArray(clArrBound,qClDurArray);
#endif

	/* Now to move non-Sync nodes we saved in a temporary object list into their
		proper places. Relocate all J_IT & J_IP objects, and all J_D objects which
		can only be relative to SYNCs. */

	RelocateObjs(doc,headL,tailL,startMeas,endMeas,qDurArray);

	/* Relocate all J_D objects which can be relative to J_IT objects other than
		SYNCs or J_IP objects. */
	
	RelocateGenlJDObjs(doc,headL,tailL,qDurArray);
	
	/* Relocate clipboard objs as above */

LogPrintf(LOG_DEBUG, "MRearrangeAll: Browser\n"); Browser(doc, doc->headL, doc->tailL);

	InstallDoc(clipboard);
	if (IsAfter(startClMeas,endClMeas)) {
		RelocateClObjs(doc,startClMeas,endClMeas,startMeas,endMeas,qClDurArray,stfDiff,mergeMap,vMap);
		RelocateClGenlJDObjs(doc,startClMeas,endClMeas,startMeas,endMeas,qClDurArray,stfDiff,mergeMap,vMap);
	}
	InstallDoc(doc);

	GetMergeRange(startMeas,endMeas,prevL1,lastL1,firstMeas);

	/* Beams, ottavas and tuplets can be updated without reference to their
		origin. - Not true anymore ??HUH?, because of overlapping voices.
		NBJD objects are distinguished as to their origin by setting
		spareFlags of those in score false, those in clip true. qDurArray
		is passed to MFixCrossPtrs and used to update ptrs of NBJD objects
		originally in score; those merged from clipboard are updated using
		mergeMap after all measures are merged in. */

	MFixCrossPtrs(doc,startMeas,endMeas,qDurArray);

	DeleteRange(doc, RightLINK(headL), tailL);
	DeleteNode(doc, headL);
	DeleteNode(doc, tailL);
	return true;
	
broken:
	DeleteRange(doc, RightLINK(headL), tailL);
	DeleteNode(doc, headL);
	DeleteNode(doc, tailL);
	
broken1:
	NoMoreMemory();
	return false;
}


/* --------------------------------------------------------------------- Merge1Measure -- */
/* Merge one measure of the clipboard and the score. Update firstObj and lastObj
links in pre-existing objects but not new ones. */

static Boolean Merge1Measure(
						Document *doc,
						MeasInfo *measInfo,
						short clipMeasNum,
						LINK insertL, 
						LINK *prevL, LINK *lastL,
						short stfDiff,
						COPYMAP *mergeMap,
						short *vMap,
						Boolean thin
						)
{
	LINK clipMeasL, docMeasL, clipEndL, docEndL, docInsL, firstL;
	short nInMeas, nInClMeas;
	Boolean firstMeas;
	MeasInfo *pMeasInfo;

	firstMeas = (clipMeasNum==0);
	pMeasInfo = measInfo+clipMeasNum;
	if (pMeasInfo->clipEmpty) return true;

	InstallDoc(clipboard);
	clipMeasL = pMeasInfo->srcL;
	clipEndL = EndMeasSearch(clipboard, clipMeasL);

	InstallDoc(doc);
	docMeasL = pMeasInfo->dstL;
	docEndL = EndMeasSearch(doc, docMeasL);
	docInsL = docEndL;					/* Merge orphaned ranges at end of measure */

	/* Handle oddball cases. */
	
	if (pMeasInfo->docEmpty || pMeasInfo->docNoSyncs || pMeasInfo->clipNoSyncs) {
		firstL = MCopyClipRange(doc, clipMeasL, clipEndL, docInsL, mergeMap, stfDiff, vMap);
		
		if (firstMeas) *prevL = LeftLINK(firstL);
		*lastL = docEndL;

		return true;
	}

	/* Handle the usual case. If there are no objects in the score's measure, simply
	   copy the range in from the clipboard. We must update crosslinks in the same way
	   as other measures: update links of beamlike objs here, and set mergemap for NBJD
	   objs so that MergeFromClip can update them. */

	pDurArray = MPrepareSelRange(doc, docMeasL, &nInMeas);
	if (!pDurArray) goto broken;
	pClDurArray = MPrepareClSelRange(doc, clipMeasL, &nInClMeas);
	if (!pClDurArray) goto broken;

	if (!MComputePlayTimes(doc, nInMeas)) goto broken;
	if (!MComputeClipPlayTimes(doc, nInClMeas)) goto broken;

	if (!MRearrangeAll(doc, nInMeas, nInClMeas, insertL, prevL, lastL, firstMeas,
													stfDiff, mergeMap,vMap,thin))
		goto broken;

	MDisposeAllArrays();
	return true;
	
broken:
	MDisposeAllArrays();
	return false;
}


/* --------------------------------------------------------------------- MergeFromClip -- */
/* This is the top-level function to merge the clipboard into <doc> before <insertL>.
Return the end of the merged range so the calling function can respace, deselect, etc.
to the end of it.

#1. We assume the clipboard contains just a single system; if there are nodes from
more than one system in the clipboard, lastCopy will equal COPYTYPE_PAGE in
FixEditMenu and Paste Merge should be disabled, guaranteeing that. If traversal of
score measure objects goes over into a second system, break and paste the rest of
the clipboard at the end of the first system, after the measure-by-measure traversal
of clipboard and score. Reformatting may be needed, but we don't check for it.

#2. LeftLINK(measL): MCopyClipRange starts copying from RightLINK(startL); since there
are no measure objects left in the score, we need to copy in the measure object from
the clipboard.

#3. Fixing up of staff size for different rastral sizes of clipboard and doc is
dispersed through the operation:
	in MCopyClipRange, for ranges which are copied entire;
	in InsertJIT/JIP/JDBefore, for non-sync objects; and
	in MMergeSync or MCreateClSync for syncs in merged ranges. */

static LINK MergeFromClip(Document *doc, LINK insertL, short *vMap, VInfo *vInfo,
							Boolean thin)
{
	short minStf, maxStf, stfDiff, partDiff, stfOff, numObjs,nClipMeas, i;
	LINK measL, prevL, lastL, insL;
	COPYMAP *mergeMap;
	MeasInfo *measInfo;

	mergeMap = MergeSetupClip(doc, &numObjs);
	measInfo = SetupMeasInfo1(doc, &nClipMeas);

	/* Get minStf and maxStf of nodes in clipboard (doc is the doc installed
		before routines exit), and the stfDiff between clipboard and doc. */

	minStf = GetClipMinStf(doc);
	maxStf = GetClipMaxStf(doc);
	stfDiff = GetStfDiff(doc, &partDiff, &stfOff);

	SetupMeasInfo2(doc, measInfo, nClipMeas);
	
	/*
	 * If there are nothing but empty measures in the clipboard, Merge1Measure
	 * will always return true before doing anything, leaving prevL and lastL
	 * as garbage unless they are set somewhere. It is more direct to set them
	 * here before looping through the measures, and them resetting them if necessary
	 * inside the loop, than trying to figure out how to set them inside the
	 * loop if empty measures cause simple returns without any merging or copying.
	 */
	prevL = LeftLINK(insertL);
	lastL = insertL;

	for (i=0; i<nClipMeas; i++) {
		if (!Merge1Measure(doc,measInfo,i,insertL,&prevL,&lastL,stfDiff,mergeMap,vMap,thin))
			goto broken;
		
		if (measInfo[i].docLast) {
			InstallDoc(doc);
			insL = EndMeasSearch(doc,measInfo[i].dstL);
			i++;
			break;										
		}
	}

	/* If we exited the measure-merging loop before copying the entire clipboard, the
	   clipboard contents are going to extend past the current end of the score. In
	   that case, we finish copying the clipboard now. ??Actually, I'm not sure when
	   the below code is used, if ever. */
	   
	if (i<nClipMeas) {
		measL = measInfo[i].srcL;
		MCopyClipRange(doc,DLeftLINK(clipboard,measL),clipboard->tailL,insL,mergeMap,
						stfDiff,vMap);										/* #2. */

		/* Measure object crossLinks may need updating. */
		FixStructureLinks(doc, doc, prevL, insL);
	}

	/* Fix NBJD objects: Non-Beamlike-J_D objects, J_D objects other than Beams,
		Tuplets and Ottavas. */

	FixMergeLinks(clipboard, doc, prevL, lastL, mergeMap, numObjs);

	/* Fix up syncs which were merged in voices with temporal overlap. */
	
	MergeFixVOverlaps(doc, prevL, lastL, vMap, vInfo);

	/* We must fix context only up to the final object merged in. MergeFixContext
		expects the end node to be the first node after the range to be updated, which
		is what Merge1Measure returns. */

	MergeFixContext(doc, prevL, lastL, minStf, maxStf, stfDiff);

	return lastL;

broken:
	DisableUndo(doc,true);
	return NILINK;
}


/* ----------------------------------------------------------- CleanupMerge and helper -- */

static void ThinNonSyncObjs(Document *doc, LINK startL, LINK endL)
{
	LINK pL, rightL;
	
		pL = startL;
		while (pL!=endL) {
			rightL = RightLINK(pL);		/* Save the right link in case we delete this node */
LogPrintf(LOG_DEBUG, "ThinNonSyncObjs: pL=%u rightL=%u\n", pL, rightL);
		if (!LinkSPAREFLAG(pL)) goto next;
		switch (ObjLType(pL)) {
			/* Ignore these; they should never appear in the merged range. */
			case PAGEtype:
			case SYSTEMtype:
			case CONNECTtype:
			case TAILtype:
			case STAFFtype:
				break;

			/* If any of these appear in the merged range, leave them alone. */
			case KEYSIGtype:
			case TIMESIGtype:
			case MEASUREtype:
			case PSMEAStype:
			case SYNCtype:
			case GRSYNCtype:
			case SLURtype:
			case BEAMSETtype:
			case TUPLETtype:
			case OTTAVAtype:
			case SPACERtype:
				break;

LogPrintf(LOG_DEBUG, "ThinNonSyncObjs: about to delete pL=%u\n", pL);
			/* If any of these appear in the merged range and came from the clipboard,
				remove them. */
			case CLEFtype:
				DeleteNode(doc, pL);		// ??IS THIS SAFE????
				break;
			case DYNAMtype:
				DeleteNode(doc, pL);		// ??IS THIS SAFE????
				break;
			case RPTENDtype:
				DeleteNode(doc, pL);		// ??IS THIS SAFE????
				break;
			case ENDINGtype:
				DeleteNode(doc, pL);
				break;
			case GRAPHICtype:
				DeleteNode(doc, pL);
				break;
			case TEMPOtype:
				DeleteNode(doc, pL);
				break;
			default:
				;
		}
		
next:
		if (!rightL || rightL==endL || TailTYPE(rightL)) return;
		pL = rightL;
	}
}

/* Clean up after the merge operation: fix whole rests, time stamps, voice table,
respace or inval, and handle selection range and caret. If <thin>, also remove objects
other than Syncs and grouping objects needed to render them properly (beams, tuplets,
and ottavas): this is intended to support Paste as Cue. */

static void CleanupMerge(Document *doc, LINK prevMeasL, LINK lastL)
{
	LINK endMeasL;

	endMeasL = LSSearch(LeftLINK(lastL), MEASUREtype, ANYONE, GO_LEFT, false);	/* Cf. #2 in DoMerge */
	if (endMeasL)
			endMeasL = EndMeasSearch(doc, endMeasL);
	else	endMeasL = doc->tailL;

	FixWholeRests(doc, prevMeasL);
	FixTimeStamps(doc, prevMeasL, doc->tailL);
	UpdateVoiceTable(doc, true);

	if (doc->autoRespace)
		RespaceBars(doc, prevMeasL, endMeasL, 0L, false, false);
	else 
		InvalMeasures(prevMeasL, endMeasL, ANYONE);

	DeselRange(doc, prevMeasL, endMeasL);
	
	/* Set insertion point following barline terminating merged range */

	doc->selStartL = doc->selEndL = endMeasL;
	MEAdjustCaret(doc,true);
}


/* --------------------------------------------------------------------------- DoMerge -- */
/* Merge the contents of the clipboard into the score starting at doc->selStartL.
User-interface level.

#1. a. If there is an insertion point at the end of a measure, doc->selStartL
	will be set to the following measure object.
	b. If there is an insertion point at the start of a measure,
	LeftLINK(doc->selStartL) will be the measure object, and searching from it
	will return it, which is correct.
	c. If there is a non-empty selRange whose first selected object is a measure,
	doc->selStartL will be set to that measure.
		In this case, DeleteSelection will delete the measure obj and everything
		else in the range, and if the insertion point is left at the end of
		a new measure, the previous case will hold.
	Therefore, we can search for a prevMeasL from LeftLINK(doc->selStartL).
#2. a, b and c hold for doc->selEndL as well. Therefore, can search for endMeasL
	in CleanupMerge from LeftLINK(doc->selEndL).
*/

void DoMerge(Document *doc)
{
	LINK prevMeasL, lastL;
	Boolean dontResp;
	short mergeType, stfDiff, partDiff, stfOff, mergeErr;
	VInfo vInfo[MAXVOICES+1];

	if (doc->selStartL==doc->tailL) {
	
		/* In this case, we simply paste at the end of the score. DoPaste is a user-
			interface-level function, so it has to call PrepareUndo; however, we want the
			Undo menu command to reflect the fact that the user thinks they did a merge,
			so that's done here. */
		
		DoPaste(doc);
		GetIndCString(strBuf, UNDO_STRS, 28);    				/* "Undo Merge" */
		SetupUndo(doc, U_Merge, strBuf);
		return;
	}

	if (doc->selStartL!=doc->selEndL) {
		GetIndCString(strBuf, CLIPERRS_STRS, 8);    			/* "You can't Paste Merge without an insertion point." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	mergeType = CheckMainClip(doc, DRightLINK(clipboard, clipFirstMeas), clipboard->tailL);
	if (mergeType==MergePin) {
		MayErrMsg("The Merge feature 'MergePin' is not yet implemented.");
		return;
	}
	if (mergeType==MergeCancel) return;

	/* Set up the voice-handling machinery, then check for problems with any voice; if
	   any exist, bring up a dialog to say so and give up. */

	stfDiff = GetStfDiff(doc,&partDiff,&stfOff);
	SetupVMap(doc,voiceMap,stfDiff);

	if (!CheckMerge(doc,stfDiff,voiceMap,vInfo,&mergeErr)) {
		DoChkMergeDialog(doc);
		return;
	}

	PrepareUndo(doc, doc->selStartL, U_Merge, 28);  			/* "Undo Merge" */
	MEHideCaret(doc);
	WaitCursor();

	doc->selStaff = GetSelStaff(doc);

	/* If any Measure objects are only partly selected, we have to do something so
		we don't end up with a object list with Measure objs with subobjs on only
		some staves, since that is illegal. Anyway, the user probably doesn't really
		want to delete such Measures. ??Cf. Clear; for now, just avoid disasters.
		??Since we currently insist on an insertion pt before reaching here, this
		does nothing. */
		
	if (DeselPartlySelMeasSubobjs(doc)) OptimizeSelection(doc);

	/* #1. */
	prevMeasL = LSSearch(LeftLINK(doc->selStartL), MEASUREtype, ANYONE, GO_LEFT, false);	

	DeleteSelection(doc, true, &dontResp);

	/* Everything finally looks OK. Actually do the merge. */
	
	lastL = MergeFromClip(doc, doc->selEndL, voiceMap, vInfo, false);

	CleanupMerge(doc, prevMeasL, lastL);
}


/* ---------------------------------------------------------------------- DoPasteAsCue -- */

static void SetPastedAsCue(Document *doc, LINK prevMeasL, LINK lastL, short velocity)
{
	LINK pL, aNoteL;
		
	for (pL = prevMeasL; pL!=lastL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL) && LinkSPAREFLAG(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if (NoteMERGED(aNoteL)) {
					NoteCUENOTE(aNoteL) = true;
//LogPrintf(LOG_DEBUG, "SetPastedAsCue: set note %u velocity to %d\n", aNoteL, velocity);
				}
			}
	}
}


/* Merge the contents of the clipboard into the score starting at doc->selStartL,
treating the content of the clipboard as a cue. User-interface level. See the comments
on DoMerge(). */

void DoPasteAsCue(Document *doc, short voice, short velocity)
{
	LINK prevMeasL, lastL;
	Boolean dontResp;
	short mergeType, stfDiff, partDiff, stfOff, mergeErr;
	VInfo vInfo[MAXVOICES+1];

	if (doc->selStartL==doc->tailL) {
	
		/* In this case, we simply paste at the end of the score. DoPasteAsCue is a user-
			interface-level function, so it has to call PrepareUndo; however, we want the
			Undo menu command to reflect what the user thinks they did, so that's done
			here. */
		
		DoPaste(doc);
		GetIndCString(strBuf, UNDO_STRS, 52);    				/* "Undo Paste as Cue" */
		SetupUndo(doc, U_Merge, strBuf);
		return;
	}

	if (doc->selStartL!=doc->selEndL) {
		GetIndCString(strBuf, CLIPERRS_STRS, 10);    			/* "You can't Paste as Cue without an insertion point." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	mergeType = CheckMainClip(doc, DRightLINK(clipboard, clipFirstMeas), clipboard->tailL);
	if (mergeType==MergePin) {
		MayErrMsg("The Merge feature 'MergePin' is not yet implemented.");
		return;
	}
	if (mergeType==MergeCancel) return;

	/* Set up the voice-handling machinery, then check for problems with any voice; if
	   any exist, bring up a dialog to say so and give up. */

	stfDiff = GetStfDiff(doc,&partDiff,&stfOff);
	SetupVMap(doc,voiceMap,stfDiff);

	if (!CheckMerge(doc,stfDiff,voiceMap,vInfo,&mergeErr)) {
		DoChkMergeDialog(doc);
		return;
	}

	PrepareUndo(doc, doc->selStartL, U_PasteAsCue, 28);  			/* "Undo Paste as Cue" */
	MEHideCaret(doc);
	WaitCursor();

	doc->selStaff = GetSelStaff(doc);

	/* If any Measure objects are only partly selected, we have to do something so
		we don't end up with a object list with Measure objs with subobjs on only
		some staves, since that is illegal. Anyway, the user probably doesn't really
		want to delete such Measures. ??Cf. Clear; for now, just avoid disasters.
		??Since we currently insist on an insertion pt before reaching here, this
		does nothing. */
		
	if (DeselPartlySelMeasSubobjs(doc)) OptimizeSelection(doc);

	/* #1. */
	prevMeasL = LSSearch(LeftLINK(doc->selStartL), MEASUREtype, ANYONE, GO_LEFT, false);	

	DeleteSelection(doc, true, &dontResp);

	/* Everything finally looks OK. Actually do the merge, and make the merged-in notes
	   small and with the desired velocity. */
	
	lastL = MergeFromClip(doc, doc->selEndL, voiceMap, vInfo, true);
LogPrintf(LOG_INFO, "Calling SetPastedAsCue with prevMeasL=%u, lastL=%u...\n", prevMeasL, lastL);
	SetPastedAsCue(doc, prevMeasL, lastL, velocity);
	
	CleanupMerge(doc, prevMeasL, lastL);
}
