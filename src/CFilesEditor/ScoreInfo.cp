/* ScoreInfo.c for Nightingale - rev. for v. 2000 */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ------------------------------------------------------- ScoreInfo and helpers -- */
/* Display statistics for the score, including the no. of things of various types. */

static void SIDrawLine(char *);
static INT16 MeasCount(Document *);
static INT16 SICheckMeasDur(Document *, INT16 *);
static long GetScoreDuration(Document *doc);

#define LEADING 11			/* Vertical dist. between lines displayed (pixels) */

static Rect textRect;
static INT16 linenum;
static char *s;

/* Draw the specified C string on the next line in Score Info dialog */

static void SIDrawLine(char *s)
{
	MoveTo(textRect.left, textRect.top+linenum*LEADING);
	DrawCString(s);
	++linenum;
}

static INT16 MeasCount(Document *doc)
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

INT16 SICheckMeasDur(Document *doc, INT16 *pFirstBad)
{
	LINK pL, barTermL;
	long measDurNotated, measDurActual;
	INT16 nBad;

	for (nBad = 0, pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {

		if (MeasureTYPE(pL) && !FakeMeasure(doc, pL)) {
			barTermL = EndMeasSearch(doc, pL);
			if (barTermL) {
				measDurNotated = NotatedMeasDur(doc, barTermL);
				if (measDurNotated<0) {
					nBad++;
					if (nBad==1) *pFirstBad = GetMeasNum(doc, pL);
					continue;
				}

				measDurActual = GetMeasDur(doc, barTermL);
				if (measDurActual!=0 && ABS(measDurNotated-measDurActual)>=PDURUNIT) {
					nBad++;
					if (nBad==1) *pFirstBad = GetMeasNum(doc, pL);
				}
			}
		}
	}
		
	return nBad;
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
				/* Assume the score ends when the first note of the Sync stops
				 * sounding: this may not be correct, but it'll be close, and
				 * good enough for the purpose.
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
		unsigned INT16 h, count, objCount[LASTtype], objsTotal;
		long lObjCount[LASTtype], totalCount;
		long scoreDuration, qtrNTicks;
		Document *doc=GetDocumentFromWindow(TopDocument);
		INT16 nBad, firstBad;
		char fmtStr[256], commentOrig[256], commentNew[256];
		Boolean keepGoing;
		ModalFilterUPP	filterUPP;

		filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP==NULL) {
			MissingDialog(SCOREINFO_DLOG);
			return;
		}
		 
		s = (char *)NewPtr(256);
		if (!GoodNewPtr((Ptr)s))
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
		sprintf(s, fmtStr);
		SIDrawLine(s);
		if (doc) {
			GetIndCString(fmtStr, SCOREINFO_STRS, 2);   		/* "    %d pages, %d systems, %d measures." */
			sprintf(s, fmtStr, doc->numSheets,
							doc->nsystems, MeasCount(doc));
			SIDrawLine(s);
			GetIndCString(fmtStr, SCOREINFO_STRS, 3);   		/* "        A system contains %d staves." */
			sprintf(s, fmtStr, doc->nstaves);
			SIDrawLine(s);
	
			CountInHeaps(doc, objCount, FALSE);

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
		 			case OCTAVAtype:
		 			case TUPLETtype:
		 			case TEMPOtype:
		 			case SPACEtype:
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
		 			sprintf(s, "    %ld %s", lObjCount[h], ps);
					SIDrawLine(s);
					totalCount += lObjCount[h];
				}
		 	}

			objsTotal = 0;	
			for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
				objsTotal++;
			objsTotal++;
			GetIndCString(fmtStr, SCOREINFO_STRS, 4);   		/* "    %ld total symbols (%d objects)." */
			sprintf(s, fmtStr, totalCount, objsTotal);
			SIDrawLine(s);

			scoreDuration = GetScoreDuration(doc);
			qtrNTicks = Code2LDur(QTR_L_DUR, 0);
			GetIndCString(fmtStr, SCOREINFO_STRS, 8);   		/* "    Duration: approx. %ld quarters." */
			sprintf(s, fmtStr, scoreDuration/qtrNTicks); 
			SIDrawLine(s);

			if (!ShiftKeyDown()) {
				WaitCursor();
				nBad = SICheckMeasDur(doc, &firstBad);
				if (nBad) {
					GetIndCString(fmtStr, SCOREINFO_STRS, 5);   	/* "    Duration problems in %d meas. (first=%d)." */
			 		sprintf(s, fmtStr, nBad, firstBad);
				}
				else {
					GetIndCString(fmtStr, SCOREINFO_STRS, 6);   	/* "    No measures have duration problems." */
			 		sprintf(s, fmtStr);
			 	}
				SIDrawLine(s);
				ArrowCursor();
			}
			sprintf(s, fmtStr, scoreDuration/qtrNTicks); 
			SIDrawLine(s);
			
			if (HasMidiMap(doc)) {
				Str255 fName;
				FSSpec *fsSpec = (FSSpec *)*doc->midiMapFSSpecHdl;
				Pstrcpy(fName, fsSpec->name);
				sprintf(s, "MidiMap %s",PtoCstr(fName));
				SIDrawLine(s);
			}
			else {
				sprintf(s, "%s", "No MidiMap");
				SIDrawLine(s);
			}
			
			strcpy(commentOrig, (char *)doc->comment);
			CToPString(strcpy(commentNew, (char *)commentOrig));
			PutDlgString(dialogp, COMMENT_DI, (unsigned char *)commentNew, TRUE);
		}
		else {
			GetIndCString(fmtStr, SCOREINFO_STRS, 7);   			/* "    No score is open." */
			sprintf(s, fmtStr);
			SIDrawLine(s);
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
		
		DisposePtr((Ptr)s);
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dialogp);										/* Free heap space */
		SetPort(oldPort);
	}
