/******************************************************************************************
*	FILE:	TupletEdit.c
*	PROJ:	Nightingale
*	DESC:	Dialog-handling routines for creating and editing tuplets
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017-2022 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* ------------------------------------ Declarations & Help Functions for TupletDialog -- */

extern short minDlogVal, maxDlogVal;

enum {
	SET_DUR_DI=4,										/* Item numbers */
	TUPLE_NUM,
	TUPLE_PTEXT,
	ED_TUPLE_DENOM,
	BOTH_VIS,
	NUM_VIS,
	NEITHER_VIS,
	BRACK_VIS,
	SAMPLE_DI,
	STAT_TUPLE_DENOM,
	TDUMMY_DI
};

#define BREVE_DUR		3840
#define WHOLE_DUR		1920
#define HALF_DUR		960
#define QUARTER_DUR		480
#define EIGHTH_DUR		240
#define SIXTEENTH_DUR	120
#define THIRTY2ND_DUR	60
#define SIXTY4TH_DUR	30
#define ONE28TH_DUR		15
#define NO_DUR			0

static short choiceIdx;

short durUnitCode;						/* Used to index duration strings. */
short tupleDenomItem;

/* Following declarations are basically a TupleParam but with ints for num/denom. */

short accNum, accDenom;
short durUnit;
Boolean numVis, denomVis, brackVis;

/* Local prototypes for TupletDialog. */

static pascal Boolean TupleFilter(DialogPtr, EventRecord *, short *);
static void DrawTupletItems(DialogPtr, short);

static pascal Boolean TupleFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr		w;
	short			ch, field, type, byChLeftPos;
	Handle			hndl;
	Rect			box;
	Point			where;
	GrafPtr			oldPort;
	short			anInt;
	Handle			aHdl;
	Rect			tdRect;
	Boolean			denomItemVisible;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort);  SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));			
				UpdateDialogVisRgn(dlog);
				DrawTupletItems(dlog, SAMPLE_DI);
				FrameDefault(dlog, OK, True);
				GetDialogItem(dlog, SET_DUR_DI, &type, &hndl, &box);
//LogPrintf(LOG_DEBUG, "TupleFilter: box tlbr=%d,%d,%d,%d\n", box.top, box.left, box.bottom,
//box.right);
				byChLeftPos = choiceIdx*(DP_COL_WIDTH/8);
				DrawBMPChar(bmpDurationPal.bitmap, DP_COL_WIDTH/8,
						bmpDurationPal.byWidthPadded, DP_ROW_HEIGHT, byChLeftPos,
						3*DP_ROW_HEIGHT, box);
				FrameShadowRect(&box);
				HiliteDurCell(choiceIdx, &box, durPalCell);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return True;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog))
				SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			GetDialogItem(dlog, SET_DUR_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
				if (evt->what==mouseUp) {
#ifdef NOTYET
					/* If the mouseUp was in an invalid (presumably because empty) cell,
					   ignore it. Otherwise, unhilite the previously-selected cell and
					   hilite the new one. */

					short newDurIdx = FindDurationCell(where, &box);
					if (newDurIdx<0 || newDurIdx>(short)NGDDURATIONS-1) return False;
					SWITCH_CELL(choiceIdx, newDurIdx, &box);
#endif
				}
			*itemHit = SET_DUR_DI;
			return True;
		}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False))
				return True;
			ch = (unsigned char)evt->message;
			field = GetDialogKeyboardFocusItem(dlog);
			
			/* The Dialog Manager considers only EditText fields as candidates for being
			   activated by the tab key, so handle tabbing from field to field ourselves
			   so that user can direct keystrokes to the pop-up as well as the EditText
			   fields. But skip this if ED_TUPLE_DENOM isn't visible. */
			   
			GetDialogItem(dlog, ED_TUPLE_DENOM, &anInt, &aHdl, &tdRect);
			denomItemVisible = (tdRect.left<8192);
			if (ch=='\t') {
				if (denomItemVisible) {
					field = field==ED_TUPLE_DENOM? TDUMMY_DI : ED_TUPLE_DENOM;
#if 11
#else
					popUpHilited = field==ED_TUPLE_DENOM? False : True;
					SelectDialogItemText(dlog, field, 0, ENDTEXT);
					HiliteGPopUp(curPop, popUpHilited);
#endif
					*itemHit = 0;
					return True;
				}
			}
			else {
				if (field==TDUMMY_DI || !denomItemVisible) {
#if 11
#else
					ans = DurPopupKey(curPop, popKeys0dot, ch);
					*itemHit = ans? TPOPUP_DI : 0;
					HiliteGPopUp(curPop, True);
#endif
					return True;			/* so no chars get through to TDUMMY_DI edit field */
				}							/* NB: however, DlgCmdKey will let you paste into TDUMMY_DI! */
			}
			break;
	}
	return False;
}


static void DrawTupletItems(DialogPtr dlog, short /*ditem*/)
{
	short		xp, yp, itype, nchars, tupleLen, xColon;
	short		oldTxFont, oldTxSize, tupleWidth;
	Handle		tHdl;
	Rect		userRect;
	unsigned	char tupleStr[30],denomStr[20];
	DPoint		firstPt, lastPt;
	Document 	*doc=GetDocumentFromWindow(TopDocument);
	
	if (doc==NULL) return;

	GetDialogItem(dlog, SAMPLE_DI, &itype, &tHdl, &userRect);
	EraseRect(&userRect);
	xp = userRect.left + (userRect.right-userRect.left)/2;
	yp = userRect.top + (userRect.bottom-userRect.top)/2;
	
	if (numVis) {
		oldTxFont = GetPortTxFont();
		oldTxSize = GetPortTxSize();
		TextFont(sonataFontNum);					/* Set to Sonata font */
		TextSize(Staff2MFontSize(drSize[1]));		/* Nice and big for readability */	

		NumToSonataStr((long)accNum, tupleStr);
		tupleWidth = StringWidth(tupleStr);
		xColon = tupleWidth;
		if (denomVis) {
			/* Since Sonata has no ':', leave space and we'll fake it */
			
			tupleLen = tupleStr[0]+1;
			tupleStr[tupleLen] = ' ';

			/* Append the denominator string. */
			
			NumToSonataStr((long)accDenom, denomStr);
			nchars = denomStr[0];
			tupleStr[0] = nchars+tupleLen;
			while (nchars>0) {
				tupleStr[nchars+tupleLen] = denomStr[nchars];
				nchars--;
			}
		}
		tupleWidth = StringWidth(tupleStr);
		MoveTo(xp-tupleWidth/2, yp+3);
		DrawString(tupleStr);
		if (denomVis) {
			MoveTo(xp-tupleWidth/2+xColon, yp+2);
			DrawMColon(doc, True, False, 0);		/* 0 for last arg immaterial for Sonata */
		}

		SetDPt(&firstPt, p2d(userRect.left+10), p2d(yp));
		SetDPt(&lastPt, p2d(userRect.right-10), p2d(yp));
		if (brackVis)
			DrawTupletBracket(firstPt, lastPt, 0, p2d(4), p2d(xp), tupleWidth, False,
										False, NULL, False);
		TextFont(oldTxFont);
		TextSize(oldTxSize);
	}
	
	else {
		SetDPt(&firstPt, p2d(userRect.left+10), p2d(yp));
		SetDPt(&lastPt, p2d(userRect.right-10), p2d(yp));
		if (brackVis)
			DrawTupletBracket(firstPt, lastPt, 0, p2d(4), p2d(xp), -1, False,
										False, NULL, False);
	}
}

/* ---------------------------------------------------------------------- TupletDialog -- */
/* Conduct the "Fancy Tuplet" dialog for a new or existing tuplet whose initial state is
described by <ptParam>. Return False on Cancel or error, True on OK. */

Boolean TupletDialog(
					Document */*doc*/,
					TupleParam *ptParam,
					Boolean tupletNew)	/* True=tuplet about to be created, False=already exists */
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	short		ditem, type, i, logDenom, tempNum, maxChange, evenNum, radio;
	short		oldResFile;
	short		newLDurCode, lDurCode, deltaLDur, nDotsDummy;
	Rect		staffRect;
	Handle		tHdl, hndl;
	Rect		box;
	ModalFilterUPP filterUPP;
		
	filterUPP = NewModalFilterUPP(TupleFilter);
	if (filterUPP==NULL) {
		MissingDialog(FANCYTUPLET_DLOG);
		return False;
	}
	
	dlog = GetNewDialog(FANCYTUPLET_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(FANCYTUPLET_DLOG);
		return False;
	}
	
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));
	CenterWindow(GetDialogWindow(dlog), 50);

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);								/* popup code uses Get1Resource */

	accNum = ptParam->accNum;
	accDenom = ptParam->accDenom;
	durUnit = ptParam->durUnit;
	numVis = ptParam->numVis;
	denomVis = ptParam->denomVis;
	brackVis = ptParam->brackVis;

	switch (durUnit) {
		case BREVE_DUR:
			durUnitCode = BREVE_L_DUR;
			break;
		case WHOLE_DUR:
			durUnitCode = WHOLE_L_DUR;
			break;
		case HALF_DUR:
			durUnitCode = HALF_L_DUR;
			break;
		case QUARTER_DUR:
			durUnitCode = QTR_L_DUR;
			break;
		case EIGHTH_DUR:
			durUnitCode = EIGHTH_L_DUR;
			break;
		case SIXTEENTH_DUR:
			durUnitCode = SIXTEENTH_L_DUR;
			break;
		case THIRTY2ND_DUR:
			durUnitCode = THIRTY2ND_L_DUR;
			break;
		case SIXTY4TH_DUR:
			durUnitCode = SIXTY4TH_L_DUR;
			break;
		case ONE28TH_DUR:
			durUnitCode = ONE28TH_L_DUR;
			break;
		default:
			durUnitCode = NO_L_DUR;
			break;
		}

	GetDialogItem(dlog, SET_DUR_DI, &type, &hndl, &box);
	InitDurationCells(&box, DP_NCOLS, DP_NROWS_SD, durPalCell);
	
	choiceIdx = DurCodeToDurPalIdx(durUnitCode, 0, DP_NCOLS);
	if (choiceIdx<0) {
		SysBeep(1);
		LogPrintf(LOG_WARNING, "Illegal code %d for tuplet duration unit.  (TupletDialog)\n",
					durUnitCode);
		choiceIdx = 3;									/* An arbitrary choice */
	}
	lDurCode = durUnitCode;

	tupleDenomItem = (tupletNew? ED_TUPLE_DENOM : STAT_TUPLE_DENOM);
	ShowDialogItem(dlog, (tupletNew? ED_TUPLE_DENOM : STAT_TUPLE_DENOM));
	HideDialogItem(dlog, (tupletNew? STAT_TUPLE_DENOM : ED_TUPLE_DENOM));
	PutDlgWord(dlog, TUPLE_NUM, accNum, False);
	PutDlgWord(dlog, tupleDenomItem, accDenom, tupletNew);

	/* logDenom is the log2 of the accessory denominator; (durUnitCode-logDenom+1) is the
	   max. duration at which the denominator is at least 2. evenNum is the number of
	   times the numerator can be divided by 2 exactly. The minimum of these two can be
	   subtracted from durUnitCode to give the maximum duration note allowable. */
	   
	for (logDenom=0, i=1; i<accDenom; i*=2) logDenom++;
	tempNum = accNum;
	for (evenNum=0; !odd(tempNum); tempNum /= 2) evenNum++;
	maxChange = n_min(logDenom-1, evenNum);
	minDlogVal = durUnitCode-maxChange;
	
	GetDialogItem(dlog, SAMPLE_DI, &type, &tHdl, &staffRect);		/* Sample is a user Item */

	if (denomVis) radio = BOTH_VIS;
	else if (numVis) radio = NUM_VIS;
	else radio = NEITHER_VIS;
	PutDlgChkRadio(dlog,BOTH_VIS,radio==BOTH_VIS);
	PutDlgChkRadio(dlog,NUM_VIS,radio==NUM_VIS);
	PutDlgChkRadio(dlog,NEITHER_VIS,radio==NEITHER_VIS);
	PutDlgChkRadio(dlog,BRACK_VIS,brackVis);

#if 11
#else
	if (popUpHilited)
		SelectDialogItemText(dlog, TDUMMY_DI, 0, ENDTEXT);
	else
		SelectDialogItemText(dlog, TPOPUP_DI, 0, ENDTEXT);
#endif

	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	while (True) {
		ModalDialog(filterUPP, &ditem);
		if (ditem==OK) {
			if (accNum<1 || accNum>255 || accDenom<1 || accDenom>255) {
				GetIndCString(strBuf, DIALOGERRS_STRS, 12);			/* "Numbers must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
			}
			else
				break;
		}
		if (ditem==Cancel) break;
		
		switch (ditem) {
			case TUPLE_NUM:
			case ED_TUPLE_DENOM:
			case STAT_TUPLE_DENOM:
				GetDlgWord(dlog, TUPLE_NUM, &accNum);
				GetDlgWord(dlog, tupleDenomItem, &accDenom);
				break;
			case SET_DUR_DI:
#if 11
				choiceIdx = DurPalChoiceDlog(choiceIdx, 0);
				newLDurCode = DurPalIdxToDurCode(choiceIdx, &nDotsDummy);
#else
				newLDurCode = popKeys0dot[curPop->currentChoice].durCode;
#endif
				
	 			/* If user set the duration unit to something longer than the maximum
				   allowable, change it to the maximum. */
				   
				if (newLDurCode<minDlogVal) {
					SysBeep(1);
					LogPrintf(LOG_WARNING, "Selected duration unit is too long. (TupletDialog)\n");					
					newLDurCode = minDlogVal;
				}
#if 11
//LogPrintf(LOG_DEBUG, "TupletDialog: lDurCode=%d minDlogVal=%d newLDurCode=%d\n",
//lDurCode, minDlogVal, newLDurCode);  
				if (newLDurCode!=lDurCode) {
					choiceIdx = DurCodeToDurPalIdx(newLDurCode, 0, DP_NCOLS);
LogPrintf(LOG_DEBUG, "TupletDialog: lDurCode=%d newLDurCode=%d choiceIdx=%d\n",
lDurCode, newLDurCode, choiceIdx);  
					short byChLeftPos = choiceIdx*(DP_COL_WIDTH/8);
					EraseRect(&box);
					FrameRect(&box);
					DrawBMPChar(bmpDurationPal.bitmap, DP_COL_WIDTH/8,
						bmpDurationPal.byWidthPadded, DP_ROW_HEIGHT, byChLeftPos,
						3*DP_ROW_HEIGHT, box);
				}
#else
				choice = GetDurPopItem(curPop, popKeys0dot, newLDurCode, 0);
				if (choice==NOMATCH) choice = 1;
				SetGPopUpChoice(curPop, choice);
#endif



				deltaLDur = newLDurCode-lDurCode;
				if (deltaLDur>0)
					for (i= 1; i<=ABS(deltaLDur); i++) {
						accNum *= 2; accDenom *= 2;
					}
				else
					for (i= 1; i<=ABS(deltaLDur); i++) {
						accNum /= 2; accDenom /= 2;
					}
				PutDlgWord(dlog,TUPLE_NUM,accNum,False);
				PutDlgWord(dlog,tupleDenomItem,accDenom,tupletNew);
				lDurCode = newLDurCode;
				SelectDialogItemText(dlog, TDUMMY_DI, 0, ENDTEXT);
//				HiliteGPopUp(curPop, popUpHilited = True);
				break;
			case BOTH_VIS:
			case NUM_VIS:
			case NEITHER_VIS:
				if (ditem!=radio) SwitchRadio(dlog, &radio, ditem);
				numVis = (ditem==BOTH_VIS || ditem==NUM_VIS);
				denomVis = ditem == BOTH_VIS;
		   	break;
		   case BRACK_VIS:
		   	PutDlgChkRadio(dlog,ditem,brackVis = !brackVis);
		   	break;
			}
		DrawTupletItems(dlog, SAMPLE_DI);
	}
	
	if (ditem==OK) {
		ptParam->accNum = accNum;
		ptParam->accDenom = accDenom;
		ptParam->durUnit = durUnit;
		ptParam->numVis = numVis;
		ptParam->denomVis = denomVis;
		ptParam->brackVis = brackVis;
	}

broken:			
#if 11
#else
	DisposeGPopUp(&durPop0dot);
	if (popKeys0dot) DisposePtr((Ptr)popKeys0dot);
#endif
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}

