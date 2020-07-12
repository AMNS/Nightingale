/* MeasFill.c for Nightingale - functions to fill empty measures with whole-measure
rests, and functions to fill non-empty measures with rests before, between, and after
the notes/rests/chords. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* =============================================================== Fill empty measures == */

static short Fill1EmptyMeas(Document *, LINK, LINK, Boolean *);

/* ------------------------------------------------------------------- FillEmptyDialog -- */

static enum {
	STARTMEAS_DI=4,
	ENDMEAS_DI=6
} E_MeasFillItems;

Boolean FillEmptyDialog(Document *doc, short *startMN, short *endMN)
{
	DialogPtr dlog;
	GrafPtr oldPort;
	short ditem;
	short dialogOver, value;
	ModalFilterUPP filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(FILLEMPTY_DLOG);
		return False;
	}
	
	GetPort(&oldPort);
	dlog = GetNewDialog(FILLEMPTY_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MayErrMsg("FillEmptyDialog: unable to get dialog %ld", (long)FILLEMPTY_DLOG);
		return False;
	}
 	SetPort(GetDialogWindowPort(dlog)); 			
	
	PutDlgWord(dlog, STARTMEAS_DI, *startMN, False);
	PutDlgWord(dlog, ENDMEAS_DI, *endMN, True);

	ShowWindow(GetDialogWindow(dlog));
	OutlineOKButton(dlog, True);
	
	dialogOver = 0;
	do {
		ModalDialog(filterUPP, &ditem);
		if (ditem==OK) {
			if (!GetDlgWord(dlog,STARTMEAS_DI,&value)
			||  value<doc->firstMNNumber
			||  !MNSearch(doc, doc->headL, value-doc->firstMNNumber, GO_RIGHT, True)) {
				GetIndCString(strBuf, MEASFILLERRS_STRS, 1);    /* "Start measure is not in the score." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
			}
			else if (!GetDlgWord(dlog,ENDMEAS_DI,&value)
			||  value<doc->firstMNNumber
			||  !MNSearch(doc, doc->headL, value-doc->firstMNNumber, GO_RIGHT, True)) {
				GetIndCString(strBuf, MEASFILLERRS_STRS, 2);    /* "End measure is not in the score." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
			}
			else
				dialogOver = ditem;
		}
		else
			dialogOver = ditem;
	} while (!dialogOver);
	
	GetDlgWord(dlog, STARTMEAS_DI, startMN);
	GetDlgWord(dlog, ENDMEAS_DI, endMN);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);

	return (ditem==OK);
}

/* ---------------------------------------------------------------------- IsRangeEmpty -- */
/* Return True if the given range and staff contains no notes or rests in any voice,
and the staff's default voice contains no notes or rests on any staff. Intended for use
by the Fill Empty Measures command. */

Boolean IsRangeEmpty(LINK startL, LINK endL,
						short staff,
						Boolean *pNonEmptyVoice)	/* Return True = staff's default voice is on another staff */
{
	Boolean staffEmpty, voiceEmpty;
	LINK pL;
	
	staffEmpty = voiceEmpty = True;
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			if (NoteOnStaff(pL, staff)) {
				staffEmpty = False;
				break;
			}
			if (NoteInVoice(pL, staff, False))		/* Is this staff's dflt voice on ANY staff? */
				voiceEmpty = False;					/* can't break bcs staff might not be empty */
		}

	*pNonEmptyVoice = (staffEmpty && !voiceEmpty);
	return (staffEmpty && voiceEmpty);
}


/* -------------------------------------------------------------------- Fill1EmptyMeas -- */
/* If the given Measure is fake, do nothing and return False. Otherwise, add a whole-
measure rest to each staff in the Measure that contains no notes or rests and whose
default voice contains no notes or rests (they might be on another staff), and return
True. If the Measure already contains at least one Sync and we do add any whole-measure
rests, we add all of them to the Measure's first Sync. FIXME: Should return an error
indication: probably should return short FAILURE, NOTHING_TO_DO or OP_COMPLETE. */

static short Fill1EmptyMeas(
					Document *doc,
					LINK barL, LINK barTermL,
					Boolean *nonEmptyVoice)	 /* True=found at least 1 empty staff w/default voice nonempty */
{
	LINK syncL, pL, aRestL;
	short staff, saveVRole;
	Boolean addRest, foundNonEmptyVoice;
	short nFilled=0;
	
	*nonEmptyVoice = False;
	if (MeasISFAKE(barL)) return 0;
	
	/* If the Measure contains any Syncs now, use the first one to add the whole-measure
	   rests to; otherwise we'll have to create a Sync. */
	   
	for (syncL = NILINK, pL = barL; pL!=barTermL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			syncL = pL;
			break;
		}

	for (staff = 1; staff<=doc->nstaves; staff++) {
		addRest = IsRangeEmpty(barL, barTermL, staff, &foundNonEmptyVoice);
		if (addRest) {
			if (!syncL) {
				syncL = InsertNode(doc, barTermL, SYNCtype, 1);
				if (!syncL) {
					NoMoreMemory();
					return nFilled;
				}

				/* FIXME: Maybe we should initialize object YD and <tweaked> in NewNode? */
				
				SetObject(syncL, 0, 0, False, True, False);
				LinkTWEAKED(syncL) = False;
				aRestL = FirstSubLINK(syncL);
			}
			else if (!ExpandNode(syncL, &aRestL, 1)) {
				NoMoreMemory();
				return nFilled;
			}

			/* The rest's vertical position should probably ignore multivoice
			   position offset. To do this, we  set doc->voiceTab[voice].voiceRole to
			   single before calling SetupNote and restore its old value afterwards. */
			   
			saveVRole = doc->voiceTab[staff].voiceRole;				/* Default voice no. = staff no. */
			doc->voiceTab[staff].voiceRole = VCROLE_SINGLE;
			SetupNote(doc, syncL, aRestL, staff, 0, WHOLEMR_L_DUR, 0, staff, True, 0, 0);
			doc->voiceTab[staff].voiceRole = saveVRole;
			nFilled++;
		}
		if (foundNonEmptyVoice) *nonEmptyVoice = True;
	}

	return nFilled;
}


/* --------------------------------------------------------------------- FillEmptyMeas -- */
/* In each non-fake Measure of the given inclusive range of Measures, in each staff that
contains no notes or rests, put a whole-measure rest in the default voice. Exception:
if the staff's default voice is on another staff, do nothing. Return the number of rests
added. */

short FillEmptyMeas(
					Document *doc,
					LINK startBarL, LINK endBarL)		/* 1st and last Measures to fill or NILINK */
{
	LINK barFirstL, barTermL, barL;
	Boolean nonEmptyVoice, anyNonEmptyVoice=False;
	DDIST mWidth;
	short nFilled=0;
			
	/* If start is after end Measure, there's nothing to do. */
	
	if (IsAfter(endBarL, startBarL)) return False;

	if (!startBarL)
		startBarL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);
	if (!endBarL)
		endBarL = LSSearch(doc->tailL, MEASUREtype, ANYONE, GO_LEFT, False);

	barL = startBarL;
	for ( ; barL && barL!=LinkRMEAS(endBarL); barL = LinkRMEAS(barL)) {
		barFirstL = RightLINK(barL);
		barTermL = EndMeasSearch(doc, barL);
		nFilled += Fill1EmptyMeas(doc, barL, barTermL, &nonEmptyVoice);
		if (nonEmptyVoice) anyNonEmptyVoice = True;
	}
	
	if (nFilled>0) {
		doc->changed = True;
		if (doc->autoRespace)
			RespaceBars(doc, RightLINK(startBarL), RightLINK(endBarL), 0L, False, False);
		else {
			InvalMeasures(startBarL, endBarL, ANYONE);					/* Force redrawing */
			if (config.alwaysCtrWholeMR) {
				/* Regardless of autoRespace, center the new whole-measure rests. */
				
				barL = startBarL;
				for ( ; barL && barL!=LinkRMEAS(endBarL); barL = LinkRMEAS(barL)) {
					barFirstL = RightLINK(barL);
					barTermL = EndMeasSearch(doc, barL);
					mWidth = MeasWidth(barL);
					CenterWholeMeasRests(doc, barFirstL, barTermL, mWidth);
				}
			}
		}
	}
	
	if (anyNonEmptyVoice) {
		GetIndCString(strBuf, MEASFILLERRS_STRS, 3);    /* "Nightingale left one or more empty measures unfilled because the default voice was on another staff." */
		CParamText(strBuf, "", "", "");
		NoteInform(GENERIC_ALRT);
	}
	
	return nFilled;
}


/* =========================================================== Fill non-empty measures == */

static LINK FindTStampInMeas(LINK, short, Boolean);
static Boolean AddFillRest(Document *, LINK, short, short, short);
static Boolean Fill1NonemptyMeas(Document *, LINK, LINK, short, short);

static LINK FindTStampInMeas(LINK pL, short time, Boolean goLeft)
{
	LINK qL;

	for (qL=pL; qL; qL=GetNext(qL, goLeft)) {
		if (MeasureTYPE(qL)) return NILINK;
		if (SyncTYPE(qL) && SyncTIME(qL)<=time)
			return qL;
	}

	return NILINK;
}

LINK NewRestSync(Document *doc, LINK pL, short time, LINK *pSyncL)
{
	LINK syncL;
	
	pL = FindInsertPt(pL);
	syncL = InsertNode(doc, pL, SYNCtype, 1);
	SetObject(syncL, 0, 0, False, True, False);
	LinkTWEAKED(syncL) = False;
	SyncTIME(syncL) = time;
	if (syncL) {
		*pSyncL = syncL;
		return FirstSubLINK(syncL);
	}
	else {
		NoMoreMemory();
		return NILINK;
	}
}

/* Generate rest(s) to fill the given amount of time. Return True normally, False if
there's an error. */

static Boolean AddFillRest(Document *doc, LINK pL, short voice, short startTime, short fillDur)
{
	LINK syncL, newSyncL, aRestL; short nPieces, n, prevEndTime;
	
	/* Look for a Sync at the time we want. If we find one, just add a subobject to it.
	   If not, if we find a Sync earlier than we want, add a new Sync after it. If we
	   can't find an earlier one either, add a new Sync right here. */
		
	syncL = FindTStampInMeas(LeftLINK(pL), startTime, GO_LEFT);
	if (!syncL) {
		aRestL = NewRestSync(doc, pL, startTime, &syncL);
		if (!aRestL) return False;
	}
	else {
		if (SyncTIME(syncL)!=startTime) {
			aRestL = NewRestSync(doc, RightLINK(syncL), startTime, &newSyncL);
			if (!aRestL) return False;
			syncL = newSyncL;
		}
		else if (!ExpandNode(syncL, &aRestL, 1)) {
			NoMoreMemory();
			return False;
		}
	}

	/* Set the rest's duration and clarify it. We expect a normal data structure here
	   with legitimate Measures and correct time signature fields in the context, so
	   call the standard function SetAndClarifyDur. Finally, we have to set the
	   timeStamps of the new Syncs to keep the data structure "normal". */
		
	SetupNote(doc, syncL, aRestL, voice, 0, UNKNOWN_L_DUR, 0, voice, True, 0, 0);
	
	nPieces = SetAndClarifyDur(doc, syncL, aRestL, fillDur);
	prevEndTime = SyncTIME(syncL)+SimpleLDur(aRestL);
	for (n = 1, pL = RightLINK(syncL); n<nPieces; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aRestL = NoteInVoice(pL, voice, False);
			if (aRestL) {
				SyncTIME(pL) = prevEndTime;
				prevEndTime += SimpleLDur(aRestL);					
				n++;
			}
		}

	return True;
}


/* If the given voice of the given Measure contains no notes or rests, do nothing.
Otherwise, add any rests necessary to fill gaps in the voice within the Measure,
putting them on the voice's default staff. */

static Boolean Fill1NonemptyMeas(Document *doc, LINK barL, LINK barTermL, short voice,
											short quantum)
{
	short timeUsed; LINK pL, aNoteL, nextBarL; short fillDur, measDur;
	Boolean foundNotes=False;
	
	timeUsed = 0;
	
	for (pL = RightLINK(barL); pL!=barTermL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:												/* Should never get here */
				break;
			case SYNCtype:
				aNoteL = NoteInVoice(pL, voice, False);
				if (!aNoteL) break;

				foundNotes = True;				
				fillDur = SyncTIME(pL)-timeUsed;
				if (fillDur>=quantum) 
					if (!AddFillRest(doc, pL, voice, timeUsed, fillDur)) return False;
				timeUsed = SyncTIME(pL)+CalcNoteLDur(doc, aNoteL, pL);
				break;
			default:
				;
		}
	}
	
	if (foundNotes) {
		/* Fill out the Measure unless it's the last in the score. */
		nextBarL = LinkRMEAS(barL);
		if (nextBarL) {
			measDur = MeasureTIME(nextBarL)-MeasureTIME(barL);
			fillDur = measDur-timeUsed;
			if (fillDur>=quantum) 
				if (!AddFillRest(doc, barTermL, voice, timeUsed, fillDur)) return False;
		}
	}	
	return True;
}


/* In every Measure of the given INCLUSIVE range of Measures that contains any notes/
rests/chords, fill gaps by adding rests as necessary before, between, and after the notes/
rests/chords. Considers only the default voice on each staff. */

Boolean FillNonemptyMeas(Document *doc,
								LINK startMeasL, LINK endMeasL,	/* 1st and last Measures to fill or NILINK */
								short staff, short quantum
								)
{
	LINK measL, lastL;
	
	if (!startMeasL)
		startMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);
	if (!endMeasL)
		endMeasL = LSSearch(doc->tailL, MEASUREtype, ANYONE, GO_LEFT, False);

	measL = startMeasL;
	for ( ; measL && measL!=LinkRMEAS(endMeasL); measL = LinkRMEAS(measL)) {
		lastL = EndMeasSearch(doc, measL);
		
		/* Treat staff and voice as identical because we always want the default voice. */
		if (!Fill1NonemptyMeas(doc, measL, lastL, staff, quantum))
			return False;
	}
	
	return True;
}

