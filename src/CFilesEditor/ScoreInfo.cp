/* ScoreInfo.c for Nightingale - display statistics for the score, including the
numbers of things of various types. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Copyright © 2016 by Avian Music Notation Foundation.
 * All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


static void SIDrawTextLine(char *);
static short MeasCount(Document *);
static short SICheckMeasDur(Document *doc, short *pFirstBad);
static short SICheckEmptyMeas(Document *doc, short *pFirstEmpty);
static short SICheckRange(Document *doc, short *pFirstOutOfRange);
static long GetScoreDuration(Document *doc);
static long DisplayHeapAndNoteCounts(Document *doc, unsigned short symbolCount[]);

#define LEADING 11			/* Vertical dist. between lines displayed (pixels) */

static Rect textRect;
static short linenum;
static char *str;

/* Draw the specified C string on the next line in Score Info dialog */

static void SIDrawTextLine(char *theStr)
{
	MoveTo(textRect.left, textRect.top+linenum*LEADING);
	DrawCString(theStr);
	++linenum;
	if (ShiftKeyDown() && OptionKeyDown()) LogPrintf(LOG_INFO, "%s\n", theStr);
}

/* Return the actual number of measures in the document. (Measure numbers in the object
list always start at 0, so the starting measure number is irrelevant.) */

static short MeasCount(Document *doc)
{
	LINK measL, syncL, theRestL;
	short intMeasNum;

	measL = LSSearch(doc->tailL, MEASUREtype, ANYONE, GO_LEFT, False);
	if (measL) {
		intMeasNum = FirstMeasMEASURENUM(measL);
		syncL = LSSearch(measL, SYNCtype, ANYONE, GO_RIGHT, False);
		if (!syncL) return intMeasNum;
		theRestL = FirstSubLINK(syncL);
		/* The subtype of a multibar rest is _minus_ the no. of measures. */
		return intMeasNum+abs(NoteType(theRestL));
	}
	else
		return 1;										/* should never happen */
}


/* Check the actual duration of every measure against its time signature in the same
way the Show Duration Problems command does. Return the no. of measures with "problems";
if the no. is non-zero, also set *pFirstBad to the measure no. of the first one that
has a problem. */

static short SICheckMeasDur(Document *doc, short *pFirstBad)
{
	LINK pL, barTermL, syncL, aNoteL, nextSyncL;
	long measDurFromTS, measDurActual;
	short nBad;

	for (nBad = 0, pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (MeasureTYPE(pL) && !IsFakeMeasure(doc, pL)) {
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
					/* If the "measure" contains an n-measure multibar rest, the only
						problem case is if there's another Sync in the measure. */
//LogPrintf(LOG_DEBUG, "SICheckMeasDur: suspicious measDurActual=%d\n", measDurActual);
					syncL = LSSearch(pL, SYNCtype, ANYONE, GO_RIGHT, False);
					aNoteL = FirstSubLINK(syncL);
					if (NoteREST(aNoteL) && NoteType(aNoteL)<WHOLEMR_L_DUR) {
						nextSyncL = LSSearch(RightLINK(syncL), SYNCtype, ANYONE, GO_RIGHT, False);
						if (!nextSyncL || IsAfter(barTermL, nextSyncL)) continue;
						LogPrintf(LOG_NOTICE, "SICheckMeasDur: problem with multibar rest. syncL=%u barTermL=%u nextSyncL=%u\n",
									syncL, barTermL, nextSyncL);
					}
					nBad++;
					if (nBad==1) *pFirstBad = GetMeasNum(doc, pL);

				}
			}
		}
	}
		
	return nBad;
}


/* Check for empty measures in the same way the Fill Empty Measures command does,
on a staff-by-staff basis. Return the no. of empty measures; if the number is non-zero,
also set *pFirstEmpty to the measure no. of the first one. */

static short SICheckEmptyMeas(Document *doc, short *pFirstEmpty)
{
	LINK barFirstL, barTermL, barL;
	short staff;
	Boolean foundNonEmptyVoice;
	short nEmpty;

	nEmpty = 0;
	barL = SSearch(doc->headL, MEASUREtype, False);
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


/* Return the duration of the score in units of our internal ticks, perhaps not exactly.
If there's a Sync after the last Measure object, use the approximate (see comments) end
time of the Sync; otherwise use the time of the last Measure.  */

static long GetScoreDuration(Document *doc)
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
				lastMeasL = LSSearch(pL, MEASUREtype, ANYONE, GO_LEFT, False);
//LogPrintf(LOG_DEBUG, "GetScoreDuration: Sync L%u lastSyncTime=%ld\n", pL, lastSyncTime);
				return lastSyncTime+MeasureTIME(lastMeasL);
			case MEASUREtype:
//LogPrintf(LOG_DEBUG, "GetScoreDuration: Measure L%u MeasureTIME=%ld\n", pL, MeasureTIME(pL));
				return MeasureTIME(pL);
			default:
				;
		}
	
	/* We found no Syncs (reasonable) and no Measures (should never happen). */
	return 0L;
}


#define PLURALQ(n) ((n)!=1? "s" : "")

/* Find and display the number of symbols and "sets" of symbols (ordinarily objects) of
each type, with a few exceptions. NB these numbers must agree with the user's concept
of things. In particular, they can't include invisible context-setting KeySigs: see
comments below. Return the total number of symbols listed. */

static long DisplayHeapAndNoteCounts(Document *doc, unsigned short symbolCount[])
{
	LINK pL, aKeySigL;
	HEAP *theHeap;
	const char *ps;
	short nInvisibleKSSets, objCount[LASTtype];
	long totalCount;
	unsigned short count, noteAttackCount;
	char strSets[256];
	Boolean allInvisibleInSet;
	
	noteAttackCount = CountNoteAttacks(doc);

	/* The symbolCount[KEYSIGtype] we've been given might context-setting KeySigs at
		the beginning of each system; if there's no actual key signature in effect,
		i.e., it's no sharps or flats, the user doesn't think they exist, so leave
		them out. */
		
	nInvisibleKSSets = 0;
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==KEYSIGtype) {
			allInvisibleInSet = True;
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
				if (KeySigNKSITEMS(aKeySigL)==0) symbolCount[KEYSIGtype]--;
				else allInvisibleInSet = False;
			}
			if (allInvisibleInSet) nInvisibleKSSets++;
		}
	}

	totalCount = 0L;
	for (short h = SYNCtype; h<OBJtype; h++) {
		theHeap = Heap+h;
		
		objCount[h] = 0;
		count = 0;
		for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
			if (ObjLType(pL)==h) count++;
		objCount[h] = count;

		if (h==KEYSIGtype) objCount[h] -= nInvisibleKSSets;
		
		switch (h) {
		
			/* Skip these types: they're uninteresting and/or we assume the info about
				them is given elsewhere. */

			case PAGEtype:
			case SYSTEMtype:
			case STAFFtype:
			case MEASUREtype:
			case CONNECTtype:
				break;

			/* For these types, there are no "sets". */
			 
			case BEAMSETtype:
			case OTTAVAtype:
			case TUPLETtype:
			case TEMPOtype:
				symbolCount[h] = objCount[h];
				if (objCount[h]>=0) {
					ps = NameHeapType(h, True);
					sprintf(str, "    %d %s%s", symbolCount[h], ps,
													PLURALQ(symbolCount[h]));
					SIDrawTextLine(str);
					totalCount += symbolCount[h];
				}
				break;

			/* These types are the most complex. */
			
			default:
				if (objCount[h]>=0) {
					ps = NameHeapType(h, True);
					if (objCount[h]>0)	sprintf(strSets, " (%d set%s)", objCount[h],
														PLURALQ(objCount[h]));
					else				sprintf(strSets, " ");

					if (h==SYNCtype)	sprintf(str, "    %d %s %s, %d note attack%s",
											symbolCount[h], ps, strSets, noteAttackCount,
														PLURALQ(noteAttackCount));
					else				sprintf(str, "    %d %s%s%s", symbolCount[h], ps,
														PLURALQ(symbolCount[h]), strSets);
					SIDrawTextLine(str);
					totalCount += symbolCount[h];
				}
		}
	}
	
	return totalCount;
}


#define OK_DI 1
#define CANCEL_DI 2
#define TEXT_DI 3
#define COMMENT_DI 5

void ScoreInfo()
	{
		DialogPtr dialogp;  GrafPtr oldPort;
		short ditem, aShort;  Handle aHdl;
		LINK pL, startPageL;
		Document *doc=GetDocumentFromWindow(TopDocument);
		unsigned short subobjCount[LASTtype], objsTotal;
		long totalCount, scoreDuration, qtrNTicks;
		short nBad, nEmpty, nOutOfRange, nUnjustSys, firstBad=0, firstEmpty=0,
				firstOutOfRange=0, firstUnjustPg=0;
		char fmtStr[256], commentOrig[256], commentNew[256];
		Boolean keepGoing;
		ModalFilterUPP filterUPP;

		if (!doc) return;
		
		filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP==NULL) {
			MissingDialog(SCOREINFO_DLOG);
			return;
		}
		 
		str = (char *)NewPtr(256);
		if (!GoodNewPtr((Ptr)str)) { OutOfMemory(256L); return; }

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
		OutlineOKButton(dialogp, True);
	
		linenum = 1;
		GetIndCString(fmtStr, SCOREINFO_STRS, 1);   		/* "SCORE INFORMATION" */
		sprintf(str, fmtStr);
		SIDrawTextLine(str);

		GetIndCString(fmtStr, SCOREINFO_STRS, 2);   		/* "%d page{s}, %d system{s}, %d measure{s}." */
		sprintf(str, fmtStr, doc->numSheets, PLURALQ(doc->numSheets),
						doc->nsystems, PLURALQ(doc->nsystems),
						MeasCount(doc), PLURALQ(MeasCount(doc)));
		SIDrawTextLine(str);
		GetIndCString(fmtStr, SCOREINFO_STRS, 3);   		/* "    A system contains %d stave{s}." */
		sprintf(str, fmtStr, doc->nstaves, PLURALQ(doc->nstaves));
		SIDrawTextLine(str);

		CountSubobjsByHeap(doc, subobjCount, False);
		totalCount = DisplayHeapAndNoteCounts(doc, subobjCount);

		objsTotal = 0;	
		for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
			objsTotal++;
		objsTotal++;
		GetIndCString(fmtStr, SCOREINFO_STRS, 4);   		/* "%ld total symbols (%d object{s})." */
		sprintf(str, fmtStr, totalCount, objsTotal);
		SIDrawTextLine(str);

		scoreDuration = GetScoreDuration(doc);
		qtrNTicks = Code2LDur(QTR_L_DUR, 0);
		sprintf(str, "------------------------------------------");
		SIDrawTextLine(str);
		GetIndCString(fmtStr, SCOREINFO_STRS, 8);   		/* "Duration: approx. %ld quarter{s} (%ld ticks)." */
		sprintf(str, fmtStr, scoreDuration/qtrNTicks, PLURALQ(scoreDuration/qtrNTicks), scoreDuration); 
		SIDrawTextLine(str);
		
		WaitCursor();
		nBad = SICheckMeasDur(doc, &firstBad);
		if (nBad>0) {
			GetIndCString(fmtStr, SCOREINFO_STRS, 5);   /* "Duration problems in %d measure{s} (first=%d)." */
			sprintf(str, fmtStr, nBad, PLURALQ(nBad), firstBad);
		}
		else {
			GetIndCString(fmtStr, SCOREINFO_STRS, 6);   /* "No measures have duration problems." */
			sprintf(str, fmtStr);
		}
		SIDrawTextLine(str);

		nEmpty = SICheckEmptyMeas(doc, &firstEmpty);
		if (nEmpty>0) {
			GetIndCString(fmtStr, SCOREINFO_STRS, 9);   /* "%d empty staff-measure{s} (first in measure %d)." */
			sprintf(str, fmtStr, nEmpty, PLURALQ(nEmpty), firstEmpty);
		}
		else {
			GetIndCString(fmtStr,  SCOREINFO_STRS, 10);   /* "No empty staff-measures found." */
			sprintf(str, fmtStr);
		}
		SIDrawTextLine(str);

		nOutOfRange = SICheckRange(doc, &firstOutOfRange);
		if (nOutOfRange>0) {
			GetIndCString(fmtStr, SCOREINFO_STRS, 11);   /* "%d out-of-range note{s} (first in measure %d)." */
			sprintf(str, fmtStr, nOutOfRange, PLURALQ(nOutOfRange), firstOutOfRange);
		}
		else {
			GetIndCString(fmtStr,  SCOREINFO_STRS, 12);   /* "No out-of-range notes found." */
			sprintf(str, fmtStr);
		}
		SIDrawTextLine(str);

		startPageL = SSearch(doc->headL, PAGEtype, False);
		nUnjustSys = CountUnjustifiedSystems(doc, startPageL, NILINK, &firstUnjustPg);
		if (nUnjustSys>0) {
			GetIndCString(fmtStr, SCOREINFO_STRS, 13);   /* "%d systems aren't right justified (first on page %d)." */
			sprintf(str, fmtStr, nUnjustSys, firstUnjustPg);
		}
		else {
			GetIndCString(fmtStr,  SCOREINFO_STRS, 14);   /* "All systems are right justified." */
			sprintf(str, fmtStr);
		}
		SIDrawTextLine(str);
		
		ArrowCursor();
					
		if (HasMidiMap(doc)) {
			Str255 fName;
			FSSpec *fsSpec = (FSSpec *)*doc->midiMapFSSpecHdl;
			Pstrcpy(fName, fsSpec->name);
			sprintf(str, "MidiMap %s", PtoCstr(fName));
			SIDrawTextLine(str);
		}
		else {
			sprintf(str, "%s", "No MidiMap");
			SIDrawTextLine(str);
		}
		
		strcpy(commentOrig, (char *)doc->comment);
		CToPString(strcpy(commentNew, (char *)commentOrig));
		PutDlgString(dialogp, COMMENT_DI, (unsigned char *)commentNew, True);
		keepGoing = True;

		while (keepGoing) {
			ModalDialog(filterUPP, &ditem);
			if (ditem==OK_DI || ditem==CANCEL_DI) keepGoing = False;
		}
		HideWindow(GetDialogWindow(dialogp));

		/* If user OK'd dialog and changed the comment, save the change. */
		if (ditem==OK_DI) {
			GetDlgString(dialogp, COMMENT_DI, (unsigned char *)commentNew);
			PToCString((StringPtr)commentNew);
			if (strlen(commentNew)>MAX_COMMENT_LEN) {
				GetIndCString(fmtStr, MISCERRS_STRS, 31);	/* "Comment exceeds the limit of %d characters: it will be truncated." */
				sprintf(strBuf, fmtStr, MAX_COMMENT_LEN);
				CParamText(strBuf, "", "", "");
				CautionInform(GENERIC_ALRT);
				commentNew[MAX_COMMENT_LEN] = '\0';
			}
			if (doc && !streql(commentOrig, (char *)commentNew)) {
				strcpy((char *)doc->comment, (char *)commentNew);
				doc->changed = True;
			}
		}
		
		DisposePtr((Ptr)str);
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dialogp);								/* Free heap space */
		SetPort(oldPort);
	}
