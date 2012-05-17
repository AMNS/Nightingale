/***************************************************************************
*	FILE:	InfoDialog.c																		*
*	PROJ:	Nightingale, rev. for v.2000													*
*	DESC:	Handling routines for "Get Info" and "Modifier Info" dialogs		*
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

/* File InfoDialog.c - "Get Info" dialog routines:
	InfoDialog				LegalStaff/Vert
	SyncInfoDialog			GenInfoDialog				ExtendInfoDialog
	ModNRDialog and friends
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean LegalStaff(Document *, INT16, Boolean);
static Boolean LegalVert(Document *, INT16);
static void SyncInfoDialog(Document *, LINK, char []);
static Boolean PageRelGraphic(LINK);
static void GenInfoDialog(Document *, LINK, char []);
static void ExtendInfoDialog(Document *, LINK, char []);

#define DD2I(num) (((num)+scaleRound)>>scaleShift)			/* Convert DDIST to Get Info */
#define I2DD(num)	((num)<<scaleShift)							/* Convert Get Info to DDIST */

/* Macros for changing fields in the data structure: DIST_ACCEPT for DDIST distances;
GRAF_ACCEPT for other possibly-graphics-related fields; GEN_ACCEPT for all others. */

#define DIST_ACCEPT(field)	if (newval!=DD2I(field)) \
									{ (field) = I2DD(newval);  graphicDirty = TRUE; }
#define GRAF_ACCEPT(field)	if (newval!=(field)) \
									{ (field) = newval;  graphicDirty = TRUE; }
#define GEN_ACCEPT(field)	if (newval!=(field)) \
									{ (field) = newval;  nodeDirty = TRUE; }

static INT16	scaleShift, scaleRound;
static INT16	lastEditField = 0, lastObjType = 0;

/* Note: these dialogs are intended for use mostly by expert users. Accordingly, error
checking is definitely on the permissive side! */


/* ------------------------------------------------------------------- InfoDialog -- */

enum {
	DDIST_UNITS=0,
	QTRPOINT_UNITS,
	POINT_UNITS
};

void InfoDialog(Document *doc)
{
	short units; char unitLabel[256]; LINK pL;
	
	units = config.infoDistUnits;
	
	if (CapsLockKeyDown() && ShiftKeyDown())
		units = DDIST_UNITS;

	if (units==POINT_UNITS) { scaleShift = 4; scaleRound = 0; }
	else if (units==QTRPOINT_UNITS) { scaleShift = 2; scaleRound = 0; }
	else { scaleShift = 0; scaleRound = 0; }
	
	GetIndCString(unitLabel, INFOUNITS_STRS, units+1);
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL)) break;
		
	switch (ObjLType(pL)) {
		case SYNCtype:
		case GRSYNCtype:
			SyncInfoDialog(doc, pL, unitLabel);
			break;
		case OCTAVAtype:
		case TUPLETtype:
			ExtendInfoDialog(doc, pL, unitLabel);
			break;
		case GRAPHICtype:
			if (GraphicTYPE(pL) && GraphicSubType(pL)==GRDraw)
				ExtendInfoDialog(doc, pL, unitLabel);
			else
				GenInfoDialog(doc, pL, unitLabel);
			break;
		default:
			GenInfoDialog(doc, pL, unitLabel);
	}
}


/* -------------------------------------------------------------- LegalStaff/Vert -- */

/* As of v. 3.1, LegalStaff is unused, since staff no. is always read only. */

static Boolean LegalStaff(Document *doc, INT16 staff, Boolean noStaffOK)
{
	char fmtStr[256];

	if ((staff<1 || staff>doc->nstaves) && !(noStaffOK && staff==NOONE))
	{
		GetIndCString(fmtStr, INFOERRS_STRS, 1);			/* "Staff number must be 1 to..." */
		sprintf(strBuf, fmtStr, doc->nstaves);
		CParamText(strBuf, "", "", "");
		Inform(GINFO_ALRT);
		return FALSE;
	}
	else
		return TRUE;
}


static Boolean LegalVert(Document *doc, INT16 newval)
{
	if (I2DD(newval)<ABOVE_VLIM(doc) || I2DD(newval)>BELOW_VLIM(doc))
	{
		GetIndCString(strBuf, INFOERRS_STRS, 2);			/* "Vertical position out of bounds" */
		CParamText(strBuf, "", "", "");
		Inform(GINFO_ALRT);
		return FALSE;
	}
	else
		return TRUE;
}


#define INFO_ERROR(str) { CParamText((str), "", "", ""); Inform(GINFO_ALRT); dialogOver = 0; }

/* --------------------------------------------------------------- SyncInfoDialog -- */
/* Handle Get Info dialog for SYNCs and GRSYNCs. */

static enum										/* Dialog item numbers */
{
	TYPE=4,
	TWEAKED,
	LBL_OBJ_HORIZ,
	OBJ_HORIZ,
	LBL_SUBOBJ_HORIZ,
	SUBOBJ_HORIZ,
	LBL_VERT,
	VERT,
	STAFF_NO=13,
	VOICE_NO=15,

	ACCIDENT=17,
	XMOVE_ACC=20,
	COURTESY_ACC=22,
	L_DUR=24,
	NDOTS=26,
	XMOVE_DOTS=29,
	YMOVE_DOTS=31,
	STEM_VERT=33,
	HEADSTEM_SIDE=35,
	HEAD_SHAPE=37,
	SMALLNR=39,
	DOUBLEDUR=41,

	P_TIME=43,
	P_DUR=45,
	NOTENUM=47,
	ON_VELOCITY=49,
	OFF_VELOCITY=51,
	UPDATE,
	UNITLABEL_SYNC,
	START_TIME=55
} E_SyncInfoItems;

static void SyncInfoDialog(Document *doc, LINK pL, char unitLabel[])
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	INT16			dialogOver;
	short			ditem, aShort;
	INT16			newval, newval2, max_dots, staff,
					minl_dur, userVoice;
	Handle		twHdl, ulHdl;
	Rect			tRect;
	PANOTE		aNote;
	PAGRNOTE		aGRNote;
	LINK			aNoteL, aGRNoteL, partL;
	Boolean		graphicDirty,								/* Anything graphic changed? */
					nodeDirty;										/* Anything non-graphic changed? */
	Boolean		hasStem;
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButDragFilter);
	if (filterUPP == NULL) {
		MissingDialog(SYNCINFO_DLOG);
		return;
	}
				
	GetPort(&oldPort);
	ArrowCursor();
	graphicDirty = nodeDirty = FALSE;									/* Nothing changed yet */

/* --- 1. Create the dialog and initialize its contents. --- */

	PrepareUndo(doc, pL, U_GetInfo, 12);    							/* "Undo Get Info" */

	dlog = GetNewDialog(SYNCINFO_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SYNCINFO_DLOG);
		return;
	}
	CenterWindow(GetDialogWindow(dlog), 194);
	SetPort(GetDialogWindowPort(dlog));
	
	SetDlgFont(dlog, textFontNum, textFontSmallSize, 0);

	PushLock(NOTEheap);
	PushLock(GRNOTEheap);
	
	PutDlgWord(dlog, OBJ_HORIZ, DD2I(LinkXD(pL)), FALSE);
	GetDialogItem(dlog, TWEAKED, &aShort, &twHdl, &tRect);
	SetDialogItemCText(twHdl, (char *)(LinkTWEAKED(pL)? "T" : " "));
	GetDialogItem(dlog, UNITLABEL_SYNC, &aShort, &ulHdl, &tRect);
	SetDialogItemCText(ulHdl, unitLabel);

	switch (ObjLType(pL))													/* Initialize by type... */
	{
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL)) break;
	
			aNote = GetPANOTE(aNoteL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aNote->xd), FALSE);
			PutDlgWord(dlog, STAFF_NO, aNote->staffn, FALSE);
			Int2UserVoice(doc, aNote->voice, &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, FALSE);
			PutDlgWord(dlog, VERT, DD2I(aNote->yd), FALSE);
			PutDlgWord(dlog, STEM_VERT, DD2I(aNote->ystem), FALSE);
			PutDlgWord(dlog, P_TIME, aNote->playTimeDelta, FALSE);
			PutDlgWord(dlog, P_DUR, aNote->playDur, FALSE);
			PutDlgWord(dlog, L_DUR, aNote->subType, FALSE);			/* logical dur. */
			PutDlgWord(dlog, NDOTS, aNote->ndots, FALSE);
			PutDlgWord(dlog, XMOVE_DOTS, aNote->xmovedots, FALSE);
			PutDlgWord(dlog, YMOVE_DOTS, aNote->ymovedots, FALSE);
			PutDlgWord(dlog, HEAD_SHAPE, aNote->headShape, FALSE);
			PutDlgLong(dlog, START_TIME, SyncAbsTime(pL), FALSE);
			
			hasStem = (aNote->ystem!=aNote->yd);
			
			strcpy(strBuf, NameNodeType(pL));
			if (aNote->rest)
			{
				strcat(strBuf, " Rest");
				PutDlgString(dlog, TYPE, CToPString(strBuf), FALSE);
				PutDlgWord(dlog, SMALLNR, aNote->small, FALSE);		/* whether small note/rest */
			}
			else
			{
				strcat(strBuf, " Note");
				PutDlgString(dlog, TYPE, CToPString(strBuf), FALSE);
				PutDlgWord(dlog, ACCIDENT, aNote->accident, FALSE);
				PutDlgWord(dlog, XMOVE_ACC, aNote->xmoveAcc, FALSE);
				PutDlgWord(dlog, COURTESY_ACC, aNote->courtesyAcc, FALSE);
				PutDlgWord(dlog, HEADSTEM_SIDE, aNote->otherStemSide, FALSE);
				PutDlgWord(dlog, DOUBLEDUR, aNote->doubleDur, FALSE);
				PutDlgWord(dlog, NOTENUM, aNote->noteNum, FALSE);
				PutDlgWord(dlog, ON_VELOCITY, aNote->onVelocity, FALSE);
				PutDlgWord(dlog, OFF_VELOCITY, aNote->offVelocity, FALSE);
				PutDlgWord(dlog, SMALLNR, aNote->small, FALSE);		/* whether small note/rest */
			}
			break;
			
		case GRSYNCtype:
			strcpy(strBuf, NameNodeType(pL));
			PutDlgString(dlog, TYPE, CToPString(strBuf), FALSE);
			
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
				if (GRNoteSEL(aGRNoteL)) break;
	
			aGRNote = GetPAGRNOTE(aGRNoteL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aGRNote->xd), FALSE);
			PutDlgWord(dlog, STAFF_NO, aGRNote->staffn, FALSE);
			Int2UserVoice(doc, aGRNote->voice, &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, FALSE);
			PutDlgWord(dlog, VERT, DD2I(aGRNote->yd), FALSE);
			PutDlgWord(dlog, L_DUR, aGRNote->subType, FALSE);
			PutDlgWord(dlog, HEAD_SHAPE, aGRNote->headShape, FALSE);
			PutDlgWord(dlog, ACCIDENT, aGRNote->accident, FALSE);
			PutDlgWord(dlog, XMOVE_ACC, aGRNote->xmoveAcc, FALSE);
			PutDlgWord(dlog, COURTESY_ACC, aGRNote->courtesyAcc, FALSE);
			PutDlgWord(dlog, HEADSTEM_SIDE, aGRNote->otherStemSide, FALSE);
			PutDlgWord(dlog, NOTENUM, aGRNote->noteNum, FALSE);
			PutDlgWord(dlog, STEM_VERT, DD2I(aGRNote->ystem), FALSE);
			PutDlgWord(dlog, ON_VELOCITY, aGRNote->onVelocity, FALSE);
			PutDlgWord(dlog, OFF_VELOCITY, aGRNote->offVelocity, FALSE);
			PutDlgWord(dlog, SMALLNR, aGRNote->small, FALSE);	/* whether small grace note */

			hasStem = (aGRNote->ystem!=aGRNote->yd);

			break;
			
		 default:
		  	break;
	}

	if (lastEditField!=0 && (lastObjType==SYNCtype || lastObjType==GRSYNCtype))
		SelectDialogItemText(dlog, lastEditField, 0, ENDTEXT);		/* Initial selection & hiliting */
	else
		SelectDialogItemText(dlog, OBJ_HORIZ, 0, ENDTEXT);

	ShowWindow(GetDialogWindow(dlog));
	OutlineOKButton(dlog,TRUE);

/*--- 2. Interact with user til they push OK or Cancel. --- */

	dialogOver = 0;
	while (dialogOver==0)
	{
		do
		{
			ModalDialog(filterUPP, &ditem);						/* Handle events in dialog window */
			switch (ditem) {
				case OK:
				case Cancel:
					dialogOver = ditem;
					break;
#ifdef NOTYET
				case UPDATE:
					SetPort(doc->theWindow);
					measureL = LSSearch(pL, MEASUREtype, ANYONE, TRUE, FALSE);
					pMeasure = GetPMEASURE(measureL);
					r = pMeasure->measureBBox;
					if (LastMeasInSys(measureL)) r.right = doc->paperRect.right;
					OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
					EraseAndInval(&r);
					/*
					DrawRange(doc, measureL, pMeasure->rMeasure,&doc->viewRect);
					*/
					MayErrMsg("SyncInfoDialog: DrawRange expects the paper bounds\r");
					SetPort(GetDialogWindowPort(dlog));
					break;
#endif
			 	default:
			 		;
			}
		} while (!dialogOver);
	
	/* --- 3. If dialog was terminated with OK, check any new values. ----- */
	/* ---    If any are illegal, keep dialog on the screen to try again. - */
	
		if (dialogOver==Cancel)
		{
			PopLock(NOTEheap);
			PopLock(GRNOTEheap);
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);									/* Free heap space */
			SetPort(oldPort);
			return;
		}
		else
		{
			GetDlgWord(dlog, OBJ_HORIZ, &newval);
			if (I2DD(newval)<0 || I2DD(newval)>RIGHT_HLIM(doc))
			{
				GetIndCString(fmtStr, INFOERRS_STRS, 7);		/* "Horizontal position must be..." */
				sprintf(strBuf, fmtStr, DD2I(RIGHT_HLIM(doc)));
				INFO_ERROR(strBuf);
			}
	
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			if (newval<-1999 || newval>1999) {					/* ??MAGIC NOS!! */
				GetIndCString(fmtStr, INFOERRS_STRS, 8);			/* "Note/rest horiz pos must be..." */
				sprintf(strBuf, fmtStr, -1999, 1999);
				INFO_ERROR(strBuf);
			}
			
			GetDlgWord(dlog, VERT, &newval);
			if (!LegalVert(doc, newval)) dialogOver = 0;

			GetDlgWord(dlog, L_DUR, &newval);
			minl_dur = (aNote->rest ? -127 : UNKNOWN_L_DUR);
			if (newval<minl_dur || newval>MAX_L_DUR) {
				GetIndCString(fmtStr, INFOERRS_STRS, 9);		/* "CMN duration must be..." */
				sprintf(strBuf, fmtStr, minl_dur, MAX_L_DUR);
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, HEAD_SHAPE, &newval);
			if (newval<0 || newval>31) {
				GetIndCString(strBuf, INFOERRS_STRS, 10);		/* "Head shape must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, SMALLNR, &newval);
			if (newval<0 || newval>1) {
				GetIndCString(strBuf, INFOERRS_STRS, 11);		/* "Small note/rest value must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, VERT, &newval);
			GetDlgWord(dlog, STEM_VERT, &newval2);
			if (hasStem!=(newval!=newval2)) {
				GetIndCString(strBuf, INFOERRS_STRS, 12);		/* "can't add or remove stem" */
				INFO_ERROR(strBuf);
			}

			if (SyncTYPE(pL)) {
				GetDlgWord(dlog, P_TIME, &newval);
				if (newval<0 || newval>32000) {						/* ??MAGIC NOS!! */
					GetIndCString(fmtStr, INFOERRS_STRS, 13);		/* "Play time must be..." */
					sprintf(strBuf, fmtStr, 0, 32000);
					INFO_ERROR(strBuf);
				}

				GetDlgWord(dlog, P_DUR, &newval);
				if (newval<1 || newval>32000) {						/* ??MAGIC NOS!! */
					GetIndCString(fmtStr, INFOERRS_STRS, 14);		/* "Play duration must be..." */
					sprintf(strBuf, fmtStr, 1, 32000);
					INFO_ERROR(strBuf);
				}

				GetDlgWord(dlog, NDOTS, &newval);
				aNote = GetPANOTE(aNoteL);
				max_dots = MAX_L_DUR-aNote->subType;
				if (newval<0 || newval>max_dots) {
					GetIndCString(fmtStr, INFOERRS_STRS, 15);		/* "Number of dots must be..." */
					sprintf(strBuf, fmtStr, max_dots);
					INFO_ERROR(strBuf);
				}
	
				GetDlgWord(dlog, XMOVE_DOTS, &newval);
				if (newval<0 || newval>7) {
					GetIndCString(strBuf, INFOERRS_STRS, 16);		/* "Aug dot H offset must be..." */
					INFO_ERROR(strBuf);
				}
	
				GetDlgWord(dlog, YMOVE_DOTS, &newval);
				if (newval<0 || newval>3) {
					GetIndCString(strBuf, INFOERRS_STRS, 17);		/* "Aug dot V offset must be..." */
					INFO_ERROR(strBuf);
				}

				if (!NoteREST(aNoteL)) {
					GetDlgWord(dlog, DOUBLEDUR, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 18);		/* "Dbl duration val must be..." */
						INFO_ERROR(strBuf);
					}
				}
			}

/* The remaining "note" parameters don't apply to rests. */
			if (NoteREST(aNoteL)) continue;
			
			GetDlgWord(dlog, ACCIDENT, &newval);
			if (newval<0 || newval>5) {
				GetIndCString(strBuf, INFOERRS_STRS, 19);			/* "Accidental must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, XMOVE_ACC, &newval);
			if (newval<0 || newval>31) {
				GetIndCString(strBuf, INFOERRS_STRS, 20);			/* "Accidental H offset must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, COURTESY_ACC, &newval);
			if (newval<0 || newval>1) {
				GetIndCString(strBuf, INFOERRS_STRS, 46);			/* "Courtesy accidental must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, HEADSTEM_SIDE, &newval);
			if (newval<0 || newval>1) {
				GetIndCString(strBuf, INFOERRS_STRS, 21);			/* "Notehead's stem side must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, NOTENUM, &newval);
			if (newval<0 || newval>127) {								/* MAX_NOTENUM */
				GetIndCString(strBuf, INFOERRS_STRS, 22);			/* "MIDI note number must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, STEM_VERT, &newval);
			if (newval<ABOVE_VLIM(doc) || newval>BELOW_VLIM(doc)) {
				GetIndCString(strBuf, INFOERRS_STRS, 23);			/* "Stem position out of bounds" */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, ON_VELOCITY, &newval);
			if (newval<0 || newval>127) {								/* MAX_VELOCITY */
				GetIndCString(strBuf, INFOERRS_STRS, 24);			/* "MIDI On velocity must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, OFF_VELOCITY, &newval);
			if (newval<0 || newval>127) {
				GetIndCString(strBuf, INFOERRS_STRS, 25);			/* "MIDI Off velocity must be..." */
				INFO_ERROR(strBuf);
			}

		}
	}
	
/* --- 4. Dialog was terminated with OK and all values are legal. --- */

	GetDlgWord(dlog, OBJ_HORIZ, &newval);
	DIST_ACCEPT(LinkXD(pL));

	staff = 1;
	switch (ObjLType(pL))										/* Get new values by type */
	{
		case SYNCtype:
			aNote = GetPANOTE(aNoteL);
	
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			DIST_ACCEPT(aNote->xd);
	
			GetDlgWord(dlog, VERT, &newval);
			DIST_ACCEPT(aNote->yd);
	
			GetDlgWord(dlog, STEM_VERT, &newval);
			DIST_ACCEPT(aNote->ystem);
	
			GetDlgWord(dlog, P_TIME, &newval);
			GRAF_ACCEPT(aNote->playTimeDelta);					/* Will affect graphics if pianoroll view */
	
			GetDlgWord(dlog, P_DUR, &newval);
			GRAF_ACCEPT(aNote->playDur);							/* Will affect graphics if pianoroll view */
	
			GetDlgWord(dlog, L_DUR, &newval);
			GRAF_ACCEPT(aNote->subType);
	
			GetDlgWord(dlog, NDOTS, &newval);
			GRAF_ACCEPT(aNote->ndots);
	
			GetDlgWord(dlog, XMOVE_DOTS, &newval);
			GRAF_ACCEPT(aNote->xmovedots);
	
			GetDlgWord(dlog, YMOVE_DOTS, &newval);
			GRAF_ACCEPT(aNote->ymovedots);
	
			GetDlgWord(dlog, HEAD_SHAPE, &newval);
			GRAF_ACCEPT(aNote->headShape);
	
			GetDlgWord(dlog, SMALLNR, &newval);
			GRAF_ACCEPT(aNote->small);
	
			if (!aNote->rest) {
	
				GetDlgWord(dlog, ACCIDENT, &newval);
				GRAF_ACCEPT(aNote->accident);
		
				GetDlgWord(dlog, XMOVE_ACC, &newval);
				GRAF_ACCEPT(aNote->xmoveAcc);
		
				GetDlgWord(dlog, COURTESY_ACC, &newval);
				GRAF_ACCEPT(aNote->courtesyAcc);

				GetDlgWord(dlog, HEADSTEM_SIDE, &newval);
				GRAF_ACCEPT(aNote->otherStemSide);
	
				GetDlgWord(dlog, DOUBLEDUR, &newval);
				GRAF_ACCEPT(aNote->doubleDur);
	
				GetDlgWord(dlog, NOTENUM, &newval);
				GEN_ACCEPT(aNote->noteNum);
				
				GetDlgWord(dlog, ON_VELOCITY, &newval);
				GEN_ACCEPT(aNote->onVelocity);
	
				GetDlgWord(dlog, OFF_VELOCITY, &newval);
				GEN_ACCEPT(aNote->offVelocity);
			}
	
		  	break;
		  	
		case GRSYNCtype:
			aGRNote = GetPAGRNOTE(aGRNoteL);
			
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			DIST_ACCEPT(aGRNote->xd);
	
			GetDlgWord(dlog, VERT, &newval);
			DIST_ACCEPT(aGRNote->yd);
	
			GetDlgWord(dlog, L_DUR, &newval);
			GRAF_ACCEPT(aGRNote->subType);
	
			GetDlgWord(dlog, HEAD_SHAPE, &newval);
			GRAF_ACCEPT(aGRNote->headShape);
	
			GetDlgWord(dlog, SMALLNR, &newval);
			GRAF_ACCEPT(aGRNote->small);
	
			GetDlgWord(dlog, ACCIDENT, &newval);
			GRAF_ACCEPT(aGRNote->accident);
	
			GetDlgWord(dlog, XMOVE_ACC, &newval);
			GRAF_ACCEPT(aGRNote->xmoveAcc);
	
			GetDlgWord(dlog, COURTESY_ACC, &newval);
			GRAF_ACCEPT(aGRNote->courtesyAcc);

			GetDlgWord(dlog, HEADSTEM_SIDE, &newval);
			GRAF_ACCEPT(aGRNote->otherStemSide);
	
			GetDlgWord(dlog, NOTENUM, &newval);
			GEN_ACCEPT(aGRNote->noteNum);
			
			GetDlgWord(dlog, STEM_VERT, &newval);
			DIST_ACCEPT(aGRNote->ystem);
	
			GetDlgWord(dlog, ON_VELOCITY, &newval);
			GEN_ACCEPT(aGRNote->onVelocity);
	
			GetDlgWord(dlog, OFF_VELOCITY, &newval);
			GEN_ACCEPT(aGRNote->offVelocity);
	
		 	break;
	}

	SetPort(oldPort);
	if (graphicDirty)	{										/* Was anything graphic changed? */
		InvalMeasure(pL, staff);
	}
	if (graphicDirty || nodeDirty)						/* Was anything changed? */
	{
		doc->changed = TRUE;									/* Yes. */
		LinkTWEAKED(pL) = TRUE;								/* Flag to show node was edited */
		LinkVALID(pL) = FALSE;								/* Force recalc. objrect */
	}
	
	lastEditField = GetDialogKeyboardFocusItem(dlog); /* Save itemNum of last edit field */
	lastObjType = ObjLType(pL);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);									/* Free heap space */
	PopLock(NOTEheap);
	PopLock(GRNOTEheap);
	return; 
}


/* -------------------------------------------------------------- PageRelGraphic -- */

/* Return TRUE if the given object is a Page-relative Graphic. */

Boolean PageRelGraphic(LINK pL)
{
	PGRAPHIC pGraphic;
	
	if (!GraphicTYPE(pL)) return FALSE;
	pGraphic = GetPGRAPHIC(pL);
	if (pGraphic->firstObj==NILINK) return FALSE;
	return PageTYPE(pGraphic->firstObj);
}


/* --------------------------------------------------------------- GenInfoDialog -- */
/* Handle Get Info dialog for miscellaneous object types. */

static enum {
	LBL_PARAM1=16,
	PARAM1,
	LBL_PARAM2,
	PARAM2,
	UNITLABEL_GEN=21
} E_GenInfoItems;

static void GenInfoDialog(Document *doc, LINK pL, char unitLabel[])
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	INT16			dialogOver;
	short			ditem, aShort;
	INT16			newval, staff, userVoice;
	Handle		tHdl, twHdl, ulHdl, p1Hdl, p2Hdl;
	Rect			tRect, aRect;
	PMEVENT		p;
	PGRAPHIC		pGraphic;
	PSPACE		pSpace;
	PTEMPO		pTempo;
	PENDING		pEnding;
	PAMEASURE	aMeas;
	PACLEF		aClef;
	PAKEYSIG		aKeySig;
	PATIMESIG	aTimeSig;
	PBEAMSET		pBeamset;
	PACONNECT	aConnect;
	PADYNAMIC	aDynamic;
	PASLUR		aSlur;
	LINK			aMeasL, aClefL, aKeySigL, aTimeSigL,
					aConnectL, aDynamicL, aSlurL, partL;
	Boolean		graphicDirty,									/* Anything graphic changed? */
					nodeDirty;										/* Anything non-graphic changed? */
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButDragFilter);
	if (filterUPP == NULL) {
		MissingDialog(GENERALINFO_DLOG);
		return;
	}
				
	GetPort(&oldPort);
	ArrowCursor();
	graphicDirty = nodeDirty = FALSE;									/* Nothing changed yet */

/* --- 1. Create the dialog and initialize its contents. --- */

	PrepareUndo(doc, pL, U_GetInfo, 12);    							/* "Undo Get Info" */

	dlog = GetNewDialog(GENERALINFO_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(GENERALINFO_DLOG);
		return;
	}
	CenterWindow(GetDialogWindow(dlog), 194);
	SetPort(GetDialogWindowPort(dlog));

	SetDlgFont(dlog, textFontNum, textFontSmallSize, 0);

	if (GraphicTYPE(pL))
		strcpy(strBuf, NameGraphicType(pL));
	else
		strcpy(strBuf, NameNodeType(pL));
	if (SlurTYPE(pL) && SlurTIE(pL))
		strcat(strBuf, " Tie");
	PutDlgString(dlog, TYPE, CToPString(strBuf), FALSE);
	PutDlgWord(dlog, OBJ_HORIZ, DD2I(LinkXD(pL)), FALSE);
	GetDialogItem(dlog, TWEAKED, &aShort, &twHdl, &tRect);
	SetDialogItemCText(twHdl, (char *)(LinkTWEAKED(pL)? "T" : " "));
	GetDialogItem(dlog, UNITLABEL_GEN, &aShort, &ulHdl, &tRect);
	SetDialogItemCText(ulHdl, unitLabel);

	/* For objects without subobjects, change labels appropriately. */
	
	switch (ObjLType(pL))
	{
		case MEASUREtype:
		case BEAMSETtype:
		case GRAPHICtype:
		case SLURtype:
		case TEMPOtype:
		case DYNAMtype:				/* Dynamics have subobjs., but subobj. xd is unused */
		case SPACEtype:
		case RPTENDtype:
		case ENDINGtype:
			GetDialogItem(dlog, LBL_OBJ_HORIZ, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "Symbol Horiz.*");
			GetDialogItem(dlog, LBL_SUBOBJ_HORIZ, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "(unused)");
		default:
			;
	}

	/* Do other object-type-specific initialization. */
	
	switch (ObjLType(pL))
	{
		case MEASUREtype:
		 	aMeasL = FirstSubLINK(pL);
		 	for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
		 		if (MeasureSEL(aMeasL)) break;
		 		
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Barline type");

			GetDialogItem(dlog, LBL_SUBOBJ_HORIZ, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "MeasNum Horiz.");

			GetDialogItem(dlog, LBL_VERT, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "MeasNum Vert.");

			PushLock(MEASUREheap);
		 	aMeas = GetPAMEASURE(aMeasL);
			PutDlgWord(dlog, PARAM1, aMeas->subType, FALSE);
			PutDlgWord(dlog, SUBOBJ_HORIZ, aMeas->xMNStdOffset, FALSE);
			PutDlgWord(dlog, VERT, aMeas->yMNStdOffset, FALSE);
			PopLock(MEASUREheap);
			break;
			
		case RPTENDtype:
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Rpt bar type");

			PutDlgWord(dlog, PARAM1, RptType(pL), FALSE);
			break;
			
		case CLEFtype:
			aClefL = FirstSubLINK(pL);
			for ( ; aClefL; aClefL = NextCLEFL(aClefL))
				if (ClefSEL(aClefL)) break;
	
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "Small");

			PushLock(CLEFheap);
			aClef = GetPACLEF(aClefL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aClef->xd), FALSE);
			PutDlgWord(dlog, STAFF_NO, aClef->staffn, FALSE);
			PutDlgWord(dlog, VERT, DD2I(aClef->yd), FALSE);
			PutDlgWord(dlog, PARAM1, aClef->small, FALSE);
			PopLock(CLEFheap);
		  	break;
		  	
		case KEYSIGtype:
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
				if (KeySigSEL(aKeySigL)) break;
	
			PushLock(KEYSIGheap);
			aKeySig = GetPAKEYSIG(aKeySigL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aKeySig->xd), FALSE);
			PutDlgWord(dlog, STAFF_NO, aKeySig->staffn, FALSE);
			PopLock(KEYSIGheap);
		  	break;
	
		case TIMESIGtype:
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
				if (TimeSigSEL(aTimeSigL)) break;
	
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Time sig. type");

			PushLock(TIMESIGheap);
			aTimeSig = GetPATIMESIG(aTimeSigL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aTimeSig->xd), FALSE);
			PutDlgWord(dlog, STAFF_NO, aTimeSig->staffn, FALSE);
			PutDlgWord(dlog, VERT, DD2I(aTimeSig->yd), FALSE);
			PutDlgWord(dlog, PARAM1, aTimeSig->subType, FALSE);
			PopLock(TIMESIGheap);
		  	break;
		  	
		case CONNECTtype:
			aConnectL = FirstSubLINK(pL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL))
				if (ConnectSEL(aConnectL)) break;
	
			PushLock(CONNECTheap);
			aConnect = GetPACONNECT(aConnectL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aConnect->xd), FALSE);
			PopLock(CONNECTheap);
			break;

		case DYNAMtype:
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL))
				if (DynamicSEL(aDynamicL)) break;

			PushLock(DYNAMheap);
			aDynamic = GetPADYNAMIC(aDynamicL);
			
			if (DynamType(pL)>=FIRSTHAIRPIN_DYNAM) {
				GetDialogItem(dlog, LBL_SUBOBJ_HORIZ, &aShort, &tHdl, &aRect);
				SetDialogItemCText(tHdl, "RightEnd Horiz.*");
				GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
				SetDialogItemCText(p1Hdl, "Mouth width");
				GetDialogItem(dlog, LBL_PARAM2, &aShort, &p2Hdl, &aRect);
				SetDialogItemCText(p2Hdl, "Other width");
			}
			else {
				GetDialogItem(dlog, LBL_SUBOBJ_HORIZ, &aShort, &tHdl, &aRect);
				SetDialogItemCText(tHdl, "Small");
			}
	
			PutDlgWord(dlog, STAFF_NO, aDynamic->staffn, FALSE);
			PutDlgWord(dlog, VERT, DD2I(aDynamic->yd), FALSE);
			if (DynamType(pL)>=FIRSTHAIRPIN_DYNAM) {
				PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aDynamic->endxd), FALSE);
				PutDlgWord(dlog, PARAM1, aDynamic->mouthWidth, FALSE);
				PutDlgWord(dlog, PARAM2, aDynamic->otherWidth, FALSE);
			}
			else
				PutDlgWord(dlog, SUBOBJ_HORIZ, aDynamic->small, FALSE);
			PopLock(DYNAMheap);
		  	break;
		  	
		case GRAPHICtype:
			switch GraphicSubType(pL) {
				case GRArpeggio:
					GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
					SetDialogItemCText(p1Hdl, "Height");
					break;
				case GRDraw:
					GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
					SetDialogItemCText(p1Hdl, "Thickness");
					break;
				default:
					NULL;
			}
		 	
			PushLock(OBJheap);
		 	pGraphic = GetPGRAPHIC(pL);
			PutDlgWord(dlog, STAFF_NO, pGraphic->staffn, FALSE);
			if (pGraphic->voice>=0)
				Int2UserVoice(doc, pGraphic->voice, &userVoice, &partL);
			else
				userVoice = NOONE;
			PutDlgWord(dlog, VOICE_NO, userVoice, FALSE);
			PutDlgWord(dlog, VERT, DD2I(pGraphic->yd), FALSE);
			switch GraphicSubType(pL) {
				case GRArpeggio:
					PutDlgWord(dlog, PARAM1, pGraphic->info, FALSE);
					break;
				case GRDraw:
					PutDlgWord(dlog, PARAM1, pGraphic->gu.thickness, FALSE);
					break;
				default:
					NULL;
			}			
			PopLock(OBJheap);
			break;
	
		case BEAMSETtype:
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Thin");
		
			PutDlgWord(dlog, STAFF_NO, BeamSTAFF(pL), FALSE);
			Int2UserVoice(doc, BeamVOICE(pL), &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, FALSE);
			pBeamset = GetPBEAMSET(pL);
			PutDlgWord(dlog, PARAM1, pBeamset->thin, FALSE);
			break;
			
		case SLURtype:
			aSlurL = FirstSubLINK(pL);
			for ( ; aSlurL; aSlurL = NextSLURL(aSlurL))
				if (SlurSEL(aSlurL)) break;

			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Dashed");

			PutDlgWord(dlog, STAFF_NO, SlurSTAFF(pL), FALSE);
			Int2UserVoice(doc, SlurVOICE(pL), &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, FALSE);
			aSlur = GetPASLUR(aSlurL);
			PutDlgWord(dlog, PARAM1, aSlur->dashed, FALSE);
			break;
			
		case TEMPOtype:
			PushLock(OBJheap);
			pTempo = GetPTEMPO(pL);
			PutDlgWord(dlog, STAFF_NO, pTempo->staffn, FALSE);
			PutDlgWord(dlog, VERT, DD2I(pTempo->yd), FALSE);
			PopLock(OBJheap);
			break;

		case SPACEtype:
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Space width");
			
			pSpace = GetPSPACE(pL);
			PutDlgWord(dlog, PARAM1, pSpace->spWidth, FALSE);
		 	break;
			
		case ENDINGtype:
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Right End Horiz.*");
			GetDialogItem(dlog, LBL_PARAM2, &aShort, &p2Hdl, &aRect);
			SetDialogItemCText(p2Hdl, "String number");

			pEnding = GetPENDING(pL);
			PutDlgWord(dlog, VERT, DD2I(pEnding->yd), FALSE);
			PutDlgWord(dlog, PARAM1, DD2I(pEnding->endxd), FALSE);
			PutDlgWord(dlog, PARAM2, pEnding->endNum, FALSE);
			break;
			
		default:
		  	break;
	}

	if (lastEditField!=0 && ObjLType(pL)==lastObjType)
		SelectDialogItemText(dlog, lastEditField, 0, ENDTEXT);		/* Initial selection & hiliting */
	else
		SelectDialogItemText(dlog, OBJ_HORIZ, 0, ENDTEXT);

	ShowWindow(GetDialogWindow(dlog));
	OutlineOKButton(dlog,TRUE);

/*--- 2. Interact with user til they push OK or Cancel. --- */

	dialogOver = 0;
	while (dialogOver==0)
	{
		do
		{
			ModalDialog(filterUPP, &ditem);						/* Handle events in dialog window */
			switch (ditem) {
				case OK:
				case Cancel:
					dialogOver = ditem;
					break;
			 	default:
			 		;
			}
		} while (!dialogOver);
	
	/* --- 3. If dialog was terminated with OK, check any new values. ----- */
	/* ---    If any are illegal, keep dialog on the screen to try again. - */
	
		if (dialogOver==Cancel) {
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);									/* Free heap space */
			SetPort(oldPort);
			return;
		}
		else {
			GetDlgWord(dlog, OBJ_HORIZ, &newval);
			if (PageRelGraphic(pL)) {
				if (I2DD(newval)<0 || I2DD(newval)>p2d(doc->paperRect.right)) {
					GetIndCString(fmtStr, INFOERRS_STRS, 26);		/* "Horizontal position must be..." */
					sprintf(strBuf, fmtStr, DD2I(p2d(doc->paperRect.right)));
					INFO_ERROR(strBuf);
				}
			}
			else
				if (I2DD(newval)<LEFT_HLIM(doc, pL) || I2DD(newval)>RIGHT_HLIM(doc)) {
					GetIndCString(fmtStr, INFOERRS_STRS, 27);		/* "Horizontal position must be..." */
					sprintf(strBuf, fmtStr, DD2I(LEFT_HLIM(doc, pL)), DD2I(RIGHT_HLIM(doc)));
					INFO_ERROR(strBuf);
				}
	
			switch (ObjLType(pL))										/* Get new values by type */
			{
				case MEASUREtype:
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<1 || newval>7 || newval==BAR_HEAVYDBL) {	/* BAR_HEAVYDBL not supported */
						GetIndCString(strBuf, INFOERRS_STRS, 28);	/* "Barline/Repeat bar type must be..." */
						INFO_ERROR(strBuf);
					}
					
					GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
					if (newval<-128 || newval>127) {					/* ??MAGIC NOS!! */
						GetIndCString(strBuf, INFOERRS_STRS, 47);	/* "Measure Number Horizontal offset must be...." */
						INFO_ERROR(strBuf);
					}
					
					GetDlgWord(dlog, VERT, &newval);
					if (newval<-128 || newval>127) {					/* ??MAGIC NOS!! */
						GetIndCString(strBuf, INFOERRS_STRS, 48);	/* "Measure Number Vertical offset must be...." */
						INFO_ERROR(strBuf);
					}

					break;

				case RPTENDtype:
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<1 || newval>7) {
						GetIndCString(strBuf, INFOERRS_STRS, 28);	/* "Barline/Repeat bar type must be..." */
						INFO_ERROR(strBuf);
					}
					
					break;
					
				case CLEFtype:
				case TIMESIGtype:
					GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
					if (newval<-999 || newval>999) {					/* ??MAGIC NOS!! */
						GetIndCString(fmtStr, INFOERRS_STRS, 29);	/* "Subobj H position must be..." */
						sprintf(strBuf, fmtStr, -999, 999);
						INFO_ERROR(strBuf);
					}
					
					GetDlgWord(dlog, VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;
	
					GetDlgWord(dlog, PARAM1, &newval);
					if (ObjLType(pL)==TIMESIGtype) {
						if (newval<N_OVER_D || newval>ZERO_TIME) {
							GetIndCString(fmtStr, INFOERRS_STRS, 30);	/* "Time sig type must be..." */
							sprintf(strBuf, fmtStr, N_OVER_D, ZERO_TIME);
							INFO_ERROR(strBuf);
						}
					}
				  	break;
		
				case DYNAMtype:
					if (DynamType(pL)>=FIRSTHAIRPIN_DYNAM) {
						GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
						if (newval<-999 || newval>4999) {					/* ??MAGIC NOS!! */
							GetIndCString(fmtStr, INFOERRS_STRS, 31);	/* "Right end position must be..." */
							sprintf(strBuf, fmtStr, -999, 4999);
							INFO_ERROR(strBuf);
						}

						GetDlgWord(dlog, PARAM1, &newval);
						if (newval<0 || newval>31) {
							GetIndCString(strBuf, INFOERRS_STRS, 32);	/* "Mouth width must be..." */
							INFO_ERROR(strBuf);
						}

						GetDlgWord(dlog, PARAM2, &newval);
						if (newval<0 || newval>31) {
							GetIndCString(strBuf, INFOERRS_STRS, 33);	/* "Other width must be..." */
							INFO_ERROR(strBuf);
						}
					}

					break;
					
				case KEYSIGtype:
					GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
					if (newval<-999 || newval>999) {					/* ??MAGIC NOS!! */
						GetIndCString(fmtStr, INFOERRS_STRS, 34);	/* "Subobj H position must be..." */
						sprintf(strBuf, fmtStr, -999, 999);
						INFO_ERROR(strBuf);
					}
					
				  	break;
	
				case GRAPHICtype:
					GetDlgWord(dlog, VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;

					switch GraphicSubType(pL) {
						case GRArpeggio:
							GetDlgWord(dlog, PARAM1, &newval);
							if (newval<1 || newval>999) {
								GetIndCString(strBuf, INFOERRS_STRS, 35);	/* "Arpeggio sign ht must be..." */
								INFO_ERROR(strBuf);
							}
							break;
						case GRDraw:
							GetDlgWord(dlog, PARAM1, &newval);
							if (newval<1 || newval>127) {
								GetIndCString(strBuf, INFOERRS_STRS, 45);	/* "Line thickness must be..." */
								INFO_ERROR(strBuf);
							}
							break;
						default:
							NULL;
					}			
					break;
					
				case BEAMSETtype:
					pBeamset = GetPBEAMSET(pL);
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 36);	/* "Thin value must be..." */
						INFO_ERROR(strBuf);
					}
					break;

				case SLURtype:
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 37);	/* "Dashed value must be..." */
						INFO_ERROR(strBuf);
					}
					break;
					
				case TEMPOtype:
					GetDlgWord(dlog, VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;
					
					break;
					
				case SPACEtype:
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<1 || newval>999) {
						GetIndCString(strBuf, INFOERRS_STRS, 38);	/* "Space width must be..." */
						INFO_ERROR(strBuf);
					}
				 
				 	break;
				 	
				case ENDINGtype:
					GetDlgWord(dlog, VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;
					
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<-999 || newval>4999) {				/* ??MAGIC NOS!! */
						GetIndCString(fmtStr, INFOERRS_STRS, 39);	/* "Right end position must be..." */
						sprintf(strBuf, fmtStr, -999, 4999);
						INFO_ERROR(strBuf);
					}

					GetDlgWord(dlog, PARAM2, &newval);
					if (newval<0 || newval>MAX_ENDING_STRINGS) {
						GetIndCString(fmtStr, INFOERRS_STRS, 40);	/* "Ending number must be..." */
						sprintf(strBuf, fmtStr, MAX_ENDING_STRINGS);
						INFO_ERROR(strBuf);
					}

					break;

				default:
				  		;
			}
		}
	}
	
/* --- 4. Dialog was terminated with OK and all values are legal. --- */

	GetDlgWord(dlog, OBJ_HORIZ, &newval);
	DIST_ACCEPT(LinkXD(pL));

	staff = 1;
	switch (ObjLType(pL))										/* Get new values by type */
	{
		case MEASUREtype:
		 	aMeas = GetPAMEASURE(aMeasL);
	
			GetDlgWord(dlog, PARAM1, &newval);
			GRAF_ACCEPT(aMeas->subType);

			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			GRAF_ACCEPT(aMeas->xMNStdOffset);

			GetDlgWord(dlog, VERT, &newval);
			GRAF_ACCEPT(aMeas->yMNStdOffset);

			break;
			
		case RPTENDtype:	
			GetDlgWord(dlog, PARAM1, &newval);
			GRAF_ACCEPT(RptType(pL));
			break;

		case CLEFtype:
			PushLock(CLEFheap);
			aClef = GetPACLEF(aClefL);
	
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			DIST_ACCEPT(aClef->xd);
	
			GetDlgWord(dlog, VERT, &newval);
			DIST_ACCEPT(aClef->yd);
			
			GetDlgWord(dlog, PARAM1, &newval);
			GRAF_ACCEPT(aClef->small);

			PopLock(CLEFheap);
		  	break;
	
		case KEYSIGtype:
			PushLock(KEYSIGheap);
			aKeySig = GetPAKEYSIG(aKeySigL);
	
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			DIST_ACCEPT(aKeySig->xd);
	
			PopLock(KEYSIGheap);
		  	break;
	
		case TIMESIGtype:
			PushLock(TIMESIGheap);
			aTimeSig = GetPATIMESIG(aTimeSigL);
	
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			DIST_ACCEPT(aTimeSig->xd);
	
			GetDlgWord(dlog, VERT, &newval);
			DIST_ACCEPT(aTimeSig->yd);

			GetDlgWord(dlog, PARAM1, &newval);
			GRAF_ACCEPT(aTimeSig->subType);

			PopLock(TIMESIGheap);
		  	break;
	
		case CONNECTtype:
			PushLock(CONNECTheap);
			aConnect = GetPACONNECT(aConnectL);
			
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			DIST_ACCEPT(aConnect->xd);

			PopLock(CONNECTheap);
			break;

		case DYNAMtype:
			PushLock(DYNAMheap);
			aDynamic = GetPADYNAMIC(aDynamicL);
		
			GetDlgWord(dlog, VERT, &newval);
			DIST_ACCEPT(aDynamic->yd);

			if (DynamType(pL)>=FIRSTHAIRPIN_DYNAM) {
				GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
				DIST_ACCEPT(aDynamic->endxd);
				GetDlgWord(dlog, PARAM1, &newval);
				GRAF_ACCEPT(aDynamic->mouthWidth);
				GetDlgWord(dlog, PARAM2, &newval);
				GRAF_ACCEPT(aDynamic->otherWidth);
			}
			else
				GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
				GRAF_ACCEPT(aDynamic->small);

			PopLock(DYNAMheap);
		  	break;
		  	
		case GRAPHICtype:
			PushLock(OBJheap);
		 	pGraphic = GetPGRAPHIC(pL);
		 	
			GetDlgWord(dlog, VERT, &newval);
			DIST_ACCEPT(pGraphic->yd);

			switch GraphicSubType(pL) {
				case GRArpeggio:
					GetDlgWord(dlog, PARAM1, &newval);
					GRAF_ACCEPT(pGraphic->info);
					break;
				case GRDraw:
					GetDlgWord(dlog, PARAM1, &newval);
					GRAF_ACCEPT(pGraphic->gu.thickness);
					break;
				default:
					NULL;
			}			

			PopLock(OBJheap);
		 	break;
	
		case BEAMSETtype:
			GetDlgWord(dlog, PARAM1, &newval);
			pBeamset = GetPBEAMSET(pL);
			GRAF_ACCEPT(pBeamset->thin);
			break;
			
		case SLURtype:
			GetDlgWord(dlog, PARAM1, &newval);
			aSlur = GetPASLUR(aSlurL);
			GRAF_ACCEPT(aSlur->dashed);
			break;
			
		case TEMPOtype:
			GetDlgWord(dlog, VERT, &newval);
			p = GetPMEVENT(pL);
			DIST_ACCEPT(p->yd);
		  	break;
	
		case SPACEtype:
			GetDlgWord(dlog, PARAM1, &newval);
			pSpace = GetPSPACE(pL);
			GRAF_ACCEPT(pSpace->spWidth);
		  	break;

		case ENDINGtype:
			GetDlgWord(dlog, VERT, &newval);
			pEnding = GetPENDING(pL);
			DIST_ACCEPT(pEnding->yd);

			GetDlgWord(dlog, PARAM1, &newval);
			DIST_ACCEPT(pEnding->endxd);

			GetDlgWord(dlog, PARAM2, &newval);
			GRAF_ACCEPT(pEnding->endNum);
		  	break;
	}

	SetPort(oldPort);
	if (graphicDirty)	{										/* Was anything graphic changed? */
		InvalMeasure(pL, staff);
	}
	if (graphicDirty || nodeDirty)						/* Was anything changed? */
	{
		doc->changed = TRUE;									/* Yes. */
		LinkTWEAKED(pL) = TRUE;
		LinkVALID(pL) = FALSE;								/* Force recalc. objrect */
	}
	
	lastEditField = GetDialogKeyboardFocusItem(dlog); /* Save itemNum of last edit field */
	lastObjType = ObjLType(pL);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);									/* Free heap space */
	return; 
}



/* ------------------------------------------------------------- ExtendInfoDialog -- */
/* Handle Get Info dialog for "extended" object types. */

static enum {
	LEFT_HORIZ=OBJ_HORIZ,
	LEFT_VERT=SUBOBJ_HORIZ,
	LBL_EXPARAM1=LBL_VERT,
	EXPARAM1=VERT,
	RIGHT_HORIZ=PARAM1,
	LBL_RIGHT_VERT=LBL_PARAM2,
	RIGHT_VERT=PARAM2,
	LBL_EXPARAM2=20,
	EXPARAM2,
	UNITLABEL_EX=23
} E_ExtendInfoItems;

static void ExtendInfoDialog(Document *doc, LINK pL, char unitLabel[])
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	INT16			dialogOver;
	short			ditem, aShort;
	INT16			newval, staff, userVoice;
	Handle		tHdl, twHdl, ulHdl, p1Hdl, p2Hdl;
	Rect			tRect, aRect;
	POCTAVA		octavap;
	PTUPLET		pTuplet;
	Boolean		graphicDirty,									/* Anything graphic changed? */
					nodeDirty;										/* Anything non-graphic changed? */
	LINK			partL;
	char			fmtStr[256];
	PGRAPHIC		pGraphic;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButDragFilter);
	if (filterUPP == NULL) {
		MissingDialog(EXTINFO_DLOG);
		return;
	}
								
	GetPort(&oldPort);
	ArrowCursor();
	graphicDirty = nodeDirty = FALSE;									/* Nothing changed yet */

/* --- 1. Create the dialog and initialize its contents. --- */

	PrepareUndo(doc, pL, U_GetInfo, 12);    							/* "Undo Get Info" */

	dlog = GetNewDialog(EXTINFO_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(EXTINFO_DLOG);
		return;
	}
	CenterWindow(GetDialogWindow(dlog), 194);
	SetPort(GetDialogWindowPort(dlog));

	SetDlgFont(dlog, textFontNum, textFontSmallSize, 0);
	if (GraphicTYPE(pL))
		strcpy(strBuf, NameGraphicType(pL));
	else
		strcpy(strBuf, NameNodeType(pL));
	PutDlgString(dlog, TYPE, CToPString(strBuf), FALSE);
	PutDlgWord(dlog, LEFT_HORIZ, DD2I(LinkXD(pL)), FALSE);
	
	GetDialogItem(dlog, TWEAKED, &aShort, &twHdl, &tRect);
	SetDialogItemCText(twHdl, (char *)(LinkTWEAKED(pL)? "T" : " "));
	GetDialogItem(dlog, UNITLABEL_EX, &aShort, &ulHdl, &tRect);
	SetDialogItemCText(ulHdl, unitLabel);

	/* Do object-type-specific initialization. */
	
	switch (ObjLType(pL))
	{
		case GRAPHICtype:									/* NB: only GRDraw now */
			PushLock(OBJheap);
		 	pGraphic = GetPGRAPHIC(pL);
			PutDlgWord(dlog, STAFF_NO, pGraphic->staffn, FALSE);
			if (pGraphic->voice>=0)
				Int2UserVoice(doc, pGraphic->voice, &userVoice, &partL);
			else
				userVoice = NOONE;
			PutDlgWord(dlog, VOICE_NO, userVoice, FALSE);
			PutDlgWord(dlog, LEFT_VERT, DD2I(pGraphic->yd), FALSE);

			GetDialogItem(dlog, LBL_EXPARAM1, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "Thickness");
			
			PutDlgWord(dlog, EXPARAM1, pGraphic->gu.thickness, FALSE);
			PutDlgWord(dlog, RIGHT_HORIZ, DD2I(pGraphic->info), FALSE);
			PutDlgWord(dlog, RIGHT_VERT, DD2I(pGraphic->info2), FALSE);

			PopLock(OBJheap);
			break;

		case OCTAVAtype:
			GetDialogItem(dlog, LBL_EXPARAM1, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "noCutoff");
			GetDialogItem(dlog, LBL_RIGHT_VERT, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "(unused)");

			octavap = GetPOCTAVA(pL);
			PutDlgWord(dlog, STAFF_NO, octavap->staffn, FALSE);
			PutDlgWord(dlog, LEFT_HORIZ, DD2I(octavap->xdFirst), FALSE);
			PutDlgWord(dlog, LEFT_VERT, DD2I(octavap->ydFirst), FALSE);
			PutDlgWord(dlog, RIGHT_HORIZ, DD2I(octavap->xdLast), FALSE);
			PutDlgWord(dlog, EXPARAM1, octavap->noCutoff, FALSE);
			break;

		case TUPLETtype:
			GetDialogItem(dlog, LBL_EXPARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "accNum vis?");
			GetDialogItem(dlog, LBL_EXPARAM2, &aShort, &p2Hdl, &aRect);
			SetDialogItemCText(p2Hdl, "Bracket vis?");

			PushLock(OBJheap);
		 	pTuplet = GetPTUPLET(pL);
			PutDlgWord(dlog, LEFT_HORIZ, DD2I(pTuplet->xdFirst), FALSE);
			PutDlgWord(dlog, LEFT_VERT, DD2I(pTuplet->ydFirst), FALSE);
			PutDlgWord(dlog, RIGHT_HORIZ, DD2I(pTuplet->xdLast), FALSE);
			PutDlgWord(dlog, RIGHT_VERT, DD2I(pTuplet->ydLast), FALSE);

			PutDlgWord(dlog, STAFF_NO, pTuplet->staffn, FALSE);
			Int2UserVoice(doc, pTuplet->voice, &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, FALSE);
			PutDlgWord(dlog, EXPARAM1, pTuplet->numVis, FALSE);
			PutDlgWord(dlog, EXPARAM2, pTuplet->brackVis, FALSE);
			PopLock(OBJheap);
		 	break;
		 	
		default:
		  	break;
	}

	if (lastEditField!=0 && ObjLType(pL)==lastObjType)
		SelectDialogItemText(dlog, lastEditField, 0, ENDTEXT);		/* Initial selection & hiliting */
	else
		SelectDialogItemText(dlog, OBJ_HORIZ, 0, ENDTEXT);

	ShowWindow(GetDialogWindow(dlog));
	OutlineOKButton(dlog,TRUE);

/*--- 2. Interact with user til they push OK or Cancel. --- */

	dialogOver = 0;
	while (dialogOver==0)
	{
		do
		{
			ModalDialog(filterUPP, &ditem);						/* Handle events in dialog window */
			switch (ditem) {
				case OK:
				case Cancel:
					dialogOver = ditem;
					break;
			 	default:
			 		;
			}
		} while (!dialogOver);
	
	/* --- 3. If dialog was terminated with OK, check any new values. ----- */
	/* ---    If any are illegal, keep dialog on the screen to try again. - */
	
		if (dialogOver==Cancel)
		{
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);									/* Free heap space */
			SetPort(oldPort);
			return;
		}
		else
		{
			GetDlgWord(dlog, LEFT_HORIZ, &newval);
			if (I2DD(newval)<LEFT_HLIM(doc, pL) || I2DD(newval)>RIGHT_HLIM(doc)) {
				GetIndCString(fmtStr, INFOERRS_STRS, 41);			/* "Horizontal pos must be..." */
				sprintf(strBuf, fmtStr, DD2I(LEFT_HLIM(doc, pL)), DD2I(RIGHT_HLIM(doc)));
				INFO_ERROR(strBuf);
			}
	
			GetDlgWord(dlog, RIGHT_HORIZ, &newval);
			if (I2DD(newval)<LEFT_HLIM(doc, pL) || I2DD(newval)>RIGHT_HLIM(doc)) {
				GetIndCString(fmtStr, INFOERRS_STRS, 41);			/* "Horizontal pos must be..." */
				sprintf(strBuf, fmtStr, DD2I(LEFT_HLIM(doc, pL)), DD2I(RIGHT_HLIM(doc)));
				INFO_ERROR(strBuf);
			}
		
			GetDlgWord(dlog, LEFT_VERT, &newval);
			if (!LegalVert(doc, newval)) dialogOver = 0;

			switch (ObjLType(pL))										/* Check new values by type */
			{
				case GRAPHICtype:												/* NB: only GRDraw now */
					GetDlgWord(dlog, RIGHT_VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;

					GetDlgWord(dlog, EXPARAM1, &newval);
					if (newval<1 || newval>127) {
						GetIndCString(strBuf, INFOERRS_STRS, 45);		/* "Line thickness must be..." */
						INFO_ERROR(strBuf);
					}
					break;

				case OCTAVAtype:
					GetDlgWord(dlog, EXPARAM1, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 42);		/* "noCutoff must be..." */
						INFO_ERROR(strBuf);
					}

					break;

				case TUPLETtype:
					GetDlgWord(dlog, RIGHT_VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;

					GetDlgWord(dlog, EXPARAM1, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 43);		/* "accNum Visible must be..." */
						INFO_ERROR(strBuf);
					}
					
					GetDlgWord(dlog, EXPARAM2, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 44);		/* "Bracket Visible must be..." */
						INFO_ERROR(strBuf);
					}
	
				  	break;
				  	
				default:
				  		;
			}
		}
	}
	
/* --- 4. Dialog was terminated with OK and all values are legal. --- */

	GetDlgWord(dlog, OBJ_HORIZ, &newval);
	DIST_ACCEPT(LinkXD(pL));

	staff = 1;
	switch (ObjLType(pL))										/* Get new values by type */
	{
		case GRAPHICtype:											/* NB: only GRDraw now */
			PushLock(OBJheap);
		 	pGraphic = GetPGRAPHIC(pL);
		 	
			GetDlgWord(dlog, LEFT_VERT, &newval);
			DIST_ACCEPT(pGraphic->yd);
		 	
			GetDlgWord(dlog, RIGHT_HORIZ, &newval);
			DIST_ACCEPT(pGraphic->info);

			GetDlgWord(dlog, RIGHT_VERT, &newval);
			DIST_ACCEPT(pGraphic->info2);

			GetDlgWord(dlog, EXPARAM1, &newval);
			GRAF_ACCEPT(pGraphic->gu.thickness);

			PopLock(OBJheap);
			break;

		case OCTAVAtype:
			PushLock(OBJheap);
			octavap = GetPOCTAVA(pL);

			GetDlgWord(dlog, LEFT_HORIZ, &newval);
			DIST_ACCEPT(octavap->xdFirst);

			GetDlgWord(dlog, LEFT_VERT, &newval);
			DIST_ACCEPT(octavap->ydFirst);
			octavap->ydLast = octavap->ydFirst;				/* We don't support diagonal octave signs yet */

			GetDlgWord(dlog, EXPARAM1, &newval);
			GRAF_ACCEPT(octavap->noCutoff);

			GetDlgWord(dlog, RIGHT_HORIZ, &newval);
			DIST_ACCEPT(octavap->xdLast);

			PopLock(OBJheap);
		  	break;

		case TUPLETtype:
			PushLock(OBJheap);
		 	pTuplet = GetPTUPLET(pL);
	
			GetDlgWord(dlog, LEFT_HORIZ, &newval);
			DIST_ACCEPT(pTuplet->xdFirst);

			GetDlgWord(dlog, LEFT_VERT, &newval);
			DIST_ACCEPT(pTuplet->ydFirst);

			GetDlgWord(dlog, RIGHT_HORIZ, &newval);
			DIST_ACCEPT(pTuplet->xdLast);

			GetDlgWord(dlog, RIGHT_VERT, &newval);
			DIST_ACCEPT(pTuplet->ydLast);
	
			GetDlgWord(dlog, EXPARAM1, &newval);
			GRAF_ACCEPT(pTuplet->numVis);
	
			GetDlgWord(dlog, EXPARAM2, &newval);
			GRAF_ACCEPT(pTuplet->brackVis);

			PopLock(OBJheap);
		  	break;
	
		default:
			;
	}

	SetPort(oldPort);
	if (graphicDirty)	{										/* Was anything graphic changed? */
		InvalMeasure(pL, staff);
	}
	if (graphicDirty || nodeDirty)						/* Was anything changed? */
	{
		doc->changed = TRUE;									/* Yes. */
		LinkTWEAKED(pL) = TRUE;
		LinkVALID(pL) = FALSE;								/* Force recalc. objRect */
	}
	
	lastEditField = GetDialogKeyboardFocusItem(dlog); /* Save itemNum of last edit field */
	lastObjType = ObjLType(pL);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	return; 
}


/* ======================================================= ModNRDialog and friends == */

static enum {
	HORIZ=4,
	MVERT=6,
	DATA=8,
	SHOW_MOD,
	PREV,
	NEXT
} E_ModifierItems;

#define MAX_MODNRS 50		/* Max. modifiers per note/rest we can edit */

static DialogPtr mDlog;
static Rect modNRArea;
static INT16 modCode, noteSubType;
static Boolean noteSmall;
static DDIST noteYD;
static Boolean above, center, dirty;
static short lastModEditField=0;

static void MIDrawSlashes(INT16, INT16, INT16, DDIST);
static void InfoDrawModNR(void);
static pascal void UserDrawModNR(DialogPtr, short);
static void PrepDrawModNR(AMODNR, PCONTEXT);
static Boolean GetModNRValues(PAMODNR);

void MIDrawSlashes(INT16 xhead, INT16 ystem, INT16 nslashes, DDIST dEighthLn)
{
	DDIST		slashLeading, slashThick, slashWidth, slashHeight;
	INT16		i, xslash, yslash;
	
	slashLeading = 6*dEighthLn;
	slashThick = 3*dEighthLn;
	slashWidth = 10*dEighthLn;
	slashHeight = -6*dEighthLn;
	
	PenSize(1, d2p(slashThick)>1? d2p(slashThick) : 1);
	xslash = xhead+d2p(2*dEighthLn);
	yslash = ystem+d2p(8*dEighthLn);
	for (i = 1; i<=nslashes; i++) {
		MoveTo(xslash, yslash);
		Line(d2p(slashWidth), d2p(slashHeight));
		yslash += d2p(slashLeading);
	}
	PenNormal();	
}

#define DISP_RASTRAL 2				/* Arbitrary but preferably big for readability */	

static void InfoDrawModNR()
{
	short oldtxFont, oldtxSize;
	INT16 xOffset, yOffset, sizePercent, width, height, lnSpace, yoff;
	unsigned char glyph;
	Rect portRect;
	
	EraseRect(&modNRArea);
	ClipRect(&modNRArea);

	oldtxFont = GetPortTxFont();
	oldtxSize = GetPortTxSize();
	TextFont(sonataFontNum);								/* Use Sonata font */
	TextSize(Staff2MFontSize(drSize[DISP_RASTRAL]));

	if (GetModNRInfo(modCode, noteSubType, noteSmall, above, &glyph,
							&xOffset, &yOffset, &sizePercent)) {
		width = modNRArea.right-modNRArea.left;
		height = modNRArea.bottom-modNRArea.top;

		if (modCode>=MOD_TREMOLO1 && modCode<=MOD_TREMOLO6) {
			lnSpace = p2d(pdrSize[DISP_RASTRAL])/4;
			MIDrawSlashes(modNRArea.left+width/2-5, modNRArea.top+height/2-10,
								modCode-MOD_TREMOLO1+1, lnSpace/8);
		}
		else if (glyph!=' ') {
			PenPat(NGetQDGlobalsGray());
			MoveTo(modNRArea.left+4, modNRArea.top+height/2);
			LineTo(modNRArea.right-4, modNRArea.top+height/2);
			PenNormal();

			if (!center) yoff =
				(above? (-pdrSize[DISP_RASTRAL])/2 : (pdrSize[DISP_RASTRAL])/2+2);
			else
				yoff = 0;
			MoveTo(modNRArea.left+width/2-5, modNRArea.top+height/2+yoff);
			DrawChar(glyph);
		}
	}
	portRect = GetQDPortBounds();
	ClipRect(&portRect);

	TextFont(oldtxFont);
	TextSize(oldtxSize);
}


/* User Item-drawing procedure to draw note modifier */

static pascal void UserDrawModNR(DialogPtr /*thewindow*/, short /*ditem*/)
{
	InfoDrawModNR();
}


static void PrepDrawModNR(AMODNR modNR, PCONTEXT pContext)
{
	DDIST ydMod;
	
	modCode = modNR.modCode;
	ydMod = qd2d(modNR.ystdpit, pContext->staffHeight, pContext->staffLines);
	above = (ydMod<=noteYD);
	center = (ydMod==0);

	PutDlgWord(mDlog, HORIZ, modNR.xstd-XSTD_OFFSET, FALSE);
	PutDlgWord(mDlog, DATA, modNR.data, FALSE);
	PutDlgWord(mDlog, MVERT, modNR.ystdpit, TRUE);	/* Last so it can start out selected */
}


static Boolean GetModNRValues(PAMODNR aModNR)
{
	INT16 newval;
	Boolean okay=TRUE;
	
	GetDlgWord(mDlog, HORIZ, &newval);
	if (newval<-16 || newval>15) {
		GetIndCString(strBuf, INFOERRS_STRS, 3);			/* "Horizontal must be..." */
		CParamText(strBuf, "", "", "");
		Inform(MODNRINFO_ALRT);
		okay = FALSE;
	}
	aModNR->xstd = newval+XSTD_OFFSET;

	GetDlgWord(mDlog, MVERT, &newval);
	if (newval<-128 || newval>127) {
		GetIndCString(strBuf, INFOERRS_STRS, 4);			/* "Vertical must be..." */
		CParamText(strBuf, "", "", "");
		Inform(MODNRINFO_ALRT);
		okay = FALSE;
	}
	aModNR->ystdpit = newval;

	GetDlgWord(mDlog, DATA, &newval);
	if (newval<-128 || newval>127) {
		GetIndCString(strBuf, INFOERRS_STRS, 5);			/* "Data must be..." */
		CParamText(strBuf, "", "", "");
		Inform(MODNRINFO_ALRT);
		okay = FALSE;
	}
	aModNR->data = newval;
	
	return okay;
}


/* ----------------------------------------------------------------- ModNRDialog -- */
/* Handler for note/rest modifier dialog. Assumes the first selected node is a
Sync. Returns TRUE if dialog is OK'ed, FALSE if cancelled or an error occurs. */

Boolean ModNRDialog(Document *doc)
{
	LINK pL, aNoteL, aModNRL;
	PANOTE aNote;
	PAMODNR aModNR;
	INT16 i, modCount, dialogOver;
	short ditem, aShort;
	INT16 curr;
	Handle aHdl;
	ControlHandle prevHdl, nextHdl;
	Rect aRect;
	GrafPtr oldPort;
	AMODNR modNR[MAX_MODNRS];
	CONTEXT context;
	UserItemUPP userDrawModNRUPP;
	ModalFilterUPP	filterUPP;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL)) break;

	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteSEL(aNoteL)) break;

	aNote = GetPANOTE(aNoteL);
	aModNRL = aNote->firstMod;
	if (!aModNRL) {
		GetIndCString(strBuf, INFOERRS_STRS, 6);			/* "1st sel note/rest has no modifiers" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	
	noteSubType = aNote->subType;
	noteSmall = aNote->small;
	noteYD = aNote->yd;
	 
	/* Copy all the modifiers into a local array to facilitate Cancelling. */

	for (i = 0; aModNRL && i<MAX_MODNRS; aModNRL = NextMODNRL(aModNRL), i++) {
		aModNR = GetPAMODNR(aModNRL);
		modNR[i] = *aModNR;
	}
	modCount = i;
	curr = 0;
	dirty = FALSE;														/* Nothing changed yet */

	userDrawModNRUPP = NewUserItemUPP(UserDrawModNR);
	if (userDrawModNRUPP==NULL) {
		MissingDialog(MODNR_DLOG);
		return FALSE;
	}
	filterUPP = NewModalFilterUPP(OKButDragFilter);
	if (filterUPP == NULL) {
		DisposeUserItemUPP(userDrawModNRUPP);
		MissingDialog(MODNR_DLOG);
		return FALSE;
	}
								
	GetPort(&oldPort);
	mDlog = GetNewDialog(MODNR_DLOG, NULL, BRING_TO_FRONT);
	if (!mDlog) {
		DisposeUserItemUPP(userDrawModNRUPP);
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MODNR_DLOG);
		return FALSE;
	}
	SetPort(GetDialogWindowPort(mDlog));

	/* Prepare user item to display the note modifier. First get the rectangle
	and hide it so it doesn't cover up any other items. */
	
	GetDialogItem(mDlog, SHOW_MOD, &aShort, &aHdl, &modNRArea);		
	HideDialogItem(mDlog, SHOW_MOD);
	SetDialogItem(mDlog, SHOW_MOD, 0, (Handle)userDrawModNRUPP, &modNRArea);

	SetDlgFont(mDlog, textFontNum, textFontSmallSize, 0);

	GetDialogItem(mDlog, PREV, &aShort, (Handle *)&prevHdl, &aRect);		
	GetDialogItem(mDlog, NEXT, &aShort, (Handle *)&nextHdl, &aRect);		
	HiliteControl(prevHdl, curr>0 ? CTL_ACTIVE : CTL_INACTIVE);
	HiliteControl(nextHdl, curr+1<modCount ? CTL_ACTIVE : CTL_INACTIVE);

	GetContext(doc, pL, NoteSTAFF(aNoteL), &context);
	PrepDrawModNR(modNR[curr], &context);
	if (lastModEditField!=0)
		SelectDialogItemText(mDlog, lastModEditField, 0, ENDTEXT);	/* Initial selection & hiliting */
	ShowWindow(GetDialogWindow(mDlog));
	
	dialogOver = 0;
	while (dialogOver==0)
	{
		do
		{
			ModalDialog(filterUPP, &ditem);						/* Handle events in dialog window */
			switch (ditem) {
				case OK:
				case Cancel:
					dialogOver = ditem;
					break;
				case PREV:
					if (curr-1>=0) {
						if (GetModNRValues(&modNR[curr])) {
							curr--;
							HiliteControl(prevHdl, curr>0 ? CTL_ACTIVE : CTL_INACTIVE);
							HiliteControl(nextHdl, curr+1<modCount ? CTL_ACTIVE : CTL_INACTIVE);
							PrepDrawModNR(modNR[curr], &context);
							InfoDrawModNR();
						}
					}
					else
						SysBeep(1);
					break;
				case NEXT:
					if (curr+1<modCount) {
						if (GetModNRValues(&modNR[curr])) {
							curr++;
							HiliteControl(prevHdl, curr>0 ? CTL_ACTIVE : CTL_INACTIVE);
							HiliteControl(nextHdl, curr+1<modCount ? CTL_ACTIVE : CTL_INACTIVE);
							PrepDrawModNR(modNR[curr], &context);
							InfoDrawModNR();
						}
					}
					else
						SysBeep(1);
					break;
				default:
			 		;
			}
		} while (!dialogOver);
		
		if (dialogOver==OK) {
			if (GetModNRValues(&modNR[curr])) {
				aNote = GetPANOTE(aNoteL);
				aModNRL = aNote->firstMod;
				for (i = 0; aModNRL && i<modCount; aModNRL = NextMODNRL(aModNRL), i++) {
					aModNR = GetPAMODNR(aModNRL);
					if (aModNR->xstd!=modNR[i].xstd) dirty = TRUE;
					if (aModNR->ystdpit!=modNR[i].ystdpit) dirty = TRUE;
					if (aModNR->data!=modNR[i].data) dirty = TRUE;
					*aModNR = modNR[i];
				}
			}
			else
				dialogOver = 0;
		}
	}

	SetPort(oldPort);
	if (dirty)	{													/* Was anything changed? */
		InvalMeasure(pL, NoteSTAFF(aNoteL));
		doc->changed = TRUE;										/* Yes. */
		LinkTWEAKED(pL) = TRUE;									/* Flag to show node was edited */
	}

	if (dialogOver==OK)
		lastModEditField = GetDialogKeyboardFocusItem(mDlog);
	DisposeUserItemUPP(userDrawModNRUPP);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(mDlog);										/* Free heap space */
	return (dialogOver==OK);
}


/* ------------------------------------------------------ XLoadInfoDialogSeg -- */
/* Null function to allow loading or unloading InfoDialog.c's segment. */

void XLoadInfoDialogSeg()
{
}
