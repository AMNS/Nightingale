/* MergeUtils.c for Nightingale - utilities for Merge */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */ 

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ------------------------------------------------------------------------------------ */
/* Local prototypes. */

static short CountVNotes(LINK syncL,short v);
static void GetMrgdUnmrgd(LINK syncL,short v,Boolean *merged,Boolean *unMerged);
static void MFixOverlapSync(Document *doc, LINK syncL,short v);
static void MergeFixBeamRange(Document *doc, LINK startL,LINK endL,short v);

static void MUnOttavaSync(Document *,LINK,LINK,DDIST,short,CONTEXT,Boolean);
static void MUnOttavaGRSync(Document *,LINK,LINK,DDIST,short,CONTEXT,Boolean);
static void MRemoveOctOnStf(Document *doc,LINK octL,short s,Boolean merged);
static void MergeFixOctRange(Document *doc,LINK startL,LINK endL,VInfo *vInfo,short v);

static void MEFixContForClef(Document *, LINK, LINK, short, SignedByte, SignedByte, CONTEXT);
static void MFixAllAccidentals(LINK, LINK, short, Boolean);
static void MEFixAccsForKeySig(Document *, LINK, LINK, short, KSINFO, KSINFO);
static void MergeFixAllContexts(Document *, LINK, LINK, short, CONTEXT, CONTEXT);

static void MFixOttavaLinks(Document *oldDoc,Document *fixDoc,LINK startL,LINK endL);
static void MFixBeamLinks(Document *oldDoc,Document *fixDoc,LINK startL,LINK endL);
static void MFixAllBeamLinks(Document *oldDoc,Document *fixDoc,LINK startL,LINK endL);

/* ------------------------------------------------------------------------------------ */

/* --------------------------------------------------------------------- CountVNotes -- */
/* Return the number of notes in voice v in sync syncL. */

static short CountVNotes(LINK syncL, short v)
{
	LINK aNoteL;
	short vNotes=0;

	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==v)
			vNotes++;

	return vNotes;
}

/* ------------------------------------------------------------------- GetMrgdUnmrgd -- */
/* Does the given voice and sync contain merged notes (from the clipboard)? Does it
contain unmerged notes (from the doc)? */

static void GetMrgdUnmrgd(LINK syncL, short v, Boolean *merged, Boolean *unMerged)
{
	LINK aNoteL;

	*merged = *unMerged = false;
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==v) {
			if (NoteMERGED(aNoteL))	*merged = true;
			else					*unMerged = true;
		}
}


/* ----------------------------------------------------------------- MFixOverlapSync -- */
/* Fix up syncL for possible inconsistencies which may have resulted from its having
been merged in a region of temporal overlap. May want to avoid calling FixSyncForChord
for every chord in overlap-voices in merged range; many or all of these chords may be
ok already and need no fixing - if all of their notes are from either doc or clip they
should be ok; they should only need fixing if they contain notes from both doc and
clip. Final question: What if the merged chord contains notes from more than one
staff? */

static void MFixOverlapSync(Document *doc, LINK syncL, short v)
{
	short vNotes=0,staff,noteDur,noteNDots;
	LINK aNoteL;
	Boolean hasMerged=false,hasUnmerged=false,hadRest;
	CONTEXT context;
	PANOTE aNote;

	/* Determine if there is a chord. If there's only one note in the voice, there
		is no need to do anything. */

	vNotes = CountVNotes(syncL,v);
	if (vNotes<=1) return;

	/* Determine if there are notes from both score and clipboard in this voice. If
		not, there should have been a consistent chord in either score or clipboard
		before we started, and so nothing needs to be done. */

	GetMrgdUnmrgd(syncL,v,&hasMerged,&hasUnmerged);
	if (!(hasMerged && hasUnmerged)) return;

	/* Process rests: 
		1. If there was a rest in the clipboard, it should not have been merged;
			remove it. Remove only one; there should never be more than one rest in
			a voice.
		2. After removing rests from the clipboard, see if there is still a chord
			in the voice. If not, we are done. If there is, continue to process
			possible rests from the score.
		3. If there was a rest in the score, replace it by a note. As with the
			clipboard, there should have been only one; do not try to replace more
			than one. All notes from the clipboard must be the same duration as the
			rest was: set the merged notes durations to this value; this includes
			duration and nDots. */

	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==v && NoteREST(aNoteL)) {
			if (NoteMERGED(aNoteL)) {
				RemoveLink(syncL, NOTEheap, FirstSubLINK(syncL), aNoteL);
				HeapFree(NOTEheap,aNoteL);
				LinkNENTRIES(syncL)--;
				break;
			}						
		}
		
	vNotes = CountVNotes(syncL,v);
	if (vNotes<=1) return;
	
	/* See if there are any rests from the score in the voice. If there are any rests
		in the voice, there should be only one, and it should be from the score. If
		there is one, get its duration, which will be used to set duration of merged
		notes, and delete the rest from the data structure. */

	hadRest = false;
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==v && NoteREST(aNoteL)) {
			if (!NoteMERGED(aNoteL)) {
				hadRest = true;
				aNote = GetPANOTE(aNoteL);
				noteDur = aNote->subType;							/* Use score note's values for duration */
				noteNDots = aNote->ndots;							/*   and no. of dots */

				RemoveLink(syncL, NOTEheap, FirstSubLINK(syncL), aNoteL);
				HeapFree(NOTEheap,aNoteL);
				LinkNENTRIES(syncL)--;
				break;
			}
		}

	/* Check if there is only one note remaining in the voice; if there is, insure
		that its inChord flag is no longer set. */

	vNotes = CountVNotes(syncL,v);
	if (vNotes==1) {
		aNoteL = NoteInVoice(syncL, v, false);
		aNote = GetPANOTE(aNoteL);
		aNote->inChord = false;
	}
	
	/* If the score had a rest in the voice, set the duration of all the notes
		merged into the voice to that of the rest which was deleted. If there are
		any notes in this voice, they should be from the clipboard.
		Otherwise, get the duration of the note/chord from the score; if there was
		a chord before the merge, all its notes should have had the same duration,
		so its enough to just check 1 note in the voice. Then set the duration of
		all the notes merged into the voice to the duration of the note/chord that
		was there before. */

	if (hadRest) {
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==v) {
				if (NoteMERGED(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					aNote->subType = noteDur;
					aNote->ndots = noteNDots;
				}
			}
	}
	else {
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==v) {
				if (!NoteMERGED(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					noteDur = aNote->subType;
					noteNDots = aNote->ndots;
					break;
				}
			}
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==v) {
				if (NoteMERGED(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					aNote->subType = noteDur;
					aNote->ndots = noteNDots;
				}
			}
	}

	/* Now fix inChord status for the chord which remains after processing rests. */

	vNotes = CountVNotes(syncL,v);
	if (vNotes>1) {
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==v) {
				FixSyncForChord(doc,syncL,v,NoteBEAMED(aNoteL),0,0,NULL);
				break;
			}
	}

	/* Now try to respell the chord to avoid unisons. */
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==v) {
			staff = NoteSTAFF(aNoteL);
			break;
		}
	GetContext(doc, syncL, staff, &context);
	AvoidUnisons(doc, syncL, v, &context);
}


/* --------------------------------------------------------------- MergeFixBeamRange -- */
/* Remove all merged beams in the range merged and set the beamed flags of all
merged notes to the beamed status of the chords they entered in the doc. Then
traverse the range again and fix up beams which remain and include notes that
have been merged from the clipboard. */

static void MergeFixBeamRange(Document *doc, LINK startL, LINK endL, short v)
{
	LINK pL,qL,aNoteL,firstSyncL,lastSyncL,beamL,nextL;
	PANOTE aNote;
	Boolean docBeamed,needRebeam;
	short nInBeam;

		/* Remove all merged beams, then fix the status of beamed flags in all merged
		   notes. We do not have to search for beams, since all merged beams will
		   actually be in the merged range. Likewise, no association of beams with their
		   bpSyncs is needed, since all merged beams will be removed and all merged
		   beamed notes will be incorporated into chords from the doc and reflect the
		   beamed status of those chords. */

	for (pL=startL; pL!=endL; pL=nextL) {
		nextL = RightLINK(pL);
		if (BeamsetTYPE(pL) && LinkSPAREFLAG(pL) && BeamVOICE(pL)==v)
			DeleteNode(doc,pL);

		if (SyncTYPE(pL))
			if (SyncInVoice(pL,v)) {
				docBeamed = false;
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (NoteVOICE(aNoteL)==v && aNote->beamed && !aNote->merged) {
						docBeamed = true;
						break;
					}
				}
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (NoteVOICE(aNoteL)==v && aNote->merged) {
						aNote->beamed = docBeamed;
					}
				}
			}
	}
	
	/* Now retraverse the range, unbeaming and rebeaming all beams from the document.
		Start with the first beam that needs fixing up. */

	beamL = NILINK;
	for (pL=startL; pL!=endL && beamL==NILINK; pL=RightLINK(pL)) {
		if (SyncTYPE(pL))
			if (SyncInVoice(pL,v)) {
				aNoteL = NoteInVoice(pL,v,false);
				if (NoteBEAMED(aNoteL)) {
					beamL = LVSearch(pL,BEAMSETtype,v,GO_LEFT,false);
				}
			}
	}
	
	if (!beamL) return;

	for ( ; beamL!=endL; beamL=RightLINK(beamL)) {
		if (BeamsetTYPE(beamL) && BeamVOICE(beamL)==v) {
			nInBeam = LinkNENTRIES(beamL);
			firstSyncL = FirstInBeam(beamL);
			lastSyncL = LastInBeam(beamL);
			needRebeam = false;

			/* Prologue to CreateBEAMSET says: 'The range must contain <nInBeam> notes 
				(counting each chord as one note)'. Here nInBeam seems to require
				the counting of each additional note, or making sure the inChord
				flag is properly set: CountBeamable doesn't provide any insight into
				the situation, except that it seems clear that setting the inChord
				flag is the proper approach. */

			for (qL=firstSyncL;qL!=RightLINK(lastSyncL);qL=RightLINK(qL))
				if (SyncTYPE(qL))
					if (SyncInVoice(qL,v)) {
						for (aNoteL=FirstSubLINK(qL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
							aNote = GetPANOTE(aNoteL);
							if (NoteVOICE(aNoteL)==v && aNote->merged) {
								aNote->ystem = aNote->yd;	/* So multiple main notes won't confuse CreateBEAMSET */
								aNote->inChord = true;
								needRebeam = true;
							}
						}
					}
		
			if (needRebeam) {
				RemoveBeam(doc,beamL,v,false);
				CreateBEAMSET(doc,firstSyncL,RightLINK(lastSyncL),v,nInBeam,false,
										doc->voiceTab[v].voiceRole);
			}
		}
	}
}


/* --------------------------------------------------- MUnOttavaSync,MUnOttavaGRSync -- */
/* Unottava a Sync or GRSync of whose notes only those with merged status <merged>
actually require un-Ottava-ing. */

static void MUnOttavaSync(Document *doc, LINK octL, LINK pL, DDIST yDelta, short s,
									CONTEXT context, Boolean merged)
{
	LINK aNoteL; 
	PANOTE aNote;
	Boolean stemDown, multiVoice;
	QDIST qStemLen;
	STDIST dystd;
	
	multiVoice = IsSyncMultiVoice(pL, s);

	/* Loop through the notes and set their yds and ystems. */
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->staffn==s && aNote->inOttava && aNote->merged==merged) {
			aNote->inOttava = false;
			aNote->yd -= yDelta;
			aNote->yqpit -= halfLn2qd(noteOffset[OctType(octL)-1]);

			stemDown = GetStemInfo(doc, pL, aNoteL, &qStemLen);
			NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL), NFLAGS(NoteType(aNoteL)),
											stemDown,
											context.staffHeight, context.staffLines,
											qStemLen, false);
			if (odd(noteOffset[OctType(octL)-1]))
				ToggleAugDotPos(doc, aNoteL, stemDown);

			dystd = -halfLn2std(noteOffset[OctType(octL)-1]);
			if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
		}
	}

	/* Loop through the notes again and fix up the ystems of the non-extreme notes. */
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->inChord && NoteSTAFF(aNoteL)==s && aNote->merged==merged) {
			FixSyncForChord(doc,pL,NoteVOICE(aNoteL),aNote->beamed,0,
											(multiVoice? -1 : 1), &context);
			break;
		}
	}
}

static void MUnOttavaGRSync(Document *doc, LINK octL, LINK pL, DDIST yDelta,
									short s, CONTEXT context, Boolean /*merged*/)
{
	LINK aGRNoteL;
	PAGRNOTE aGRNote;
	Boolean stemDown=false, multiVoice=false;
	short stemLen;
	
	/* Loop through the grace notes and set their yds and ystems. */
	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->staffn==s && aGRNote->inOttava) {
			aGRNote->inOttava = false;
			aGRNote->yd -= yDelta;
			aGRNote->yqpit -= halfLn2qd(noteOffset[OctType(octL)-1]);
			stemLen = QSTEMLEN(!multiVoice, ShortenStem(aGRNoteL, context, stemDown));
			aGRNote->ystem = CalcYStem(doc, aGRNote->yd, NFLAGS(aGRNote->subType),
											stemDown,
											context.staffHeight, context.staffLines,
											stemLen, false);
		}
	}

	/* Loop through the grace notes again and fix up the ystems of the non-extreme
		notes. */
	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (!aGRNote->rest && GRNoteSTAFF(aGRNoteL)==s) {
			if (aGRNote->inChord) {
				FixGRSyncForChord(doc,pL,GRNoteVOICE(aGRNoteL),aGRNote->beamed,0,
															multiVoice, &context);
				break;
			}
		}
	}
}


/* ----------------------------------------------------------------- MRemoveOctOnStf -- */
/* Remove the octave sign and change notes and grace notes it affected accordingly.
Specifically for use by Merge. */

static void MRemoveOctOnStf(Document *doc, LINK octL,
							short s,
							Boolean merged			/* true if removing merged-in Ottava */
							)
{
	DDIST yDelta;
	CONTEXT context;
	LINK syncL, aNoteOttavaL;
	PANOTEOTTAVA aNoteOttava;

	/* Update note's y position. */
	GetContext(doc, octL, s, &context);
	yDelta = halfLn2d(noteOffset[OctType(octL)-1],context.staffHeight,context.staffLines);
							
	aNoteOttavaL = FirstSubLINK(octL);
	for ( ; aNoteOttavaL; aNoteOttavaL = NextNOTEOTTAVAL(aNoteOttavaL)) {
		aNoteOttava = GetPANOTEOTTAVA(aNoteOttavaL);
		syncL = aNoteOttava->opSync;

		if (SyncTYPE(syncL)) {
			MUnOttavaSync(doc,octL,syncL,yDelta,s,context,merged);
		}
		else if (GRSyncTYPE(syncL)) {
			MUnOttavaGRSync(doc,octL,syncL,yDelta,s,context,merged);
		}
	}
	
	DeleteNode(doc, octL);
}


/* ---------------------------------------------------------------- MergeFixOctRange -- */
/* ??It's totally unclear how to write this function, for the following reason:
In order to rebeam properly, it appears necessary to pre-process notes
merged into a beamset from the score, by: i. setting aNote->ystem = aNote->yd
and ii. setting the inChord flag. Similar pre-processing would appear
necessary for ottavas, but it is not clear how to distinguish the notes which
would not need the pre-processing and in fact should not have it because they
have already been processed for beams. */

static void MergeFixOctRange(Document *doc, LINK startL, LINK endL, VInfo *vInfo, short v)
{
	LINK pL,qL,aNoteL,aNoteOctL,firstSyncL,lastSyncL;
	short firstStf,lastStf,s,nInOct,octType;
	Boolean hasMerged,hasUnmerged;
	PANOTE aNote;
	PANOTEOTTAVA aNoteOct;
	
	if (vInfo[v].singleStf)
		firstStf = lastStf = v;
	else {
		firstStf = vInfo[v].firstStf;
		lastStf = vInfo[v].lastStf;
	}

	/*
	 * An additional problem with the following tests beyond that described in the
	 * body of the loops is that the MainNote(aNoteL) test is the standard way to
	 * traverse the ottavas, and this information has already been wiped out by the
	 * process of fixing up beams. If we call MergeFixOctRange before fixing up beams,
	 * it will interfere with the function to fix up beams.
	 */
	for (s=firstStf; s<=lastStf; s++) {
		for (pL=startL; pL!=endL; pL=RightLINK(pL)) {
			if (OttavaTYPE(pL) && OttavaSTAFF(pL)==s) {
				if (LinkSPAREFLAG(pL)) {
					hasUnmerged = false;
					aNoteOctL = FirstSubLINK(pL);
					for ( ; aNoteOctL; aNoteOctL=NextNOTEOTTAVAL(aNoteOctL)) {
						aNoteOct = GetPANOTEOTTAVA(aNoteOctL);
						qL = aNoteOct->opSync;
						aNoteL = FirstSubLINK(qL);
						for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
							aNote = GetPANOTE(aNoteL);
							if (NoteSTAFF(aNoteL)==OttavaSTAFF(pL) && !NoteREST(aNoteL)) {
								if (aNote->inOttava) {
									if (!NoteMERGED(aNoteL))
										hasUnmerged = true;
										goto doneCheck;
								}
							}
						}
					}	

doneCheck:
					/* This is a merged Ottava. If hasUnmerged is true, we have problems
						and must remove the Ottava, taking into account that its internal
						structure may be inconsistent at this point due to the presence of
						unmerged notes. */
				
					if (hasUnmerged) {
						MRemoveOctOnStf(doc, pL, s, true);
					}
				
				}
			}
		}
	}

	for (s=firstStf; s<=lastStf; s++) {
		for (pL=startL; pL!=endL; pL=RightLINK(pL)) {
			if (OttavaTYPE(pL) && OttavaSTAFF(pL)==s) {
				if (!LinkSPAREFLAG(pL)) {
					hasMerged = false;
					aNoteOctL = FirstSubLINK(pL);
					for ( ; aNoteOctL; aNoteOctL=NextNOTEOTTAVAL(aNoteOctL)) {
						aNoteOct = GetPANOTEOTTAVA(aNoteOctL);
						qL = aNoteOct->opSync;
						aNoteL = FirstSubLINK(qL);
						for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
							aNote = GetPANOTE(aNoteL);
							if (NoteSTAFF(aNoteL)==OttavaSTAFF(pL) && !NoteREST(aNoteL)) {
								if (aNote->inOttava) {
									if (NoteMERGED(aNoteL))
										hasMerged = true;
										goto doneCheck1;
								}
							}
						}
					}	

doneCheck1:
					/* This is an Ottava from the score. If hasMerged is true, we have
						problems; we must remove and recreate the Ottava, taking into
						account that its internal structure may be inconsistent at this
						point due to the presence of merged notes. To recreate the Ottava,
						note: first & last syncs and nEntries are unchanged, because
						merging with overlapping voices does not change syncing. */

					if (hasMerged) {
						firstSyncL = FirstInOttava(pL);
						lastSyncL = LastInOttava(pL);
						nInOct = LinkNENTRIES(pL);
						octType = OctType(pL);
						MRemoveOctOnStf(doc, pL, s, false);
						CreateOTTAVA(doc,firstSyncL,lastSyncL,s,nInOct,octType,false,false);
					}
				
				}
			}
		}
	}
}


/* --------------------------------------------------------------- MergeFixVOverlaps -- */
/* Fix up all syncs in merged range for voices which had temporal overlaps. */

void MergeFixVOverlaps(Document *doc, LINK initL, LINK succL, short *vMap, VInfo *vInfo)
{
	LINK pL;
	short v,docV;
	
	for (v=1; v<=MAXVOICES; v++) {
		if (vInfo[v].overlap) {
			docV = vMap[v];
			MergeFixBeamRange(doc, RightLINK(initL),succL,docV);

			MergeFixOctRange(doc, RightLINK(initL),succL,vInfo,docV);
		
			for (pL=RightLINK(initL); pL!=succL; pL=RightLINK(pL))
				if (SyncTYPE(pL))
					if (SyncInVoice(pL,docV)) {
						MFixOverlapSync(doc,pL,docV);					/* Fix up this sync! */
					}
		}
	}
}


/* ---------------------------------------------------------------- MEFixContForClef -- */
/* In the range [startL,doneL), update (1) the clef in context fields of following
STAFFs and MEASUREs for the given staff, (2) notes' y-positions, and (3) beam positions
to match the new note positions. context just picks up staffHeight and staffLines. If
a clef change is found on the staff before doneL, we stop there. May inval some or all
of the range (via Rebeam) but may not. */

void MEFixContForClef(Document *doc,
						LINK startL, LINK doneL,
						short staffn,
						SignedByte oldClef, SignedByte newClef,	/* Previously-effective and new clefTypes */
						CONTEXT context
						)
{
	short		voice, yPrev, yHere, qStemLen;
	DDIST		yDelta;
	PANOTE		aNote;
	PAGRNOTE	aGRNote;
	PASTAFF		aStaff;
	PACLEF		aClef;
	PAMEASURE	aMeasure;
	LINK		pL, aClefL, aNoteL, aGRNoteL, aStaffL, aMeasureL;
	Boolean		stemDown;
	
	/*
	 * Update <context> to reflect the clef change. This is necessary to get the
	 * correct stem direction from GetCStemInfo.
	 */
	context.clefType = newClef;
	
	yPrev = ClefMiddleCHalfLn(oldClef);
	yHere = ClefMiddleCHalfLn(newClef);
	yDelta = halfLn2d(yHere-yPrev, context.staffHeight, context.staffLines);

	for (pL = startL; pL!=doneL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (!aNote->rest && NoteSTAFF(aNoteL)==staffn && aNote->merged) {
						aNote->yd += yDelta;
						if (aNote->inChord) {
							voice = aNote->voice;
							FixSyncForChord(doc, pL, voice, aNote->beamed, 0, 0, NULL);
						}
						else {
							stemDown = GetCStemInfo(doc, pL, aNoteL, context, &qStemLen);
							NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL),
															NFLAGS(NoteType(aNoteL)),
															stemDown,
															context.staffHeight, context.staffLines,
															qStemLen, false);
						}
					}
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					aGRNote = GetPAGRNOTE(aGRNoteL);
					if (GRNoteSTAFF(aGRNoteL)==staffn) {
						aGRNote->yd += yDelta;
						if (aGRNote->inChord)
							FixGRSyncForChord(doc, pL, aGRNote->voice, aGRNote->beamed, 0,
													true, NULL);
						else
							FixGRSyncNote(doc, pL, aGRNote->voice, &context);
					}
				}
				break;
			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
					if (StaffSTAFF(aStaffL)==staffn) {
						aStaff = GetPASTAFF(aStaffL);
						aStaff->clefType = newClef; 
					}
				break;
			case CLEFtype:
				aClefL = FirstSubLINK(pL);
				for ( ; aClefL; aClefL = NextCLEFL(aClefL))
					if (ClefSTAFF(aClefL)==staffn) {
						if (!ClefINMEAS(pL)) {
							aClef = GetPACLEF(aClefL);
							aClef->subType = newClef;
						}
						else {
							doneL = pL;
							goto Cleanup;
						}
					}
				break;
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL))
					if (MeasureSTAFF(aMeasureL)==staffn) {
						aMeasure = GetPAMEASURE(aMeasureL);
						aMeasure->clefType = newClef;
					} 
				break;
			default:
				;
		}

Cleanup:
	ClefFixBeamContext(doc, startL, doneL, staffn);
}


/* ---------------------------------------------------------- MFixAllAccidentals -- */

static void MFixAllAccidentals(LINK fixFirstL, LINK fixLastL,
						short staff,
						Boolean pitchMod	/* true=<accTable> has pitch modifers, else accidentals */
						)
{
	PANOTE		aNote;
	PAGRNOTE	aGRNote;
	LINK		pL, aNoteL, aGRNoteL;
	short		halfLn;

	for (pL = fixFirstL; pL!=fixLastL; pL=RightLINK(pL))
		switch(ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staff && NoteMERGED(aNoteL)) {																/* Yes. */
						aNote = GetPANOTE(aNoteL);
						halfLn = qd2halfLn(aNote->yqpit)+ACCTABLE_OFF;
						if (aNote->accident!=0)							/* Does it have an accidental? */
							accTable[halfLn] = -1;						/* Yes. Mark this line/space OK */
						else {											/* No, has no accidental */
							if (accTable[halfLn]>0 && 
									(!pitchMod || accTable[halfLn]!=AC_NATURAL)) {
								aNote->accident = accTable[halfLn];
								aNote->accSoft = true;
							}
							accTable[halfLn] = -1;						/* Mark this line/space OK */
						}
					}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					if (GRNoteSTAFF(aGRNoteL)==staff) {																/* Yes. */
						aGRNote = GetPAGRNOTE(aGRNoteL);
						halfLn = qd2halfLn(aGRNote->yqpit)+ACCTABLE_OFF;
						if (aGRNote->accident!=0)						/* Does it have an accidental? */
							accTable[halfLn] = -1;						/* Yes. Mark this line/space OK */
						else {											/* No, has no accidental */
							if (accTable[halfLn]>0 && 
									(!pitchMod || accTable[halfLn]!=AC_NATURAL)) {
								aGRNote->accident = accTable[halfLn];
								aGRNote->accSoft = true;
							}
							accTable[halfLn] = -1;						/* Mark this line/space OK */
						}
					}
				break;
			default:
				;
		}
}


/* -------------------------------------------------------------- MEFixAccsForKeySig -- */
/* Go through staff in range [startL,doneL) and change accidentals where
appropriate to keep notes' pitches the same. */

static void MEFixAccsForKeySig(Document *doc,
								LINK startL, LINK doneL,
								short staffn,					/* Desired staff no. */
								KSINFO oldKSInfo,				/* Previously-effective and new key sig. info */
								KSINFO newKSInfo
								)
{
	SignedByte oldKSTab[MAX_STAFFPOS], KSTab[MAX_STAFFPOS];	/* Orig., new accidental tables */
	LINK barFirstL,barLastL,measL,firstL,lastL;

	/* Compute an accidental table that converts the pitch modifer table after the
	 *	insert into the table before it, so we can correct notes to be the same
	 *	in the new pitch modifier environment as they were in the old environment.
 	 */
 	InitPitchModTable(oldKSTab, &oldKSInfo);
 	InitPitchModTable(KSTab, &newKSInfo);
 	CombineTables(oldKSTab, KSTab);

	/*	Now use the table <oldKSTab> to correct accidentals of notes on the staff, one
	 * measure at a time. 
	 */
	barFirstL = EitherSearch(startL,MEASUREtype,staffn,GO_LEFT,false);
	barLastL = LSSearch(doneL,MEASUREtype,staffn,GO_LEFT,false);

	firstL = startL;
	measL = barFirstL;
	for ( ; measL!=LinkRMEAS(barLastL); measL=LinkRMEAS(measL),firstL=measL) {
		lastL = EndMeasRangeSearch(doc, RightLINK(firstL), doneL);
		
		CopyTables(oldKSTab,accTable);
	
		MFixAllAccidentals(firstL, lastL, staffn, false);
	}
}


/* ------------------------------------------------------------- MergeFixAllContexts -- */
/* After merging, fix up clef, keySig, timeSig, and dynamic contexts starting at
<startL>, on staff <s>. */

static void MergeFixAllContexts(Document *doc, LINK startL, LINK endL, short s,
									CONTEXT oldContext, CONTEXT newContext)
{
	KSINFO oldKSInfo, newKSInfo;
	TSINFO timeSigInfo;

	if (newContext.clefType!=oldContext.clefType)
		MEFixContForClef(doc, startL, endL, s, oldContext.clefType, newContext.clefType, newContext);

	/* BlockCompare returns non-zero value if the arguments are different */

	if (BlockCompare(&oldContext.KSItem,&newContext.KSItem,7*sizeof(KSITEM))) {
		KeySigCopy((PKSINFO)oldContext.KSItem, &oldKSInfo);
		KeySigCopy((PKSINFO)newContext.KSItem, &newKSInfo);
		EFixContForKeySig(startL, endL, s, oldKSInfo, newKSInfo);
		MEFixAccsForKeySig(doc, startL, endL, s, oldKSInfo, newKSInfo);
	}
	
	if (oldContext.numerator!=newContext.numerator || oldContext.denominator!=newContext.denominator) {
		timeSigInfo.TSType = newContext.timeSigType;
		timeSigInfo.numerator = newContext.numerator;
		timeSigInfo.denominator = newContext.denominator;
		EFixContForTimeSig(startL, endL, s, timeSigInfo);
	}

	if (oldContext.dynamicType!=newContext.dynamicType)
		EFixContForDynamic(startL, endL, s, newContext.dynamicType);
}

/* ----------------------------------------------------------------- MergeFixContext -- */
/* Fix up clef, keysig, timesig and dynamic contexts for the range pasted in, both at
the initial and final boundaries of the range.
initL is the node before the selection range; succL is the node after it:
initL = LeftLINK(first node in range); succL = selEndL.
Note: assumes doc->selStaff is set.
Note: comment about the range seems wrong. (CER,2/91) */

void MergeFixContext(Document *doc, LINK initL, LINK succL, short minStf, short maxStf,
							short staffDiff)
{
	short s, v;
	CONTEXT oldContext, newContext;
	LINK clipMeasL;

	InstallDoc(clipboard);
	clipMeasL = LSSearch(clipboard->headL, MEASUREtype, ANYONE, false, false);

	/* First measure in clipboard establishes old context; LINK before pasted in
	material establishes new context for range pasted in. Fix up context for range
	pasted in. staffDiff allows handling paste into staves other than source. For
	example, if doc->selStaff is 3, pasted in range goes from staff 1 in clipboard to
	staff 3 in score. Therefore get the context in the clipboard to reflect this
	difference. */

	for (s = minStf+staffDiff; s<=maxStf+staffDiff; s++) {

		/* Paste occurs from staff <s-staffDiff> in the clipboard to staff s in the
		score. Context is fixed by getting context on staff s-staffDiff in the
		clipboard and using this context to fix up notes on staff s in pasted in range
		in score. But any notes pasted in must have been pasted in from legal staves
		in the clipboard, so if <s-staffDiff> is an illegal staff in the clipboard,
		there must not be any notes, etc., on this staff in the pasted in range.
		Therefore we are justified in simply continuing, without error, in case any of
		these staffns are encountered in this loop. */
			
		if (s-staffDiff < 1 || s-staffDiff > clipboard->nstaves) continue;

		InstallDoc(clipboard);
		GetContext(clipboard, clipMeasL, s-staffDiff, &oldContext);
		InstallDoc(doc);

		GetContext(doc, initL, s, &newContext);

		MergeFixAllContexts(doc, RightLINK(initL), succL, s, oldContext, newContext);
	}

	/* Link before pasted in material establishes old context; link at end of pasted-
		in material establishes new context for range past the range pasted in. Fix up
		context for range past range pasted in. */

	for (s = minStf+staffDiff; s<=maxStf+staffDiff; s++) {
		GetContext(doc, initL, s, &oldContext);
		GetContext(doc, LeftLINK(succL), s, &newContext);
		
		MergeFixAllContexts(doc, succL, doc->tailL, s, oldContext, newContext);
	}
	
	for (s = minStf+staffDiff; s<=maxStf+staffDiff; s++)
		PasteFixOttavas(doc,RightLINK(initL),s);

	/* This should be constrained to only those voices on staves in range
		[minStf+staffDiff,maxStf+staffDiff] */

	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc,v)) PasteFixBeams(doc,RightLINK(initL),v);
}


/* ----------------------------------------------------------------- MFixOttavaLinks -- */
/* Update opSync links for all Ottava objects in range. */

static void MFixOttavaLinks(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL)
{
	short			i, k;
	PANOTE 			aNote;
	PAGRNOTE		aGRNote;
	PANOTEOTTAVA 	paNoteOct;
	LINK			pL, qL, aNoteL, aNoteOctL, aGRNoteL;
	Boolean			needMerged;

	InstallDoc(fixDoc);
	
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (OttavaTYPE(pL)) {
			needMerged = LinkSPAREFLAG(pL);
			for (i=0, qL=RightLINK(pL); i<LinkNENTRIES(pL) && qL!=endL; qL=RightLINK(qL))
				if (SyncTYPE(qL)) {
					aNoteL = FirstSubLINK(qL);
					for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (NoteSTAFF(aNoteL)==OttavaSTAFF(pL) && MainNote(aNoteL) && !NoteREST(aNoteL) && needMerged==aNote->merged) {
							if (aNote->inOttava) {
								aNoteOctL = FirstSubLINK(pL);
								for (k=0; k<=i; k++,aNoteOctL=NextNOTEOTTAVAL(aNoteOctL))
									if (k==i) {
										paNoteOct = GetPANOTEOTTAVA(aNoteOctL);
										paNoteOct->opSync = qL;
									}
							}
							else 
								MayErrMsg("FixOttavaLinks: Unottavad note in sync %ld where inOttava note expected",
										(long)qL);
							i++;
						}
						if (NoteSTAFF(aNoteL)==OttavaSTAFF(pL) && aNote->inOttava)
							aNote->tempFlag = true;
					}
				}
				else if (GRSyncTYPE(qL)) {
					aGRNoteL = FirstSubLINK(qL);
					for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
						aGRNote = GetPAGRNOTE(aGRNoteL);
						if (GRNoteSTAFF(aGRNoteL)==OttavaSTAFF(pL) && GRMainNote(aGRNoteL) && needMerged==aNote->merged) {
							if (aGRNote->inOttava) {
								aNoteOctL = FirstSubLINK(pL);
								for (k=0; k<=i; k++,aNoteOctL=NextNOTEOTTAVAL(aNoteOctL))
									if (k==i) {
										paNoteOct = GetPANOTEOTTAVA(aNoteOctL);
										paNoteOct->opSync = qL;
									}
							}
							else 
								MayErrMsg("FixOttavaLinks: Unottavad note in sync %ld where inOttava note expected",
										(long)qL);
							i++;
						}
						if (NoteSTAFF(aGRNoteL)==OttavaSTAFF(pL) && aGRNote->inOttava)
							aGRNote->tempFlag = true;
					}
				}

		}
		
	InstallDoc(oldDoc);
}


/* -------------------------------------------------------------------- FixBeamLinks -- */
/* Update bpSync links for all note beamset objects in range. */

static void MFixBeamLinks(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL)
{
	short		i, k;
	PANOTE 		aNote;
	PANOTEBEAM 	paNoteBeam;
	LINK		pL, qL, aNoteL, aNoteBeamL;
	Boolean		needMerged;
	
	InstallDoc(fixDoc);
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (BeamsetTYPE(pL) && !GraceBEAM(pL)) {
			needMerged = LinkSPAREFLAG(pL);
			for (i=0, qL=RightLINK(pL); i<LinkNENTRIES(pL) && qL!=endL; qL=RightLINK(qL))
				if (SyncTYPE(qL)) {
					aNoteL = FirstSubLINK(qL);
					for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (NoteVOICE(aNoteL)==BeamVOICE(pL) && MainNote(aNoteL) && needMerged==aNote->merged) {
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
							aNote->tempFlag = true;
					}
				}
		}
	 InstallDoc(oldDoc);
}


/* ---------------------------------------------------------------- MFixAllBeamLinks -- */
/* Update bpSync links for all Beamset objects in range, either note beams or
grace note beams. */

static void MFixAllBeamLinks(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL)
{
	MFixBeamLinks(oldDoc, fixDoc, startL, endL);

	FixGRBeamLinks(oldDoc, fixDoc, startL, endL);
}


/* ------------------------------------------------------------------- MFixCrossPtrs -- */
/* Update all cross links after reconstructing the data structure. Version for
merge which fixes up selectively depending on origin of object to update. Needed
for overlapped voices, which can superimpose beams, ottavas in the same voice. */

void MFixCrossPtrs(Document *doc, LINK startMeas, LINK endMeas, PTIME *durArray)
{
	LINK sysL,firstSysL,firstMeasL;

	sysL = SSearch(startMeas,SYSTEMtype,GO_LEFT);
	firstSysL = LinkLSYS(sysL) ? LinkLSYS(sysL) : sysL;

	/* Beamsets can begin in a previous system; crossSys beamsets
		can begin in the system before the previous system. */

	MFixAllBeamLinks(doc, doc, firstSysL, endMeas);
	
	/* Rules are not clearly stated in FixGroupsMenu,NTypes.h, or DoOttava
		for Ottavas. Assuming we do not have crossSys Ottavas. Must update
		all Ottavas which can have octNotes in the measure; Ottavas located
		after endMeas are guaranteed to have none. */
	
	MFixOttavaLinks(doc, doc, sysL, endMeas);

	/* Tuplets must begin and end in the same measure. */

	FixTupletLinks(doc, doc, startMeas, endMeas);

	/* NBJD objects which start before and end in the measure being processed
		will remain in place in the data structure, and their firstSyncL/firstObjL
		will remain valid across the operation. This call to FixNBJDLinks will
		properly update their lastSyncL/lastObjL field. */

	firstMeasL = SSearch(doc->headL,MEASUREtype,GO_RIGHT);
	FixNBJDLinks(firstMeasL, doc->tailL, durArray);
}
