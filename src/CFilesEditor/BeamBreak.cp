/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* BeamBreak.c for Nightingale - functions to handle secondary beam breaks:
	BreakBeamBefore		BBFixCenterSecBeams		CountPrimaries
	DoBreakBeam
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean BreakBeamBefore(Document *, LINK, LINK, LINK *);
static Boolean BBFixCenterSecBeams(Document *, LINK [], short [], short);
static short CountPrimaries(LINK);


/* ------------------------------------------------------------- BreakBeamBefore -- */
/* Given a Beamset and a Sync (or GRSync), try to add a secondary break to the
Beamset just before the Sync. If we can do so, return TRUE, else FALSE. Regardless,
if there's a link preceding the Sync in the Beamset, return it in *pPrevL. Takes no
user-interface actions. */

Boolean BreakBeamBefore(Document */*doc*/, LINK beamL, LINK syncL, LINK *pPrevL)
{
	short nInBeam, i, beamsNow; LINK aNoteBeamL, bNoteBeamL;
	PANOTEBEAM aNoteBeam, bNoteBeam, prevNoteBeam=NULL;
	
	nInBeam = LinkNENTRIES(beamL);
	aNoteBeamL = FirstSubLINK(beamL);
	
	/*
	 * If our Sync appears in the given Beamset and is not the first, second, or
	 * last Sync in it,  add a secondary beam break between the previous Sync and
	 * our Sync in the Beamset.
	 */
	for (i = 0; i<nInBeam; i++, aNoteBeamL = NextNOTEBEAML(aNoteBeamL)) {
		aNoteBeam = GetPANOTEBEAM(aNoteBeamL);
		if (prevNoteBeam && aNoteBeam->bpSync==syncL) {
			*pPrevL = prevNoteBeam->bpSync;

			/* If another break before here would interrupt the primary beam, no good. */
			
			beamsNow = 0;
			for (bNoteBeamL = FirstSubLINK(beamL); bNoteBeamL; 
					bNoteBeamL = NextNOTEBEAML(bNoteBeamL)) {
				if (GetPANOTEBEAM(bNoteBeamL)==aNoteBeam) break;
				bNoteBeam = GetPANOTEBEAM(bNoteBeamL);		
				beamsNow += bNoteBeam->startend;
			}
			if (beamsNow<=1) return FALSE;

			/* If another break here would require a fractional beam on this or the
				preceding note, no good. This would certainly happen if this is the
				second or last note of the beam. It would also happen if this or the
				preceding note is now starting a beam and the break would cause it also
				to end one, or vice-versa. */
				
			if (i<=1 || i==nInBeam-1) return FALSE;
			if (prevNoteBeam->startend>0) return FALSE;
			if (aNoteBeam->startend<0) return FALSE;

			/* Everything's OK. Break away! */
			
			prevNoteBeam->startend -= 1;
			aNoteBeam->startend += 1;
			return TRUE;
		}
		prevNoteBeam = aNoteBeam;
	}
	
	*pPrevL = NILINK;
	return FALSE;
}


/* ---------------------------------------------------------- BBFixCenterSecBeams -- */
/* Fix a center beam for its secondary beams; intended for use after adding secondary
beam breaks. */

static Boolean BBFixCenterSecBeams(Document *doc, LINK beamLA[], short beamNPrimaryA[],
												short nBeamsets)
{
	short i, voice, nInBeam, nPrimary, nSecsA[MAXINBEAM], nSecsB[MAXINBEAM];
	LINK beamL, firstL, lastL, firstNoteL, lastNoteL;
	LINK bpSync[MAXINBEAM], noteInSync[MAXINBEAM];
	SignedByte stemUpDown[MAXINBEAM];
	STFRANGE theRange; Boolean crossStaff;
	DDIST firstystem, lastystem, extension;
	CONTEXT topContext, bottomContext;
	
	for (i = 0; i<nBeamsets; i++) {
		if (AnalyzeBeamset(beamLA[i], &nPrimary, nSecsA, nSecsB, stemUpDown)==0) {
			beamL = beamLA[i];
			firstL = FirstInBeam(beamL);
			lastL = LastInBeam(beamL);
			voice = BeamVOICE(beamL);
			firstNoteL = FindMainNote(firstL, voice);
			lastNoteL = FindMainNote(lastL, voice);
			firstystem = NoteYSTEM(firstNoteL);
			lastystem = NoteYSTEM(lastNoteL);
			nInBeam = LinkNENTRIES(beamL);
			if (!GetBeamSyncs(doc, firstL, RightLINK(lastL), voice, nInBeam, bpSync,
				noteInSync, FALSE)) return FALSE;
			crossStaff = BeamCrossSTAFF(beamL);
			theRange.topStaff = BeamSTAFF(beamL);
			theRange.bottomStaff = (crossStaff? theRange.topStaff+1 : theRange.topStaff);

			GetContext(doc, beamL, theRange.topStaff, &topContext);
			GetContext(doc, beamL, theRange.bottomStaff, &bottomContext);
			firstystem = Staff2TopStaff(firstystem, NoteSTAFF(firstNoteL), theRange, 
						bottomContext.staffTop-topContext.staffTop);
			lastystem = Staff2TopStaff(lastystem, NoteSTAFF(lastNoteL), theRange, 
						bottomContext.staffTop-topContext.staffTop);
			/*
			 *	If the end notes are on different staves, the slope we get from their
			 * endpoints is misleading because of overlapping. Roughly compensate. (We
			 * need to use not the current <nPrimary>, but the no. of primaries before
			 * the current beam break.)
			 */
			if (crossStaff && NoteSTAFF(firstNoteL)!=NoteSTAFF(lastNoteL)) {
				extension = LNSPACE(&topContext)/2
								+(beamNPrimaryA[i]-1)*FlagLeading(LNSPACE(&topContext));
				extension /= 2;
				if (NoteSTAFF(firstNoteL)==theRange.topStaff)
					{ firstystem -= extension; lastystem += extension; }
				else
					{ firstystem += extension; lastystem -= extension; }
			}		

			SlantBeamNoteStems(doc, bpSync, voice, nInBeam, firstystem, lastystem, theRange,
								crossStaff);
			
			/*
			 * We now have consistent stem lengths, except that our downstems and upstems
			 *	don't overlap.  Correct for them now.
			 */
			ExtendStems(doc, beamL, nInBeam, nPrimary, nSecsA, nSecsB);
		}
	}
	
	return TRUE;
}

/* -------------------------------------------------------------- CountPrimaries -- */
/* Return the no. of primary beams in the given beamset. */

short CountPrimaries(LINK beamL)
{
	LINK noteBeamL, syncL, aNoteL; PANOTEBEAM pNoteBeam;
	short iel, startend, firstStemUpDown, nestLevel, nPrim;
	SignedByte stemUpDown[MAXINBEAM];
	
	/* Fill in a table of stem directions for all elements of the beam. */
	
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		pNoteBeam = GetPANOTEBEAM(noteBeamL);
		syncL = pNoteBeam->bpSync;
		aNoteL = FindMainNote(syncL, BeamVOICE(beamL));
		stemUpDown[iel] = ( (NoteYSTEM(aNoteL) < NoteYD(aNoteL)) ? 1 : -1 );
	}

	/* Compute the number of primary beams. */

	firstStemUpDown = stemUpDown[0];
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		pNoteBeam = GetPANOTEBEAM(noteBeamL);
		startend = pNoteBeam->startend;
		if (iel==0)
			nPrim = nestLevel = startend;
		else {
			if (iel<LinkNENTRIES(beamL)-1) {
				nestLevel += startend;
				if (nestLevel<nPrim) nPrim = nestLevel;
			}
		}
	}
	
	return nPrim;
}


/* ----------------------------------------------------------------- DoBreakBeam -- */
/* Try to add a secondary beam break before every selected note/chord. Handles user
interface on the assumption the given document is on the screen.

Should work fine for grace notes, but (as of v.998) this hasn't been tested;
anyway, the grace-beam drawing routines probably don't handle beam breaks. */

#define MAX_BEAMSETS MAX_MEASNODES		/* Enough for any reasonable but not any possible situation */

void DoBreakBeam(Document *doc)
{
	LINK pL, beamL, prevL, partL; short v, userVoice;
	PPARTINFO pPart; char vStr[20], partName[256];
	LINK beamLA[MAX_BEAMSETS]; short beamNPrimaryA[MAX_BEAMSETS];
	short	nBeamsets=0, i, nPrimary;
	Boolean found;
	
	DisableUndo(doc, FALSE);
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			for (v = 1; v<=MAXVOICES; v++) {
				if (VOICE_MAYBE_USED(doc, v))
					if (NoteInVoice(pL, v, TRUE)) {
						beamL = LVSearch(pL, BEAMSETtype, v, GO_LEFT, FALSE);
						if (beamL) {
							nPrimary = CountPrimaries(beamL);
							if (BreakBeamBefore(doc, beamL, pL, &prevL)) {
								doc->changed = TRUE;
								InvalMeasures(pL, prevL, BeamSTAFF(beamL));

							/*
							 *	If this beam is cross-staff, we'll have to update its stemlengths.
							 * We can't do that now, since other breaks in the same beamset may
							 *	follow, but remember its owning beamset for later use; if we run
							 *	out of space to remember it, just unbeam so at least we won't end
							 *	up with stems and beams that disagree.
							 */
								if (nBeamsets<MAX_BEAMSETS) {
									for (found = FALSE, i = 0; i<nBeamsets; i++)
										if (beamL==beamLA[i]) { found = TRUE; break; }
									if (!found) {
										beamLA[nBeamsets] = beamL;
										beamNPrimaryA[nBeamsets] = nPrimary;
										nBeamsets++;
									}
								}
								else {
									UnbeamV(doc, FirstInBeam(beamL), LastInBeam(beamL),
												BeamVOICE(beamL));
								}
							}
							else {
								if (Int2UserVoice(doc, v, &userVoice, &partL)) {
									sprintf(vStr, "%d", userVoice);
									pPart = GetPPARTINFO(partL);
									strcpy(partName, (strlen(pPart->name)>14? pPart->shortName : pPart->name));
								}
								GetIndCString(strBuf, BEAMERRS_STRS, 3);		/* " that would either break the primary beam or..." */
								CParamText(vStr, partName, strBuf, "");
								if (CautionAdvise(BREAKBEAM_ALRT)==Cancel) return;
							}
						}
					}
			}
		}
	
	BBFixCenterSecBeams(doc, beamLA, beamNPrimaryA, nBeamsets);
}
