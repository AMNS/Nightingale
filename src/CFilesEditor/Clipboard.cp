/***************************************************************************
*	FILE:	Clipboard.c																			*
*	PROJ:	Nightingale, rev. for v.99														*
*	DESC:	clipboard routines																*
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

typedef struct {
	short firstStaff;
	short lastStaff;
} PARTSTAFF;

static enum {
	PasteCancel=1,
	PasteTrunc,
	PastePin,
	PasteOK
} EPasteTypeItems;

static DDIST clipxd, clipBarxd, pasteInsertpxd;
static Boolean clipHasAddedAccs;

/* Prototypes for internal functions */

static void FixClipBeams(Document *, LINK, LINK);
static void FixClipTuplets(Document *, LINK, LINK);
static void FixClipOctavas(Document *, LINK, LINK, COPYMAP *, short);
static Boolean ChkSrcDocSlur(LINK slurL,COPYMAP *clipMap,short numObjs);
static Boolean SrcSlurInRange(Document *,LINK,Boolean,Boolean,COPYMAP *,short,
									SearchParam *);
static void FixClipLinks(Document *, Document *, LINK, LINK, COPYMAP *, short, short);

static void UpdateClipContext(Document *);

static DDIST SyncSpaceNeeded(Document *doc,LINK pL);
static DDIST GetNeededSpace(Document *doc,LINK pL);

static void SetupClip(Document *, LINK, LINK, COPYMAP **, short *);
static void FixClipChords(Document *, LINK);
static void CopyToClip(Document *, LINK, LINK);
static void DelClip(Document *doc);

static void ClipCases(Document *, Boolean *, Boolean *, Boolean *);
static void InvalForPaste(Document *doc,LINK prevSelL,Boolean hasClef,Boolean hasKS,Boolean hasTS);

static void RecalcClipxds(Document *, LINK);
static DDIST GetClipDiffxd(Document *,LINK,LINK,DDIST,DDIST);
static DDIST PreparePaste(Document *, LINK, LINK *, LINK *);

static short BadStaffAlert(short, short);
static void VeryBadStaffAlert(short, short);
static short CheckStaffMapping(Document *, LINK, LINK);
static short CountSpanSlurs(Document *doc, short v, LINK slurL);
static LINK GetSlurAcrRange(Document *doc, short v, short *numSlurs);
static Boolean CheckSlurNest(Document *doc, short stfDiff);

static void UpdateMeasRects(LINK, LINK, DDIST);
static void ClipMovexd(Document *, LINK);

static short OnlyMeasures(void);
static Boolean ClipNoteInV(Document *doc, short v);
static short GetFirstStf(Document *, LINK);
static short GetLastStf(Document *, LINK);

static short GetAnyStfDiff(Document *, Document *, short, short *, short *);

static short PasteMapStaves(Document *, Document *, LINK, LINK, short);
static void PasteFixAllContexts(Document *, LINK, LINK, short, CONTEXT, CONTEXT);
static void	PasteFixSysMeasContexts(Document *doc, LINK initL);

static void PasteFromClip(Document *, LINK, short, Boolean);

static void FixOctNotes(Document *doc, LINK octL, short s);

/* ---------------------------------------------------------------- FixClipBeams -- */
/* Fix the  beam status for all notes in range [startL, endL). If a beamed
note is copied to the clipboard without its associated beamset, then FixBeamLinks
will not set its tempFlag; fix the beam flag and stem for such notes. Cf.
FixNCStem, which this should maybe call. */

static void FixClipBeams(Document *doc, LINK startL, LINK endL)
{
	LINK 		aNoteL, aGRNoteL, pL;
	PANOTE 	aNote;
	PAGRNOTE aGRNote;
	DDIST		tempystem;
	CONTEXT	context[MAXSTAVES+1];		/* current context table */
	
	GetAllContexts(doc, context, doc->selStartL);
	InstallDoc(clipboard);
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->beamed && !aNote->tempFlag) {
					aNote->beamed = FALSE;
					if (aNote->inChord) {
						if (MainNote(aNoteL)) {
							FixSyncForChord(clipboard, pL, NoteVOICE(aNoteL), FALSE, 0, 1,
													&context[aNote->staffn]);
						}
					}
					else {
						tempystem = GetNoteYStem(clipboard, pL, aNoteL, context[aNote->staffn]);
						NoteYSTEM(aNoteL) = tempystem;
					}
				}
			}
		}
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				if (aGRNote->beamed && !aGRNote->tempFlag) {
					aGRNote->beamed = FALSE;
					tempystem = GetGRNoteYStem(aGRNoteL, context[aGRNote->staffn]);
					GRNoteYSTEM(aGRNoteL) = tempystem;
				}
			}
		}
	}
	InstallDoc(doc);
}


/* -------------------------------------------------------------- FixClipTuplets -- */
/* Fix the tuplet status for all notes in range [startL, endL). If a tupled
note is copied to the clipboard without its associated tuplet, then FixTupletLinks
will not set its tempFlag; fix the inTuplet flag and duration for such notes. */

static void FixClipTuplets(Document *doc, LINK startL, LINK endL)
{
	LINK pL, aNoteL;
	PANOTE 	aNote;
	long		tempLDur;
	
	InstallDoc(clipboard);
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->inTuplet && !aNote->tempFlag) {
					aNote->inTuplet = FALSE;
					tempLDur = SimplePlayDur(aNoteL);
					aNote = GetPANOTE(aNoteL);
					aNote->playDur = tempLDur;
				}
			}
		}
	InstallDoc(doc);
}


/* -------------------------------------------------------------- FixClipOctavas -- */
/* Fix the octava status for all notes in range [startL, endL). If an octavad
note is copied to the clipboard without its associated octava, then 
FixOctavaLinks will not set its tempFlag; fix the inOctava flag and pitch
for such notes. */

static void FixClipOctavas(Document *doc, LINK startL, LINK endL, COPYMAP *clipMap,
									short numObjs)
{
	LINK pL,aNoteL,qL,octavaL;
	PANOTE aNote; short i,staff;
	CONTEXT context; DDIST yDelta;
	Boolean stemDown, multiVoice;
	short	stemLen, octType;

	InstallDoc(clipboard);
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->inOctava && !aNote->tempFlag) {
					aNote->inOctava = FALSE;
					staff = aNote->staffn;
					
					/* Get octava in score for note that was copied to
						the clipboard */

					for (i=0,qL=NILINK; i<numObjs; i++)
						if (clipMap[i].srcL)
							if (clipMap[i].dstL==pL)
								qL = clipMap[i].srcL;
								
					if (!qL) continue;

					InstallDoc(doc);

					octavaL = LSSearch(qL,OCTAVAtype,staff,GO_LEFT,FALSE);
					octType = OctType(octavaL);

					/* Update note's y position. */
					GetContext(doc, qL, staff, &context);
					yDelta = halfLn2d(noteOffset[octType-1], context.staffHeight,
											context.staffLines);
				
					/* Determine if there are multiple voices on
						staff. */
					multiVoice = GetMultiVoice(qL, staff);
					InstallDoc(clipboard);

					if (multiVoice) stemDown = (aNote->yd<=aNote->ystem);
					aNote->yd -= yDelta;
					aNote->yqpit -= halfLn2qd(noteOffset[octType-1]);

					if (!multiVoice)
						stemDown = (aNote->yd<=context.staffHeight/2);
					stemLen = QSTEMLEN(!multiVoice, ShortenStem(aNoteL, context, stemDown));
					aNote->ystem = CalcYStem(doc, aNote->yd, NFLAGS(aNote->subType),
														stemDown,
														context.staffHeight, context.staffLines,
														stemLen, FALSE);
	
					/* Loop through the notes again and fix up the ystems
						of the non-extreme notes. */

					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (!aNote->rest && NoteSTAFF(aNoteL)==staff) {
							if (aNote->inChord) {
								FixSyncForChord(clipboard,pL,NoteVOICE(aNoteL),aNote->beamed,
														0,(multiVoice? -1 : 1),&context);
								break;
							}
						}
					}
				}
			}
		}
	InstallDoc(doc);
}

/* --------------------------------------------------------------- ChkSrcDocSlur -- */
/* If the slur exists and was selected in the src Doc, check to
see if it was copied to the dst Doc. */

static Boolean ChkSrcDocSlur(LINK slurL, COPYMAP *clipMap, short numObjs)
{
	short i;

	if (slurL && LinkSEL(slurL)) 
		for (i = 0; i<numObjs; i++)
			if (clipMap[i].srcL && clipMap[i].srcL==slurL)
				return TRUE;
				
	return FALSE;
}

/* -------------------------------------------------------------- SrcSlurInRange -- */
/* Check to see if the slur in the src doc which begins/ends at qL is in
the range copied to the destination doc (currentDoc). */

static Boolean SrcSlurInRange(Document *srcDoc, LINK qL, Boolean isTie,
										Boolean slurredL, COPYMAP *clipMap, short numObjs,
										SearchParam *pbSearch)
{
	LINK slurL; Boolean inRange;
	Document *saveDoc;

	saveDoc = currentDoc;
	InstallDoc(srcDoc);

	/* Search left in the source document for the slur in the
		note's voice that ends at the note in the source doc. */

	if (slurredL)
		slurL = LeftSlurSearch(qL,pbSearch->voice,isTie);
	else {
		pbSearch->subtype = isTie;
		slurL = L_Search(qL, SLURtype, GO_LEFT, pbSearch);
	}

	/* If the slur exists and was selected in the score, check to
		see if it was copied to the destination doc. */
	
	inRange = ChkSrcDocSlur(slurL,clipMap,numObjs);

	InstallDoc(saveDoc);
	
	return inRange;
}

/* ---------------------------------------------------------------- FixClipLinks -- */
/* Use <clipMap> to correct cross links in the range [startL, endL).
FixClipLinks traverses a range in the destination doc, and fixes up LINKs
in that range. It starts out by installing heaps for the destination doc;
case SYNCtype installs and re-installs for both src & dst, but leaves heaps
for dst installed; other cases leave heaps alone.
Calling function is responsible for insuring that the heaps it wants
installed are actually installed after calling this function! */

static void FixClipLinks(Document *srcDoc, Document *dstDoc, LINK startL, LINK endL,
									COPYMAP *clipMap, short numObjs, short stfDiff)
{
	short i;
	LINK pL;
	LINK lPage, rPage, lSystem, rSystem, lStaff, rStaff, lMeas, rMeas, qL, aNoteL;
	Boolean inRange;
	SearchParam pbSearch;
	
	InstallDoc(dstDoc);

	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case PAGEtype:
				lPage = LSUSearch(LeftLINK(pL), PAGEtype, ANYONE, TRUE, FALSE);
				LinkLPAGE(pL) = lPage;
				
				rPage = LSUSearch(RightLINK(pL), PAGEtype, ANYONE, FALSE, FALSE);
				LinkRPAGE(pL) = rPage;
				break;
			case SYSTEMtype:
				lPage = LSUSearch(LeftLINK(pL), PAGEtype, ANYONE, TRUE, FALSE);
				SysPAGE(pL) = lPage;
				
				lSystem = LSUSearch(LeftLINK(pL), SYSTEMtype, ANYONE, TRUE, FALSE);
				LinkLSYS(pL) = lSystem;
				rSystem = LSUSearch(RightLINK(pL), SYSTEMtype, ANYONE, FALSE, FALSE);
				LinkRSYS(pL) = rSystem;
				break;
			case STAFFtype:
				lSystem = LSUSearch(LeftLINK(pL), SYSTEMtype, ANYONE, TRUE, FALSE);
				StaffSYS(pL) = lSystem;
				
				lStaff = LSUSearch(LeftLINK(pL), STAFFtype, ANYONE, TRUE, FALSE);
				LinkLSTAFF(pL) = lStaff;
				rStaff = LSUSearch(RightLINK(pL), STAFFtype, ANYONE, FALSE, FALSE);
				LinkRSTAFF(pL) = rStaff;
			case CONNECTtype:
				break;
			case MEASUREtype:
				lSystem = LSUSearch(LeftLINK(pL), SYSTEMtype, ANYONE, TRUE, FALSE);
				MeasSYSL(pL) = lSystem;
				lStaff = LSUSearch(LeftLINK(pL), STAFFtype, ANYONE, TRUE, FALSE);
				MeasSTAFFL(pL) = lStaff;

				lMeas = LSUSearch(LeftLINK(pL), MEASUREtype, ANYONE, TRUE, FALSE);
				LinkLMEAS(pL) = lMeas;
				rMeas = LSUSearch(RightLINK(pL), MEASUREtype, ANYONE, FALSE, FALSE);
				LinkRMEAS(pL) = rMeas;
				break;
			case SYNCtype:

				/* Fix up the tiedL/R & slurredL/R flags for notes whose ties/slurs
					may or may not be in the range copied. */

				InitSearchParam(&pbSearch);

				for (i=0,qL=NILINK; i<numObjs; i++)
					if (clipMap[i].srcL)
						if (clipMap[i].dstL==pL)
							qL = clipMap[i].srcL;
				
				/* If there was a sync in the source corresponding to a sync in the
					dest, traverse the notes of the sync in the dest. If any was tied/
					slurred, search for the corresponding slur in the source, and check
					to see if it was copied. If there was none or it wasn't copied,
					mark the note not tied/slurred.
					#1. Note has already been translated to new staff/voice; searching
					in the source document requires translation back to previous staff/
					voice. */

				if (qL) {
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					
						pbSearch.voice = NoteVOICE(aNoteL)-stfDiff;			/* #1. */
						pbSearch.inSystem = TRUE;
						pbSearch.optimize = FALSE;

						/* For each note copied from srcDoc to dstDoc, check all tied/slurred
							flags to see if any tie/slur begins/ends at that note. If so, check
							if the tie/slur was also copied to the dst doc; if not, mark the
							corresponding flag in the note FALSE. */

						if (NoteTIEDL(aNoteL)) {
							/* See if the tie was copied to the dst doc. */

							inRange = SrcSlurInRange(srcDoc,qL,TRUE,TRUE,clipMap,numObjs,
																	&pbSearch);

							/* If the tie was not in the range, the note in the dst doc
								must be marked not tied. */

							if (!inRange) NoteTIEDL(aNoteL) = FALSE;
						}
						if (NoteTIEDR(aNoteL)) {
							inRange = SrcSlurInRange(srcDoc,qL,TRUE,FALSE,clipMap,numObjs,
																	&pbSearch);

							if (!inRange) NoteTIEDR(aNoteL) = FALSE;
						}

						if (NoteSLURREDL(aNoteL)) {
							inRange = SrcSlurInRange(srcDoc,qL,FALSE,TRUE,clipMap,numObjs,
																	&pbSearch);

							if (!inRange) NoteSLURREDL(aNoteL) = FALSE;
						}
						if (NoteSLURREDR(aNoteL)) {
							inRange = SrcSlurInRange(srcDoc,qL,FALSE,FALSE,clipMap,numObjs,
																	&pbSearch);
							
							if (!inRange) NoteSLURREDR(aNoteL) = FALSE;
						}
					}
				}
				break;

			case SLURtype:
				for (i = 0; i<numObjs; i++)
					if (clipMap[i].srcL)
						if (clipMap[i].srcL==SlurFIRSTSYNC(pL)) {
							SlurFIRSTSYNC(pL) = clipMap[i].dstL;
							break;
						}
				for (i = 0; i<numObjs; i++)
					if (clipMap[i].srcL)
						if (clipMap[i].srcL == SlurLASTSYNC(pL)) {
							SlurLASTSYNC(pL) = clipMap[i].dstL;
							break;
						}
				break;
			case DYNAMtype:
				for (i = 0; i<numObjs; i++)
					if (clipMap[i].srcL)
						if (clipMap[i].srcL==DynamFIRSTSYNC(pL)) {
							DynamFIRSTSYNC(pL) = clipMap[i].dstL;
							break;
						}
				if (IsHairpin(pL))
					for (i = 0; i<numObjs; i++)
						if (clipMap[i].srcL)
							if (clipMap[i].srcL==DynamLASTSYNC(pL)) {
								DynamLASTSYNC(pL) = clipMap[i].dstL;
								break;
							}
				break;
			case GRAPHICtype:
				for (i = 0; i<numObjs; i++)
					if (clipMap[i].srcL)
						if (clipMap[i].srcL==GraphicFIRSTOBJ(pL)) {
							GraphicFIRSTOBJ(pL) = clipMap[i].dstL;
							break;
						}
				if (GraphicSubType(pL)==GRDraw)
					for (i = 0; i<numObjs; i++)
						if (clipMap[i].srcL)
							if (clipMap[i].srcL==GraphicLASTOBJ(pL)) {
								GraphicLASTOBJ(pL) = clipMap[i].dstL;
								break;
							}
				break;
			case TEMPOtype:
				for (i = 0; i<numObjs; i++)
					if (clipMap[i].srcL)
						if (clipMap[i].srcL==TempoFIRSTOBJ(pL)) {
							TempoFIRSTOBJ(pL) = clipMap[i].dstL;
							break;
						}
				break;
				
			case ENDINGtype:
				for (i = 0; i<numObjs; i++)
					if (clipMap[i].srcL)
						if (clipMap[i].srcL==EndingFIRSTOBJ(pL)) {
							EndingFIRSTOBJ(pL) = clipMap[i].dstL;
							break;
						}				
				for (i = 0; i<numObjs; i++)
					if (clipMap[i].srcL)
						if (clipMap[i].srcL==EndingLASTOBJ(pL)) {
							EndingLASTOBJ(pL) = clipMap[i].dstL;
							break;
						}				

			default:
				;
		}
	}
}

/* ----------------------------------------------------------- UpdateClipContext -- */

static void UpdateClipContext(Document *doc)
{
	short s,sharpsOrFlats; CONTEXT context; LINK firstClef, firstKSL, firstTSL;

	InstallDoc(clipboard);
	firstClef = LSSearch(clipboard->headL, CLEFtype, ANYONE, GO_RIGHT, FALSE);
	firstKSL = LSSearch(clipboard->headL, KEYSIGtype, ANYONE, GO_RIGHT, FALSE);
	firstTSL = LSSearch(clipboard->headL, TIMESIGtype, ANYONE, GO_RIGHT, FALSE);
	InstallDoc(doc);

	for (s=1; s<=doc->nstaves; s++) {

		InstallDoc(doc);
		GetContext(doc, doc->selStartL, s, &context);
		/*
		 * The Nightingale object list supports arbitrary key signatures with intermixed
		 * sharps and flats; these are indicated by the <nonstandard> flag in the KeySig
		 * subobject, which unfortunately is not kept in the context, but we don't support
		 * nonstandard ones yet anyway, so just call GetSharpsOrFlats, which assumes it's a
		 * standard one.
		 */
		sharpsOrFlats = GetSharpsOrFlats(&context);

		InstallDoc(clipboard);
		ReplaceClef(clipboard, firstClef, s, context.clefType);
		ReplaceKeySig(clipboard, firstKSL, s, sharpsOrFlats);
		ReplaceTimeSig(clipboard, firstTSL, s, context.timeSigType, context.numerator,
							context.denominator);
	}
	
	InstallDoc(doc);
}

/* Return the total space required by sync <pL>, in DDISTs. */

static DDIST SyncSpaceNeeded(Document *doc, LINK pL)
{
	LINK aNoteL; long maxLen,tempLen;
	short staff; CONTEXT context;
	STDIST symWidth,space;
	
	maxLen = tempLen = 0L;
	space = 0;
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		tempLen = CalcNoteLDur(doc, aNoteL, pL);
		if (tempLen > maxLen) {
			maxLen = tempLen;
			staff = NoteSTAFF(aNoteL);
		}
	}
	space = IdealSpace(doc, maxLen, RESFACTOR*doc->spacePercent);
	symWidth = SymWidthRight(doc, pL, staff, FALSE);
	GetContext(doc, pL, staff, &context);
	space = n_max(space,symWidth);
	return (std2d(space,context.staffHeight,5));
}

/* Return the total space required by pL in DDISTs. */

static DDIST GetNeededSpace(Document *doc, LINK pL)
{
	short j; DDIST width=-32000; CONTEXT context;

	if (SyncTYPE(pL))
		return SyncSpaceNeeded(doc, pL);

	/* Can forego checking with FirstValidxd because pL has
		already been tested by calling function SetupClip.
		Any other calling function will also need to check. */

	for (j=1; j<=doc->nstaves; j++) {
		GetContext(doc, pL, j, &context);
		width = n_max(width, SymDWidthRight(doc, pL, j, FALSE, context));
	}
	return width;
}

/* ------------------------------------------------------------------- SetupClip -- */
/* Set up the clipboard: delete the main clip, and compute the clipboard
parameters clipxd, clipBarxd. */

static void SetupClip(Document *doc, LINK startL, LINK endL, COPYMAP **clipMap,
								short *objCount)
{
	LINK pL,baseL,nextL; DDIST diff;

	DelClip(doc);													/* Kill current clipboard */
	SetupClipDoc(doc,TRUE);
	clipxd = 0;
	UpdateTailxd(doc);

	/* Store the total length of the clipboard in clipxd: accumulate
		the PMDist between each valid-xd symbol and the next in the range
		[startL,endL).
		If the symbol is not in the same system or page as the next one
		(which can currently only happen if endL is on the following system)
		add the space needed for the symbol instead of its PMDist to the
		next symbol. */

	for (pL = startL; pL!=endL && !IsAfter(endL,pL); pL = nextL) {
		if (LinkSEL(pL)) {
			baseL = FirstValidxd(pL,GO_RIGHT);
			nextL = FirstValidxd(RightLINK(pL),GO_RIGHT);
			if (SameSystem(baseL,nextL) && !PageTYPE(nextL)) {
				diff = PMDist(baseL, nextL);
				clipxd += diff;
			}
			else {
				diff = GetNeededSpace(doc,baseL);
				clipxd += diff;
				break;
			}
		}
		else
			nextL = FirstValidxd(RightLINK(pL),GO_RIGHT);
	}
	
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (LinkSEL(pL) && MeasureTYPE(pL)) {
			clipBarxd = PMDist(FirstValidxd(LeftLINK(pL),GO_LEFT), pL);
			break;
		}

	SetupCopyMap(startL, endL, clipMap, objCount);
}


/* --------------------------------------------------------------- FixClipChords -- */
/* Fix stem, inChord flags, etc. of notes in case any chords were only partly
copied. This version doesn't check whether in fact a voice was only partly copied,
which looks difficult to do; it simply resets the note/chord in every voice to
that voice's default. */

static void FixClipChords(Document *doc, LINK syncL)
{
	short		v;
	short		nInChord[MAXVOICES+1];
	PANOTE	aNote;
	LINK		aNoteL, mainNoteL;
	
	for (v = 1; v<=MAXVOICES; v++)
		nInChord[v] = 0;
		
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		nInChord[aNote->voice]++;
	}
		
	for (v = 1; v<=MAXVOICES; v++)
		if (nInChord[v]==1)
			FixSyncForNoChord(doc, syncL, v, NULL);
		else if (nInChord[v]>1) {
			mainNoteL = FindMainNote(syncL, v);
			FixSyncForChord(doc, syncL, v, NoteBEAMED(mainNoteL), 0, 0, NULL);
		}
}


/* Given a Sync or GRSync, if any of its notes or grace notes has an explicit
accidental, even a natural, return TRUE; else return FALSE. */
 
Boolean HasAnyAccidentals(LINK pL);
Boolean HasAnyAccidentals(LINK pL)
{
	LINK aNoteL, aGRNoteL;
	
	if (SyncTYPE(pL)) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
			if (!NoteREST(aNoteL) && NoteACC(aNoteL)!=0) return TRUE;
		}
	}
	else if (GRSyncTYPE(pL)) {
		aGRNoteL = FirstSubLINK(pL);
		for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
			if (GRNoteACC(aGRNoteL)!=0) return TRUE;
		}
	}
	
	return FALSE;
}

/* ------------------------------------------------------------------ CopyToClip -- */
/* Copy the specified range of nodes to the clipboard. Assumes doc, not clipboard,
is installed! */

static void CopyToClip(Document *doc, LINK startL, LINK endL)
{
	LINK		pL, copyL, prevL; short i, numObjs, v;
	COPYMAP	*clipMap;
	Boolean	addAccidentals, addAccsKnown;
	
	/* If at the beginning of the first Measure we're copying there are notes that
		we're not copying and that have accidentals, set a flag to tell us to make
		implied accidentals explicit. */
	
	addAccidentals = addAccsKnown = FALSE;
	
	/* Scanning the object list to left, if we find a Sync or GRSync with accidentals
		before we find a Measure, we'll add accidentals, otherwise we won't. */
	
	for (pL = LeftLINK(startL); pL && !addAccsKnown; pL = LeftLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				addAccsKnown = TRUE;
				break;
			case SYNCtype:
			case GRSYNCtype:
				if (HasAnyAccidentals(pL)) {
					addAccidentals = TRUE;
					addAccsKnown = TRUE;
				}
				break;
			default:
				;
		}
	}

	prevL = NILINK;
	SetupClip(doc, startL, endL, &clipMap, &numObjs);

	for (pL=startL, i=0; pL!=endL; pL=RightLINK(pL), i++)
		if (LinkSEL(pL) || MeasureTYPE(pL) || PSMeasTYPE(pL)) {
			copyL = NILINK;
			switch (ObjLType(pL)) {
				case PAGEtype:
				case SYSTEMtype:
				case STAFFtype:
				case CONNECTtype:
					MayErrMsg("CopyToClip: illegal type %ld copied", 
						(long)ObjLType(pL));
					break;
				case MEASUREtype:
					copyL = DuplicateObject(MEASUREtype, pL, FALSE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case PSMEAStype:
					copyL = DuplicateObject(PSMEAStype, pL, FALSE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case CLEFtype:
					copyL = DuplicateObject(CLEFtype, pL, TRUE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case KEYSIGtype:
					copyL = DuplicateObject(KEYSIGtype, pL, TRUE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case TIMESIGtype:
					copyL = DuplicateObject(TIMESIGtype, pL, TRUE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case SYNCtype:
					copyL = DuplicateObject(SYNCtype, pL, TRUE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case GRSYNCtype:
					copyL = DuplicateObject(GRSYNCtype, pL, TRUE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case DYNAMtype:
					if (!LinkSEL(DynamFIRSTSYNC(pL)))
						break;
					if (IsHairpin(pL) && !LinkSEL(DynamLASTSYNC(pL)))
						break;

					copyL = DuplicateObject(DYNAMtype, pL, TRUE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case RPTENDtype:
					copyL = DuplicateObject(RPTENDtype, pL, TRUE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case ENDINGtype:
					if (!LinkSEL(EndingFIRSTOBJ(pL)))
						break;
					if (EndingLASTOBJ(pL) && !LinkSEL(EndingLASTOBJ(pL)))
						break;

					copyL = DuplicateObject(ENDINGtype, pL, TRUE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case GRAPHICtype:
					if (!LinkSEL(GraphicFIRSTOBJ(pL)))
						break;
					if (GraphicSubType(pL)==GRDraw && !LinkSEL(GraphicLASTOBJ(pL)))
						break;
					copyL = DuplicateObject(GRAPHICtype, pL, FALSE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case TEMPOtype:
					if (!LinkSEL(TempoFIRSTOBJ(pL)))
						break;
					copyL = DuplicateObject(TEMPOtype, pL, FALSE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case SPACEtype:
					copyL = DuplicateObject(SPACEtype, pL, FALSE, doc, clipboard, FALSE);
					clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
					break;
				case BEAMSETtype: {
					short			ncopy;
					PANOTEBEAM	pSub;
					LINK			pSubL;
						pSubL = FirstSubLINK(pL);
						for (ncopy = 0; pSubL; ncopy++, pSubL=NextNOTEBEAML(pSubL))
							if (ncopy==LinkNENTRIES(pL)-1) {
								pSub = GetPANOTEBEAM(pSubL);
								if (pSub->bpSync==endL || 
										IsAfter(endL,pSub->bpSync))
									 goto BDone;
							}
						copyL = DuplicateObject(BEAMSETtype, pL, TRUE, doc, clipboard, FALSE);
						clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
						BDone:
							;
					}
					break;
				case TUPLETtype: {
					short			ncopy;
					PANOTETUPLE	pSub;
					LINK			pSubL;
						pSubL = FirstSubLINK(pL);
						for (ncopy = 0; pSubL; ncopy++, pSubL=NextNOTETUPLEL(pSubL))
							if (ncopy==LinkNENTRIES(pL)-1) {
								pSub = GetPANOTETUPLE(pSubL);
								if (pSub->tpSync==endL || 
										IsAfter(endL,pSub->tpSync))
									 goto TDone;
							}
						copyL = DuplicateObject(TUPLETtype, pL, TRUE, doc, clipboard, FALSE);
						clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
						TDone:
							;
					}
					break;
				case OCTAVAtype: {
					short				ncopy;
					PANOTEOCTAVA	pSub,sub;
					LINK				pSubL,oPrevL,subL;
					
					/* An octave sign is the only extended object where, if not all its
					attached items are selected, we copy the extended object anyway.
					
					If the octava extends past the end of the selection, keep track
					of how many opSyncs are selected, copy the octava object, cut off
					the subObj list of the copied octava at the first unselected opSync,
					and set its nEntries to be the number of selected opSyncs. We
					assume that at least the first opSync is selected!
					
					NB: It appears that, for octavas, DuplicateObject with <selectedOnly>
					looks at, not the octava subobjs' selection status, but that of the
					Syncs it refers to! That means that much of the code below is
					unnecessary. On the other hand, I'm not at all sure DuplicateObject
					should be left this way...sigh. */
	
						pSubL = FirstSubLINK(pL);
						for (ncopy=0; pSubL; ncopy++,pSubL=NextNOTEOCTAVAL(pSubL)) {
							pSub = GetPANOTEOCTAVA(pSubL);
							if (!LinkSEL(pSub->opSync))			/* ??But may be sel. on another stf! */
								if (ncopy) {
									copyL = DuplicateObject(OCTAVAtype, pL, TRUE, doc, clipboard, FALSE);
									InstallDoc(clipboard);
									subL = FirstSubLINK(copyL);
									for ( ; subL; oPrevL=subL,subL=NextNOTEOCTAVAL(subL)) {
										sub = GetPANOTEOCTAVA(subL);
										if (!DLinkSEL(doc, sub->opSync)) {
											NextNOTEOCTAVAL(oPrevL) = NILINK;
											HeapFree(NOTEOCTAVAheap, subL);
											LinkNENTRIES(copyL) = ncopy+1;
											InstallDoc(doc);
											goto ODone;
										}
									}
								/* I don't think we can ever get here, but just in case */
								InstallDoc(doc);
								}
						}
						copyL = DuplicateObject(OCTAVAtype, pL, TRUE, doc, clipboard, FALSE);
						clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
						ODone:
							;
					}
					break;
				case SLURtype: {
					LINK firstSync, lastSync, aNoteL;
	
					/* If first and last "syncs" are not syncs, it is a cross system slur,
					which will not be copied; if the notes in the slur's voice are not
					selected, it also will not be copied. */
	
						firstSync = SlurFIRSTSYNC(pL);
						lastSync = SlurLASTSYNC(pL);
	
						if (ObjLType(firstSync)!=SYNCtype || ObjLType(lastSync)!=SYNCtype)
							break;
	
						if (LinkSEL(firstSync) && LinkSEL(lastSync)) {
	
							aNoteL = FirstSubLINK(firstSync);
							for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
								if (NoteVOICE(aNoteL)==SlurVOICE(pL) && !NoteSEL(aNoteL))
									goto SDone;
							aNoteL = FirstSubLINK(lastSync);
							for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
								if (NoteVOICE(aNoteL)==SlurVOICE(pL) && !NoteSEL(aNoteL))
									goto SDone;
	
							copyL = DuplicateObject(SLURtype, pL, TRUE, doc, clipboard, FALSE);
							clipMap[i].srcL = pL;	clipMap[i].dstL = copyL;
							SDone:
								;
						}
					}
					break;
				default:
					MayErrMsg("CopyToClip: can't handle object type %ld at %d",
								(long)ObjLType(pL), pL);
			}
						
			if (copyL) {
				if (!prevL)	{
					DRightLINK(clipboard, clipFirstMeas) = copyL;			/* Link in after the first measure obj */
					prevL = clipFirstMeas;
				}
				else
					DRightLINK(clipboard, prevL) = copyL;		/* Link to previous node */
				DLeftLINK(clipboard, copyL) = prevL;
				prevL = copyL;
			}
		}
	
	if (prevL) {
		DRightLINK(clipboard, prevL) = clipboard->tailL;
		DLeftLINK(clipboard, clipboard->tailL) = prevL;
	}

	/* Structure links need to be fixed up in the clipboard if we wish
		to use LSSearch in the clipboard; this is called, for example, by
		GetContext called by GetNoteYStem called by FixClipBeams. */

	FixStructureLinks(doc, clipboard, clipboard->headL, clipboard->tailL);
	
	/* Fix up notes in beamsets. Use the tempFlags to determine which notes
		need fixing up because they are in beamsets not completely copied, then
		fix up beam ptrs for notes in beamsets completely copied, then fix up notes
		whose beamsets are not copied to the clipboard. Do the same for tuplets &
		octavas. */
	SetTempFlags(doc, clipboard, clipboard->headL, clipboard->tailL, FALSE);
	FixAllBeamLinks(doc, clipboard, clipboard->headL, clipboard->tailL);
	FixClipBeams(doc, clipboard->headL, clipboard->tailL);
	
	SetTempFlags(doc, clipboard, clipboard->headL, clipboard->tailL, FALSE);
	FixTupletLinks(doc, clipboard, clipboard->headL, clipboard->tailL);
	FixClipTuplets(doc, clipboard->headL, clipboard->tailL);
	
	SetTempFlags(doc, clipboard, clipboard->headL, clipboard->tailL, FALSE);
	FixOctavaLinks(doc, clipboard, clipboard->headL, clipboard->tailL);
	FixClipOctavas(doc, clipboard->headL, clipboard->tailL, clipMap, numObjs);
	
	SetTempFlags(doc, clipboard, clipboard->headL, clipboard->tailL, FALSE);
	FixClipLinks(doc,clipboard,clipboard->headL,clipboard->tailL,clipMap,numObjs,0);
	/* clipboard's heaps are now installed. */
	
	DisposePtr((Ptr)clipMap);
	
	for (v = 1; v<=MAXVOICES; v++)						/* Not clear where to do this. */
		clipboard->voiceTab[v] = doc->voiceTab[v];
		
	InstallDoc(clipboard);
	for (pL = clipFirstMeas; pL!=clipboard->tailL; pL = RightLINK(pL)) {

		/* Because of special treatment of measures described in ContinSelection,
			they may be unselected in the clipboard, so select them now. */

		if (MeasureTYPE(pL) || PSMeasTYPE(pL)) {
			SelAllSubObjs(pL);
			LinkSEL(pL) = TRUE;
		}

		/* Fix stem, inChord flags, etc. of notes in case any chords were only
			partly copied. */
		if (SyncTYPE(pL)) FixClipChords(clipboard, pL);
	}

	BuildVoiceTable(clipboard, TRUE);

	/* If we haven't copied the beginning of the first measure of the score, notes
		in the clipboard's first measure with implied accidentals may no longer
		have the correct context: to avoid problems, add explicit accidentals. ??Too
		extreme: should then remove redundant accs in the clipboard, or add them only
		if not in the keysig, etc.! */
		
	clipHasAddedAccs = FALSE;
	if (addAccidentals) {
		LINK endAddAccsL; Boolean addNaturals; short s;
		CONTEXT	context[MAXSTAVES+1];
	
		endAddAccsL = LSUSearch(RightLINK(clipFirstMeas), MEASUREtype, ANYONE, GO_RIGHT, FALSE);

		/* If no staff has a key signature, there's no need to add redundant naturals. */
		addNaturals = FALSE;
		GetAllContexts(clipboard, context, clipFirstMeas);
		for (s=1; s<=clipboard->nstaves; s++)
			if (context[s].nKSItems!=0) {
				addNaturals = TRUE;
				break;
			}
		
		clipHasAddedAccs = AddMIDIRedundantAccs(clipboard, endAddAccsL, FALSE, addNaturals);
	}
	

	InstallDoc(doc);

	clipboard->selStartL = clipFirstMeas;
	clipboard->selEndL = clipboard->tailL;
}

/*
 *	Replace the clipboard's fontTable with the fontTable of the given Document.
 */

void CopyFontTable(Document *doc)
{
	short i;
	
	for (i = 0; i<doc->nfontsUsed; i++)
		clipboard->fontTable[i] = doc->fontTable[i];
	clipboard->nfontsUsed = doc->nfontsUsed;
}

/*
 *	Update fields of the clipboard document to reflect their values in the document
 * from which the selection range was copied.
 */

void SetupClipDoc(Document *doc, Boolean updContext)
{
	short i;
	
	clipboard->nstaves = doc->nstaves;
	clipboard->srastral = doc->srastral;
	clipboard->altsrastral = doc->altsrastral;
	clipboard->spacePercent = doc->spacePercent;
	
	clipboard->ledgerYSp = doc->ledgerYSp;
	clipboard->magnify = doc->magnify;

	clipboard->tempo = doc->tempo;
	clipboard->channel = doc->channel;
	clipboard->velocity = doc->velocity;
	clipboard->feedback = doc->feedback;
	clipboard->transposed = doc->transposed;
	clipboard->polyTimbral = doc->polyTimbral;

	clipboard->selStaff = doc->selStaff;
	clipboard->numberMeas = doc->numberMeas;
	clipboard->lookVoice = doc->lookVoice;
	clipboard->pianoroll = doc->pianoroll;
	clipboard->showSyncs = doc->showSyncs;
	clipboard->frameSystems = doc->frameSystems;
	clipboard->colorVoices = doc->colorVoices;
	clipboard->showInvis = doc->showInvis;
	clipboard->yBetweenSys = doc->yBetweenSys;

	clipboard->firstNames = NONAMES;							/* Show no part names */
	clipboard->firstIndent = 0;
	clipboard->otherNames = NONAMES;							/* Show no part names */
	clipboard->otherIndent = 0;
	
	clipboard->nonstdStfSizes = doc->nonstdStfSizes;

	for (i = 0; i<doc->nfontsUsed; i++)
		clipboard->fontTable[i] = doc->fontTable[i];
	clipboard->nfontsUsed = doc->nfontsUsed;

	if (updContext) UpdateClipContext(doc);
}

/* ---------------------------------------------------------------------- DelClip -- */
/* Kill all nodes contained in the clipboard, except for the last one. */

static void DelClip(Document *doc)
{
	LINK	pL, prevL, startL, endL, copyL, pageL, sysL, staffL;
	Boolean loopDone = FALSE;

	InstallDoc(clipboard);
	startL = clipboard->headL; endL = clipboard->tailL;

	/* Delete everything in the clipboard, including initial structure
		objects. */
	DeleteRange(clipboard,RightLINK(clipboard->headL),clipboard->tailL);
	DeleteNode(clipboard,clipboard->headL);

	/* Replace the initial structure objects with duplicates of those
		in the document whose selRange is to be copied to the clipboard. */	
	pageL = sysL = staffL = prevL = NILINK;
	for (pL=doc->headL; !loopDone; pL=DRightLINK(doc,pL)) {
		switch (DObjLType(doc, pL)) {
			case HEADERtype:
				copyL = DuplicateObject(HEADERtype, pL, FALSE, doc, clipboard, FALSE);
				clipboard->headL = copyL;
				break;
			case PAGEtype:
				copyL = DuplicateObject(PAGEtype, pL, FALSE, doc, clipboard, FALSE);
				LinkLPAGE(copyL) = LinkRPAGE(copyL) = NILINK;
				pageL = copyL;
				break;
			case SYSTEMtype:
				copyL = DuplicateObject(SYSTEMtype, pL, FALSE, doc, clipboard, FALSE);
				LinkLSYS(copyL) = LinkRSYS(copyL) = NILINK;
				SysPAGE(copyL) = pageL;
				sysL = copyL;
				break;
			case STAFFtype:
				copyL = DuplicateObject(STAFFtype, pL, FALSE, doc, clipboard, FALSE);
				LinkLSTAFF(copyL) = LinkRSTAFF(copyL) = NILINK;
				StaffSYS(copyL) = sysL;
				staffL = copyL;
				break;
			case CONNECTtype:
				copyL = DuplicateObject(CONNECTtype, pL, FALSE, doc, clipboard, FALSE);
				break;
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
				copyL = DuplicateObject(DObjLType(doc, pL), pL, FALSE, doc, clipboard, FALSE);
				break;
			case MEASUREtype:
				copyL = DuplicateObject(MEASUREtype, pL, FALSE, doc, clipboard, FALSE);
				clipFirstMeas = copyL;
				LinkLMEAS(copyL) = LinkRMEAS(copyL) = NILINK;
				MeasSYSL(copyL) = sysL; MeasSTAFFL(copyL) = staffL; 
				loopDone = TRUE;
				break;
			default:
				break;
		}
		/* If prevL has not been set (we are at clipboard->headL), set it to the
			copied node. If no node has been copied (a BeforeFirstMeas graphic will
			not be copied), then prevL will still be equal to copyL, having been
			set to it in the previous iteration of the loop; just set it to copyL
			again; else link copyL in after prevL, and move prevL along to be copyL. */
		if (prevL)
			if (prevL!=copyL) {
				RightLINK(prevL) = copyL;
				LeftLINK(copyL) = prevL;
			}
		prevL = copyL;
	}
	/* Link in the tail of the clipboard after the last copied object. */
	RightLINK(prevL) = endL;
	LeftLINK(endL) = prevL;
	
	InstallDoc(doc);
}


/* ------------------------------------------- Functions for Measure consistency -- */

/* For every selected measure object and pseudoMeasure object in the selection range,
if only some of its subobjects are selected, deselect it completely and unhilite it.
If called by Cut and Clear before they call DeleteSelection, partly-selected measure
objects will be left alone; likewise for pseudoMeasures. If it does anything, 
returns TRUE, else FALSE. */

Boolean DeselPartlySelMeasSubobjs(Document *doc)
{
	LINK 			pL;
	Boolean		didSomething=FALSE;
	CONTEXT		context[MAXSTAVES+1];
	Boolean		found;
	short			pIndex;
	STFRANGE	stfRange={0,0};
	
	GetAllContexts(doc, context, doc->selStartL);

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		ContextObject(doc, pL, context);
		if (MeasureTYPE(pL) || PSMeasTYPE(pL))
			if (LinkSEL(pL))
				if (!MeasHomoSel(pL, TRUE)) {
					CheckObject(doc, pL, &found, NULL, context, SMDeselect, &pIndex, stfRange);
					didSomething = TRUE;
				}
	}
	
	return didSomething;
}

/* Select all subobjects of every measure object and every pseudoMeasure object
object in the selection range that's already at least partly selected. If called
prior to Cut and Clear, entire measure objects will be cut, not just selected
barlines; likewise for pseudoMeasures. */

void SelPartlySelMeasSubobjs(Document *doc)
{
	LINK pL;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && (MeasureTYPE(pL) || PSMeasTYPE(pL)))
			SelAllSubObjs(pL);
	}
}
	
/* Return TRUE iff every subobject in <measL> (which must be a Measure or pseudoMeasure)
has the specified selection status. */

Boolean MeasHomoSel(LINK measL, Boolean selected)
{
	LINK aMeasL;
	
	if (MeasureTYPE(measL)) {
		for (aMeasL = FirstSubLINK(measL); aMeasL; aMeasL = NextMEASUREL(aMeasL))
			if (MeasureSEL(aMeasL)!=selected) return FALSE;
	}
	else if (PSMeasTYPE(measL)) {
		for (aMeasL = FirstSubLINK(measL); aMeasL; aMeasL = NextPSMEASL(aMeasL))
			if (PSMeasSEL(aMeasL)!=selected) return FALSE;
	}
		
	return TRUE;
}

/* Return TRUE iff every subobject in every selected Measure or pseudoMeasure has
the specified selection status. */

Boolean AllMeasHomoSel(Document *doc)
{
	LINK pL;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (MeasureTYPE(pL) || PSMeasTYPE(pL))
			if (LinkSEL(pL))
				if (!MeasHomoSel(pL, TRUE)) return FALSE;
	}
	
	return TRUE;
}


/* ----------------------------------------------------------------------- DoCut -- */
/* Cut the selection from the score, copy it to our clipboard. */

void DoCut(Document *doc)
{
	Boolean haveJI; LINK pL;
	
	haveJI = FALSE;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && (J_ITTYPE(pL) || J_IPTYPE(pL)))
			{ haveJI = TRUE; break; }

	if (!haveJI) {
		GetIndCString(strBuf, CLIPERRS_STRS, 1);		/* "You can't Cut: the only symbols selected are attached to unselected symbols." */
		CParamText(strBuf,	"", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	PrepareUndo(doc, doc->selStartL, U_Cut, 6);						/*  "Undo Cut" */
	
	CopyToClip(doc, doc->selStartL, doc->selEndL);					/* Copy selection to clipboard */
	Clear(doc);
	if (IsWindowVisible(clipboard->theWindow))
		InvalWindow(clipboard);
}


/* ---------------------------------------------------------------------- DoCopy -- */
/* Copy the selection from the score to our clipboard. */

void DoCopy(Document *doc)
{
	Boolean haveJI; LINK pL;
	
	haveJI = FALSE;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && (J_ITTYPE(pL) || J_IPTYPE(pL)))
			{ haveJI = TRUE; break; }
	if (!haveJI) {
		GetIndCString(strBuf, CLIPERRS_STRS, 2);		/* "You can't Copy: the only symbols selected are attached to unselected symbols." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	doc->undo.lastCommand = U_NoOp;

	CopyToClip(doc, doc->selStartL, doc->selEndL);				/* Copy selection to clipboard */
	if (IsWindowVisible(clipboard->theWindow))
		InvalWindow(clipboard);
}


/* -------------------------------------------------------------------------------- */
/* Functions for DoPaste. */

static void ClipCases(Document *doc, Boolean *hasClef, Boolean *hasKS, Boolean *hasTS)
{
	LINK pL;

	InstallDoc(clipboard);

	*hasClef = *hasKS = *hasTS = FALSE;
	for (pL=clipFirstMeas; pL!=clipboard->tailL; pL=RightLINK(pL)) {
		if (ClefTYPE(pL)) *hasClef = TRUE;
		else if (KeySigTYPE(pL)) *hasKS = TRUE;
		else if (TimeSigTYPE(pL)) *hasTS = TRUE;
	}
	
	InstallDoc(doc);	
}

/*
 * Handle invalidating after paste operations. The Boolean parameters refer to
 *	various special cases: a clef, key sig., or time sig. in the deleted range.
 */

static void InvalForPaste(Document *doc, LINK prevSelL, Boolean hasClef, Boolean hasKS,
									Boolean hasTS)
{
	LINK endInval;

	/* Changes in clefs and key signatures can affect notation for the rest of the
		score; if either results from the paste, the easiest thing is to clear objects'
		valid flags to the end of the score and redraw to the end of the window. In
		addition, if <showDurProb>, changes in time signatures can affect the way the
		score is drawn, so if this results from the paste, we need to redraw to the end
		of the window. For now, if any of this happens, redraw the entire window.
	
		??If measure nos. are visible, we should also redraw to the end on changes in
		Measures. This would be easy to do but would result in so much extra drawing so
		much of the time, it's hard to accept. Also, inserting Measures has the same
		problem.
	
		Note that, since we'll delete the current selection before pasting, we have to
		check both the score's selection and the clipboard for the relevant objects. */

	endInval = (hasClef || hasKS) ? doc->tailL : EndSystemSearch(doc, doc->selEndL);
	EraseAndInvalRange(doc,RightLINK(prevSelL),endInval);

	if (hasClef || hasKS || (hasTS && doc->showDurProb))
		InvalWindow(doc);	
}


/* -------------------------------------------------------- PasteDelRedAccsDialog -- */
/* Delete redundant accidentals in material just pasted in? Return TRUE to delete
them, FALSE to leave them alone. If user has answered this question before and
said not to ask again, just return the previous answer; else ask them. */

static enum {
	BUT1_DelSoft=1,
	BUT2_DelNone=2,
	CHK3_Assume=5
} E_PasteDRAItems;

/* --------------------------------------------------------------------- DoPaste -- */
/* Paste the clipboard into the selection range. */

Boolean DoPaste(Document *doc)
{
	Boolean	dontResp,delRedAccs;
	LINK		prevSelStartL;
	short		pasteType;
	Boolean	hasClef,hasKS,hasTS,befFirst,hasPgRelGraphic,	/* for special cases */
				chasClef,chasKS,chasTS;
	LINK		tupletL;
	short		stfDiff, v, userV, dummy1, dummy2;
	LINK		partL;
	PPARTINFO pPart;
	char		partName[32]; 
	char		fmtStr[256];

	pasteType = CheckMainClip(doc, DRightLINK(clipboard, clipFirstMeas), clipboard->tailL);
	if (pasteType==PasteCancel) return FALSE;

	/* If there are any tuplets spanning the insertion point and notes are to be
		pasted in the tuplet's voice, we can't paste. */

	stfDiff  = GetAnyStfDiff(doc,doc,pasteType,&dummy1,&dummy2);
	for (v=1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(clipboard,v) && VOICE_MAYBE_USED(doc,v+stfDiff)) {
			tupletL = VHasTupleAcross(doc->selEndL, v+stfDiff, FALSE);
			if (tupletL && ClipNoteInV(doc,v)) {
				Int2UserVoice(doc, v+stfDiff, &userV, &partL);
				pPart = GetPPARTINFO(partL);
				strcpy(partName, (strlen(pPart->name)>14 ? pPart->shortName : pPart->name));

				GetIndCString(fmtStr, CLIPERRS_STRS, 3);		/* "You can't paste notes in the middle of a tuplet (in voice %d of %s)." */
				sprintf(strBuf, fmtStr,	userV, partName);
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				InstallDoc(doc);
				return FALSE;
			}
		}
	InstallDoc(doc);

	PrepareUndo(doc, doc->selStartL, U_Paste, 7);				/* "Undo Paste" */
	MEHideCaret(doc);
	
	/*
	 *	Internal routines will need the selection staff, but we won't be able to
	 * call GetSelStaff because anything selected when DoPaste was called may have
	 * been deleted by then. So we call GetSelStaff here and store its result in
	 * the Document's selStaff.
	 */
	doc->selStaff = GetSelStaff(doc);

	/* If any Measure objects are only partly selected, we have to do something so
		we don't end up with a object list with Measure objs with subobjs on only
		some staves, since that is illegal. Anyway, the user probably doesn't really
		want to delete such Measures. ??Cf. Clear; for now, just avoid disasters. */
		
	if (DeselPartlySelMeasSubobjs(doc))
		OptimizeSelection(doc);

	SelRangeCases(doc,&hasClef,&hasKS,&hasTS,&befFirst,&hasPgRelGraphic);
	ClipCases(doc,&chasClef,&chasKS,&chasTS);
	hasClef |= chasClef;  hasKS |= chasKS;  hasTS |= chasTS;

#define NoDELRED
#ifdef DELRED
	/* If the last Copy to the clipboard added accidentals, decide whether to
		delete redundant accidentals in the notes we're about to paste in. */
		 
	if (clipHasAddedAccs) {
		ArrowCursor();
		delRedAccs = PasteDelRedAccsDialog();
		WaitCursor();
	}
#else
	delRedAccs = FALSE;
#endif

	prevSelStartL = LeftLINK(doc->selStartL);
	DeleteSelection(doc, TRUE, &dontResp);							/* Kill selection */
	PasteFromClip(doc, doc->selEndL,pasteType,delRedAccs);	/* Paste in clipboard */

	InitAntikink(doc, prevSelStartL, doc->selEndL);
	MEAdjustCaret(doc,FALSE);
	if (doc->autoRespace) {
		RespaceBars(doc,prevSelStartL,doc->selEndL,RESFACTOR*(long)doc->spacePercent,FALSE,FALSE);
	}
	Antikink();																/* Fix slur shapes */

	InvalForPaste(doc,prevSelStartL,hasClef,hasKS,hasTS);
	FixWholeRests(doc, RightLINK(prevSelStartL));
	FixTimeStamps(doc, RightLINK(prevSelStartL), doc->tailL); /* Overcautious but easy */

	return TRUE;
}


/* ------------------------------------------------------ PasteFromClip functions -- */

static DDIST GetClipDiffxd(Document *doc, LINK a, LINK b, DDIST c, DDIST d)
{
	DDIST xda, xdb;
	xda = DLinkXD(doc, a);
	xdb = DLinkXD(clipboard, b);
	return (DLinkXD(doc, a) - DLinkXD(clipboard, b) + c + d);
}

#define SP_PASTEAFTERBAR 	std2d(config.spAfterBar,context.staffHeight,5)		
#define STAFF1 				1
#define pLeftxd				( DLinkXD(clipboard, DFirstValidxd(doc, clipboard, DLeftLINK(clipboard,pL),TRUE)) )

/* --------------------------------------------------------------- RecalcClipxds -- */
/* Recalculate the clipboard xds so that objects in the clipboard will be properly
located horizontally for pasting into doc at insertL.
Actually stores the re-computed xds in LinkXD fields of the clipboard objects
themselves, i.e. moves objects in the clipboard graphically. */

static void RecalcClipxds(Document *doc, LINK insertL)
{
	LINK		pL, prevMeasL, startL, endL, insertLeft,
				validLeft, startRight, endLeft;
	DDIST		clipDiffxd, insertpxd, barxd;
	CONTEXT  context;
	
	/* Initialize parameters from the various clipboard globals. */
	startL = DRightLINK(clipboard, clipFirstMeas);
	endL = clipboard->tailL;
	insertpxd = pasteInsertpxd;
	barxd = clipBarxd;
	
	/* Precompute objects for calculation of xds. */
	insertLeft = LeftLINK(insertL);
	validLeft = FirstValidxd(insertLeft,TRUE);
	startRight = DFirstValidxd(doc, clipboard, startL, FALSE);
	endLeft = DFirstValidxd(doc, clipboard, DLeftLINK(clipboard, endL),TRUE);
	GetContext(doc, insertL, STAFF1, &context);
	
	switch (ObjLType(insertL)) {
		case PAGEtype:
		case SYSTEMtype:
			if (MeasureTYPE(insertLeft)) 	{
				if (DMeasureTYPE(clipboard,startL)) {
					clipDiffxd = GetClipDiffxd(doc,insertLeft,startL,SP_PASTEAFTERBAR,0);
					DMoveMeasures(doc, clipboard, startL,endL,clipDiffxd);
				}
				else {
					clipDiffxd = -DLinkXD(clipboard,startRight)+SP_PASTEAFTERBAR;
					pL = DMoveInMeasure(doc, clipboard, startL, endL, clipDiffxd);
					clipDiffxd = GetClipDiffxd(doc,validLeft,pL,pLeftxd,barxd);
					DMoveMeasures(doc, clipboard, pL, endL, clipDiffxd);			/* Move following measures */
				}
			}
			else {
				if (DMeasureTYPE(clipboard,startL)) {				
					prevMeasL = LSSearch(insertLeft, MEASUREtype, STAFF1, TRUE, FALSE);
					clipDiffxd = GetClipDiffxd(doc,prevMeasL,startL,0,0);
					clipDiffxd += CalcSpaceNeeded(doc, insertL);
					DMoveMeasures(doc, clipboard, startL, endL, clipDiffxd);		/* Move following measures */
				}
				else {
					clipDiffxd = GetClipDiffxd(doc,insertL,startRight,0,0);

					/* Assuming LinkXD of insertL (the following PAGE object) is 0; add the
					space required for the last object on the previous page. If insertL is
					a SYSTEM, its xd won't be 0; account for it by subtracting it. */
					clipDiffxd += CalcSpaceNeeded(doc, insertL);
					if (SystemTYPE(insertL)) clipDiffxd -= LinkXD(insertL);

					pL = DMoveInMeasure(doc, clipboard, startL, endL, clipDiffxd);
					prevMeasL = LSSearch(insertLeft, MEASUREtype, STAFF1, TRUE, FALSE);
					clipDiffxd = GetClipDiffxd(doc,prevMeasL,pL,pLeftxd,barxd);
					DMoveMeasures(doc, clipboard, pL, endL, clipDiffxd);				/* Move following measures */
				}
			}
			break;
		case TAILtype:
			if (MeasureTYPE(insertLeft)) 	{
				if (DMeasureTYPE(clipboard,startL)) {
					clipDiffxd = GetClipDiffxd(doc,insertLeft,startL,SP_PASTEAFTERBAR,0);
					DMoveMeasures(doc, clipboard, startL,endL,clipDiffxd);
				}
				else {
					clipDiffxd = -DLinkXD(clipboard,startRight)+SP_PASTEAFTERBAR;
					pL = DMoveInMeasure(doc, clipboard, startL, endL, clipDiffxd);
					clipDiffxd = GetClipDiffxd(doc,validLeft,pL,pLeftxd,barxd);
					DMoveMeasures(doc, clipboard, pL, endL, clipDiffxd);				/* Move following measures */
				}
			}
			else {
				if (DMeasureTYPE(clipboard,startL)) {				
					prevMeasL = LSSearch(insertLeft, MEASUREtype, STAFF1, TRUE, FALSE);
					clipDiffxd = GetClipDiffxd(doc,prevMeasL,startL,LinkXD(validLeft),insertpxd);
					DMoveMeasures(doc, clipboard, startL, endL, clipDiffxd);			/* Move following measures */
				}
				else {
					clipDiffxd = GetClipDiffxd(doc,insertL,startRight,0,0);
					pL = DMoveInMeasure(doc, clipboard, startL, endL, clipDiffxd + LinkXD(insertLeft));
					prevMeasL = LSSearch(insertLeft, MEASUREtype, STAFF1, TRUE, FALSE);
					clipDiffxd = GetClipDiffxd(doc,prevMeasL,pL,pLeftxd,barxd);
					DMoveMeasures(doc, clipboard, pL, endL, clipDiffxd);				/* Move following measures */
				}
			}
			break;
		case MEASUREtype:
			if (MeasureTYPE(insertLeft))  {				/* Case for two consec. barlines */
				if (DMeasureTYPE(clipboard,startL)) {
					clipDiffxd = GetClipDiffxd(doc,insertLeft,startL,SP_PASTEAFTERBAR,0);
					DMoveMeasures(doc, clipboard, startL, endL, clipDiffxd);							/* Move following measures */
				}
				else {
					clipDiffxd = -DLinkXD(clipboard,startRight)+SP_PASTEAFTERBAR;
					pL = DMoveInMeasure(doc, clipboard, startL, endL, clipDiffxd);
					clipDiffxd = GetClipDiffxd(doc,validLeft,pL,pLeftxd,barxd);
					DMoveMeasures(doc, clipboard, pL, endL, clipDiffxd);				/* Move following measures */
				}
			}
			else {
				if (DMeasureTYPE(clipboard,startL)) {				
					prevMeasL = LSSearch(insertLeft, MEASUREtype, STAFF1, TRUE, FALSE);
					clipDiffxd = GetClipDiffxd(doc,prevMeasL,startL,LinkXD(validLeft),insertpxd);
					DMoveMeasures(doc, clipboard, startL, endL, clipDiffxd);				/* Move following measures */
				}
				else {
					clipDiffxd = GetClipDiffxd(doc,validLeft,startRight,insertpxd,0);
					pL = DMoveInMeasure(doc, clipboard, startL, endL, clipDiffxd);
					prevMeasL = LSSearch(insertLeft, MEASUREtype, STAFF1, TRUE, FALSE);
					clipDiffxd = GetClipDiffxd(doc,prevMeasL,pL,pLeftxd,barxd);
					DMoveMeasures(doc, clipboard, pL, endL, clipDiffxd);				/* Move following measures */
				}
			}
			break;
		default:
			if (DMeasureTYPE(clipboard,startL)) {
				prevMeasL = LSSearch(insertLeft, MEASUREtype, STAFF1, TRUE, FALSE);
				clipDiffxd = GetClipDiffxd(doc,prevMeasL,startL,insertpxd,
									(MeasureTYPE(insertLeft)?SP_PASTEAFTERBAR:LinkXD(validLeft)));
				DMoveMeasures(doc, clipboard, startL, endL, clipDiffxd);					/* Move following measures */
			}
			else {
				clipDiffxd = GetClipDiffxd(doc,FirstValidxd(insertL,FALSE),startRight,0,0);
				pL = DMoveInMeasure(doc, clipboard, startL, endL, clipDiffxd);
				
				prevMeasL = LSSearch(insertLeft, MEASUREtype, STAFF1, TRUE, FALSE);
				clipDiffxd = GetClipDiffxd(doc,prevMeasL,pL,pLeftxd,barxd);
				DMoveMeasures(doc, clipboard, pL, endL, clipDiffxd);		  				/* Move following measures */
			}
			break;
	}
}


/* ---------------------------------------------------------------- PreparePaste -- */
/* Recalculate xds in the clipboard and the main object list and set the
clipboard endpoint LINKs startL, endL. RecalcClipxds actually moves clipboard
objects prior to pasting. */

static DDIST PreparePaste(Document *doc, LINK insertL, LINK *startL, LINK *endL)
{
	LINK insertLeft;
	
	insertLeft = FirstValidxd(LeftLINK(insertL), TRUE);
	pasteInsertpxd = PMDist(insertLeft, FirstValidxd(insertL, FALSE));
	UpdateTailxd(doc);		/* Force calculation of tail xd, needed by RecalcClipxds */
										
	*startL = DRightLINK(clipboard, clipFirstMeas);
	*endL = clipboard->tailL;

	RecalcClipxds(doc, insertL);
	
	return pasteInsertpxd;
}


static enum {
	PIN_DI=1,
	CANCEL_DI
} E_PINItems;

/*-------------------------------------------------------- Bad/VeryBadStaffAlert -- */
/* Alert the user that they have attempted to paste something into a staff in the
score which doesn't exist. */

static short BadStaffAlert(short clipStaves, short docStaves)
{
	DialogPtr	dialogp;
	short			ditem;
	GrafPtr		oldPort;
	char			str1[20],str2[20];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(PASTE_STFCLIP_ALRT);
		return PasteCancel;
	}
	GetPort(&oldPort);
	sprintf(str1, "%d", clipStaves);
	sprintf(str2, "%d", docStaves);
	CParamText(str1, str2, "", "");
	dialogp = GetNewDialog(PASTE_STFCLIP_ALRT, NULL, BRING_TO_FRONT);
	if (dialogp) {
		SetPort(GetDialogWindowPort(dialogp));

		ShowWindow(GetDialogWindow(dialogp));
		ArrowCursor();

		ditem = 0;
		do {
			ModalDialog(filterUPP, &ditem);
		} while (ditem==0);
		
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dialogp);
	}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(PASTE_STFCLIP_ALRT);
	}
	
	SetPort(oldPort);
	
	return (ditem==PIN_DI? PastePin : PasteCancel);
}

static void VeryBadStaffAlert(short clipStaves, short docStaves)
{
	char		str1[20],str2[20];

	sprintf(str1, "%d", clipStaves);
	sprintf(str2, "%d", docStaves);
	CParamText(str1, str2, "", "");
	StopInform(MORE_CLIPSTAVES_ALRT);
}


/* ----------------------------------------------------------- CheckStaffMapping -- */
/* Function to verify clipboard for pasting into staves other than src. If a LINK
has subobjects on staves that, when offset by the current staff doc->selStaff, are
illegal in the destination document, tell the user and return PastePin or
PasteCancel; else return PasteOK. Assumes clipboard document is installed. */ 

static short CheckStaffMapping(Document *doc, LINK startL, LINK endL)
{
	short		staff,stfDiff,partDiff,stfOff,pasteType=PasteOK,minStf,maxStf;
	Boolean	staffOK=TRUE;
	PMEVENT	p;
	HEAP		*tmpHeap;
	GenSubObj 	*subObj;
	LINK			subObjL,pL;
	
	stfDiff = GetStfDiff(doc,&partDiff,&stfOff);
	InstallDoc(clipboard);

	/* Check each object in the clipboard to determine if pasting that object
		into the score, starting at the staff the insertion pt is on, will
		result in pasting an object or subobject onto an illegal staff. */

	for (pL = startL; pL!=endL && staffOK; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case MEASUREtype:
			case PSMEAStype:
				break;
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case SYNCtype:
			case GRSYNCtype:
			case DYNAMtype:
			case RPTENDtype:
				p = GetPMEVENT(pL);
				tmpHeap = Heap + p->type;		/* p may not stay valid during loop */

				for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
					if (subObj->staffn+stfDiff>doc->nstaves)
						{ staffOK = FALSE; staff = subObj->staffn; break; }
				}	
				break;
			case SLURtype:
			case BEAMSETtype:
			case TUPLETtype:
			case OCTAVAtype:
			case GRAPHICtype:
			case ENDINGtype:
				p = GetPMEVENT(pL);
				if (((PEXTEND)p)->staffn+stfDiff>doc->nstaves)
					{ staffOK = FALSE; staff = ((PEXTEND)p)->staffn; }
				break;
			case TEMPOtype:
				if (TempoSTAFF(pL)+stfDiff>doc->nstaves)
					{ staffOK = FALSE; staff = TempoSTAFF(pL); }
				break;
			case SPACEtype:
				if (SpaceSTAFF(pL)+stfDiff>doc->nstaves)
					{ staffOK = FALSE; staff = SpaceSTAFF(pL); }
				break;
			default:
				;
		}

	if (!staffOK) {

		/* doc is the doc to be reinstalled; GetClipMinStf/GetClipMaxStf return values
			of min/maxStf in clipboard. */

		minStf = GetClipMinStf(doc);
		maxStf = GetClipMaxStf(doc);
		
		/* maxStf-minStf+1 is the range of occupied staves in the clipboard;
			if this range is greater than doc->nstaves, no pasting at all will
			be possible.
			doc->nstaves-doc->selStaff+1 is the range of staves past the doc's
			selStaff; if the range in the clipboard is greater than this, it
			will be possible to paste by pinning the range to the last staff
			of the doc. */

		if (maxStf-minStf+1 > doc->nstaves) {
			VeryBadStaffAlert(maxStf-minStf+1, doc->nstaves);
			pasteType = PasteCancel;
		}
		else
			pasteType = BadStaffAlert(maxStf-minStf+1, doc->nstaves-doc->selStaff+1);
	}

	return pasteType;
}


/* -------------------------------------------------- CrossStaff2CrossPart and ally -- */

static short GetTupletTopStaff(LINK);
static Boolean CrossStaff2CrossPart(LINK, short, short []);

/* Get the upper staff of any note in the given, presumably cross-staff, Tuplet. */

static short GetTupletTopStaff(LINK tupL)
{
	LINK aNoteTupleL, syncL, aNoteL; PANOTETUPLE aNoteTuple;
	short i, voice, topStaff;

	voice = TupletVOICE(tupL);
	topStaff = 999;
	aNoteTupleL = FirstSubLINK(tupL);
	for (i = 0; aNoteTupleL; i++, aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
		aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
		syncL = aNoteTuple->tpSync;
		aNoteL = FindMainNote(syncL, voice);
		topStaff = n_min(NoteSTAFF(aNoteL), topStaff);
	}
	
	return topStaff;
}

/* Return TRUE if pasting the given (potentially cross-staff) object with the given
<staffDiff> would cross parts. */

static Boolean CrossStaff2CrossPart(LINK pL, short staffDiff, short staffTab[])
{

	short topStf;
	
	switch (ObjLType(pL)) {
		case BEAMSETtype:
			if (!BeamCrossSTAFF(pL)) return FALSE;
			topStf = BeamSTAFF(pL)+staffDiff;
			return (staffTab[topStf]!=staffTab[topStf+1]);
		case SLURtype:
			if (!SlurCrossSTAFF(pL)) return FALSE;
			topStf = SlurSTAFF(pL)+staffDiff;
			return (staffTab[topStf]!=staffTab[topStf+1]);
		case TUPLETtype:
			/*  TupletSTAFF is of the first note in the tuplet, not necessarily the upper
				staff! By far the best solution would be to change things so it is the
				of the upper stuff. But that'd be tricky and affect existing files and
				other code (e.g., for drawing), and this inconsistency doesn't seem to cause
				any other problems. For now, we'll just handle it here. */
			if (!IsTupletCrossStf(pL)) return FALSE;
			topStf = GetTupletTopStaff(pL)+staffDiff;
			return (staffTab[topStf]!=staffTab[topStf+1]);
		default:
			return FALSE;
	}
}

/* -------------------------------------------------------- MultipleVoicesInClip -- */
/* Return TRUE if the clipboard contains stuff in two or more different voices. */

static Boolean MultipleVoicesInClip(void);
static Boolean MultipleVoicesInClip()
{
	short voice=NOONE;
	LINK pL, aNoteL,aGRNoteL;
	
	for (pL=RightLINK(clipFirstMeas); pL!=clipboard->tailL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (voice==NOONE) voice = NoteVOICE(aNoteL);
					else if (NoteVOICE(aNoteL)!=voice) return TRUE;
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
					if (voice==NOONE) voice = GRNoteVOICE(aGRNoteL);
					else if (GRNoteVOICE(aGRNoteL)!=voice) return TRUE;
				break;
			case SLURtype:
				if (voice==NOONE) voice = SlurVOICE(pL);
				else if (SlurVOICE(pL)!=voice) return TRUE;
				break;
			case BEAMSETtype:
				if (voice==NOONE) voice = BeamVOICE(pL);
				else if (BeamVOICE(pL)!=voice) return TRUE;
				break;
			case TUPLETtype:
				if (voice==NOONE) voice = TupletVOICE(pL);
				else if (TupletVOICE(pL)!=voice) return TRUE;
				break;
			case GRAPHICtype:
				if (voice==NOONE) voice = GraphicVOICE(pL);
				else if (GraphicVOICE(pL)!=voice) return TRUE;
				break;
			default:
				;
		}
	}
	
	return FALSE;
}


/* --------------------------------------------------------------------------------- */
/* Functions to determine if the user is trying to paste illegally nested slurs.
Much of the complexity is due to the fact that the function has to analyze the
paste operation as if a potentially non-empty selRange has already been removed
by DeleteSelection(), before it actually is removed. */

/* Count the number of slurs spanning doc's potential insertion point, given slurL
returned by LVSearch. */

static short CountSpanSlurs(Document *doc, short v, LINK slurL)
{
	short nSlurs=0;

	while (slurL && IsAfterIncl(doc->selEndL,SlurLASTSYNC(slurL))) {

		if (!LinkSEL(slurL) && !LinkSEL(SlurFIRSTSYNC(slurL)) && !LinkSEL(SlurLASTSYNC(slurL)))
			nSlurs++;

		slurL = LVSearch(LeftLINK(slurL),SLURtype,v,GO_LEFT,FALSE);
	}
	
	return nSlurs;
}

/* Return a slur across the selRange in voice v.

If either of the slur's endSyncs or the slur itself is selected, it will
be deleted before being pasted.

Also returns number of slurs spanning insertion point. If one, can nest
a tie; if 2, there is already a nest spanning, can paste no slurs in this
voice; if more than 2, an error. */

static LINK GetSlurAcrRange(Document *doc, short v, short *numSlurs)
{
	LINK slurL=NILINK; short nSlurs=0; Boolean foundSlur=FALSE;

	slurL = LVSearch(LeftLINK(doc->selStartL),SLURtype,v,GO_LEFT,FALSE);
	nSlurs = CountSpanSlurs(doc,v,slurL);
	
	/*
	 * Get the first slur which will remain after DeleteSelection()
	 * removes the selRange.
	 */
	while (slurL && IsAfterIncl(doc->selEndL,SlurLASTSYNC(slurL)) && !foundSlur) {

		if (!LinkSEL(slurL) && !LinkSEL(SlurFIRSTSYNC(slurL)) && !LinkSEL(SlurLASTSYNC(slurL)))
			foundSlur=TRUE;
		else
			slurL = LVSearch(LeftLINK(slurL),SLURtype,v,GO_LEFT,FALSE);

	}
	
	*numSlurs = nSlurs;
	return slurL;
}

/* Check to see if paste will create illegal slur nesting; return TRUE if nesting is
okay, FALSE if there's a problem. */

static Boolean CheckSlurNest(Document *doc, short stfDiff)
{
	short v,nSlurs; Boolean noSlur=FALSE; LINK firstMeasL,slurL,clipSlurL;
	
	InstallDoc(doc);

	/* If we are before the first measure of the score, there can be no slur
		across the insertion pt after the doc's selRange has been deleted, so
		illegal nesting will not be possible; return TRUE. */

	firstMeasL = LSSearch(LeftLINK(doc->selStartL),MEASUREtype,ANYONE,GO_LEFT,FALSE);
	if (!firstMeasL) return TRUE;

	/* If there is a slur in any voice in the doc before doc->selStartL, check if
		it spans the selRange by testing if its lastSync is after or equal to
		doc->selEndL. If it spans, see if there is a slur in the corresponding
		voice in the clipboard; if there is, the doc slur must be a slur and
		the clip slur a tie for the paste to be legal. If there is more than
		one slur in the clip's corresponding voice, the paste cannot be legal. */

	for (v=1; v<=MAXVOICES; v++,noSlur=FALSE) {
		if (VOICE_MAYBE_USED(doc,v+stfDiff) && VOICE_MAYBE_USED(clipboard,v)) {
			
			slurL = GetSlurAcrRange(doc,v+stfDiff,&nSlurs);
			if (nSlurs<1) continue;
			if (nSlurs>1) noSlur = TRUE;
			if (slurL && SlurTIE(slurL)) noSlur = TRUE;
			
			if (slurL) {
				InstallDoc(clipboard);
				
				clipSlurL = LVSearch(clipboard->headL,SLURtype,v,GO_RIGHT,FALSE);
				
				/* Clipboard slur illegal: already a spanning nest, or tie. */
	
				if (clipSlurL && noSlur)
					{ InstallDoc(doc); return FALSE; }
				
				/* Clipboard slur must be a tie */
	
				if (clipSlurL && !SlurTIE(clipSlurL))
					{ InstallDoc(doc); return FALSE; }
				
				/* Must be only one */
				
				while (clipSlurL) {
					clipSlurL = LVSearch(RightLINK(clipSlurL),SLURtype,v,GO_RIGHT,FALSE);
					if (clipSlurL && !SlurTIE(clipSlurL))
						{ InstallDoc(doc); return FALSE; }
				}
	
				InstallDoc(doc);
			}
		}
	}
	
	return TRUE;
}


/* --------------------------------------------------------------- CheckMainClip -- */
/* Check staff numbers and selection status for nodes in the main clipboard.
Also check whether any cross-staff objects are to be pasted crossing part
boundaries, since we can't handle that, and for bad situations involving Looking
at a voice and slur/tie nesting. startL and endL are clipboard nodes. */

short CheckMainClip(Document *doc, LINK startL, LINK endL)
{
	short pasteType, s, staffDiff, partDiff, stfOff, anInt;
	short staffTab[MAXSTAVES+1];
	LINK pL, lookPartL, selPartL;

	/* Determine type of mapping between staves.  */

	InstallDoc(clipboard);
	pasteType = CheckStaffMapping(doc, startL, endL);

	/* Check for problems with cross-staff objects. */
	
	InstallDoc(doc);
	for (s=1; s<=doc->nstaves; s++)
		staffTab[s] = Staff2Part(doc, s);
	staffDiff = GetAnyStfDiff(doc,doc,pasteType,&partDiff,&stfOff);

	InstallDoc(clipboard);
	for (pL=RightLINK(clipFirstMeas); pL!=clipboard->tailL; pL=RightLINK(pL))
		if ( (BeamsetTYPE(pL) && BeamCrossSTAFF(pL))
		||   (SlurTYPE(pL) && SlurCrossSTAFF(pL))
		||   (TupletTYPE(pL) && IsTupletCrossStf(pL)) )
			if (CrossStaff2CrossPart(pL, staffDiff, staffTab)) {
				InstallDoc(doc);
				GetIndCString(strBuf, CLIPERRS_STRS, 4);		/* "You can't paste a cross-staff beam, slur, or tuplet so it ends up in two different parts." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return PasteCancel;
			}
	
	/* Check for problems with Look at Voice. */
	
	if (doc->lookVoice>=0) {
		if (MultipleVoicesInClip()) {
			InstallDoc(doc);
			GetIndCString(strBuf, CLIPERRS_STRS, 5);		/* "The clipboard contains two or more voices. Please Look at All Voices, then Paste again." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return PasteCancel;
		}

		Int2UserVoice(doc, doc->lookVoice, &anInt, &lookPartL);
		Int2UserVoice(doc, doc->selStaff, &anInt, &selPartL);		/* since staff no.=default voice no. */
		if (lookPartL!=selPartL) {
			InstallDoc(doc);
			GetIndCString(strBuf, CLIPERRS_STRS, 6);		/* "You can't paste onto a staff in one part while Looking at a voice of another part." */
			CParamText(strBuf,	"", "", "");
			StopInform(GENERIC_ALRT);
			return PasteCancel;
		}
	}
	
	/* Check for problems with nested slurs. */

	if (!CheckSlurNest(doc,staffDiff)) {
		InstallDoc(doc);
		GetIndCString(strBuf, CLIPERRS_STRS, 7);			/* "You can't paste here: the clipboard contains..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return PasteCancel;
	}
	
	InstallDoc(doc);
	return pasteType;
}

/* ------------------------------------------------------------ PasteFixStfSize -- */
/* Update pasted-in range for changes in staff size. */

void PasteFixStfSize(Document *doc, LINK startL, LINK endL)
{
	/* If the score's and clipboard's standard sizes are the same, do nothing;
	 * otherwise, convert from one standard size to the other. ??If all staves of both
	 * documents are in the standard sizes, this is fine; if not, it may be okay, or
	 * (depending on the sizes of the specific staves involved) the resulting symbol
	 * y-positions may be a mess!
	 */
	if (doc->srastral == clipboard->srastral) return;
	
	SetStaffSize(doc, startL, endL, clipboard->srastral, doc->srastral);
}

/* ------------------------------------------------------------ UpdateMeasRects -- */
/*	Updates measureRects (lefts and rights) after paste,starting from first
measure object before selStartL.	*/

static void UpdateMeasRects(LINK init, LINK succ, DDIST systemWidth)
{
	LINK prevMeasL, pasteLastMeasL;
	
	prevMeasL = LSSearch(init, MEASUREtype, 1, TRUE, FALSE);
	PasteFixMeasureRect(prevMeasL, systemWidth);
	pasteLastMeasL = LSSearch(succ, MEASUREtype, 1, TRUE, FALSE);
	PasteFixMeasureRect(pasteLastMeasL, systemWidth);
}


/* --------------------------------------------------------- PasteFixMeasureRect -- */

void PasteFixMeasureRect(LINK pMeasureL,
							DDIST systemWidth)	/* Only used if pMeasure->rMeasure==NILINK */
{
	PAMEASURE	aMeasure;
	LINK			aMeasureL;
	DDIST			measureWidth;
	
	if (LastMeasInSys(pMeasureL))
		measureWidth = systemWidth-LinkXD(pMeasureL);
	else
		measureWidth = LinkXD(LinkRMEAS(pMeasureL))-LinkXD(pMeasureL);
	
	aMeasureL = FirstSubLINK(pMeasureL);
	for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		aMeasure->measureRect.left = 0;
		aMeasure->measureRect.right = measureWidth;
	}
	LinkVALID(pMeasureL) = FALSE;
}


/* ------------------------------------------------------------------ ClipMovexd -- */
/* Move objects following the pasted-in range based on the following parameters:
the xd of the first valid object to the left of the insertion point,
the xd of the clipboard, and the endxd of the clipboard.
insertL is the insertion point, before which the nodes have been pasted. */

static void ClipMovexd(Document *doc, LINK insertL)
{
	LINK pL;
	
	/* pL is the first valid node at or to the right of insertL.
		Translate everything in pL's measure by the total length of the
		clipboard. Then move all following measures by the total length
		of the clipboard. */

	pL = FirstValidxd(insertL, FALSE);
	pL = MoveInMeasure(pL, doc->tailL, clipxd);
	MoveMeasures(pL, doc->tailL, clipxd);
}

/* Required for mapping clipboard staves. The contents of the clipboard
are guaranteed to begin with a measure object. From that point on to the
tail, if there are only measures or PSMeasures, return TRUE, else return
FALSE. */

static short OnlyMeasures()
{
	LINK measL;
	
	measL = SSearch(clipboard->headL, MEASUREtype, GO_RIGHT);
	measL = RightLINK(measL);

	for ( ; MeasureTYPE(measL) || PSMeasTYPE(measL); measL = RightLINK(measL)) ;

	return (measL==clipboard->tailL);
}

/* Return TRUE if the clipboard has any notes in v. Installs doc
before returning. */

static Boolean ClipNoteInV(Document *doc, short v)
{
	LINK pL,aNoteL;

	InstallDoc(clipboard);
	
	for (pL = clipFirstMeas; pL!=clipboard->tailL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = NoteInVoice(pL,v,FALSE);
			if (aNoteL) {
				InstallDoc(doc); return TRUE;
			}
		}
	}	

	InstallDoc(doc);
	return FALSE;
}

/*
 * Return the first staff (lowest staffn) of any subobj of pL, or the
 * staffn of pL itself. If pL is a structural object, return doc->nstaves,
 * which is a value which will not enter into the computation of the minimum
 * staffn of some range.
 */

static short GetFirstStf(Document *doc, LINK pL)
{
	short minStf=1000; LINK subObjL;
	PMEVENT p; GenSubObj *subObj; HEAP *tmpHeap;

	switch (ObjLType(pL)) {

		case PAGEtype:
		case SYSTEMtype:
		case STAFFtype:
		case CONNECTtype:
		case MEASUREtype:
		case PSMEAStype:
			return doc->nstaves;

		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case SYNCtype:
		case GRSYNCtype:
		case DYNAMtype:
		case RPTENDtype:
			tmpHeap = doc->Heap + ObjLType(pL);
			p = GetPMEVENT(pL);
			
			for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
				subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
				minStf = n_min(subObj->staffn, minStf);
			}	
			return minStf;

		case SLURtype:
		case BEAMSETtype:
		case TUPLETtype:
		case OCTAVAtype:
		case GRAPHICtype:
		case ENDINGtype:
		case TEMPOtype:
		case SPACEtype:
			p = GetPMEVENT(pL);
			return ((PEXTEND)p)->staffn;
		
		default:
			return doc->nstaves;
	}
}

/*
 *	Gets the minimum staffn of any object/subObj in the clipboard, and then re-
 * installs the doc heaps of fixDoc. ??SHOULD PROBABLY TOSS THE PARAM AND USE <currentDoc>.
 */

short GetClipMinStf(Document *fixDoc)
{
	LINK pL; short minStf=1000;
	
	InstallDoc(clipboard);
	
	if (OnlyMeasures()) return 1;

	pL = SSearch(clipboard->headL, MEASUREtype, GO_RIGHT);
	pL = RightLINK(pL);

	for ( ; pL!=clipboard->tailL && minStf>1; pL=RightLINK(pL))
		minStf = n_min(GetFirstStf(clipboard, pL), minStf);

	InstallDoc(fixDoc);
	
	return minStf;
}

/*
 * Return the last staff (highest staffn) of any subobj of pL, or the
 * staffn of pL itself. If pL is a structural object, return 1, which is
 * a value which will not enter into the computation of the maximum
 * staffn of some range.
 */

static short GetLastStf(Document *doc, LINK pL)
{
	short maxStf=-1000; LINK subObjL;
	PMEVENT p; GenSubObj *subObj; HEAP *tmpHeap;

	switch (ObjLType(pL)) {

		case PAGEtype:
		case SYSTEMtype:
		case STAFFtype:
		case CONNECTtype:
		case MEASUREtype:
		case PSMEAStype:
			return 1;

		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case SYNCtype:
		case GRSYNCtype:
		case DYNAMtype:
		case RPTENDtype:
			tmpHeap = doc->Heap + ObjLType(pL);
			p = GetPMEVENT(pL);
			
			for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
				subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
				maxStf = n_max(subObj->staffn, maxStf);
			}	
			return maxStf;

		case SLURtype:
		case BEAMSETtype:
		case TUPLETtype:
		case OCTAVAtype:
		case GRAPHICtype:
		case ENDINGtype:
		case TEMPOtype:
		case SPACEtype:
			p = GetPMEVENT(pL);
			return ((PEXTEND)p)->staffn;
		
		default:
			return 1;
	}
}

/*
 *	Gets the maximum staffn of any object/subObj in the clipboard, and then re-
 * installs the doc heaps of fixdoc. ??SHOULD PROBABLY TOSS THE PARAM AND USE <currentDoc>.
 */

short GetClipMaxStf(Document *fixDoc)
{
	LINK pL; short maxStf=-1000;
	
	InstallDoc(clipboard);
	
	if (OnlyMeasures()) return 1;

	pL = SSearch(clipboard->headL, MEASUREtype, GO_RIGHT);
	pL = RightLINK(pL);

	for ( ; pL!=clipboard->tailL && maxStf<clipboard->nstaves; pL=RightLINK(pL))
		maxStf = n_max(GetLastStf(clipboard, pL), maxStf);

	InstallDoc(fixDoc);
	
	return maxStf;
}

/* minStaff is the minimum staffn of any object/subObj in the
clipboard. minPart is the part in the clipboard containing minStaff.
selPart is the part in fixDoc containing the insertion point.
stfOff is set to the offset in the clipboard of minStaff in its part.
partDiff is the difference in parts between minPart and selPart.  
Returns the difference in staves of fixDoc->selStaff and minStaff. */

short GetStfDiff(Document *fixDoc, short *partDiff, short *stfOff)
{
	short minStaff, minPart, staffDiff, selPart; LINK partL;

	minStaff = GetClipMinStf(fixDoc);				/* minStf of any subObj/obj in clip */

	InstallDoc(clipboard);
	minPart = Staff2Part(clipboard, minStaff);	/* Part containing minStf */

	partL = FindPartInfo(clipboard, minPart);		/* OffSet in clip of minStf in its part */
	*stfOff = minStaff-GetPPARTINFO(partL)->firstStaff;

	InstallDoc(fixDoc);
	selPart = Staff2Part(fixDoc, fixDoc->selStaff);	/* Part containing insPt in fixDoc */
	staffDiff = fixDoc->selStaff-minStaff;			/* Diff in staves of selStf, minStf */
	*partDiff = selPart-minPart;						/* Diff in parts of selPart,minPart */ 						

	return staffDiff;
}

/* -------------------------------------------------------------- GetAnyStfDiff -- */
/* Like GetStfDiff, but it adjusts for <PastePin> . */

static short GetAnyStfDiff(Document *doc, Document *fixDoc, short pasteType,
									short *partDiff, short *stfOff)
{
	short staffDiff,minStf,maxStf,stfRange,overStf;

	staffDiff = GetStfDiff(fixDoc,partDiff,stfOff);
	if (pasteType==PastePin) {
		minStf = GetClipMinStf(fixDoc);
		maxStf = GetClipMaxStf(fixDoc);

		stfRange = maxStf-minStf+1;										/* +1 gets the correct value for the range */
	
		overStf = (stfRange+doc->selStaff)-(doc->nstaves+1);		/* +1 corrects for 1-based indexing of doc->selStaff */
		staffDiff -= overStf;
	}
	
	return staffDiff;
}

/* ------------------------------------------------------------------- MapStaves -- */
/* Translate the staff numbers of LINKs in the range [startL,endL) by staffDiff. */

void MapStaves(Document *doc, LINK startL, LINK endL, short staffDiff)
{
	LINK 			pL,subObjL;
	PMEVENT		p;
	GenSubObj 	*subObj;
	HEAP 			*tmpHeap;

	for (pL = startL; pL!=endL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case PAGEtype:
			case SYSTEMtype:
			case STAFFtype:
			case CONNECTtype:
			case MEASUREtype:
			case PSMEAStype:
				break;
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case SYNCtype:
			case GRSYNCtype:
			case DYNAMtype:
			case RPTENDtype:
				tmpHeap = doc->Heap + ObjLType(pL);
				p = GetPMEVENT(pL);
				
				for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
					subObj->staffn += staffDiff;
				}	
				break;
			case SLURtype:
			case BEAMSETtype:
			case TUPLETtype:
			case OCTAVAtype:
			case GRAPHICtype:
			case ENDINGtype:
			case TEMPOtype:
			case SPACEtype:
				p = GetPMEVENT(pL);
				((PEXTEND)p)->staffn += staffDiff;
				break;
		}
	}
}


/* =================================================== Voice-remapping functions == */

static VOICEINFO clip2DocVoiceTab[MAXVOICES+1];

static LINK ClipStf2Part(short stf,Document *doc,short stfDiff,short *partStf);
static short Part2UserVoice(short cIV,short cUV,Document *doc,LINK dPartL);
static void MapVoice(Document *doc,LINK pL,short stfDiff);
static void MapVoices(Document *doc,LINK startL,LINK endL,short stfDiff);

static LINK ClipStf2Part(short stf, Document *doc, short stfDiff, short *partStf)
{
	*partStf = stf+stfDiff;
	return Staff2PartL(doc,doc->headL,*partStf);
}

/* Given clipboard internal voice no. and score part, if we have a user voice assigned
in the document for pasting the clipboard voice into, return it; else add one to our
table in that part and return it. */
		
static short Part2UserVoice(
					short cIV, short /*cUV*/,	/* clipboard internal and (unused) user voice nos. */
					Document *doc, LINK dPartL)
{
	short v, partn, uV=-1;

	uV = clip2DocVoiceTab[cIV].relVoice;
	if (clip2DocVoiceTab[cIV].partn>0) return uV;

	/* We don't yet have a (user) voice number assigned in the document for pasting
		this voice into. Find the lowest number that's not used in the part, add
		it to <clip2DocVoiceTab>, and return it. ??Loop is very confusing and looks
		unsafe in that it assumes user voice nos. in a given part are non-decreasing,
		but it certainly seems to work in ordinary cases. */
		
	partn = PartL2Partn(doc,dPartL);
	for (uV = 1, v = 1; v<=MAXVOICES; v++)
		if (clip2DocVoiceTab[v].partn==partn && clip2DocVoiceTab[v].relVoice==uV)
			uV = clip2DocVoiceTab[v].relVoice+1;

	clip2DocVoiceTab[cIV].partn = partn;
	clip2DocVoiceTab[cIV].relVoice = uV;
	return uV;
}

/* Given staff number and internal voice number in the clipboard and <stfDiff>,
return internal voice number in the document we're pasting into. Leaves <doc>
installed.

This function should translate user voices in as intuitive a way as possible. In
fact, it always preserves default voices, which is a good start; but beyond that,
it doesn't do very well. In particular, if the staff offset in the destination part
is zero (e.g., copying both staves of a 2-staff part to another *or the same*
2-staff part), ALL user voice numbers should be preserved, but they're not. The
equivalent function for Double does much better, and its technique should work fine
here. Cf. DblSetupVMap. */

short NewVoice(Document *doc, short stf,
				short cIV,					/* Internal voice no. in clipboard */
				short stfDiff)
{
	LINK cPartL,dPartL; short uV,partStf,cUV;

	/* If we're Looking at a Voice, everything goes into that voice. */
	
	if (doc->lookVoice>=0)
		return doc->lookVoice;
	else {
		InstallDoc(clipboard);
		
		/* ??Since Part2UserVoice ignores its 2nd param., the call to Int2UserVoice is
			effectively ignored! No wonder this isn't handling non-dflt voices well! */
			
		Int2UserVoice(clipboard, cIV, &cUV, &cPartL);					/* iV => uV: clip */
		InstallDoc(doc);
		dPartL = ClipStf2Part(stf,doc,stfDiff,&partStf);				/* p => p: clip => doc */
		uV = Part2UserVoice(cIV,cUV,doc,dPartL);							/* p => uV: doc */
		return User2IntVoice(doc, uV, dPartL);								/* uV => iV: doc */	
	}
}


/* If the given object has voice nos., update its voice no. or the voice nos. of all
its subobjects for the given staff-number offset. */

static void MapVoice(Document *doc, LINK pL, short stfDiff)
{
	LINK aNoteL,aGRNoteL; short voice;

	switch (ObjLType(pL)) {
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				voice = NewVoice(doc,NoteSTAFF(aNoteL),NoteVOICE(aNoteL),stfDiff);
				NoteVOICE(aNoteL) = voice;
			}
			break;
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				voice = NewVoice(doc,GRNoteSTAFF(aGRNoteL),GRNoteVOICE(aGRNoteL),stfDiff);
				GRNoteVOICE(aGRNoteL) = voice;
			}
			break;
		case SLURtype:
			voice = NewVoice(doc,SlurSTAFF(pL),SlurVOICE(pL),stfDiff);
			SlurVOICE(pL) = voice;
			break;
		case BEAMSETtype:
			voice = NewVoice(doc,BeamSTAFF(pL),BeamVOICE(pL),stfDiff);
			BeamVOICE(pL) = voice;
			break;
		case TUPLETtype:
			voice = NewVoice(doc,TupletSTAFF(pL),TupletVOICE(pL),stfDiff);
			TupletVOICE(pL) = voice;
			break;
		case GRAPHICtype:
			voice = NewVoice(doc,GraphicSTAFF(pL),GraphicVOICE(pL),stfDiff);
			GraphicVOICE(pL) = voice;
			break;
		default:
			;
	}
}

/* Set up a voice mapping-table that indexes on clipboard (internal) voices and
gives the equivalent user voices and parts in the score into which the clipboard
is being pasted. Note that this function only fills in the table entry for the
default voice for each staff; if any non-default voices are used, they can have
entries added later, or they could be handled entirely "on the fly". Also note
that if we're Looking at a Voice, this table should be ignored and everything should
map to that voice.

Requires doc's heaps to be installed and leaves same doc installed. NB: as of v.1.02,
vMapTable and NewVoice function are used by Merge as well as Paste. */

void InitVMapTable(Document *doc, short stfDiff)	/* doc is target score for the paste */
{
	short v; LINK partL;
	
	for (v = 1; v<=MAXVOICES; v++)
		clip2DocVoiceTab[v].partn = 0;
	
	for (v = 1; v<=clipboard->nstaves; v++) {
		/*
		 * Anything in voice v (the default voice for staff v) in the clipboard should
		 * be pasted into the default voice for staff v+stfDiff in the doc.
		 */
		if (v+stfDiff>=1 && v+stfDiff<=doc->nstaves) {
			clip2DocVoiceTab[v].partn = Staff2Part(doc, v+stfDiff);
			partL = Staff2PartL(doc, doc->headL, v+stfDiff);
			clip2DocVoiceTab[v].relVoice = v+stfDiff-PartFirstSTAFF(partL)+1;
		}
	}
}

/* Translate the voice numbers of LINKs in the range [startL,endL) for a staff-
number offset of stfDiff. */

static void MapVoices(Document *doc, LINK startL, LINK endL, short stfDiff)
{
	LINK pL;
	
	InitVMapTable(doc,stfDiff);
#ifdef DEBUG_VMAPTABLE
	for (v = 1; v<=MAXVOICES; v++)
		if (clip2DocVoiceTab[v].partn!=0)
			DebugPrintf("%ciVoice %d pastes to part %d relVoice=%d\n",
							(v==1? '•' : ' '),
							v, clip2DocVoiceTab[v].partn, clip2DocVoiceTab[v].relVoice);
#endif
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		MapVoice(doc,pL,stfDiff);
}


/* -------------------------------------------------------------------------------- */
/* Charlie's (nice) utilities for Voice maps, formerly in Merge.c. NB: these are
currently used only by Merge, NOT by Paste Insert--though they should be! */

void SetNewVoices(Document *doc, short *vMap, LINK pL, short stfDiff);

/* Set up a mapping for any voices in which pL may participate, either the object
	or the subObj: from that sub/Object's original voice v to the new voice contained
	in position v of the table. NB: NewVoice installs doc, which is not what we want
	here! By far the best solution would be to make NewVoice re-install the installed
	doc; for now, just re-install clipboard immediately after each call to NewVoice. */

void SetNewVoices(Document *doc, short *vMap, LINK pL, short stfDiff)
{
	LINK aNoteL,aGRNoteL; short voice;

	switch (ObjLType(pL)) {
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (vMap[NoteVOICE(aNoteL)] == NOONE) {
					voice = NewVoice(doc,NoteSTAFF(aNoteL),NoteVOICE(aNoteL),stfDiff);
					InstallDoc(clipboard);
					vMap[NoteVOICE(aNoteL)] = voice;
				}
			}
			break;
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				if (vMap[GRNoteVOICE(aGRNoteL)] == NOONE) {
					voice = NewVoice(doc,GRNoteSTAFF(aGRNoteL),GRNoteVOICE(aGRNoteL),stfDiff);
					InstallDoc(clipboard);
					vMap[GRNoteVOICE(aGRNoteL)] = voice;
				}
			}
			break;
		case SLURtype:
			if (vMap[SlurVOICE(pL)] == NOONE) {
				voice = NewVoice(doc,SlurSTAFF(pL),SlurVOICE(pL),stfDiff);
				InstallDoc(clipboard);
				vMap[SlurVOICE(pL)] = voice;
			}
			break;
		case BEAMSETtype:
			if (vMap[BeamVOICE(pL)] == NOONE) {
				voice = NewVoice(doc,BeamSTAFF(pL),BeamVOICE(pL),stfDiff);
				InstallDoc(clipboard);
				vMap[BeamVOICE(pL)] = voice;
			}
			break;
		case TUPLETtype:
			if (vMap[TupletVOICE(pL)] == NOONE) {
				voice = NewVoice(doc,TupletSTAFF(pL),TupletVOICE(pL),stfDiff);
				InstallDoc(clipboard);
				vMap[TupletVOICE(pL)] = voice;
			}
			break;
		case GRAPHICtype:
			if (vMap[GraphicVOICE(pL)] == NOONE) {
				voice = NewVoice(doc,GraphicSTAFF(pL),GraphicVOICE(pL),stfDiff);
				InstallDoc(clipboard);
				vMap[GraphicVOICE(pL)] = voice;
			}
			break;
		default:
			;
	}
}

/* Given a staff-number offset, set up voice mapping for the clipboard. InitVMapTable
	has not yet been handled correctly(??what does this mean?). NB: this function leaves
	doc's heaps installed when it returns. */

void SetupVMap(Document *doc, short *vMap, short stfDiff)
{
	short v; LINK pL,clipFirstMeas;

	/* Initialize the vMap table: (internal) old voice -> new voice. A value of NOONE at
		position v in the table indicates no mapping has been established for voice v. */

	for (v=0; v<MAXVOICES+1; v++)
		vMap[v] = NOONE;

	/* Initialize the VOICEINFO (standard voice-mapping) table: (internal) voice ->
		user voice and part, for default voices only. */
	
 	InitVMapTable(doc,stfDiff);

	InstallDoc(clipboard);
	
	clipFirstMeas = LSSearch(clipboard->headL,MEASUREtype,ANYONE,GO_RIGHT,FALSE);

	for (pL=clipFirstMeas; pL!=clipboard->tailL; pL=RightLINK(pL)) {
		SetNewVoices(doc,vMap,pL,stfDiff);
	}

	InstallDoc(doc);
}


/* -------------------------------------------------------------- PasteMapStaves -- */
/* Update the staff numbers after the paste operation: re-number the staves
of all objects so that they are pasted in starting at the staff of the
insertion point. Also re-number the voices of all objects that have voice
numbers.

This function is much simpler than the corresponding function in Extract.c.
The paste operation cannot paste any structural objects, and measure objects
must be pasted in in entirety, and therefore they alone will not have any of
their staffns re-numbered, and therefore also their connStaffs, etc will
not be re-numbered. */

static short PasteMapStaves(Document *doc, Document *newDoc, LINK startL, LINK endL,
										short pasteType)
{
	short staffDiff,partDiff,stfOff;

	InstallDoc(newDoc);
	staffDiff = GetAnyStfDiff(doc,newDoc,pasteType,&partDiff,&stfOff);

	/* MapVoices depends on staff numbers being as they were in the clipboard,
		so it must be called before MapStaves. NB: Even if staffDiff=0, MapVoices
		must be called, at least to handle lookVoice>0! */
		
	MapVoices(newDoc,startL,endL,staffDiff);
	MapStaves(newDoc,startL,endL,staffDiff);
	InstallDoc(doc);

	return staffDiff;
}

/* --------------------------------------------------------- PasteFixMeasStruct -- */
/* Fix up the part-staff structure of measures in the range [startL,endL). */

void PasteFixMeasStruct(Document *doc, LINK startL, LINK endL, Boolean fixMeasure)
{
	LINK pL, beforeStartL;
	LINK measL,aMeasL,fixMeasL,staffL,newMeas,insertL,copyL,rightL,sysL;
	PAMEASURE aMeas,fixMeas; Boolean replaceMeas=FALSE;
	DDIST xd; Rect bbox;
	
	measL = LSSearch(LeftLINK(startL),MEASUREtype,ANYONE,GO_LEFT,FALSE);
	staffL = LSSearch(LeftLINK(startL),STAFFtype,ANYONE,GO_LEFT,FALSE);

	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (MeasureTYPE(pL)) {
			newMeas = pL;
			if (LinkNENTRIES(newMeas)!=LinkNENTRIES(measL))
				replaceMeas = TRUE;
			break;
		}

	if (fixMeasure) {
		if (replaceMeas) {
			beforeStartL = LeftLINK(startL);
			for (pL=startL; pL!=endL; pL=rightL) {
				rightL = RightLINK(pL);
				if (MeasureTYPE(pL)) {
					insertL = RightLINK(pL);
					xd = LinkXD(pL);
					bbox = MeasureBBOX(pL);
					copyL = DuplicateObject(MEASUREtype,measL,FALSE,doc,doc,FALSE);
					LinkVALID(copyL) = FALSE;
					LinkVIS(copyL) = TRUE;
					LinkXD(copyL) = xd;
					MeasureBBOX(copyL) = bbox;
					SetMeasVisible(copyL, TRUE);
					DeleteNode(doc,pL);
					InsNodeInto(copyL, insertL);
				}
			}
			FixStructureLinks(doc,doc,beforeStartL,endL);
		}
		else {
			for (pL=startL; pL!=endL; pL=RightLINK(pL)) {
				if (MeasureTYPE(pL)) {
					aMeasL = FirstSubLINK(measL);
					fixMeasL = FirstSubLINK(pL);
					for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL))
						for ( ; fixMeasL; fixMeasL=NextMEASUREL(fixMeasL))
							if (MeasureSTAFF(fixMeasL)==MeasureSTAFF(aMeasL)) {
								aMeas = GetPAMEASURE(aMeasL);
								fixMeas = GetPAMEASURE(fixMeasL);
								fixMeas->connStaff = aMeas->connStaff;
								fixMeas->connAbove = aMeas->connAbove;
								break;
							}
				}
			}
		}
	}

	/* If user has tweaked the score format during look at format, the measureBBoxes
		may extend outside of the system rects; fix this here. */
 
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (MeasureTYPE(pL)) {
			LINK prevMeasL,aMeasL,bMeasL; DRect mRectA,mRectB;

			sysL = LSSearch(pL,SYSTEMtype,ANYONE,GO_LEFT,FALSE);
			bbox = MeasureBBOX(pL);
			bbox.top = d2p(SystemRECT(sysL).top);
			bbox.bottom = d2p(SystemRECT(sysL).bottom);
			MeasureBBOX(pL) = bbox;
			prevMeasL = LSSearch(LeftLINK(pL),MEASUREtype,ANYONE,GO_LEFT,FALSE);
			aMeasL = FirstSubLINK(pL);
			bMeasL = FirstSubLINK(prevMeasL);
			for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL),bMeasL=NextMEASUREL(bMeasL)) {
				mRectA = MeasMRECT(aMeasL);
				mRectB = MeasMRECT(bMeasL);
				mRectA.top = mRectB.top;
				mRectA.bottom = mRectB.bottom;
				MeasMRECT(aMeasL) = mRectA;
			}
		}
}

/* --------------------------------------------------------------- PasteUpdate - */
/* Update xds, rects, etc., and handle re-drawing after the paste operation. */

void PasteUpdate(Document *doc, LINK initL, LINK succL, DDIST systemWidth)
{
	WaitCursor();

	ClipMovexd(doc, succL);
	doc->changed = TRUE;
	UpdateTailxd(doc);
	
	PasteFixStfSize(doc, RightLINK(initL), succL);
	UpdateMeasRects(initL, succL, systemWidth);
	if (UpdateMeasNums(doc, NILINK) && doc->numberMeas!=0)
		InvalWindow(doc);
	else
		InvalSystem(initL);
	doc->selStartL = doc->selEndL = succL;					/* Insertion pt. after pasted nodes */
}


/* ------------------------------------------------------- PasteFixOctavas et al -- */
/* Update v position and inOctava status for all notes on staff <s> in
range [startL,endL). */

void FixOctNotes(Document *doc,LINK octL,short s);

/* UnOctavaRange is overkill; want simplified version which just removes
	a single octava; not all octavas in range.
	Problem: needSelected and doOct parameters are unclear in their application.
	This may leave the selection status of the newly created octava incorrect,
	though it should still be consistent. */

static void FixOctNotes(Document *doc, LINK octL, short s)
{
	LINK firstL,lastL; short numNotes;
	Byte octType = OctType(octL);

	firstL = FirstInOctava(octL);
	lastL = LastInOctava(octL);
	numNotes = OctCountNotesInRange(s,firstL,RightLINK(lastL), FALSE);
	
	if (numNotes!=LinkNENTRIES(octL)) {
		UnOctavaRange(doc,firstL,RightLINK(lastL),s);
		CreateOCTAVA(doc,firstL,RightLINK(lastL),s,numNotes,octType,FALSE,FALSE);
	}
}

/* Don't need to process every octava in the range, since the only possible case
	requiring updating of the octava is that in which notes have been inserted
	inbetween FirstInOctava and LastInOctava; the single octava on any staff for
	this case will be the one gotten by searching from startL to the left for
	an octava.
	Problem: we want to call GetStfSelRange(doc,s,&stfStartL,&stfEndL); and
	search to the left from stfStartL. But by the time we get here, the selRange
	is empty (either initially an insertion pt or deleted by DeleteSelection),
	so GetStfSelRange delivers NILINKs. Need to search right from clipStfStartL,
	but such a node is not immediately available. This will only affect obscure
	cases where the selRange is backwards (or upside-down backwards) L-shaped,
	and there is an octava in the L's cutout, into the middle of which notes
	have been inserted. */
 
void PasteFixOctavas(Document *doc, LINK startL, short s)
{
	LINK octL;
	
	octL = LSSearch(startL,OCTAVAtype,s,GO_LEFT,FALSE);
	
	if (octL) FixOctNotes(doc,octL,s);
}

/* ------------------------------------------------------------- PasteFixBeams -- */
/* Update v position and inBeam status for all notes in voice <v> in range
[startL,endL). */

static void PasteRebeam(Document *doc,LINK pL,short nInRange);
static void FixBeamNotes(Document *doc,LINK beamL,short v);

/* Rebeam the notes beamed by <pL>. */

static void PasteRebeam(Document *doc, LINK pL, short nInRange)
{
	LINK firstSyncL,lastSyncL; Boolean graceBeam; short v;

	graceBeam = GraceBEAM(pL); v = BeamVOICE(pL);
	RemoveBeam(doc, pL, v, FALSE);
	firstSyncL = FirstInBeam(pL);
	lastSyncL = LastInBeam(pL);

	if (graceBeam) {
		if (nInRange>1)
			CreateGRBEAMSET(doc, firstSyncL, RightLINK(lastSyncL), v,
				nInRange, FALSE, FALSE);
	}
	else {
		if (nInRange>1)
			CreateBEAMSET(doc, firstSyncL, RightLINK(lastSyncL), v,
				nInRange, FALSE, doc->voiceTab[v].voiceRole);
	}
}


/* --------------------------------------------------------- PasteFixBeams et al -- */
/* Regardless of whether the range is beamed now, returns the number of beamable
Syncs in the range. Notice that the range can't really be beamed if the result is 1.
Similar to CountBeamable, except that function considers the range unbeamable if
anything in it in the voice is currently beamed or has an unbeamable duration; this
function doesn't care. */

short PFCountBeamable(Document *doc, LINK, LINK, short, Boolean, Boolean);
short PFCountBeamable(
				Document *doc,
				LINK startL, LINK endL,
				short voice,
				Boolean beamRests,		/* TRUE=treat rests like notes (as in doc->beamRests) */
				Boolean /*needSelected*//* TRUE if we only want selected items */
				)
{
	short		nbeamable;
	LINK		pL, aNoteL, firstNoteL, lastNoteL;

	if (voice<1 || voice>MAXVOICES) return 0;
	if (endL==startL) return 0;
	
	firstNoteL = lastNoteL = NILINK;
	for (nbeamable = 0, pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteVOICE(aNoteL)==voice)	{
					if (!firstNoteL) firstNoteL = aNoteL;
					lastNoteL = aNoteL;
					if (!doc->beamRests && NoteREST(aNoteL)) break;	/* Really a rest? Ignore for now */
					nbeamable++;
					break;													/* Still possible */
				}
			}
		}

	/*
	 *	If !beamRests, we've ignored all rests, but we shouldn't ignore the first and
	 * last "notes" if they're rests AND there's at least one genuine notes.
	 */
	if (!beamRests && nbeamable>0) {
		if (NoteREST(firstNoteL)) nbeamable++;
		if (NoteREST(lastNoteL)) nbeamable++;
	}

	return nbeamable;
}


static short GRBeamNotesInRange(
						Document */*doc*/,							/* unused */
						short v, LINK startL, LINK endL)
{
	short numNotes=0;
	LINK pL, aGRNoteL;

	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
				if (GRNoteVOICE(aGRNoteL)==v)
					if (GRMainNote(aGRNoteL))
						numNotes++;
		}
	return numNotes;
}

/* If notes have been pasted into the middle of the given beamset, rebeam
that beamset. We decide if notes have been pasted into it by comparing the
number of notes (counting chords as 1) in its range now to the number of
subobjs in the beamset. For ordinary (non-grace) beams, this method has the
following problem: Say the user changes doc->beamRests from TRUE to FALSE.
Beam previously contained <n> rests, user pastes <n> notes. => numNotes will
equal LinkNENTRIES(beamL), but beamL still needs to be re-created.
Solution: We store the value of doc->beamRests inside the beam object when
it's created (by CreateBEAMSET); beams are updated by re-creating them with
CreateBEAMSET for any case that this flag's value will depend on, so that
should be adequate.
Then, here, count the number of objects in range based on the beam object's
previous setting of beamRests, compare this to LinkNENTRIES(beamL), count the
number of objects in range based on beam object's new setting of beamRests,
and pass this as <numNotes> to PasteRebeam. If doc->beamRests has the same
value as pBeam->beamRests, there is no problem.

Re-create the beam with the new setting of doc->beamRests, and include rests
based on this setting as well.

Grace-note beams can never contain rests, so they can't have this problem. */

static void FixBeamNotes(Document *doc, LINK beamL, short v)
{
	LINK firstL,lastL;
	short numNotes,prevNumNotes;

	firstL = FirstInBeam(beamL);
	lastL = LastInBeam(beamL);

	if (GraceBEAM(beamL)) {
		numNotes = GRBeamNotesInRange(doc,v,firstL,RightLINK(lastL));
		if (numNotes!=LinkNENTRIES(beamL))
			PasteRebeam(doc, beamL, numNotes);
	}
	else {
		prevNumNotes = PFCountBeamable(doc,firstL,RightLINK(lastL),v,BeamRESTS(beamL),FALSE);
		if (prevNumNotes!=LinkNENTRIES(beamL)) {
			numNotes = PFCountBeamable(doc,firstL,RightLINK(lastL),v,doc->beamRests,FALSE);
			PasteRebeam(doc, beamL, numNotes);
		}
	}
}


/* Don't need to process every Beam in the range, since the only possible case
	requiring updating of the Beam is that in which notes have been inserted
	inbetween FirstInBeam and LastInBeam; the single Beam on any staff for
	this case will be the one gotten by searching from startL to the left for
	an Beam.
	Problem: we want to call GetStfSelRange(doc,s,&stfStartL,&stfEndL); and
	search to the left from stfStartL. But by the time we get here, the selRange
	is empty (either initially an insertion pt or deleted by DeleteSelection),
	so GetStfSelRange delivers NILINKs. Need to search right from clipStfStartL,
	but such a node is not immediately available. This will only affect obscure
	cases where the selRange is backwards (or upside-down backwards) L-shaped,
	and there is an Beam in the L's cutout, into the middle of which notes
	have been inserted. */
 
void PasteFixBeams(Document *doc, LINK startL, short v)
{
	LINK beamL;

	beamL = LVSearch(startL,BEAMSETtype,v,GO_LEFT,FALSE); /* ??MAY BE FAR BEFORE RANGE! */

	if (beamL) FixBeamNotes(doc,beamL,v);
}

/* --------------------------------------------------------- PasteFixAllContexts -- */
/* After pasting, fix up clef, keySig, timeSig, and dynamic contexts starting
at <startL>, on staff <s>. */

static void PasteFixAllContexts(Document *doc, LINK startL, LINK endL, short s,
											CONTEXT oldContext, CONTEXT newContext)
{
	KSINFO oldKSInfo, newKSInfo;
	TSINFO timeSigInfo;

	if (newContext.clefType!=oldContext.clefType)
		EFixContForClef(doc, startL, endL, s, oldContext.clefType, newContext.clefType, newContext);

	/* BlockCompare returns non-zero value if the arguments are different */

	if (BlockCompare(&oldContext.KSItem,&newContext.KSItem,7*sizeof(KSITEM))) {
		KeySigCopy((PKSINFO)oldContext.KSItem, &oldKSInfo);
		KeySigCopy((PKSINFO)newContext.KSItem, &newKSInfo);
		EFixContForKeySig(startL, endL, s, oldKSInfo, newKSInfo);
		EFixAccsForKeySig(doc, startL, endL, s, oldKSInfo, newKSInfo);
	}
	
	if (oldContext.numerator!=newContext.numerator || oldContext.denominator!=newContext.denominator) {
		timeSigInfo.TSType = newContext.timeSigType;
		timeSigInfo.numerator = newContext.numerator;
		timeSigInfo.denominator = newContext.denominator;
		EFixContForTimeSig(startL, endL, s, timeSigInfo);
	}

	if (oldContext.dynamicType!=newContext.dynamicType)
		EFixContForDynamic(startL, endL, s, oldContext.dynamicType, newContext.dynamicType);
}

/* Fix up measure context for all measures in the system containing the pasted
in range. Requires assumption that pasted in material be within a single system,
but would be straightforward to extend to multiple systems.
Assumed use is to handle case where user is pasting into different staves than
source, and a context-establishing object (clef,keysig,timesig,dynamic) is pasted
into a different staff than its original. This action causes the context fields
in the measure (e.g. clefType) to become obselete, which would cause problems
for context routines (e.g. EFixContextForClef) if they were not updated. */

static void PasteFixSysMeasContexts(Document *doc, LINK initL)
{
	LINK sysL,measL,nextSysL,endMeasL,prevMeasL,aMeasL,aPrevMeasL,
			clefL,aClefL,keySigL,aKSL,timeSigL,aTSL,dynamL;
	short i,k; SearchParam pbSearch;
	PAMEASURE aMeas,aPrevMeas; PAKEYSIG aKeySig; PATIMESIG aTimeSig;

	InstallDoc(doc);
	sysL = LSSearch(initL,SYSTEMtype,ANYONE,GO_LEFT,FALSE);
	prevMeasL = measL = SSearch(sysL,MEASUREtype,GO_RIGHT);

	nextSysL = LinkRSYS(sysL);
	endMeasL = nextSysL ? SSearch(nextSysL,MEASUREtype,GO_RIGHT) : NILINK;

	InitSearchParam(&pbSearch);
	pbSearch.voice = ANYONE;
	pbSearch.needSelected = FALSE;
	pbSearch.needInMeasure = TRUE;
	pbSearch.inSystem = TRUE;

	measL=LinkRMEAS(measL);
	for ( ; measL!=endMeasL; prevMeasL=measL,measL=LinkRMEAS(measL))
		for (i=1; i<=doc->nstaves; i++ ) {
			aMeasL = MeasOnStaff(measL,i);
			aPrevMeasL = MeasOnStaff(prevMeasL,i);
			pbSearch.id = i;
			clefL = L_Search(measL,CLEFtype,GO_LEFT,&pbSearch);
			if (clefL && IsAfter(prevMeasL,clefL)) {
				aClefL = pbSearch.pEntry;
				MeasCLEFTYPE(aMeasL) = ClefType(aClefL);
			}
			else
				MeasCLEFTYPE(aMeasL) = MeasCLEFTYPE(aPrevMeasL);

			keySigL = L_Search(measL,KEYSIGtype,GO_LEFT,&pbSearch);
			if (keySigL && IsAfter(prevMeasL,keySigL)) {
				aKSL = pbSearch.pEntry;
				aMeas = GetPAMEASURE(aMeasL);
				aKeySig = GetPAKEYSIG(aKSL);
				aMeas->nKSItems = aKeySig->nKSItems;
				for (k = 0; k<aKeySig->nKSItems; k++)
					aMeas->KSItem[k] = aKeySig->KSItem[k];
			}
			else {
				aMeas = GetPAMEASURE(aMeasL);
				aPrevMeas = GetPAMEASURE(aPrevMeasL);
				aMeas->nKSItems = aPrevMeas->nKSItems;
				for (k = 0; k<aPrevMeas->nKSItems; k++)
					aMeas->KSItem[k] = aPrevMeas->KSItem[k];
			}
			timeSigL = L_Search(measL,TIMESIGtype,GO_LEFT,&pbSearch);
			if (timeSigL && IsAfter(prevMeasL,timeSigL)) {
				aTSL = pbSearch.pEntry;
				aMeas = GetPAMEASURE(aMeasL);
				aTimeSig = GetPATIMESIG(aTSL);
				aMeas->timeSigType = aTimeSig->subType;
				aMeas->numerator = aTimeSig->numerator;
				aMeas->denominator = aTimeSig->denominator;
			}
			else {
				aMeas = GetPAMEASURE(aMeasL);
				aPrevMeas = GetPAMEASURE(aPrevMeasL);
				aMeas->timeSigType = aPrevMeas->subType;
				aMeas->numerator = aPrevMeas->numerator;
				aMeas->denominator = aPrevMeas->denominator;
			}
			dynamL = L_Search(measL,DYNAMtype,GO_LEFT,&pbSearch);
			if (dynamL && IsAfter(prevMeasL,dynamL)) {
				aMeas = GetPAMEASURE(aMeasL);
				aMeas->dynamicType = DynamType(dynamL);
			}
			else {
				aMeas = GetPAMEASURE(aMeasL);
				aPrevMeas = GetPAMEASURE(aPrevMeasL);
				aMeas->dynamicType = aPrevMeas->dynamicType;
			}
		}
}


/* ------------------------------------------------------------ PasteFixContext -- */
/* Fix up clef, keysig, timesig and dynamic contexts for the range pasted in,
both at the initial and final boundaries of the range.
initL is the node before the selection range; succL is the node after it:
initL = LeftLINK(first node in range); succL = selEndL.
Note: assumes doc->selStaff is set.
Note: comment about the range seems wrong. (CER,2/91) */

void PasteFixContext(Document *doc, LINK initL, LINK succL, short staffDiff)
{
	short s,v; CONTEXT oldContext, newContext;
	LINK clipMeasL;

	/* If staffDiff, meas objs can have context establishing objects translated
		by PasteMapStaves into the previous measure, outdating the context fields
		of the meas objs. */

	if (staffDiff) PasteFixSysMeasContexts(doc,initL);

	InstallDoc(clipboard);
	clipMeasL = LSSearch(clipboard->headL, MEASUREtype, ANYONE, FALSE, FALSE);

	/* First measure in clipboard establishes old context; LINK before
		pasted in material establishes new context for range pasted in.
		Fix up context for range pasted in. staffDiff allows handling
		paste into staves other than source. For example, if doc->selStaff
		is 3, pasted in range goes from staff 1 in clipboard to staff 3 in
		score. Therefore get the context in the clipboard to reflect this
		difference. */

	for (s = doc->selStaff; s<=doc->nstaves; s++) {

		/* Paste occurs from staff <s-staffDiff> in the clipboard to
			staff s in the score. Context is fixed by getting context
			on staff s-staffDiff in the clipboard and using this context
			to fix up notes on staff s in pasted in range in score.
			But any notes pasted in must have been pasted in from legal
			staves in the clipboard, so if <s-staffDiff> is an illegal
			staff in the clipboard, there must not be any notes, etc.,
			on this staff in the pasted in range. Therefore we are
			justified in simply continuing, without error, in case any of
			these staffns are encountered in this loop. */
			
		if (s-staffDiff < 1 || s-staffDiff > clipboard->nstaves) continue;

		InstallDoc(clipboard);
		GetContext(clipboard, clipMeasL, s-staffDiff, &oldContext);
		InstallDoc(doc);

		GetContext(doc, initL, s, &newContext);

		PasteFixAllContexts(doc, RightLINK(initL), succL, s, oldContext, newContext);
	}

	/* Link before pasted in material establishes old context; link at end
		of pasted in material establishes new context for range past the
		range pasted in. Fix up context for range past range pasted in. */

	for (s = doc->selStaff; s<=doc->nstaves; s++) {
		GetContext(doc, initL, s, &oldContext);
		GetContext(doc, LeftLINK(succL), s, &newContext);
		
		PasteFixAllContexts(doc, succL, doc->tailL, s, oldContext, newContext);
	}
	
	for (s = 1; s<=doc->nstaves; s++)
		PasteFixOctavas(doc,RightLINK(initL),s);

	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc,v))
			PasteFixBeams(doc,RightLINK(initL),v);
}


/* ------------------------------------------------------------- PasteFromClip -- */
/* Paste the clipboard before <insertL>. After pasting, initL and succL bracket
the pasted in range. */

static void PasteFromClip(Document *doc, LINK insertL, short pasteType, Boolean
									/*delRedAccs*/)
{
	LINK		pL, copyL, prevL, succL, initL, startL, endL;
	DDIST		systemWidth;
	short		i, numObjs,stfDiff,fixMeas=FALSE;
	COPYMAP	*clipMap;
		
	clipMap = NULL;

	succL = insertL;													/* Prev node precedes insertL */
	prevL = initL = LeftLINK(insertL);
	
	/* If the doc & clipboard have different formats, check if any measures
		will be pasted into doc; if so, it is necessary to fix up the part-
		staff structure of the measure objects to reflect the structure in
		the new score. */
	if (!CompareScoreFormat(doc,clipboard,pasteType)) {
		InstallDoc(clipboard);
		for (pL=RightLINK(clipFirstMeas); pL!=clipboard->tailL; pL=RightLINK(pL))
			if (MeasureTYPE(pL)) 
				{ fixMeas = TRUE; break; }
		InstallDoc(doc);
	}

	systemWidth = GetSysWidth(doc);

	InstallDoc(clipboard);
	UpdateTailxd(clipboard);
	InstallDoc(doc);

	/* Calculate xds in clipboard and score; set clipboard end LINKS 
		startL, endL */
	PreparePaste(doc, insertL, &startL, &endL);

	InstallDoc(clipboard);
	SetupCopyMap(startL, endL, &clipMap, &numObjs);
	InstallDoc(doc);

	if (!clipMap) return;

	/* Traverse range in clipboard, duplicate objects into score.
		clipMap[i].srcL is obj copied in clipboard; clipMap[i].dstL
		is its copy in the score. */

	for (pL=startL, i=0; pL!=endL; pL=DRightLINK(clipboard, pL), i++) {
		copyL = DuplicateObject(DObjLType(clipboard, pL),pL,FALSE,clipboard,doc,FALSE);
		if (!copyL) continue;

		clipMap[i].srcL = pL; clipMap[i].dstL = copyL;

		RightLINK(copyL) = insertL;
		LeftLINK(insertL) = copyL;
		RightLINK(prevL) = copyL;									/* Link to previous object */
		LeftLINK(copyL) = prevL;
		prevL = copyL;
		DeselectNode(copyL);
#ifdef DELRED
		SetTempFlags(doc, doc, copyL, RightLINK(copyL), TRUE);
#endif
		LinkVALID(copyL) = FALSE;
		if (GraphicTYPE(copyL)) FixGraphicFont(doc, copyL);
  	}

	stfDiff = PasteMapStaves(doc,doc,RightLINK(initL),succL,pasteType);	/* Map voices AND staves */
	FixCrossLinks(doc, doc, initL, succL);						/* Structure and extended objects */
	InstallDoc(doc);
#ifdef DELRED
	TempFlags2NotesSel(doc);
DebugPrintf("<DelRedundantAccs: delRedAccs=%d\n", delRedAccs);
	if (delRedAccs) DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);
DebugPrintf(">DelRedundantAccs.\n");
	DeselAllNoHilite(doc);
#endif

	PasteFixMeasStruct(doc,RightLINK(initL),succL,fixMeas);
	PasteUpdate(doc, initL, succL, systemWidth);				/* xds, rects, measNums */

	FixClipLinks(clipboard, doc, RightLINK(initL), succL, clipMap, numObjs, stfDiff);	/* Slurs, dynamics, graphics, slurred syncs, and structure objects (?) */
	PasteFixContext(doc, initL, succL, stfDiff);

	if (clipMap) DisposePtr((Ptr)clipMap);
	UpdateVoiceTable(doc, TRUE);
}
