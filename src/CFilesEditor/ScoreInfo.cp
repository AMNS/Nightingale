/* ScoreInfo.c for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Copyright © 2016 by Avian Music Notation Foundation.
 * All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ------------------------------------------------------- ScoreInfo and helpers -- */
/* Display statistics for the score, including the no. of things of various types. */

static void SIDrawLine(char *);
static short MeasCount(Document *);
static short SICheckMeasDur(Document *doc, short *pFirstBad);
static short SICheckEmptyMeas(Document *doc, short *pFirstEmpty);
static short SICheckRange(Document *doc, short *pFirstOutOfRange);
static short CountNoteAttacks(Document *doc);
static long GetScoreDuration(Document *doc);

#define LEADING 11			/* Vertical dist. between lines displayed (pixels) */

static Rect textRect;
static short linenum;
static char *str;

/* Draw the specified C string on the next line in Score Info dialog */

static void SIDrawLine(char *s)
{
	MoveTo(textRect.left, textRect.top+linenum*LEADING);
	DrawCString(s);
	++linenum;
}

static short MeasCount(Document *doc)
{
	LINK measL; PAMEASURE aMeasure;

	measL = MNSearch(doc, doc->tailL, ANYONE, GO_LEFT, TRUE);
	if (measL) {
		aMeasure = GetPAMEASURE(FirstSubLINK(measL));
		return aMeasure->measureNum+1;
	}
	else
		return 1;
}


/* Check the actual duration of every measure against its time signature in the same
way the Show Duration Problems command does. Return the no. of measures with "problems";
if the no. is non-zero, also set *pFirstBad to the measure no. of the first one that
has a problem. */

static short SICheckMeasDur(Document *doc, short *pFirstBad)
{
	LINK pL, barTermL;
	long measDurFromTS, measDurActual;
	short nBad;

	for (nBad = 0, pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (MeasureTYPE(pL) && !FakeMeasure(doc, pL)) {
			barTermL = EndMeasSearch(doc, pL);
			if (barTermL) {
				measDurFromTS = GetTimeSigMeasDur(doc, barTermL);
				if (measDurFromTS<0) {
					nBad++;
					if (nBad==1) *pFirstBad = GetMeasNum(doc, pL);
					continue;
				}

				measDurActual = GetMeasDur(doc, barTermL, ANYONE);
				if (measDurActual!=0 && ABS(measDurFromTS-measDurActual)>=PDURUNIT) {
					nBad++;
					if (nBad==1) *pFirstBad = GetMeasNum(doc, pL);
				}
			}
		}
	}
		
	return nBad;
}


/* Check for empty measures in the same way the Fill Empty Measures command does,
on a staff-by-staff basis. Return the no. of empty measures; if the number is
non-zero, also set *pFirstEmpty to the measure no. of the first one. */

static short SICheckEmptyMeas(Document *doc, short *pFirstEmpty)
{
	LINK barFirstL, barTermL, barL;
	short staff;
	Boolean foundNonEmptyVoice;
	short nEmpty;

	nEmpty = 0;
	barL = SSearch(doc->headL, MEASUREtype, FALSE);
	for ( ; barL && barL!=NILINK; barL = LinkRMEAS(barL)) {
		if (MeasISFAKE(barL)) continue;
		barFirstL = RightLINK(barL);
		barTermL = EndMeasSearch(doc, barL);

		for (staff = 1; staff<=doc->nstaves; staff++) {
			if (IsRangeEmpty(barL, barTermL, staff, &foundNonEmptyVoice)) {
						nEmpty++;
						if (nEmpty==1) *pFirstEmpty = GetMeasNum(doc, barL);
			}
		}
	//LogPrintf(LOG_DEBUG, "SICheckEmptyMeas >: barL=%d nEmpty=%d\n", barL, nEmpty);
	}
		
	return nEmpty;
}


/* Check MIDI noteNums for all notes in the score against the ranges of the instruments
assigned to their parts the same way the CheckRange command does: cf. CheckRange(). */

static short SICheckRange(Document *doc, short *pFirstOutOfRange)
{
	LINK pL, partL, aNoteL;  PPARTINFO pPart;
	short firstStaff, lastStaff, hiKeyNum, loKeyNum, nOutOfRange;
	short transposition, writtenNoteNum, firstOutOfRangeMNum, thisMNum;
	
	nOutOfRange = 0;
	firstOutOfRangeMNum = SHRT_MAX;
	partL = FirstSubLINK(doc->headL);
	for ( ; partL; partL = NextPARTINFOL(partL)) {
		pPart = GetPPARTINFO(partL);
		firstStaff = pPart->firstStaff;
		lastStaff = pPart->lastStaff;
		hiKeyNum = pPart->hiKeyNum;
		loKeyNum = pPart->loKeyNum;
		transposition = pPart->transpose;
		//LogPrintf(LOG_DEBUG, "SICheckRange: firstStaff=%d loKeyNum=%d hiKeyNum=%d\n", firstStaff, loKeyNum, hiKeyNum);
		for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
			if (SyncTYPE(pL))
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)>=firstStaff && NoteSTAFF(aNoteL)<=lastStaff
					&&	!NoteREST(aNoteL)) {
						/* hiKeyNum and loKeyNumIf are written pitches, but if the score
							isn't transposed, it contains sounding pitches; convert. */
						writtenNoteNum = NoteNUM(aNoteL);
						if (!doc->transposed) writtenNoteNum -= transposition;
						if (writtenNoteNum>hiKeyNum || writtenNoteNum<loKeyNum) {
							nOutOfRange++;
							thisMNum = GetMeasNum(doc, pL);
							if (thisMNum<firstOutOfRangeMNum)
								*pFirstOutOfRange = firstOutOfRangeMNum = thisMNum;
						}
					}
		}
	}
	
	return nOutOfRange;
}


static short CountNoteAttacks(Document *doc)
{
	LINK pL, aNoteL;
	short nNoteAttacks = 0;
	
	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (!NoteREST(aNoteL) && !NoteTIEDL(aNoteL)) nNoteAttacks++;
	}
	return nNoteAttacks;
}


/* Return the approximate (see comments) duration of the score. */

long GetScoreDuration(Document *doc)
{
	LINK pL, lastMeasL;
	long lastSyncTime;
	
	for (pL = doc->tailL; pL && pL!=doc->headL; pL = LeftLINK(pL)) 
		switch (ObjLType(pL)) {
			case SYNCtype:
				lastSyncTime = SyncTIME(pL);
				/* Assume the score ends when the first note of the last Sync stops
				 * sounding: this may not be correct, but it'll be close, and good
				 * enough for the purpose.
				 */
				lastSyncTime += NotePLAYDUR(FirstSubLINK(pL));
				lastMeasL = LSSearch(pL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
				return lastSyncTime+MeasureTIME(lastMeasL);
			case MEASUREtype:
				return MeasureTIME(pL);
			default:
				;
		}
	
	/* We found no Syncs (reasonable) and no Measures (should never happen). */
	return 0L;
}


#define OK_DI 1
#define CANCEL_DI 2
#define TEXT_DI 3
#define COMMENT_DI 5

void ScoreInfo()
	{
		DialogPtr dialogp; GrafPtr oldPort;
		short ditem, aShort; Handle aHdl;
		LINK pL; const char *ps; HEAP *theHeap;
		unsigned short h, count, objCount[LASTtype], objsTotal, noteAttackCount;
		long lObjCount[LASTtype], totalCount, scoreDuration, qtrNTicks;
		Document *doc=GetDocumentFromWindow(TopDocument);
		short nBad, firstBad, nEmpty, firstEmpty, nOutOfRange, firstOutOfRange;
		char fmtStr[256], commentOrig[256], commentNew[256];
		Boolean keepGoing;
		ModalFilterUPP filterUPP;

		filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP==NULL) {
			MissingDialog(SCOREINFO_DLOG);
			return;
		}
		 
		str = (char *)NewPtr(256);
		if (!GoodNewPtr((Ptr)str))
			{ OutOfMemory(256L); return; }

		GetPort(&oldPort);
		dialogp = GetNewDialog(SCOREINFO_DLOG, NULL, BRING_TO_FRONT);
		if (!dialogp) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(SCOREINFO_DLOG);
			return;
		}
		SetPort(GetDialogWindowPort(dialogp));
		GetDialogItem(dialogp, TEXT_DI, &aShort, &aHdl, &textRect);
		SetDlgFont(dialogp, textFontNum, textFontSmallSize, 0);

		CenterWindow(GetDialogWindow(dialogp), 55);
		ShowWindow(GetDialogWindow(dialogp));
						
		ArrowCursor();
		OutlineOKButton(dialogp, TRUE);
	
		linenum = 1;
		GetIndCString(fmtStr, SCOREINFO_STRS, 1);   			/* "SCORE INFORMATION:" */
		sprintf(str, fmtStr);
		SIDrawLine(str);
		if (doc) {
			GetIndCString(fmtStr, SCOREINFO_STRS, 2);   		/* "%d pages, %d systems, %d measures." */
			sprintf(str, fmtStr, doc->numSheets,
							doc->nsystems, MeasCount(doc));
			SIDrawLine(str);
			GetIndCString(fmtStr, SCOREINFO_STRS, 3);   		/* "    A system contains %d staves." */
			sprintf(str, fmtStr, doc->nstaves);
			SIDrawLine(str);
	
			CountInHeaps(doc, objCount, FALSE);
			noteAttackCount = CountNoteAttacks(doc);

			totalCount = 0L;
		 	for (h = SYNCtype; h<OBJtype; h++) {
		 		theHeap = Heap+h;
				
		 		switch (h) {
		 			/*
		 			 *	Skip these types: they're uninteresting and/or info already given.
					 */
					case STAFFtype:
					case MEASUREtype:
					case CONNECTtype:
						lObjCount[h] = -1L;
						break;
		 			/*
		 			 * For these types, users are interested in the numbers of objects.
		 			 */
		 			case BEAMSETtype:
		 			case OTTAVAtype:
		 			case TUPLETtype:
		 			case TEMPOtype:
		 			case SPACERtype:
		 			case ENDINGtype:
						count = 0;
		 				for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		 					if (ObjLType(pL)==h) count++;
		 				lObjCount[h] = count;
		 				break;
		 			default:
		 			/*
		 			 * For other types, users care about the numbers of subobjects, which
		 			 * we've already computed, or the type is uninteresting.
		 			 */
						if (theHeap->nObjs<=0)
							lObjCount[h] = -1L;
						else
							lObjCount[h] = objCount[h];
		 		}

				if (lObjCount[h]>=0) {
					ps = NameHeapType(h, TRUE);
					if (h==SYNCtype)	sprintf(str, "    %ld %s;  %d note attacks",
												lObjCount[h], ps, noteAttackCount);
		 			else				sprintf(str, "    %ld %s", lObjCount[h], ps);
					SIDrawLine(str);
					totalCount += lObjCount[h];
				}
		 	}

			objsTotal = 0;	
			for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
				objsTotal++;
			objsTotal++;
			GetIndCString(fmtStr, SCOREINFO_STRS, 4);   		/* "%ld total symbols (%d objects)." */
			sprintf(str, fmtStr, totalCount, objsTotal);
			SIDrawLine(str);

			scoreDuration = GetScoreDuration(doc);
			qtrNTicks = Code2LDur(QTR_L_DUR, 0);
			sprintf(str, "------------------------------------------");
			SIDrawLine(str);
			GetIndCString(fmtStr, SCOREINFO_STRS, 8);   		/* "Duration: approx. %ld quarters." */
			sprintf(str, fmtStr, scoreDuration/qtrNTicks); 
			SIDrawLine(str);

			if (!ShiftKeyDown()) {
				WaitCursor();
				nBad = SICheckMeasDur(doc, &firstBad);
				if (nBad>0) {
					GetIndCString(fmtStr, SCOREINFO_STRS, 5);   /* "Duration problems in %d measure(s) (first=%d)." */
			 		sprintf(str, fmtStr, nBad, firstBad);
				}
				else {
					GetIndCString(fmtStr, SCOREINFO_STRS, 6);   /* "No measures have duration problems." */
			 		sprintf(str, fmtStr);
			 	}
				SIDrawLine(str);

				nEmpty = SICheckEmptyMeas(doc, &firstEmpty);
				if (nEmpty>0) {
					GetIndCString(fmtStr, SCOREINFO_STRS, 9);   /* "%d empty staff-measure(s) (first in measure %d)." */
			 		sprintf(str, fmtStr, nEmpty, firstEmpty);
				}
				else {
					GetIndCString(fmtStr,  SCOREINFO_STRS, 10);   /* "No empty staff-measures found." */
			 		sprintf(str, fmtStr);
			 	}
				SIDrawLine(str);

				nOutOfRange = SICheckRange(doc, &firstOutOfRange);
				if (nOutOfRange>0) {
					GetIndCString(fmtStr, SCOREINFO_STRS, 11);   /* "%d out-of-range notes (first in measure %d)." */
			 		sprintf(str, fmtStr, nOutOfRange, firstOutOfRange);
				}
				else {
					GetIndCString(fmtStr,  SCOREINFO_STRS, 12);   /* "No out-of-range notes found." */
			 		sprintf(str, fmtStr);
			 	}
				SIDrawLine(str);
				ArrowCursor();
			}
			
			
			if (HasMidiMap(doc)) {
				Str255 fName;
				FSSpec *fsSpec = (FSSpec *)*doc->midiMapFSSpecHdl;
				Pstrcpy(fName, fsSpec->name);
				sprintf(str, "MidiMap %s", PtoCstr(fName));
				SIDrawLine(str);
			}
			else {
				sprintf(str, "%s", "No MidiMap");
				SIDrawLine(str);
			}
			
			strcpy(commentOrig, (char *)doc->comment);
			CToPString(strcpy(commentNew, (char *)commentOrig));
			PutDlgString(dialogp, COMMENT_DI, (unsigned char *)commentNew, TRUE);
		}
		else {
			GetIndCString(fmtStr, SCOREINFO_STRS, 7);   	/* "No score is open." */
			sprintf(str, fmtStr);
			SIDrawLine(str);
		}

		keepGoing = TRUE;

		while (keepGoing) {
			ModalDialog(filterUPP, &ditem);
			if (ditem==OK_DI || ditem==CANCEL_DI) keepGoing = FALSE;
		}
		HideWindow(GetDialogWindow(dialogp));

		/* If user OK'd dialog and changed the comment, save the change. */
		if (ditem==OK_DI) {
			GetDlgString(dialogp, COMMENT_DI, (unsigned char *)commentNew);
			PToCString((unsigned char *)commentNew);
			if (strlen(commentNew)>MAX_COMMENT_LEN) {
				GetIndCString(fmtStr, MISCERRS_STRS, 31);	/* "Comment exceeds the limit of %d characters: it will be truncated." */
				sprintf(strBuf, fmtStr, MAX_COMMENT_LEN);
				CParamText(strBuf, "", "", "");
				CautionInform(GENERIC_ALRT);
				commentNew[MAX_COMMENT_LEN] = '\0';
			}
			if (doc && !streql(commentOrig, (char *)commentNew)) {
				strcpy((char *)doc->comment, (char *)commentNew);
				doc->changed = TRUE;
			}
		}
		
		DisposePtr((Ptr)str);
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dialogp);								/* Free heap space */
		SetPort(oldPort);
	}
