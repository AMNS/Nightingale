/******************************************************************************************
*	FILE:	SpaceHighLevel.c
*	PROJ:	Nightingale
*	DESC:	High-level spacing routines, including justifying.
*
	CenterNR					CenterWholeMeasRests		RemoveObjSpace
	RespaceAll					PrevITSym
	LyricWidthLR				ArpWidthLR					SyncGraphicWidthLR
	IPGroupWidth				IPMarginWidth				IPSpaceNeeded
	ConsidITWidths				ConsidIPWidths				ConsiderWidths
	Respace1Bar					GetJustProp					PositionSysByTable
	RespaceBars					StretchToSysEnd				SysJustFact
	JustifySystem				JustifySystems
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include <ctype.h>
#include "Nightingale.appl.h"

#if 1
#define SPACEBUG
#endif

#define STFHEIGHT drSize[doc->srastral]	/* For now, assume all staves are same height */

typedef struct {
	LINK		link;
	DDIST		xd;
	DDIST		xdaux;		/* for hairpin endxd */
} XDDATA;

static short PrevITSym(Document *, short, short, SPACETIMEINFO []);
static void LyricWidthLR(Document *, LINK, short, DDIST *, DDIST *);
static void ArpWidthLR(Document *, LINK, short, STDIST *, STDIST *);
static void SyncGraphicWidthLR(Document *, LINK, short, DDIST *, DDIST *);
static STDIST IPGroupWidth(short, short, SPACETIMEINFO [], STDIST [], STDIST [], short *);
static STDIST IPMarginWidth(Document *, SPACETIMEINFO [], short, short);
static STDIST IPSpaceNeeded(Document *, short, short, SPACETIMEINFO [], STDIST [], STDIST [],
							LONGSTDIST []);
static void ConsidITWidths(Document *, LINK, short, SPACETIMEINFO [], STDIST []);
static void ConsidIPWidths(Document *, LINK, short, SPACETIMEINFO [], STDIST [], LONGSTDIST []);
static void ConsiderWidths(Document *, LINK, short, SPACETIMEINFO [], STDIST [], LONGSTDIST []);


/* -------------------------------------------------------------------------- CenterNR -- */
/* Center the given note or rest (normally a whole-measure or multi-bar rest)
around the given <measCenter> position. */

void CenterNR(Document *doc, LINK syncL, LINK aRestL, DDIST measCenter)
{
	CONTEXT	context;

	GetContext(doc, syncL, NoteSTAFF(aRestL), &context);
	measCenter -= SymDWidthRight(doc, syncL, NoteSTAFF(aRestL), False, context)/2;
	NoteXD(aRestL) = measCenter-LinkXD(syncL);
}

/* -------------------------------------------------------------- CenterWholeMeasRests -- */
/* Center all whole-measure and multi-bar rests in the given Measure. Makes no
user-interface assumptions: in particular, does not Inval or redraw anything. */

void CenterWholeMeasRests(
			Document	*doc,
			LINK		barFirst,		/* First node within measure, i.e., after barline */
			LINK		barLast,		/* Last node to consider (usually the next MEASURE obj.) */
			DDIST		measWidth 		/* Width of the measure */		
			)		
{
	LINK	pL, aNoteL;
	DDIST	measCenter, spAfterBar, KSTSWidth;

	spAfterBar = std2d(config.spAfterBar, STFHEIGHT, STFLINES);

	KSTSWidth = 0;
	for (pL = barFirst; pL!=RightLINK(barLast); pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case KEYSIGtype:
				KSTSWidth += GetKeySigWidth(doc, pL, ANYONE);	/* NB: criticisms of this function in SpaceTime.c */
				break;
			case TIMESIGtype:
				KSTSWidth += GetTSWidth(pL);
				break;
			case SYNCtype:
				if (KSTSWidth>0)
					measCenter = (spAfterBar + KSTSWidth + measWidth) / 2;
				else
					measCenter = measWidth / 2;
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteREST(aNoteL))
						if (NoteType(aNoteL)<=WHOLEMR_L_DUR)		/* It's a whole-measure or multi-bar rest. */
							CenterNR(doc, pL, aNoteL, measCenter);
				break;

		}
	}
}


/* -------------------------------------------------------------------- RemoveObjSpace -- */
/* Move everything after <pL> on its system to the left by the amount of space currently
occupied by <pL>. Return False in the unlikely case there's nothing after <pL>, i.e.,
it's the tail of the object list; else return True. Does not adjust measureRects and
measureBBoxes. ??In-line code in DeleteWhole, etc., should be replace by calls to this
or something similar. */

Boolean RemoveObjSpace(Document *doc, LINK pL)
{
	DDIST xMove; LINK tempL;

	if (!RightLINK(pL)) return False;
	
	xMove = PMDist(pL, ObjWithValidxd(RightLINK(pL), True));
	tempL = MoveInMeasure(RightLINK(pL), doc->tailL, -xMove);
	MoveMeasures(tempL, doc->tailL, -xMove);
	
	return True;
}


/* ------------------------------------------------------------------------ RespaceAll -- */
/* Go thru entire data structure and perform global punctuation by fixing up
x-coordinates so each allows previous thing the correct space, scaled by a
given percentage. */

void RespaceAll(Document *doc, short percent)
{
	LINK pL;
	
	if (RightLINK(doc->headL)==doc->tailL) return;					/* If nothing in data structure, do nothing */
	WaitCursor();
	pL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, False);		/* Start at first measure */
	RespaceBars(doc, pL, doc->tailL, RESFACTOR*(long)percent, False, False);
}


/* ------------------------------------------------------------------------- PrevITSym -- */
/* Given a SPACETIMEINFO array, staff number, and starting index, deliver the index
of the previous J_IT object on the staff; if there is none, deliver -1. */

static short PrevITSym(Document */*doc*/, short staff, short ind, SPACETIMEINFO spaceTimeInfo[])
{
	short j;
	
	for (j = ind-1; j>=0; j--) {
		if (spaceTimeInfo[j].justType==J_IT)
			if (staff==ANYONE || ObjOnStaff(spaceTimeInfo[j].link, staff, False))
				return j;
	}
	
	return -1;
}


/* ---------------------------------------------------------- LyricWidthLR and friends -- */

static Boolean IsLyricPunct(unsigned char);
static Boolean AllowOverlap(LINK);

static Boolean IsLyricPunct(unsigned char ch)
{
	return (ch==':' || ch==';' || ch=='.' || ch==',' || ch=='?');
}

/* Return True if the lyric should be allowed to overlap following notes, i.e., if
its last non-whitespace, non-punctuation character is hyphen or underline (poor
person's extender) or hard blank. ??Should use C string (via CCopy) instead of Pascal. */

#define CH_HARDSPACE 0xCA

static Boolean AllowOverlap(LINK lyricL)
{
	Str255 string;  unsigned char lastChar;  short i;

	Pstrcpy((StringPtr)string, (StringPtr)PCopy(GetPAGRAPHIC(FirstSubLINK(lyricL))->strOffset));
	
	/* Skip trailing white space or punctuation marks before checking. */
	
	i = *string;
	while (i>0 && (isspace(string[i]) || IsLyricPunct(string[i])))
		i--;
	lastChar = string[i];
	
	/* Allow overlap for opt-space as well */
   return (lastChar=='-' || lastChar=='_' || lastChar==(unsigned char)CH_HARDSPACE);
}

/* Return the distance the given lyric Graphic needs to the left and to the right
of the origin of the object it's attached to, to avoid overlap. */

static void LyricWidthLR(Document *doc, LINK lyricL, short staff, STDIST *pNeedLeft,
									STDIST *pNeedRight)
{
	CONTEXT		context;
	short		width;
	DDIST		dNeedLeft, dNeedRight;

	GetContext(doc, lyricL, staff, &context);
	width = NPtGraphicWidth(doc, lyricL, &context);
	dNeedLeft = -LinkXD(lyricL);
	dNeedRight = pt2d(width)-dNeedLeft;
	
	if (AllowOverlap(lyricL)) dNeedRight = 0;

	*pNeedRight = d2std(dNeedRight, context.staffHeight, context.staffLines);
	*pNeedLeft = d2std(dNeedLeft, context.staffHeight, context.staffLines);
}

/* ------------------------------------------------------------------------ ArpWidthLR -- */
/* Return the distance the given arpeggio sign Graphic needs to the left and to
the right of the origin of the object it's attached to, to avoid overlap. */

static void ArpWidthLR(Document *doc, LINK arpL, short staff, STDIST *pNeedLeft,
							STDIST *pNeedRight)
{
	CONTEXT	context;
	DDIST	dNeedLeft=0, dNeedRight=0;

	GetContext(doc, arpL, staff, &context);
	if (LinkXD(arpL)<0) dNeedLeft = -LinkXD(arpL);
	else					  dNeedRight = LinkXD(arpL);
	
	*pNeedRight = d2std(dNeedRight, context.staffHeight, context.staffLines);
	*pNeedLeft = d2std(dNeedLeft, context.staffHeight, context.staffLines);
}

/* ---------------------------------------------------------------- SyncGraphicWidthLR -- */
/* Return the extreme distance any lyric or arpeggio sign attached to the given Sync
on the given staff needs to avoid overlap, both to the left and to the right, plus
extra space in between as specified in the config struct. */

void SyncGraphicWidthLR(Document *doc,
							LINK syncL,
							short staff,				/* Number of staff to consider */
							STDIST *pNeedLeft,
							STDIST *pNeedRight)
{
	short voice;  LINK pL, endL;  DDIST needLeft, needRight;
	
	*pNeedLeft = *pNeedRight = 0;
	voice = SyncVoiceOnStaff(syncL, staff);
	
	/*
	 *	Scan the entire system looking for lyrics attached to the Sync. This is
	 * probably overkill; just its measure should be enough.
	 */
	pL = LSSearch(syncL, SYSTEMtype, ANYONE, GO_LEFT, False);
	endL = LinkRSYS(pL);
	for ( ; pL!=endL; pL = RightLINK(pL)) {
		if (GraphicTYPE(pL))
				if (GraphicVOICE(pL)==voice && GraphicFIRSTOBJ(pL)==syncL) {
					if (GraphicSubType(pL)==GRLyric) {		
						LyricWidthLR(doc, pL, staff, &needLeft, &needRight);
						if (needLeft>*pNeedLeft) *pNeedLeft = needLeft;
						if (needRight>*pNeedRight) *pNeedRight = needRight;
					}
					else if (GraphicSubType(pL)==GRArpeggio) {		
						ArpWidthLR(doc, pL, staff, &needLeft, &needRight);
						if (needLeft>*pNeedLeft) *pNeedLeft = needLeft;
						if (needRight>*pNeedRight) *pNeedRight = needRight;
					}
			}
	}

	/*
	 *	If we need ANY space to the left, i.e., if there's a lyric extending to the
	 * left, add extra space, but only half the amount needed since if there's a lyric
	 *	extending to the right of the preceding sync, it'll add the other half. Likewise
	 *	on the other side (a lyric extending to the right). This won't work all the time
	 * but it certainly seems to work in typical real-life situations. 
	 */
	if (*pNeedLeft>0) *pNeedLeft += config.minLyricRSpace/2;
	if (*pNeedRight>0) *pNeedRight += config.minLyricRSpace/2;
}


/* config.spAfterBar is the exact normal position of the origin of the first object in
a measure; config.minSpAfterBar is the minimum space before the left edge of the
first object in a measure. With typical settings of spAfterBar, minSpAfterBar rarely
has any effect except for notes with accidentals and for downstemmed chords containing
seconds. */

#define STD2F(fd)	((fd)*FIDEAL_RESOLVE)	/* Convert STDIST to FIdealSpace units */
#define F2STD(fd)	((fd)/FIDEAL_RESOLVE)	/* Convert FIdealSpace units to STDIST */

/* ---------------------------------------------------------------------- IPGroupWidth -- */
/* Given a staff number, SPACETIMEINFO and fine "space before/after" arrays, and an
index into those arrays of a J_IP object, deliver the Fine STDIST width of a group of
J_IP objects that ends there. Also return (in *pnInGroup) the number of objects in the
group. */

static STDIST IPGroupWidth(short staff, short ind, SPACETIMEINFO spaceTimeInfo[],
									STDIST fSpBefore[], STDIST fSpAfter[], short *pnInGroup)
{
	STDIST	groupWidth;
	short	count, j;
	
	groupWidth = 0;
	count = 0;
	for (j = ind; j>=0; j--) {
	
		/* If this object is not J_IP, this is the end of the group. If it's J_IP but
		 *	not on this staff, the situation depends on whether we've found any J_IPs
		 *	on this staff yet: if so, we're at the end of the group; if not, we may
		 * not even have reached the beginning of the group yet.
		 */
		if (spaceTimeInfo[j].justType!=J_IP) break;
		if (!ObjOnStaff(spaceTimeInfo[j].link, staff, False)) {
			if (count>0)	break;
			else			continue;
		}

		groupWidth += fSpBefore[j];
		groupWidth += fSpAfter[j];
		count++;
	}
	
	*pnInGroup = count;
	return groupWidth;
}


/* --------------------------------------------------------------------- IPMarginWidth -- */
/* Given SPACETIMEINFO and indices into it for two objects, look at the second and
find its maximum Fine SymWidthLeft for any staff that the first is on. Intended for use
in positioning J_IP symbols, which are normally flush right against following J_IT
objects, or, where consecutive J_IP objects have a common staff, J_IP ones. */

static STDIST IPMarginWidth(Document *doc, SPACETIMEINFO spaceTimeInfo[],
								short iFirst, short iSecond)
{
	STDIST widthLeft; short s;
	
	widthLeft = 0;	
	for (s = 1; s<=doc->nstaves; s++) {
		if (ObjOnStaff(spaceTimeInfo[iFirst].link, s, False))
			widthLeft = n_max(widthLeft, SymWidthLeft(doc, spaceTimeInfo[iSecond].link, s, -1));
	}
	
	return STD2F(widthLeft);
}


/* --------------------------------------------------------------------- IPSpaceNeeded -- */
/* Given a staff number, SPACETIMEINFO, fine "space before/after", and fine postion
arrays, and an index into those arrays of a J_IP object, deliver the fine additional
space needed by a group of J_IP objects on that staff that ends with the index'th
object. */

static STDIST IPSpaceNeeded(
				Document *doc,
				short staff, short ind,
				SPACETIMEINFO spaceTimeInfo[],
				STDIST fSpBefore[],
				STDIST fSpAfter[],
				LONGSTDIST fPosition[] 		/* Position table for J_IT & J_IP objs. for the measure */
				)
{
	short prevIT, nInGroup;
	STDIST groupWidth, prevRightEnd, needRight, availSp, needLeft, spNeeded;

	groupWidth = IPGroupWidth(staff, ind-1, spaceTimeInfo, fSpBefore, fSpAfter, &nInGroup);
	if (nInGroup<=0) return 0;

	/*
	 * Find out how much space there already is in the slot the IP group goes into
	 * and compute how much more, if any, we need.
	 */
	prevIT = PrevITSym(doc, staff, ind, spaceTimeInfo);
	if (prevIT>=0) {
		needRight = SymWidthRight(doc, spaceTimeInfo[prevIT].link, staff, False);
		prevRightEnd = fPosition[prevIT]+STD2F(needRight);
	}
	else
		prevRightEnd = 0;
	availSp = fPosition[ind]-prevRightEnd;
	needLeft = IPMarginWidth(doc, spaceTimeInfo, ind-1, ind);	/* ??THE INDICES MAY BE WRONG */
	availSp -= needLeft;
	spNeeded = -(availSp-groupWidth);
	return spNeeded;
}


/* ------------------------------------------------------------------------- Utilities -- */

static void DebugPrintSpacing(short nLast, STDIST fSpBefore[]);
static void FillIgnoreChordTable(Document *doc, short nLast, SPACETIMEINFO spaceTimeInfo[]);

static void DebugPrintSpacing(short nLast, STDIST fSpBefore[])
{
	short k;
	LogPrintf(LOG_DEBUG, "fSpBefores:");
	for (k = 0; k<=nLast; k++)
		LogPrintf(LOG_DEBUG, " %5d", fSpBefore[k]);
	LogPrintf(LOG_DEBUG, "\n");
}


/* Fill in a table of voice nos. of note/chords the user doesn't want to affect spacing. */
static void FillIgnoreChordTable(Document *doc, short nLast, SPACETIMEINFO spaceTimeInfo[])
{
	short	i, v;
	LINK	aNoteL;
	PANOTE	aNote;

	for (i = 0; i<=nLast; i++) {
		for (v = 1; v<=MAXVOICES; v++) {
			ignoreChord[i][v] = False;
			
			if (SyncTYPE(spaceTimeInfo[i].link)) {
				aNoteL = FindMainNote(spaceTimeInfo[i].link, v);
				if (aNoteL!=NILINK) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->rspIgnore!=0) ignoreChord[i][v] = True;
if (aNote->rspIgnore!=0) LogPrintf(LOG_DEBUG, "FillIgnoreChordTable: i=%d v=%d: rspIgnore=%hd ignoreChord[]=%d\n",
	i, v, aNote->rspIgnore, ignoreChord[i][v]);
				}
			} 

		}
	}
}


/* -------------------------------------------------------------------- ConsidITWidths -- */
/* Consider widths of all J_IT objects (syncs, space objects, etc.) and adjust
spacing accordingly. The effect of lyric-subtype and arpeggio-sign-subtype
Graphics, the only J_D symbols that can affect spacing, are also considered here.
Handles one measure. */

static void ConsidITWidths(
					Document *doc,
					LINK /*barTermL*/,					/* Object ending the Measure */
					short nLast,
					SPACETIMEINFO spaceTimeInfo[],
					STDIST fSpBefore[]					/* Space-before table: only entries for J_IT objs are used */
					)
{
	register short i, s, t;
	STDIST needLeft, prevNeedRight[MAXSTAVES+1], gNeedLeft, gNeedRight;
	LONGSTDIST fSpNeeded, fAvailSp[MAXSTAVES+1];
	Boolean staffUsed[MAXSTAVES+1];
	
	//FillIgnoreChordTable(doc, nLast, spaceTimeInfo);

	/*
	 *	For each J_IT object, go thru each staff and update spacing table entries for
	 *  the objects to reflect any additional space they need on that staff. (This is
	 *  overly simple-minded, especially for Syncs. It will give bad results when two
	 *	voices share a staff and, say, one has dotted quarters while the other has
	 *	continuous 16ths. We could do this by voice instead of by staff, but that
	 *  would cause a different set of problems. What's really needed is an image-
	 *  space representation, as discussed in Chapter 5 of my dissertation, but
	 *  implementing that would be a major undertaking.)
	 */
	for (s = 1; s<=doc->nstaves; s++) {
		prevNeedRight[s] = 0;
		fAvailSp[s] = 0;
	}
	
	for (i = 0; i<=nLast; i++) {
		if (spaceTimeInfo[i].justType==J_IT) {
			for (s = 1; s<=doc->nstaves; s++) {
				fAvailSp[s] += fSpBefore[i];			/* Accumulate space avail. on all staves */
				staffUsed[s] = False;
			}
			
			for (s = 1; s<=doc->nstaves; s++) {

#ifdef SPACEBUG
	if (DEBUG_PRINT) {
		LogPrintf(LOG_DEBUG, "CIT2. "); DebugPrintSpacing(nLast, fSpBefore);
	}
#endif
				if (ObjOnStaff(spaceTimeInfo[i].link, s, False)) {
					if (SyncTYPE(spaceTimeInfo[i].link))
						SyncGraphicWidthLR(doc, spaceTimeInfo[i].link, s, &gNeedLeft, &gNeedRight);
					else
						gNeedLeft = gNeedRight = 0;
					needLeft = SymWidthLeft(doc, spaceTimeInfo[i].link, s, -1);
					if (needLeft<gNeedLeft) needLeft = gNeedLeft;

					/*
					 *	First set fSpNeeded to an amount just enough to prevent this object
					 *	overlapping the previous one, then increase it by the required
					 *	minimum space (which is different for the first and last symbols to
					 * allow more space around the barline).
					 */
					fSpNeeded = STD2F(prevNeedRight[s]+needLeft)-fAvailSp[s];
					if (i==0) fSpNeeded += STD2F(config.minSpAfterBar);
					else if (i==nLast) fSpNeeded += STD2F(config.minSpBeforeBar);
					else fSpNeeded += STD2F(config.minRSpace);

					if (fSpNeeded>0) {
						/*
						 * We need more space here. Move this object to the right, and
						 * increase the space available on all staves by the same amount.
						 */
						fSpBefore[i] += fSpNeeded;
						for (t = 1;t<=doc->nstaves; t++)
							fAvailSp[t] += fSpNeeded;
					}
					
					/*
					 *	Re-initialize for the next J_IT object on this staff.
					 */
					prevNeedRight[s] = SymWidthRight(doc, spaceTimeInfo[i].link, s, False);	/* Incl. stuff fllwng notehead */
					if (prevNeedRight[s]<gNeedRight) prevNeedRight[s] = gNeedRight;
					staffUsed[s] = True;
				}
			}
			
			/*
			 *	The appearance of any staff in this obj cuts off any accumulated space
			 * from being usable for following objs on that staff.
			 */
			for (s = 1; s<=doc->nstaves; s++)
				if (staffUsed[s]) fAvailSp[s] = 0;
		}
	}
}


/* -------------------------------------------------------------------- ConsidIPWidths -- */
/*	Consider widths of all J_IP objects (clefs, key signatures, time signatures, grace
syncs, etc.) and adjust spacing accordingly. Handles one measure. */

static void ConsidIPWidths(
				register Document *doc,
				LINK			/*barTermL*/,		/* Object ending the Measure */
				short			nLast,
				SPACETIMEINFO	spaceTimeInfo[],
				STDIST			fSpBefore[],		/* Space-before table: entries for J_IT & J_IP objs are used */
				LONGSTDIST		position[] 			/* Position table for J_IT & J_IP objs. for the measure */
				)
{
	register short i;
	short s, j;
	STDIST spNeeded, xpos, fSpNeeded;
	STDIST fSpAfter[MAX_MEASNODES];	/* Fine STDIST space-after table for J_IP objs. for the measure */
	
	/*
	 * Go thru the measure, considering all staves simultaneously, and, for each J_IP
	 * object, compute the space needed before and after it. Then go thru the measure
	 *	for each staff, looking at each J_IT object, and adjust the position for that
	 *	and all following objects to reflect any further space J_IP objects preceding it
	 *	need on that staff. Finally, position J_IP objects just to the left of following
	 * J_IT objects. (Actually, as in Respace1Bar, we treat J_STRUC objects like J_ITs,
	 *	since the measure may be ended by a System or Page, and we need a position for
	 *	the measure-ending object so we can compute locations of symbols before it.)
	 *
	 *	The theory behind this is as follows: In a group of contiguous J_IP objects on a
	 *	staff, the last one is right-justified against the following J_IT object, and the
	 *	others huddle as close to it as possible. (There is always a following J_IT or
	 *	J_STRUC object--the object ending the measure, if nothing else.) Note that this
	 *	can result in positioning several J_IP symbols (in practice, very likely grace
	 *	notes) to the left of a J_IT symbol that precedes all of them in the object list!
	 *	That's the way CMN works: horizontal positions of symbols are not entirely a
	 *	monotonic function of their logical order.
	 */

	/*	Before starting, convert distances between J_IT symbols to Fine STDIST positions. */
	
	for (xpos = 0, i = 0; i<=nLast; i++) {
		xpos += fSpBefore[i];
		position[i] = xpos;
	}
	
	/*
	 *	Get space needed before and after each J_IP symbol on all staves. (We assume
	 *	that, if object 0 is J_IP, its fSpBefore has already been set to the normal
	 *	value.)
	 */
	for (i = 0; i<=nLast; i++)
		if (spaceTimeInfo[i].justType==J_IP) {
			spNeeded = SymWidthLeft(doc, spaceTimeInfo[i].link, ANYONE, -1);
			if (i==0) {
				fSpNeeded = STD2F(spNeeded+config.minSpAfterBar);
				if (fSpBefore[0]-fSpNeeded<0)
					 fSpBefore[0] = fSpNeeded;
			}
			else {
				spNeeded += config.minRSpace;
				fSpBefore[i] = STD2F(spNeeded);
			}
			spNeeded = SymWidthRight(doc, spaceTimeInfo[i].link, ANYONE, False);

			/* Key sig. or time sig. followed by Sync or grace Sync has special min. space. */
			
			if (i<nLast
			&& (KeySigTYPE(spaceTimeInfo[i].link) || TimeSigTYPE(spaceTimeInfo[i].link))
			&& (SyncTYPE(spaceTimeInfo[i+1].link) || GRSyncTYPE(spaceTimeInfo[i+1].link)))
				spNeeded += config.minRSpace_KSTS_N;
				
			fSpAfter[i] = STD2F(spNeeded);					
		}

#ifdef SPACEBUG
	if (DEBUG_PRINT) {
		LogPrintf(LOG_DEBUG, "Final "); DebugPrintSpacing(nLast, fSpBefore);
	}
#endif

	/* Move J_IT symbols where needed to leave room for preceding J_IP symbols. */
	
	for (s = 1; s<=doc->nstaves; s++)
		for (i = 1; i<=nLast; i++) {
			if ((spaceTimeInfo[i].justType==J_IT || spaceTimeInfo[i].justType==J_STRUC)
			&&   spaceTimeInfo[i-1].justType==J_IP) {
				spNeeded = IPSpaceNeeded(doc, s, i, spaceTimeInfo, fSpBefore, fSpAfter,
											position);
				if (spNeeded>0)
					for (j = i; j<=nLast; j++)
						position[j] += spNeeded;
			}
		}

	/* Finally, position the J_IP symbols. If two consecutive (in the object list)
	 *	J_IP objects have any staves in common, they must be positioned side-by-side,
	 *	otherwise they can overlap totally.
	 */
	for (i = nLast-1; i>=0; i--)
		if (spaceTimeInfo[i].justType==J_IP) {
			short iRef, iLeftmost; LONGSTDIST posRef;
			
			iRef = i+1;
			posRef = position[iRef];
			position[i] = posRef-IPMarginWidth(doc, spaceTimeInfo, i, iRef)-fSpAfter[i];
			iLeftmost = i;
			
			while (--i>=0 && spaceTimeInfo[i].justType==J_IP) {
				if (CommonStaff(doc, spaceTimeInfo[i].link, spaceTimeInfo[i+1].link)
						!=NOONE) {
					iRef = iLeftmost;
					posRef = position[iRef];
				}
				
				position[i] = posRef-IPMarginWidth(doc, spaceTimeInfo, i, iRef)-fSpAfter[i];
				if (position[i]<position[iLeftmost]) iLeftmost = i;
			}
		}

	/* And convert fine STDISTs back to normal ones. */
		
	for (i = 0; i<=nLast; i++) {
		position[i] = F2STD(position[i]);
	}
}


#ifdef NOTYET

#define SpaceSPWIDTH(link)	( (GetPSPACER(link))->spWidth )

/* -------------------------------------------------------------------- ConsidSPWidths -- */
/*	Consider widths of all J_SP objects (only Spacers) and adjust spacing accordingly.
Handles one measure. */

static void ConsidSPWidths(
				Document	*doc,
				LINK		barTermL,			/* Object ending the Measure */
				short		nLast,
				SPACETIMEINFO spaceTimeInfo[],
				LONGSTDIST	position[] 			/* Position table for J_IT & J_IP objs. for the measure */
				)
{
	register short i, j;
	STDIST	spWidth;
	LINK	spaceL;

	/*
	 * Go thru the measure and, for each J_SP object, leave space for that object and
	 *	move everything following to the right by its space.
	 */
	for (i = 0; i<=nLast; i++)
		if (spaceTimeInfo[i].justType==J_SP) {
			spaceL = spaceTimeInfo[i].link;
			spWidth = SpaceSPWIDTH(spaceL);
			
			for (j=i+1; j<=nLast; j++)
				position[j] += spWidth;
		}
}

#endif


/* -------------------------------------------------------------------- ConsiderWidths -- */
/* Consider objects' widths (both to left and right of their origins) and increase
spacing as needed, not only to avoid overprinting, but to insure a separation of
config.minRSpace between objects. First, handle all J_IT objects (syncs and
space objects); then handle all J_IP objects (clefs, key signatures, time sig-
natures, grace syncs, etc.), since these objects have no effect on spacing if
there's room for them between notes and rests. The only J_D objects that can
affect spacing--some subtypes of Graphics attached to syncs--are handled with the
syncs they're attached to. ConsiderWidths handles one measure per call. */

static void ConsiderWidths(
					register Document *doc,
					LINK			barTermL,		/* Object ending the Measure */
					short			nLast,
					SPACETIMEINFO	spaceTimeInfo[],
					STDIST			fSpBefore[],
					LONGSTDIST		position[]		/* Position table for J_IT & J_IP objs. for the measure */
					)
{
	ConsidITWidths(doc, barTermL, nLast, spaceTimeInfo, fSpBefore);

#ifdef SPACEBUG
	if (DEBUG_PRINT) {
		LogPrintf(LOG_DEBUG, "Inter."); DebugPrintSpacing(nLast, fSpBefore);
	}
#endif
	
	ConsidIPWidths(doc, barTermL, nLast, spaceTimeInfo, fSpBefore, position);
	
	/*
	 *	NB: At this point, we should have config.minRSpace between the right end of
	 *	each object and the left end of the next whenever the two share any staves;
	 *	we really should also guarantee at least that much space between the left ends
	 *	of consecutive objects that don't share staves, or maybe we should do this
	 *	only for IT objects. Either way, it could easily (I think) be done here...
	 */
	 
#ifdef NOTYET
	ConsidSPWidths(doc, barTermL, nLast, spaceTimeInfo, position);
#endif
}


/* ----------------------------------------------------------------------- Respace1Bar -- */
/* Perform global punctuation in the specified measure by fixing up object x-
coordinates (xd's) so that each is allowed the "correct" space. For principles
of operation, see Donald Byrd's dissertation, Sec. 4.6, and John Gourlay's
"Spacing a Line of Music" (Tech. Report TR-35, Ohio State Univ. Computer &
Information Science Res. Ctr., 1987). What we actually do is essentially Gourlay's
algorithm with these changes:
	Blocking widths are computed by staff instead of by voice (though by voice would
		generally be better when there's any difference between them).
	A table for the ideal spacings of basic durations instead of his system of built-in
		values automatically adjusted for the neighborhood.
	Much more attention to positioning J_IP symbols (clefs, grace notes, etc.).

Return value is the measure's new width, or 0 if it exceeds Respace1Bar's limit
of MAX_MEASNODES J_IT and J_IP objects. */

#define EMPTYMEAS_WIDTH 2*STD_LINEHT		/* STDIST width to use for measures with no J_IT or IP symbols */

DDIST Respace1Bar(
			Document		*doc,
			LINK			barTermL,			/* Object ending the Measure */
			short			nLast,
			SPACETIMEINFO	spaceTimeInfo[],
			long			spaceProp 			/* Use spaceProp/(RESFACTOR*100) of normal spacing */
			)
{
	LINK		pL, prevBarL;
	short		i, prevIT,
				fIdealSp; 						/* Multiple of STDIST/FIDEAL_RESOLVE */
	STDIST		prevBarWidth,
				fSpBefore[MAX_MEASNODES];		/* Fine STDIST space-before table for J_IT & J_IP objs. for the measure */
	DDIST		minWidth, currentxd;
	LONGSTDIST	position[MAX_MEASNODES];		/* Position table for J_IT & J_IP objs. for the measure */

	if (nLast>=MAX_MEASNODES) return 0;
	
	/*
	 *	Allow for the right width of the preceding barline if it exists and is in the
	 *	same System as this Measure.
	 */
	prevBarL = SSearch(LeftLINK(barTermL), MEASUREtype, GO_LEFT);
	if (!prevBarL || !SameSystem(prevBarL, LeftLINK(barTermL)))
		prevBarWidth = 0;
	else
		prevBarWidth = SymWidthRight(doc, prevBarL, ANYONE, False);
		
	minWidth = std2d(prevBarWidth+config.spAfterBar+EMPTYMEAS_WIDTH, STFHEIGHT, STFLINES);

	/*
	 *	If the Measure contains no J_IT or IP symbols, give it the minimum width.
	 */
	if (nLast<=0) return minWidth;

	/*
	 *	Initialize the spacing table with "ideal" values based solely on durations plus
	 *	a predetermined space after the barline at the beginning the measure. Specifically,
	 * set fSpBefore[0] to a value based on the type of the first object in the measure
	 * to produce the predetermined space. Other than that, set fSpBefore[] for every
	 * J_IT object to our table-driven version of Gourlay's function of the elapsed
	 *	duration since the previous J_IT object, and set all other fSpBefore[] to 0.
	 *	(Actually, we also get fSpBefore[] for J_STRUC objs, since the measure may be
	 *	ended by a System or Page, and we need a position for the measure-ending object.)
	 */

	fSpBefore[0] = STD2F(prevBarWidth);
	if (SyncTYPE(spaceTimeInfo[0].link))	fSpBefore[0] += STD2F(config.spAfterBar);
	else									fSpBefore[0] += STD2F(config.minSpAfterBar);
	
	for (i = 1; i<=nLast; i++) {
		if (spaceTimeInfo[i].justType==J_IT || spaceTimeInfo[i].justType==J_STRUC) {
			prevIT = PrevITSym(doc, ANYONE, i, spaceTimeInfo);
			if (prevIT>=0) {
				fIdealSp = FIdealSpace(doc, spaceTimeInfo[prevIT].dur, spaceProp);
				fSpBefore[i] = spaceTimeInfo[prevIT].frac*fIdealSp;
			}
			else
				fSpBefore[i] = 0; 
		}
		else
			fSpBefore[i] = 0;
			
		if (fSpBefore[i]<0)
			MayErrMsg("Respace1Bar: node %d at %ld has negative ideal fSpBefore=%ld. frac=%f ideal=%d",
				i, (long)spaceTimeInfo[i].link, (long)fSpBefore[i], spaceTimeInfo[prevIT].frac,
					fIdealSp);
	}

#ifdef SPACEBUG
	if (DEBUG_PRINT) {
		short k;
		LogPrintf(LOG_DEBUG,   "Nodes: ---------");
		for (k = 0; k<=nLast; k++)
			LogPrintf(LOG_DEBUG, " %5d", spaceTimeInfo[k].link);
		LogPrintf(LOG_DEBUG, "\nTypes:          ");
		for (k = 0; k<=nLast; k++)
			LogPrintf(LOG_DEBUG, "  %4.4s", NameObjType(spaceTimeInfo[k].link));
		LogPrintf(LOG_DEBUG, "\nTimes:          ");
		for (k = 0; k<=nLast; k++)
			LogPrintf(LOG_DEBUG, " %5ld", spaceTimeInfo[k].startTime);
		LogPrintf(LOG_DEBUG, "\nIdeal fSpBefores:");
		for (k = 0; k<=nLast; k++)
			LogPrintf(LOG_DEBUG, " %5d", fSpBefore[k]);
		LogPrintf(LOG_DEBUG, "\n");
	}
#endif

	/*
	 *	Consider objects' widths (both to left and right of their origins) and
	 *	increase spacing as needed to avoid overprinting. (People doing proportional
	 * notation might prefer not to do this: it would be nice to make it an option.)
	 */
	ConsiderWidths(doc, barTermL, nLast, spaceTimeInfo, fSpBefore, position);

#ifdef SPACEBUG
	if (DEBUG_PRINT) {
		LogPrintf(LOG_DEBUG, "Final positions:");
		for (i = 0; i<=nLast; i++)
			LogPrintf(LOG_DEBUG, " %5ld", position[i]);
		LogPrintf(LOG_DEBUG, "\n");
	}
#endif
	
	/*
	 *	Finally, go thru object list for the measure and fill in xd's from the values
	 *	in the position table.
	 */
	for (i = 0; i<=nLast; i++) {
		pL = spaceTimeInfo[i].link;
		currentxd = std2d(position[i], STFHEIGHT, STFLINES);
		if (pL!=barTermL && HasValidxd(pL)) {
			LinkXD(pL) = currentxd;
			LinkVALID(pL) = False;							/* Make draw routines recompute objRect */
		}
	}
	
	if (currentxd<minWidth) currentxd = minWidth;

	return currentxd;
}


/* ----------------------------------------------------------------------- GetJustProp -- */
/* Given an RMEASDATA table and first and last indices into it that refer to the
first and last Measures of a single System, return a "justification proportion"
in (100L*RESFACTOR)ths by which to multiply coordinates to squeeze the Measures
into the System's width. If it doesn't need squeezing, return -1L. */

long GetJustProp(Document *doc, RMEASDATA rmTable[], short first, short last,
						CONTEXT context)
{
	LINK		systemL;
	PSYSTEM		pSystem;
	DDIST		lastMeasWidth;
	FASTFLOAT	justFact, inSysWidth, sysWidth, curWidth;
	long		justProp;
	
	/* Get distance from first (invisible) barline to end of System */
	systemL = MeasSYSL(rmTable[last].measL);
	pSystem = GetPSYSTEM(systemL);
	sysWidth = pSystem->systemRect.right - pSystem->systemRect.left;
	lastMeasWidth = MeasJustWidth(doc, rmTable[last].measL, context);
	if (rmTable[last].lxd+lastMeasWidth>=sysWidth) {
		inSysWidth = sysWidth - rmTable[first].lxd;
		
		/* Get distance from first (invisible) barline to last barline */
		curWidth = (rmTable[last].lxd + lastMeasWidth) - rmTable[first].lxd;
		
		/* Get justification proportion */
		justFact = inSysWidth/curWidth;
		justProp = (long)(justFact*100L*RESFACTOR);
		return justProp;
	}
	else
		return -1L;
}


/* ---------------------------------------------------------------- PositionSysByTable -- */
/* Given an RMEASDATA table, first and last indices into it that refer to the first
and last Measures of a single System, and an optional "justification proportion",
fix up measures first thru last inclusive. If the justification proportion is given,
stretch or squeeze accordingly; in any case, move the Measures by setting their
xd's and widths according to the table, and set the Measures' spacePercent. */

void PositionSysByTable(
			Document *doc,
			RMEASDATA *rmTable,
			short first, short last,
			long spaceProp,			/* If >0, use spaceProp/(RESFACTOR*100) of normal spacing */
			CONTEXT context
			)
{
	short m;
	long temp, startChange, resUnit, newProp;
	LINK pL, endMeasL, startL, endL, nextMeasL, measL;
	DDIST lastMeasWidth, staffWidth;
	FASTFLOAT curSpaceFact;
	
	if (spaceProp>0L) {
		resUnit = 100L*RESFACTOR;
		startChange = LinkXD(rmTable[first].measL);
		
		for (m = first; m<=last; m++) {
			/* Adjust the Measure's position w/r/t first barline (startChange) */
			temp = spaceProp * (rmTable[m].lxd - startChange);
			LinkXD(rmTable[m].measL) = (temp/resUnit) + startChange;
			
			/* New space compression is product of spaceProp and old compression */
			curSpaceFact = (FASTFLOAT)MeasSpaceProp(rmTable[m].measL)/resUnit;
			newProp = (long)(curSpaceFact*spaceProp);
			SetMeasSpacePercent(rmTable[m].measL, newProp);
			
			/* Stretch or squeeze coordinates of all objects within the Measure */
			pL = RightLINK(rmTable[m].measL);
			endMeasL = EndMeasSearch(doc, rmTable[m].measL);
			for ( ; pL!=endMeasL; pL = RightLINK(pL)) {
				temp  = spaceProp*(long)LinkXD(pL);
				LinkXD(pL) = temp/resUnit;
			}
		}
	}
	else
		for (m = first; m<=last; m++)
			LinkXD(rmTable[m].measL) = rmTable[m].lxd;
			
	startL = rmTable[first].measL;
	endL = rmTable[last].measL;

	/*
	 * If there's more than one Measure in the system, set the last one's width, based
	 * on the estimated space requirements of its contents.
	 */
	if (spaceProp>0 && endL!=startL) {
		lastMeasWidth = MeasJustWidth(doc, rmTable[last].measL, context);
		staffWidth = context.staffRight - context.staffLeft;
		LinkXD(endL) = staffWidth - lastMeasWidth;
	}
	
	/*
	 *	Now go through each Measure (from pL upto nextMeasL) and ensure that its
	 *	subobject measureRects are in sync with Measures set above. This loop
	 *	excludes the final Measure of the System.
	 */
	for (measL=startL; measL!=endL; measL=nextMeasL) {
		nextMeasL = LinkRMEAS(measL);
		SetMeasWidth(measL, LinkXD(nextMeasL) - LinkXD(measL));
	}
	
	/* Set the subobj rects of the last Measure to go to the end of the System. */
	
	SetMeasFillSystem(endL);
}


/* =========================================================== RespaceBars and helpers == */

static void		GetRespaceParams(Document *, LINK, LINK, LINK *, LINK *, LINK *, long *);
static short	CountRespMeas(Document *, LINK, LINK);
static short	RespWithinMeasures(Document *, LINK, LINK, LINK, LINK, long, RMEASDATA *,
												XDDATA *, unsigned short);
static Boolean	PositionWholeMeasures(Document *, short, RMEASDATA *,  LINK, LINK, LINK *,
										Boolean, Boolean);
static void	InvalRespBars(LINK, LINK);


static void GetRespaceParams(
				Document *doc,
				LINK startBarL,
				LINK endBarL,			/* Obj ending the last Measure, but may not be a Measure */
				LINK *startSysBarL,		/* Always a Measure */
				LINK *endSysBarL,		/* First Measure after the last System or tail */
				LINK *endSysL,			/* Obj ending the last System */
				long *spaceProp
					)
{
	LINK startSysL;
	
	startSysL = LSSearch(startBarL, SYSTEMtype, ANYONE, GO_LEFT, False);
	*startSysBarL = LSSearch(startSysL, MEASUREtype, ANYONE, GO_RIGHT, False);
	*endSysL = EndSystemSearch(doc, LeftLINK(endBarL));							/* May be tail */
	*endSysBarL = LSSearch(*endSysL, MEASUREtype, ANYONE, GO_RIGHT, False);		/* May be NILINK */
	if (*endSysBarL==NILINK) *endSysBarL = doc->tailL;
	
	if (*spaceProp<=0) *spaceProp = MeasSpaceProp(startBarL);
}

static short CountRespMeas(
					Document */*doc*/,
					LINK startMeasL,	/* First Measure to count, last Measure to count or tail */
					LINK endMeasL
					)
{
	short mindex=0;  LINK measL;
	char fmtStr[256];

	measL = startMeasL;
	for ( ; measL && measL!=LinkRMEAS(endMeasL); measL=LinkRMEAS(measL)) mindex++;

	if (mindex>=MAX_MEASURES) {
		GetIndCString(fmtStr, MISCERRS_STRS, 8);		/* "Tried to respace %d measures at once, but..." */
		sprintf(strBuf, fmtStr, mindex, MAX_MEASURES);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	
	return mindex;
}


static Boolean GetXDTabIndex(LINK pL, XDDATA *xdTable, unsigned short tabLen, unsigned short *id)
{
	unsigned short i;

	for (i = 0; i < tabLen; i++)
		if (xdTable[i].link == pL) {
			*id = i;
			return True;
		}
	return False;
}


#define HAIRPIN_STD_MINLEN (3*STD_LINEHT/2)
#define HAIRPIN_OFFSET_THRESHOLD  STD_LINEHT/2		// just for now

/* If any hairpins in the given range are below the minimum length, move their right
ends to make them the minimum length. Return the number of hairpins changed. */

static short FixHairpinLengths(Document *doc, LINK /*measL*/, LINK firstL, LINK lastL,
								DDIST oldMWidth, DDIST newMWidth, XDDATA *xdTable, unsigned short);
static short FixHairpinLengths(
		Document		*doc,
		LINK			/*measL*/,
		LINK			firstL,
		LINK			lastL,
		DDIST			oldMWidth,
		DDIST			newMWidth,
		XDDATA			*xdTable,
		unsigned short	xdTabLen)
{
	LINK			pL, aDynamicL, firstSyncL, lastSyncL;
	short			nChanged = 0;
	DDIST			dHairpinMinLen, xd, endxd, oldWid, newWid;
	DDIST			dynamXD, oldFirstSyncXD, newFirstSyncXD, oldLastSyncXD, newLastSyncXD;
	FASTFLOAT		oldFrac;
	unsigned short	id;

	dHairpinMinLen = std2d(HAIRPIN_STD_MINLEN, STFHEIGHT, STFLINES);

	for (pL = firstL; pL!=lastL; pL = RightLINK(pL)) {
		if (DynamTYPE(pL) && IsHairpin(pL)) {
			aDynamicL = FirstSubLINK(pL);
			dynamXD = LinkXD(pL) + DynamicXD(aDynamicL);
			if (ABS(dynamXD) > HAIRPIN_OFFSET_THRESHOLD) {
				firstSyncL = DynamFIRSTSYNC(pL);
				if (!GetXDTabIndex(firstSyncL, xdTable, xdTabLen, &id)) {
					// handle error
#ifndef PUBLIC_VERSION
say("GetXDTabIndex failed for firstSyncL\n");
#endif
					break;
				}
				oldFirstSyncXD = xdTable[id].xd;
				newFirstSyncXD = LinkXD(firstSyncL);
				lastSyncL = DynamLASTSYNC(pL);
				if (SameMeasure(firstSyncL, lastSyncL)) {
					if (!GetXDTabIndex(lastSyncL, xdTable, xdTabLen, &id)) {
						// handle error
#ifndef PUBLIC_VERSION
say("GetXDTabIndex failed for lastSyncL\n");
#endif
						break;
					}
					oldLastSyncXD = xdTable[id].xd;
					newLastSyncXD = LinkXD(lastSyncL);
				}
				else {
					oldLastSyncXD = oldMWidth;		/* position of terminating barline rel. to prev. barline */
					newLastSyncXD = newMWidth;
				}
				oldWid = oldLastSyncXD - oldFirstSyncXD;
				newWid = newLastSyncXD - newFirstSyncXD;
				if (newWid != oldWid) {
					oldFrac = (FASTFLOAT)dynamXD / (FASTFLOAT)oldWid;
					LinkXD(pL) = dynamXD = (DDIST)(oldFrac * newWid);
					nChanged++;
				}
#ifndef PUBLIC_VERSION
say("old1stXD=%d, new1stXD=%d, oldLstXD=%d, newLstXD=%d, oldWid=%d, newWid=%d, dynamXD=%d, oldFrac=%f\n",
oldFirstSyncXD, newFirstSyncXD, oldLastSyncXD, newLastSyncXD, oldWid, newWid, dynamXD, oldFrac);
#endif
			}
			endxd = DynamicENDXD(aDynamicL);
			if (ABS(endxd) > HAIRPIN_OFFSET_THRESHOLD) {
// stretch somehow FIXME: Do it!
			}
			/* Make sure hairpin is large enough to be selectable. */
			xd = SysRelxd(DynamFIRSTSYNC(pL)) + dynamXD;
			endxd += SysRelxd(DynamLASTSYNC(pL));
			if (endxd - xd < dHairpinMinLen) {
				DDIST moveEndxd = dHairpinMinLen - (endxd - xd);
				DynamicENDXD(aDynamicL) += moveEndxd;
#ifndef PUBLIC_VERSION
say("Moving dynamic endxd by %d\n", moveEndxd);
#endif
			}
		}
	}

	return nChanged;
}


/* Respace within measures. Go thru the area from the beginning of the System where the
selection starts to the end of the System where it ends, one Measure at a time. For each,
add its LINK to the rmTable and, if it's in the selection and not empty, build the
Measure's rhythmic "spine" and respace it. Finally, add the new widths of all Measures
in the area, whether in the selection or not and whether empty or not, to the rmTable
array. We need the rmTable to extend to the System boundaries for the benefit of
PositionWholeMeasures. */

static short RespWithinMeasures(
		Document *doc,
		LINK startBarL, LINK endBarL,
		LINK startSysBarL, LINK endSysBarL,
		long spaceProp,
		RMEASDATA *rmTable,
		XDDATA *xdTable,
		unsigned short xdTabLen)
{
	short mindex, inSel, nLast;
	DDIST oldMWidth, newMWidth; SPACETIMEINFO *spTimeInfo=0L;
	register LINK measL, firstL, lastL;		/* 1st obj in, obj ending current Measure */

	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) return -1;

	mindex = 0;
	inSel = False;
	measL = startSysBarL;
	for ( ; measL && measL!=endSysBarL; mindex++,measL=LinkRMEAS(measL)) {
		firstL = RightLINK(measL);
		lastL = EndMeasSearch(doc, measL);		/* Start from firstL fails if Measure empty */
		if (measL==startBarL) inSel = True;
		
		if (inSel) {
			oldMWidth = MeasWidth(measL);
			nLast = GetSpTimeInfo(doc, firstL, lastL, spTimeInfo, True); 		
			newMWidth = Respace1Bar(doc, lastL, nLast, spTimeInfo, spaceProp);
			SetMeasSpacePercent(measL, spaceProp);
			CenterWholeMeasRests(doc, firstL, lastL, newMWidth);

			/* ??This is a lot like antikinking--give this a more general name, at least? */
			(void)FixHairpinLengths(doc, measL, firstL, lastL, oldMWidth, newMWidth,
											xdTable, xdTabLen);
		}
		else
			newMWidth = MeasWidth(measL);
		
		rmTable[mindex].measL = measL;
		rmTable[mindex].width = newMWidth;

		if (lastL==endBarL) inSel = False;
	}
	
	DisposePtr((Ptr)spTimeInfo);
	return mindex;
}


/* Position Measures as a whole. We can't just move the respaced Measures into their
new positions, since they might overflow their Systems, so we do this: First, using the
width information we now have in rmTable, get the new positions for all Measures into
rmTable. The first Measure respaced stays where it is, and the first Measure of every
System stays where it is. Then, for each System, see if it overflows; if so, either
reformat (if the user allows), or just linearly compress it into the available space
without worrying about overlapping symbols that might result. */
 
static Boolean PositionWholeMeasures(
		Document *doc,
		short mCount,
		RMEASDATA *rmTable,
		LINK startBarL,
		LINK endSysL,
		LINK *invalStartL,
		Boolean command,
		Boolean doRfmt
		)
{
	LINK		startInvalL;
	register short m;
	short		first; 
	Boolean		reformat, goAhead;
	CONTEXT		context;
	long		newSpProp;

	startInvalL = startBarL;
	rmTable[0].lxd = LinkXD(rmTable[0].measL);
	for (m = 1; m<mCount; m++) {
		if (FirstMeasInSys(rmTable[m].measL))
			rmTable[m].lxd = LinkXD(rmTable[m].measL);
		else
			rmTable[m].lxd = rmTable[m-1].lxd+rmTable[m-1].width;
	}

	first = 0;
	for (reformat = False, m = 0; m<mCount; m++) {
		if (FirstMeasInSys(rmTable[m].measL))
			first = m;
		if (LastMeasInSys(rmTable[m].measL)) {
			GetContext(doc, rmTable[first].measL, 1, &context);
			newSpProp = GetJustProp(doc, rmTable, first, m, context);
			
			/*
			 *	If we're handling an explicit Respace command, or if <doRfmt>, just
			 *	set a flag and reformat the whole area later.
			 */
			if (command || doRfmt) {
				PositionSysByTable(doc, rmTable, first, m, 0L, context);
				if (newSpProp>0L) reformat = True;
			}
			else
				PositionSysByTable(doc, rmTable, first, m, newSpProp, context);
			
			/*
			 *	If this is the first System of the range and we're squeezing it,
			 *	redraw Measures from the beginning of the System to the beginning
			 * of the respaced area.
			 */
			if (first==0 && newSpProp>0L && !command)
				startInvalL = rmTable[0].measL;
		}
	}

	if (reformat) {
		if (doRfmt)
			goAhead = True;
		else {
			goAhead = (NoteAdvise(RSP_OVERFLOW_ALRT)==OK);
		}
		
		if (goAhead) {
			Reformat(doc, startBarL, LeftLINK(endSysL), True, 9999, False, 999,
							config.titleMargin);
		}
		else
			return False;
	}
	
	*invalStartL = startInvalL;
	return True;
}


/* It looks as if we only need to Inval to the end of the range we've moved; however,
(1) it seems safer to Inval to the end of the last System, and (2) the range moved
will almost always go to the end of the System anyway. As evidence for point (1),
RespaceForCut seems to move Measure(s) to the end of the System before calling
us. It should take care of this itself, but we'll play it safe and Inval to the
end of the System. */

static void InvalRespBars(LINK startInvalL, LINK endSysL)
{
	InvalRange(startInvalL, endSysL);
	InvalMeasures(startInvalL, endSysL, ANYONE);					/* Force redrawing */
}


/* ----------------------------------------------------------------------- RespaceBars -- */
/* RespaceBars performs global punctuation in the area specified by fixing up
object x-coordinates so that each is allowed the "correct" space. The area
extends from the beginning of the Measure including <startL> to the end of the
Measure including <endL>. If the operation results in any System overflowing,
what we do depends on the <command> and <doRfmt> parameters:
	If <command>, ask whether to reformat or cancel the entire command. (If we're
		handling an explicit command to Respace, responding to Systems overflowing
		by squeezing their contents back in makes no sense.)
	Otherwise, if <doRfmt>, reformat without asking.
	Otherwise, squeeze the Measures to fit their System.

Naturally, this is a user-interface level function.

N.B. RespaceBars acts as if barlines (the graphic manifestation of Measures)
TERMINATE Measures rather than beginning them, so if there's an insertion point
"at" a barline--selStartL=selEndL=the barline, so the insertion point appears to
the left of the barline--the preceding Measure is respaced. Areas before the initial
barlines of systems, which are not part of any Measure, are always left alone, even
if they're within the specified area. It returns True if it successfully respaces
anything, False if not (either because of an error or because the specified area is
entirely before a System's initial barline).

N.B.2. If RespaceBars reformats, it sets the selection range to an insertion point
at doc->tailL. Also, if it reformats, it may REMOVE doc->selStartL (and <endL>?) from
the object list, so saving selection status before RespaceBars and restoring it
afterwards is not easy! */

typedef struct {
	LINK		link;
	short		spacePercent;
	DDIST		width;
} MSPDATA;

#define EXTRAOBJS	4

Boolean RespaceBars(
			register Document *doc,
			LINK startL,
			LINK endL,
			long spaceProp,			/* >0 means use spaceProp/(RESFACTOR*100) of normal
										spacing; <=0 means use start meas.' current spacing */
			Boolean	command,		/* Called by explicit Reformat command  */
			Boolean	doRfmt 			/* If respacing causes overflow, reformat w/o asking? */
			)
{
	LINK		startBarL, endBarL,			/* First and last Measures to Respace */	
				startSysBarL,				/* First Measure of the first System involved */
				endSysL,					/* Obj ending the last System */
				endSysBarL,					/* First Meas after the last System involved or tail*/
				startInvalL, pL,
				measL, aMeasL;
	short		mindex, i; //??OR UNSIGNED?
	unsigned short objCount, measCount; 
	register	RMEASDATA *rmTable;
	XDDATA		*positionA;
	MSPDATA		*measA;
	PMEASURE	pMeas;
	PAMEASURE	aMeas;
	Boolean		okay=False, posOkay;
	
	/*
	 *	Start at the beginning of the Measure startL is in; if there is no such
	 *	Measure, start at the first Measure of the score. If end is not after start
	 *	Measure, there's nothing to do.
	 */
	startBarL = EitherSearch(LeftLINK(startL),MEASUREtype,ANYONE, GO_LEFT, False);
	if (endL==startBarL || IsAfter(endL, startBarL)) return False;
	endBarL = EndMeasSearch(doc, LeftLINK(endL));

	rmTable = (RMEASDATA *)NewPtr(MAX_MEASURES*sizeof(RMEASDATA));
	if (!GoodNewPtr((Ptr)rmTable)) { NoMoreMemory(); return False; }

	if (command) {
		PrepareUndo(doc, startL, U_Respace, 35);    				/* "Undo Respace Bars" */
	}

	/*
	 *	Respacing as specified may require reformatting. Unfortunately, we can't easily
	 * tell that's going to happen until we actually do the respacing; yet if it does,
	 * we want to give the user a chance to cancel the whole operation. So to implement
	 * it, make tables of the current positions of every affected object and of
	 * spacePercents of the Measures.
	 */
	objCount = CountObjects(startBarL, endBarL, ANYTYPE)+EXTRAOBJS;
	positionA = (XDDATA *)NewPtr(objCount*sizeof(XDDATA));
	if (!GoodNewPtr((Ptr)positionA)) {
		OutOfMemory(objCount*sizeof(XDDATA));
		goto Done;
	}
		
	for (i = 0, pL = startBarL; i<objCount; i++, pL = RightLINK(pL)) {
		positionA[i].link = pL;
		positionA[i].xd = LinkXD(pL);
		if (DynamTYPE(pL) && IsHairpin(pL)) {
			LINK aDynamicL = FirstSubLINK(pL);
			positionA[i].xdaux = DynamicENDXD(aDynamicL);
		}
		else
			positionA[i].xdaux = 0;
	}

	measCount = CountObjects(startBarL, endBarL, MEASUREtype)+EXTRAOBJS;
	measA = (MSPDATA *)NewPtr(measCount*sizeof(MSPDATA));
	if (!GoodNewPtr((Ptr)measA)) {
		OutOfMemory(measCount*sizeof(MSPDATA));
		goto Done;
	}
		
	for (i = 0, pL = startBarL; i<measCount; pL = RightLINK(pL)) {
		if (MeasureTYPE(pL)) {
			measA[i].link = pL;
			pMeas = GetPMEASURE(pL);
			measA[i].spacePercent = pMeas->spacePercent;
			aMeasL = FirstSubLINK(pL);
			aMeas = GetPAMEASURE(aMeasL);
			measA[i].width = aMeas->measSizeRect.right-aMeas->measSizeRect.left;
			i++;
		}
	}

	GetRespaceParams(doc,startBarL,endBarL,&startSysBarL,&endSysBarL,
													&endSysL,&spaceProp);
	if (CountRespMeas(doc,startBarL,endSysBarL)>=MAX_MEASURES) goto Done;

	/*
	 * Do the basic respacing. In case the user cancels, don't change anything but
	 * the positions of objects and of spacePercents of the Measures yet.
	 * ??Bug: RespWithinMeasures blatantly violates this for whole-measure rests!
	 * Centering them and adjusting hairpins should be done later.
	 */
	mindex = RespWithinMeasures(doc,startBarL,endBarL,startSysBarL,endSysBarL,
											spaceProp,rmTable,positionA,objCount);
	if (mindex<0) goto Done;
	
	posOkay = PositionWholeMeasures(doc,mindex,rmTable,startBarL,endSysL,&startInvalL,
															command,doRfmt);
	/*
	 * We couldn't finish Respacing, probably because it required reformatting and
	 * the user nixed it. Anyway, put everything back where it was when we started.
	 */
	if (!posOkay) {
		for (i = 0; i<objCount; i++) {
			pL = positionA[i].link;
			LinkXD(pL) = positionA[i].xd;
			if (DynamTYPE(pL) && IsHairpin(pL)) {
				LINK aDynamicL = FirstSubLINK(pL);
				DynamicENDXD(aDynamicL) = positionA[i].xdaux;
			}
		}

		for (i = 0; i<measCount; i++) {
			measL = measA[i].link;
			SetMeasWidth(measL, measA[i].width);
			pMeas = GetPMEASURE(measL);
			pMeas->spacePercent = measA[i].spacePercent;
		}

		goto Done;
	}
#ifdef NOTYET
	else {	/* center whole-measure rests */
		/* we'd like to do this here, but measures may've been reformatted, so
			how do we get the correct links -- rmTable? */
	}
#endif

	InvalRespBars(startInvalL,endSysL);

	doc->changed = True;
	okay = True;

Done:
	if (rmTable) DisposePtr((Ptr)rmTable);
	if (positionA) DisposePtr((Ptr)positionA);
#ifdef NO_COMPRENDE_ASK_CHARLIE
	if (command) DisableUndo(doc,True);
#endif
	if (doc->selStartL==doc->selEndL) MEAdjustCaret(doc,True);
	return okay;
}


/* ------------------------------------------------------------------- StretchToSysEnd -- */
/* Stretch the range of measures from startL to endL by adding space to the various
objects in the measures.  Use spaceProp/(RESFACTOR*100) of the existing spacing, so
if spaceProp < (RESFACTOR*100), it actually compress the measures. startL and endL
are expected to be LINKs to Measure objects within a System, and endL must occur
after startL in the score data structure and must end that System. Does not attempt
to keep symbols from overlapping. Returns True if  OK or the range is empty, False if
it finds a problem. */

Boolean StretchToSysEnd(
				Document	*/*doc*/,
				LINK		startL, LINK endL,		/* Starting and ending Measures */
				long		spaceProp,
				DDIST		staffWidth,
				DDIST		lastMeasWidth 			/* Desired width of last Measure */
				)
{
	PMEASURE	pMeas;
	LINK		pL, nextMeasL;
	long		temp, startChange, resUnit;
	short		tempPercent;

	if (startL==endL) return True;
	
	/* Going below MINSPACE is no problem as long as <spaceProp> is positive. */
	
	if (spaceProp<=0 || spaceProp/RESFACTOR>MAXSPACE) return False;

	resUnit = 100L*RESFACTOR;
	startChange = LinkXD(startL);
	
	/* For each object from first Measure up to but not including last barline... */
	
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (MeasureTYPE(pL)) {
			/* Adjust the Measure's position w/r/t first barline (startChange) */
			pMeas = GetPMEASURE(pL);
			temp = spaceProp * (pMeas->xd - startChange);
			pMeas->xd = (temp/resUnit) + startChange;
			tempPercent = (spaceProp*pMeas->spacePercent)/resUnit;
			pMeas->spacePercent = tempPercent;
			}
		 else {
			/* Not a Measure -- just stretch within the Measure */
			temp  = spaceProp*(long)LinkXD(pL);
			LinkXD(pL) = temp/resUnit;
			}
				
	/*
	 *	Be especially careful positioning last Measure so, if it consists of a
	 *	barline alone, it will be exactly flush with end of staff.
	 */
	LinkXD(endL) = staffWidth - lastMeasWidth;
	
	/*
	 *	Now go through each Measure (from pL upto nextMeasL) and ensure that its 
	 *	subobject measSizeRects are in sync with Measures set above.  This loop
	 *	excludes the last Measure now flush with the end of the System.
	 */
	for (pL=startL; pL!=endL; pL=nextMeasL) {
		nextMeasL = LinkRMEAS(pL);
		SetMeasWidth(pL, LinkXD(nextMeasL) - LinkXD(pL));
		}
	
	/* And set the subobj rects of the last Measure to the correct width. */
	
	SetMeasWidth(endL, lastMeasWidth);

	return True;
}


/* ----------------------------------------------------------------------- SysJustFact -- */
/* Return the stretching factor needed to right-justify the System whose first
Measure is <firstMeasL> and whose last Measure is <lastMeasL>. Also deliver two
relevant numbers. */

FASTFLOAT SysJustFact(
		Document *doc,
		LINK firstMeasL, LINK lastMeasL,
		DDIST *pStaffWidth,
		DDIST *pLastMeasWidth 			/* Justification width of last Measure */
		)
{
	CONTEXT		context;
	FASTFLOAT	justFact, inStaffWidth, curWidth;
	DDIST		lastMeasWidth, staffWidth;

	/* Get distance from first (invisible) barline to end of System's staves. */
	
	GetContext(doc, firstMeasL, 1, &context);
	staffWidth = context.staffRight - context.staffLeft;
	inStaffWidth = staffWidth - LinkXD(firstMeasL);
	
	/* Get distance from first (invisible) barline to current end of System's content. */
	
	lastMeasWidth = MeasJustWidth(doc, lastMeasL, context);
	curWidth = (LinkXD(lastMeasL) + lastMeasWidth) - LinkXD(firstMeasL);
		
	justFact = inStaffWidth/curWidth;
	
	*pStaffWidth = staffWidth;
	*pLastMeasWidth = lastMeasWidth;
	return justFact;
}


/* --------------------------------------------------------------------- JustifySystem -- */
/* Right-justify the System whose first Measure is <firstMeasL> and whose last
Measure is <lastMeasL>. */

void JustifySystem(Document *doc, LINK firstMeasL, LINK lastMeasL)
{
	LINK		endSysL;
	FASTFLOAT	justFact;
	long		justProp;
	DDIST		lastMeasWidth, staffWidth;
	char		fmtStr[256];

	justFact = SysJustFact(doc, firstMeasL, lastMeasL, &staffWidth, &lastMeasWidth);
	if (justFact*100L>MAXSPACE) {
		GetIndCString(fmtStr, MISCERRS_STRS, 9);		/* "Nightingale can't justify a system..." */
		sprintf(strBuf, fmtStr, (short)(justFact*100L));
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}
	justProp = (long)(justFact*100L*RESFACTOR);

	if (StretchToSysEnd(doc, firstMeasL, lastMeasL, (long)justProp, staffWidth,
			lastMeasWidth)) {
		InvalMeasures(firstMeasL,lastMeasL,ANYONE);
		endSysL = EndSystemSearch(doc, LeftLINK(lastMeasL));
		InvalRange(firstMeasL, endSysL);
	}
}


/* -------------------------------------------------------------------- JustifySystems -- */
/* Right-justify by (linear) stretching every System from the one containing startL
thru the one containing endL . As Paul Sadowski made very clear, this is not a great
way to justify music, but it works pretty well if it isn't making a drastic change,
i.e., if the System is already fairly close to being justified. User-interface
level. */

void JustifySystems(Document *doc, LINK startL, LINK endL)
{
	LINK startSysL, endSysL, currSysL, nextSysL, termSysL, firstMeasL, lastMeasL;
	DDIST lastMeasWidth, staffWidth;
	FASTFLOAT justFact, maxJustFact;
	Boolean didAnything=False;

	WaitCursor();

	PrepareUndo(doc, startL, U_Justify, 36);						/* "Undo Justify Systems" */

	GetSysRange(doc, startL, endL, &startSysL, &endSysL);
	endSysL = LinkRSYS(endSysL);									/* AFTER the last System to justify */
	
	maxJustFact = 0.0;
	for (currSysL = startSysL; currSysL!=endSysL; currSysL = nextSysL) {
		firstMeasL = LSSearch(currSysL, MEASUREtype, ANYONE, GO_RIGHT, False);
		termSysL = EndSystemSearch(doc, currSysL);
		lastMeasL = LSSearch(termSysL, MEASUREtype, ANYONE, GO_LEFT, False);
		nextSysL = LinkRSYS(currSysL);

		if (lastMeasL!=firstMeasL) {
			justFact = SysJustFact(doc, firstMeasL, lastMeasL, &staffWidth, &lastMeasWidth);
			maxJustFact = n_max(justFact, maxJustFact);
		}
	}
	
	/* Warn about possible bad spacing if stretching more than threshhold value. */

	if (maxJustFact>=(FASTFLOAT)config.justifyWarnThresh/10.0)
		if (CautionAdvise(JUSTSPACE_ALRT)==Cancel) return;

	InitAntikink(doc, startL, endL);
	
	/* For every System in the specified area... */
	
	for (currSysL = startSysL; currSysL!=endSysL; currSysL = nextSysL) {
		
		/* Get first and last Measure of this System and object ending it */
		
		firstMeasL = LSSearch(currSysL, MEASUREtype, ANYONE, GO_RIGHT, False);
		termSysL = EndSystemSearch(doc, currSysL);
		lastMeasL = LSSearch(termSysL, MEASUREtype, ANYONE, GO_LEFT, False);
		nextSysL = LinkRSYS(currSysL);
		
		/*
		 *	If the System isn't empty (e.g., the invisible barline isn't the last
		 *	barline in the System) then we can justify the measures in it.
		 */
		if (lastMeasL!=firstMeasL) {
			JustifySystem(doc, firstMeasL, lastMeasL);
			didAnything = True;
		}
	}
	
	Antikink();
	
	if (didAnything) doc->changed = True;
}
