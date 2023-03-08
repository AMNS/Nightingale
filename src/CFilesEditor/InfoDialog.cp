/******************************************************************************************
*	FILE:	InfoDialog.c
*	PROJ:	Nightingale
*	DESC:	Handling routines for "Get Info" and "Modifier Info" dialogs
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* File InfoDialog.c - "Get Info" dialog routines:
	InfoDialog				LegalStaff/Vert			SyncInfoDialog
	PageRelGraphic			GenInfoDialog			ExtendInfoDialog
	ModNRInfoDialog and friends
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean LegalVert(Document *, short);
static void SyncInfoDialog(Document *, LINK, char []);
static Boolean PageRelGraphic(LINK);
static void GenInfoDialog(Document *, LINK, char []);
static void ExtendInfoDialog(Document *, LINK, char []);

#define DD2I(num) (((num)+scaleRound)>>scaleShift)			/* Convert DDIST to Get Info */
#define I2DD(num)	((num)<<scaleShift)						/* Convert Get Info to DDIST */

/* Macros for changing fields in the data structure: DIST_ACCEPT for DDIST distances;
MEASNUM_ACCEPT fof fields that affect measure numbers;  GRAF_ACCEPT for other fields
that might immediately affect the display; GEN_ACCEPT for all others. */

#define DIST_ACCEPT(field)	if (newval!=DD2I(field)) \
									{ (field) = I2DD(newval);  graphicDirty = True; }
#define MEASNUM_ACCEPT(field)	if (newval!=(field)) \
									{ (field) = newval;  measNumDirty = True; }
#define GRAF_ACCEPT(field)	if (newval!=(field)) \
									{ (field) = newval;  graphicDirty = True; }
#define GEN_ACCEPT(field)	if (newval!=(field)) \
									{ (field) = newval;  nodeDirty = True; }

#define DIALOG_TOP	194

static short	scaleShift, scaleRound;
static short	lastEditField = 0, lastObjType = 0;

/* Note: the "Info" dialogs are intended for use mostly by expert users. Accordingly,
error checking is definitely on the permissive side! */


/* ------------------------------------------------------------------------ InfoDialog -- */

enum {
	DDIST_UNITS=0,
	QTRPOINT_UNITS,
	POINT_UNITS
};

void InfoDialog(Document *doc)
{
	short units;  char unitLabel[256];  LINK pL;
	
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
		case OTTAVAtype:
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

/* -------------------------------------------------------------------- SyncInfoDialog -- */

static Boolean LegalVert(Document *doc, short newval)
{
	if (I2DD(newval)<ABOVE_VLIM(doc) || I2DD(newval)>BELOW_VLIM(doc)) {
		GetIndCString(strBuf, INFOERRS_STRS, 2);			/* "Vertical position out of bounds" */
		CParamText(strBuf, "", "", "");
		Inform(GINFO_ALRT);
		return False;
	}
	else
		return True;
}

#define INFO_ERROR(str) { CParamText((str), "", "", ""); Inform(GINFO_ALRT); dialogOver = 0; }

/* Handle Get Info dialog for SYNCs and GRSYNCs. */

enum										/* Dialog item numbers */
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
	START_TIME=55,
	SEGMENT0=57,
	SEGMENT1,
	SEGMENT2,
	SEGMENT3,
	SEGMENT4,
	SEGMENT5
};

#define MAX_HORIZ_POS 1999
#define MAX_SYNCINFO_INT 32000

static void SyncInfoDialog(Document *doc, LINK pL, char unitLabel[])
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	short		dialogOver;
	short		ditem, aShort, userVoice, newval;
	Handle		twHdl, ulHdl;
	Rect		tRect;
	PANOTE		aNote;
	PAGRNOTE	aGRNote;
	LINK		aNoteL, aGRNoteL, partL, sysL;
	Boolean		measNumDirty,									/* Measure nos. maybe changed? */
				graphicDirty,									/* Anything graphic changed? */
				nodeDirty;										/* Anything non-graphic changed? */
	Boolean		hasStem, isRest;
	char		fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButDragFilter);
	if (filterUPP == NULL) {
		MissingDialog(SYNCINFO_DLOG);
		return;
	}
				
	GetPort(&oldPort);
	ArrowCursor();
	measNumDirty = graphicDirty = nodeDirty = False;				/* Nothing changed yet */

/* --- 1. Create the dialog and initialize its contents. --- */

	PrepareUndo(doc, pL, U_GetInfo, 12);    						/* "Undo Get Info" */

	dlog = GetNewDialog(SYNCINFO_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SYNCINFO_DLOG);
		return;
	}
	CenterWindow(GetDialogWindow(dlog), DIALOG_TOP);
	SetPort(GetDialogWindowPort(dlog));
	
	SetDlgFont(dlog, textFontNum, textFontSmallSize, 0);

	PushLock(NOTEheap);
	PushLock(GRNOTEheap);
	
	PutDlgWord(dlog, OBJ_HORIZ, DD2I(LinkXD(pL)), False);
	GetDialogItem(dlog, TWEAKED, &aShort, &twHdl, &tRect);
	SetDialogItemCText(twHdl, (char *)(LinkTWEAKED(pL)? "T" : " "));
	GetDialogItem(dlog, UNITLABEL_SYNC, &aShort, &ulHdl, &tRect);
	SetDialogItemCText(ulHdl, unitLabel);

	isRest = False;													/* In case it's a GRSYNC */
	
	switch (ObjLType(pL)) {											/* Initialize by type... */
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL)) break;
	
			aNote = GetPANOTE(aNoteL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aNote->xd), False);
			PutDlgWord(dlog, STAFF_NO, aNote->staffn, False);
			Int2UserVoice(doc, aNote->voice, &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, False);
			PutDlgWord(dlog, VERT, DD2I(aNote->yd), False);
			PutDlgWord(dlog, STEM_VERT, DD2I(aNote->ystem), False);
			PutDlgWord(dlog, P_TIME, aNote->playTimeDelta, False);
			PutDlgWord(dlog, P_DUR, aNote->playDur, False);
			PutDlgWord(dlog, L_DUR, aNote->subType, False);			/* logical duration */
			PutDlgWord(dlog, NDOTS, aNote->ndots, False);
			PutDlgWord(dlog, XMOVE_DOTS, aNote->xMoveDots, False);
			PutDlgWord(dlog, YMOVE_DOTS, aNote->yMoveDots, False);
			PutDlgWord(dlog, HEAD_SHAPE, aNote->headShape, False);
			PutDlgLong(dlog, START_TIME, SyncAbsTime(pL), False);
			
			hasStem = (aNote->ystem!=aNote->yd);
			isRest = aNote->rest;
			
			strcpy(strBuf, NameObjType(pL));
			if (isRest) {
				strcat(strBuf, " / Rest");
				PutDlgString(dlog, TYPE, CToPString(strBuf), False);
				PutDlgWord(dlog, SMALLNR, aNote->small, False);		/* whether small note/rest */
			}
			else {
				strcat(strBuf,  " / Note");
				PutDlgString(dlog, TYPE, CToPString(strBuf), False);
				PutDlgWord(dlog, ACCIDENT, aNote->accident, False);
				PutDlgWord(dlog, XMOVE_ACC, aNote->xmoveAcc, False);
				PutDlgWord(dlog, COURTESY_ACC, aNote->courtesyAcc, False);
				PutDlgWord(dlog, HEADSTEM_SIDE, aNote->otherStemSide, False);
				PutDlgWord(dlog, DOUBLEDUR, aNote->doubleDur, False);
				PutDlgWord(dlog, NOTENUM, aNote->noteNum, False);
				PutDlgWord(dlog, ON_VELOCITY, aNote->onVelocity, False);
				PutDlgWord(dlog, OFF_VELOCITY, aNote->offVelocity, False);
				PutDlgWord(dlog, SEGMENT0, aNote->nhSegment[0], False);
				PutDlgWord(dlog, SEGMENT1, aNote->nhSegment[1], False);
				PutDlgWord(dlog, SEGMENT2, aNote->nhSegment[2], False);
				PutDlgWord(dlog, SEGMENT3, aNote->nhSegment[3], False);
				PutDlgWord(dlog, SEGMENT4, aNote->nhSegment[4], False);
				PutDlgWord(dlog, SEGMENT5, aNote->nhSegment[5], False);
				PutDlgWord(dlog, SMALLNR, aNote->small, False);		/* whether small note/rest */
			}
			break;
			
		case GRSYNCtype:
			strcpy(strBuf, NameObjType(pL));
			PutDlgString(dlog, TYPE, CToPString(strBuf), False);
			
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
				if (GRNoteSEL(aGRNoteL)) break;
	
			aGRNote = GetPAGRNOTE(aGRNoteL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aGRNote->xd), False);
			PutDlgWord(dlog, STAFF_NO, aGRNote->staffn, False);
			Int2UserVoice(doc, aGRNote->voice, &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, False);
			PutDlgWord(dlog, VERT, DD2I(aGRNote->yd), False);
			PutDlgWord(dlog, L_DUR, aGRNote->subType, False);
			PutDlgWord(dlog, HEAD_SHAPE, aGRNote->headShape, False);
			PutDlgWord(dlog, ACCIDENT, aGRNote->accident, False);
			PutDlgWord(dlog, XMOVE_ACC, aGRNote->xmoveAcc, False);
			PutDlgWord(dlog, COURTESY_ACC, aGRNote->courtesyAcc, False);
			PutDlgWord(dlog, HEADSTEM_SIDE, aGRNote->otherStemSide, False);
			PutDlgWord(dlog, NOTENUM, aGRNote->noteNum, False);
			PutDlgWord(dlog, STEM_VERT, DD2I(aGRNote->ystem), False);
			PutDlgWord(dlog, ON_VELOCITY, aGRNote->onVelocity, False);
			PutDlgWord(dlog, OFF_VELOCITY, aGRNote->offVelocity, False);
			PutDlgWord(dlog, SMALLNR, aGRNote->small, False);		/* whether small grace note */

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
	OutlineOKButton(dlog,True);

/*--- 2. Interact with user til they push OK or Cancel. --- */

	dialogOver = 0;
	while (dialogOver==0) {
		do {
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
			PopLock(NOTEheap);
			PopLock(GRNOTEheap);
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);								/* Free heap space */
			SetPort(oldPort);
			return;
		}
		else {
			short newval2, newvalA[6], newvalMin, newvalMax, max_dots, minl_dur;

			GetDlgWord(dlog, OBJ_HORIZ, &newval);
			if (I2DD(newval)<0 || I2DD(newval)>RIGHT_HLIM(doc)) {
				GetIndCString(fmtStr, INFOERRS_STRS, 7);		/* "Horizontal position must be..." */
				sprintf(strBuf, fmtStr, DD2I(RIGHT_HLIM(doc)));
				INFO_ERROR(strBuf);
			}
	
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			if (newval<-MAX_HORIZ_POS || newval>MAX_HORIZ_POS) {
				GetIndCString(fmtStr, INFOERRS_STRS, 8);		/* "Note/rest horiz pos must be..." */
				sprintf(strBuf, fmtStr, -MAX_HORIZ_POS, MAX_HORIZ_POS);
				INFO_ERROR(strBuf);
			}
			
			GetDlgWord(dlog, VERT, &newval);
			if (!LegalVert(doc, newval)) dialogOver = 0;

			GetDlgWord(dlog, L_DUR, &newval);
			minl_dur = (isRest? -127 : UNKNOWN_L_DUR);
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
				if (newval<0 || newval>MAX_SYNCINFO_INT) {
					GetIndCString(fmtStr, INFOERRS_STRS, 13);	/* "Play time must be..." */
					sprintf(strBuf, fmtStr, 0, MAX_SYNCINFO_INT);
					INFO_ERROR(strBuf);
				}

				GetDlgWord(dlog, P_DUR, &newval);
				if (newval<1 || newval>MAX_SYNCINFO_INT) {
					GetIndCString(fmtStr, INFOERRS_STRS, 14);	/* "Play duration must be..." */
					sprintf(strBuf, fmtStr, 1, MAX_SYNCINFO_INT);
					INFO_ERROR(strBuf);
				}

				GetDlgWord(dlog, NDOTS, &newval);
				aNote = GetPANOTE(aNoteL);
				max_dots = MAX_L_DUR-aNote->subType;
				if (newval<0 || newval>max_dots) {
					GetIndCString(fmtStr, INFOERRS_STRS, 15);	/* "Number of dots must be..." */
					sprintf(strBuf, fmtStr, max_dots);
					INFO_ERROR(strBuf);
				}
	
				GetDlgWord(dlog, XMOVE_DOTS, &newval);
				if (newval<0 || newval>7) {
					GetIndCString(strBuf, INFOERRS_STRS, 16);	/* "Aug dot H offset must be..." */
					INFO_ERROR(strBuf);
				}
	
				GetDlgWord(dlog, YMOVE_DOTS, &newval);
				if (newval<0 || newval>3) {
					GetIndCString(strBuf, INFOERRS_STRS, 17);	/* "Aug dot V offset must be..." */
					INFO_ERROR(strBuf);
				}

				if (!NoteREST(aNoteL)) {
					GetDlgWord(dlog, DOUBLEDUR, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 18);	/* "Dbl duration val must be..." */
						INFO_ERROR(strBuf);
					}
				}
			}

/* The remaining "note" parameters don't apply to rests. */
			if (NoteREST(aNoteL)) continue;
			
			GetDlgWord(dlog, ACCIDENT, &newval);
			if (newval<0 || newval>5) {
				GetIndCString(strBuf, INFOERRS_STRS, 19);		/* "Accidental must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, XMOVE_ACC, &newval);
			if (newval<0 || newval>31) {
				GetIndCString(strBuf, INFOERRS_STRS, 20);		/* "Accidental H offset must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, COURTESY_ACC, &newval);
			if (newval<0 || newval>1) {
				GetIndCString(strBuf, INFOERRS_STRS, 46);		/* "Courtesy accidental must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, HEADSTEM_SIDE, &newval);
			if (newval<0 || newval>1) {
				GetIndCString(strBuf, INFOERRS_STRS, 21);		/* "Notehead's stem side must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, NOTENUM, &newval);
			if (newval<0 || newval>127) {						/* MAX_NOTENUM */
				GetIndCString(strBuf, INFOERRS_STRS, 22);		/* "MIDI note number must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, STEM_VERT, &newval);
			if (newval<ABOVE_VLIM(doc) || newval>BELOW_VLIM(doc)) {
				GetIndCString(strBuf, INFOERRS_STRS, 23);		/* "Stem position out of bounds" */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, ON_VELOCITY, &newval);
			if (newval<0 || newval>127) {						/* MAX_VELOCITY */
				GetIndCString(strBuf, INFOERRS_STRS, 24);		/* "MIDI On velocity must be..." */
				INFO_ERROR(strBuf);
			}

			GetDlgWord(dlog, OFF_VELOCITY, &newval);
			if (newval<0 || newval>127) {
				GetIndCString(strBuf, INFOERRS_STRS, 25);		/* "MIDI Off velocity must be..." */
				INFO_ERROR(strBuf);
			}

			/* The first notehead segment can't have the highest value allowed, since
			   (with the standard segment-drawing function) that's invisible and, if it's
			   the only nonzero segment, that'd make the whole note head invisible. This
			   is not ideal even with the standard segment-drawing function, but it's
			   simple and good enough for now (v. 6.0), and experience is needed before
			   we know what would be better. */
			   
			GetDlgWord(dlog, SEGMENT0, &newvalA[0]);
			GetDlgWord(dlog, SEGMENT1, &newvalA[1]);
			GetDlgWord(dlog, SEGMENT2, &newvalA[2]);
			GetDlgWord(dlog, SEGMENT3, &newvalA[3]);
			GetDlgWord(dlog, SEGMENT4, &newvalA[4]);
			GetDlgWord(dlog, SEGMENT5, &newvalA[5]);
			
			if (newvalA[0]==NHGRAPH_COLORS) {
				GetIndCString(fmtStr, INFOERRS_STRS, 50);		/* The first notehead segment must be..." */
				sprintf(strBuf, fmtStr, NHGRAPH_COLORS);
				INFO_ERROR(strBuf);
			}
			newvalMin = 999;  newvalMax = -999;
			for (short j = 0; j<6; j++) newvalMin = n_min(newvalMin, newvalA[j]);
			for (short j = 0; j<6; j++) newvalMax = n_max(newvalMax, newvalA[j]);			
			if (newvalMin<0 || newvalMax>NHGRAPH_COLORS) {
				GetIndCString(fmtStr, INFOERRS_STRS, 49);		/* Notehead segments must be..." */
				sprintf(strBuf, fmtStr, NHGRAPH_COLORS);
				INFO_ERROR(strBuf);
			}
		}
	}
	
/* --- 4. Dialog was terminated with OK and all values are legal. --- */

	GetDlgWord(dlog, OBJ_HORIZ, &newval);
	DIST_ACCEPT(LinkXD(pL));

	switch (ObjLType(pL)) {									/* Get new values by type */
		case SYNCtype:
			aNote = GetPANOTE(aNoteL);
	
			GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
			DIST_ACCEPT(aNote->xd);
	
			GetDlgWord(dlog, VERT, &newval);
			DIST_ACCEPT(aNote->yd);
	
			GetDlgWord(dlog, STEM_VERT, &newval);
			DIST_ACCEPT(aNote->ystem);
	
			GetDlgWord(dlog, P_TIME, &newval);
			GRAF_ACCEPT(aNote->playTimeDelta);				/* Will affect graphics if pianoroll view */
	
			GetDlgWord(dlog, P_DUR, &newval);
			GRAF_ACCEPT(aNote->playDur);					/* Will affect graphics if pianoroll view */
	
			GetDlgWord(dlog, L_DUR, &newval);
			MEASNUM_ACCEPT(aNote->subType);					/* Affects measure nos. if multibar rest is involved */
	
			GetDlgWord(dlog, NDOTS, &newval);
			GRAF_ACCEPT(aNote->ndots);
	
			GetDlgWord(dlog, XMOVE_DOTS, &newval);
			GRAF_ACCEPT(aNote->xMoveDots);
	
			GetDlgWord(dlog, YMOVE_DOTS, &newval);
			GRAF_ACCEPT(aNote->yMoveDots);
	
			GetDlgWord(dlog, HEAD_SHAPE, &newval);
			GRAF_ACCEPT(aNote->headShape);
	
			GetDlgWord(dlog, SMALLNR, &newval);
			GRAF_ACCEPT(aNote->small);
	
			if (!isRest) {
	
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

				GetDlgWord(dlog, SEGMENT0, &newval);
				GRAF_ACCEPT(aNote->nhSegment[0]);
				GetDlgWord(dlog, SEGMENT1, &newval);
				GRAF_ACCEPT(aNote->nhSegment[1]);
				GetDlgWord(dlog, SEGMENT2, &newval);
				GRAF_ACCEPT(aNote->nhSegment[2]);
				GetDlgWord(dlog, SEGMENT3, &newval);
				GRAF_ACCEPT(aNote->nhSegment[3]);
				GetDlgWord(dlog, SEGMENT4, &newval);
				GRAF_ACCEPT(aNote->nhSegment[4]);
				GetDlgWord(dlog, SEGMENT5, &newval);
				GRAF_ACCEPT(aNote->nhSegment[5]);
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
	if (measNumDirty) {									/* Was anything changed that could affect measure nos.? */
		UpdateMeasNums(doc, NILINK);
		sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
		EraseAndInvalRange(doc, sysL, doc->tailL);
	}
	else if (graphicDirty)	{							/* Was anything graphic changed? */
		InvalMeasure(pL, 1);							/* Staff no. doesn't matter */
	}

	if (measNumDirty || graphicDirty || nodeDirty) {	/* Was anything at all changed? */
		doc->changed = True;							/* Yes. */
		LinkTWEAKED(pL) = True;							/* Flag to show node was edited */
		LinkVALID(pL) = False;							/* Force recalc. objrect */
	}
	
	lastEditField = GetDialogKeyboardFocusItem(dlog);	/* Save itemNum of last edit field */
	lastObjType = ObjLType(pL);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);								/* Free heap space */
	PopLock(NOTEheap);
	PopLock(GRNOTEheap);
	return; 
}


/* -------------------------------------------------------------------- PageRelGraphic -- */

/* Return True if the given object is a Page-relative Graphic. */

Boolean PageRelGraphic(LINK pL)
{
	PGRAPHIC pGraphic;
	
	if (!GraphicTYPE(pL)) return False;
	pGraphic = GetPGRAPHIC(pL);
	if (pGraphic->firstObj==NILINK) return False;
	return PageTYPE(pGraphic->firstObj);
}


/* --------------------------------------------------------------------- GenInfoDialog -- */
/* Handle Get Info dialog for miscellaneous object types. */

enum {
	LBL_PARAM1=16,
	PARAM1,
	LBL_PARAM2,
	PARAM2,
	UNITLABEL_GEN=21,
	LBL_MEASTIME,
	MEASTIME
};

static void GenInfoDialog(Document *doc, LINK pL, char unitLabel[])
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	short		dialogOver;
	short		ditem, aShort;
	short		newval, userVoice;
	Handle		tHdl, twHdl, ulHdl, p1Hdl, p2Hdl;
	Rect		tRect, aRect;
	PMEVENT		p;
	PGRAPHIC	pGraphic;
	PSPACER		pSpace;
	PTEMPO		pTempo;
	PENDING		pEnding;
	PAMEASURE	aMeas;
	PACLEF		aClef;
	PAKEYSIG	aKeySig;
	PATIMESIG	aTimeSig;
	PBEAMSET	pBeamset;
	PACONNECT	aConnect;
	PADYNAMIC	aDynamic;
	PASLUR		aSlur;
	LINK		aMeasL, aClefL, aKeySigL, aTimeSigL,
				aConnectL, aDynamicL, aSlurL, partL;
	Boolean		graphicDirty,									/* Anything graphic changed? */
				nodeDirty;										/* Anything non-graphic changed? */
	char		fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButDragFilter);
	if (filterUPP == NULL) {
		MissingDialog(GENERALINFO_DLOG);
		return;
	}
				
	GetPort(&oldPort);
	ArrowCursor();
	graphicDirty = nodeDirty = False;								/* Nothing changed yet */

/* --- 1. Create the dialog and initialize its contents. --- */

	PrepareUndo(doc, pL, U_GetInfo, 12);    						/* "Undo Get Info" */

	dlog = GetNewDialog(GENERALINFO_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(GENERALINFO_DLOG);
		return;
	}
	CenterWindow(GetDialogWindow(dlog), DIALOG_TOP);
	SetPort(GetDialogWindowPort(dlog));

	SetDlgFont(dlog, textFontNum, textFontSmallSize, 0);

	strcpy(strBuf, NameObjType(pL));
	if (GraphicTYPE(pL)) {
		strcat(strBuf,  " / ");
		strcat(strBuf, NameGraphicType(pL, True));
	}
	else if (SlurTYPE(pL) && SlurTIE(pL))
		strcat(strBuf, " / Tie");
	PutDlgString(dlog, TYPE, CToPString(strBuf), False);
	
	PutDlgWord(dlog, OBJ_HORIZ, DD2I(LinkXD(pL)), False);
	GetDialogItem(dlog, TWEAKED, &aShort, &twHdl, &tRect);
	SetDialogItemCText(twHdl, (char *)(LinkTWEAKED(pL)? "T" : " "));
	GetDialogItem(dlog, UNITLABEL_GEN, &aShort, &ulHdl, &tRect);
	SetDialogItemCText(ulHdl, unitLabel);
	if (ObjLType(pL)!=MEASUREtype) {
		HideDialogItem(dlog, LBL_MEASTIME);
		HideDialogItem(dlog, MEASTIME);
	}

	/* For objects without subobjects, change labels appropriately. */
	
	switch (ObjLType(pL)) {
		case MEASUREtype:
		case BEAMSETtype:
		case GRAPHICtype:
		case SLURtype:
		case TEMPOtype:
		case DYNAMtype:				/* Dynamics have subobjs., but subobj. xd is unused */
		case SPACERtype:
		case RPTENDtype:
		case ENDINGtype:
			GetDialogItem(dlog, LBL_OBJ_HORIZ, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "Symbol H*");
			GetDialogItem(dlog, LBL_SUBOBJ_HORIZ, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "(unused)");
		default:
			;
	}

	/* Do other object-type-specific initialization. */
	
	switch (ObjLType(pL))
	{
		case MEASUREtype:
			PutDlgLong(dlog, MEASTIME, MeasureTIME(pL), False);

		 	aMeasL = FirstSubLINK(pL);
		 	for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
		 		if (MeasureSEL(aMeasL)) break;
		 		
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Barline type");

			GetDialogItem(dlog, LBL_SUBOBJ_HORIZ, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "MeasNum H");

			GetDialogItem(dlog, LBL_VERT, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "MeasNum V");

			PushLock(MEASUREheap);
		 	aMeas = GetPAMEASURE(aMeasL);
			PutDlgWord(dlog, PARAM1, aMeas->subType, False);
			PutDlgWord(dlog, SUBOBJ_HORIZ, aMeas->xMNStdOffset, False);
			PutDlgWord(dlog, VERT, aMeas->yMNStdOffset, False);
			PopLock(MEASUREheap);
			break;
			
		case RPTENDtype:
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Rpt bar type");

			PutDlgWord(dlog, PARAM1, RptType(pL), False);
			break;
			
		case CLEFtype:
			aClefL = FirstSubLINK(pL);
			for ( ; aClefL; aClefL = NextCLEFL(aClefL))
				if (ClefSEL(aClefL)) break;
	
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "Small");

			PushLock(CLEFheap);
			aClef = GetPACLEF(aClefL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aClef->xd), False);
			PutDlgWord(dlog, STAFF_NO, aClef->staffn, False);
			PutDlgWord(dlog, VERT, DD2I(aClef->yd), False);
			PutDlgWord(dlog, PARAM1, aClef->small, False);
			PopLock(CLEFheap);
		  	break;
		  	
		case KEYSIGtype:
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
				if (KeySigSEL(aKeySigL)) break;
	
			PushLock(KEYSIGheap);
			aKeySig = GetPAKEYSIG(aKeySigL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aKeySig->xd), False);
			PutDlgWord(dlog, STAFF_NO, aKeySig->staffn, False);
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
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aTimeSig->xd), False);
			PutDlgWord(dlog, STAFF_NO, aTimeSig->staffn, False);
			PutDlgWord(dlog, VERT, DD2I(aTimeSig->yd), False);
			PutDlgWord(dlog, PARAM1, aTimeSig->subType, False);
			PopLock(TIMESIGheap);
		  	break;
		  	
		case CONNECTtype:
			aConnectL = FirstSubLINK(pL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL))
				if (ConnectSEL(aConnectL)) break;
	
			PushLock(CONNECTheap);
			aConnect = GetPACONNECT(aConnectL);
			PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aConnect->xd), False);
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
				SetDialogItemCText(tHdl, "RightEnd H*");
				GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
				SetDialogItemCText(p1Hdl, "Mouth width");
				GetDialogItem(dlog, LBL_PARAM2, &aShort, &p2Hdl, &aRect);
				SetDialogItemCText(p2Hdl, "Other width");
			}
			else {
				GetDialogItem(dlog, LBL_SUBOBJ_HORIZ, &aShort, &tHdl, &aRect);
				SetDialogItemCText(tHdl, "Small");
			}
	
			PutDlgWord(dlog, STAFF_NO, aDynamic->staffn, False);
			PutDlgWord(dlog, VERT, DD2I(aDynamic->yd), False);
			if (DynamType(pL)>=FIRSTHAIRPIN_DYNAM) {
				PutDlgWord(dlog, SUBOBJ_HORIZ, DD2I(aDynamic->endxd), False);
				PutDlgWord(dlog, PARAM1, aDynamic->mouthWidth, False);
				PutDlgWord(dlog, PARAM2, aDynamic->otherWidth, False);
			}
			else
				PutDlgWord(dlog, SUBOBJ_HORIZ, aDynamic->small, False);
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
					;
			}
		 	
			PushLock(OBJheap);
		 	pGraphic = GetPGRAPHIC(pL);
			PutDlgWord(dlog, STAFF_NO, pGraphic->staffn, False);
			if (pGraphic->voice>=0)
				Int2UserVoice(doc, pGraphic->voice, &userVoice, &partL);
			else
				userVoice = NOONE;
			PutDlgWord(dlog, VOICE_NO, userVoice, False);
			PutDlgWord(dlog, VERT, DD2I(pGraphic->yd), False);
			switch GraphicSubType(pL) {
				case GRArpeggio:
					PutDlgWord(dlog, PARAM1, pGraphic->info, False);
					break;
				case GRDraw:
					PutDlgWord(dlog, PARAM1, pGraphic->gu.thickness, False);
					break;
				default:
					;
			}			
			PopLock(OBJheap);
			break;
	
		case BEAMSETtype:
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Thin");
		
			PutDlgWord(dlog, STAFF_NO, BeamSTAFF(pL), False);
			Int2UserVoice(doc, BeamVOICE(pL), &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, False);
			pBeamset = GetPBEAMSET(pL);
			PutDlgWord(dlog, PARAM1, pBeamset->thin, False);
			break;
			
		case SLURtype:
			aSlurL = FirstSubLINK(pL);
			for ( ; aSlurL; aSlurL = NextSLURL(aSlurL))
				if (SlurSEL(aSlurL)) break;

			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Dashed");

			PutDlgWord(dlog, STAFF_NO, SlurSTAFF(pL), False);
			Int2UserVoice(doc, SlurVOICE(pL), &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, False);
			aSlur = GetPASLUR(aSlurL);
			PutDlgWord(dlog, PARAM1, aSlur->dashed, False);
			break;
			
		case TEMPOtype:
			PushLock(OBJheap);
			pTempo = GetPTEMPO(pL);
			PutDlgWord(dlog, STAFF_NO, pTempo->staffn, False);
			PutDlgWord(dlog, VERT, DD2I(pTempo->yd), False);
			PopLock(OBJheap);
			break;

		case SPACERtype:
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Space width");
			
			pSpace = GetPSPACER(pL);
			PutDlgWord(dlog, PARAM1, pSpace->spWidth, False);
		 	break;
			
		case ENDINGtype:
			GetDialogItem(dlog, LBL_PARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "Right End H*");
			GetDialogItem(dlog, LBL_PARAM2, &aShort, &p2Hdl, &aRect);
			SetDialogItemCText(p2Hdl, "String number");

			pEnding = GetPENDING(pL);
			PutDlgWord(dlog, VERT, DD2I(pEnding->yd), False);
			PutDlgWord(dlog, PARAM1, DD2I(pEnding->endxd), False);
			PutDlgWord(dlog, PARAM2, pEnding->endNum, False);
			break;
			
		default:
		  	break;
	}

	if (lastEditField!=0 && ObjLType(pL)==lastObjType)
		SelectDialogItemText(dlog, lastEditField, 0, ENDTEXT);		/* Initial selection & hiliting */
	else
		SelectDialogItemText(dlog, OBJ_HORIZ, 0, ENDTEXT);

	ShowWindow(GetDialogWindow(dlog));
	OutlineOKButton(dlog,True);

/*--- 2. Interact with user til they push OK or Cancel. --- */

	dialogOver = 0;
	while (dialogOver==0) {
		do {
			ModalDialog(filterUPP, &ditem);							/* Handle events in dialog window */
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
			DisposeDialog(dlog);										/* Free heap space */
			SetPort(oldPort);
			return;
		}
		else {
			GetDlgWord(dlog, OBJ_HORIZ, &newval);
			if (PageRelGraphic(pL)) {
				if (I2DD(newval)<0 || I2DD(newval)>p2d(doc->paperRect.right)) {
					GetIndCString(fmtStr, INFOERRS_STRS, 26);			/* "Horizontal position must be..." */
					sprintf(strBuf, fmtStr, DD2I(p2d(doc->paperRect.right)));
					INFO_ERROR(strBuf);
				}
			}
			else
				if (I2DD(newval)<LEFT_HLIM(doc, pL) || I2DD(newval)>RIGHT_HLIM(doc)) {
					GetIndCString(fmtStr, INFOERRS_STRS, 27);			/* "Horizontal position must be..." */
					sprintf(strBuf, fmtStr, DD2I(LEFT_HLIM(doc, pL)), DD2I(RIGHT_HLIM(doc)));
					INFO_ERROR(strBuf);
				}
	
			switch (ObjLType(pL)) {										/* Get new values by type */
				case MEASUREtype:
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<1 || newval>7 || newval==BAR_HEAVYDBL) {	/* BAR_HEAVYDBL not supported */
						GetIndCString(strBuf, INFOERRS_STRS, 28);		/* "Barline/Repeat bar type must be..." */
						INFO_ERROR(strBuf);
					}
					
					GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
					if (newval<-128 || newval>127) {					/* ??MAGIC NOS!! */
						GetIndCString(strBuf, INFOERRS_STRS, 47);		/* "Measure Number Horizontal offset must be...." */
						INFO_ERROR(strBuf);
					}
					
					GetDlgWord(dlog, VERT, &newval);
					if (newval<-128 || newval>127) {					/* ??MAGIC NOS!! */
						GetIndCString(strBuf, INFOERRS_STRS, 48);		/* "Measure Number Vertical offset must be...." */
						INFO_ERROR(strBuf);
					}

					break;

				case RPTENDtype:
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<1 || newval>7) {
						GetIndCString(strBuf, INFOERRS_STRS, 28);		/* "Barline/Repeat bar type must be..." */
						INFO_ERROR(strBuf);
					}
					
					break;
					
				case CLEFtype:
				case TIMESIGtype:
					GetDlgWord(dlog, SUBOBJ_HORIZ, &newval);
					if (newval<-999 || newval>999) {					/* ??MAGIC NOS!! */
						GetIndCString(fmtStr, INFOERRS_STRS, 29);		/* "Subobj H position must be..." */
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
						if (newval<-999 || newval>4999) {				/* ??MAGIC NOS!! */
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
						GetIndCString(fmtStr, INFOERRS_STRS, 34);		/* "Subobj H position must be..." */
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
							;
					}			
					break;
					
				case BEAMSETtype:
					pBeamset = GetPBEAMSET(pL);
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 36);		/* "Thin value must be..." */
						INFO_ERROR(strBuf);
					}
					break;

				case SLURtype:
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 37);		/* "Dashed value must be..." */
						INFO_ERROR(strBuf);
					}
					break;
					
				case TEMPOtype:
					GetDlgWord(dlog, VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;
					
					break;
					
				case SPACERtype:
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<1 || newval>999) {
						GetIndCString(strBuf, INFOERRS_STRS, 38);		/* "Space width must be..." */
						INFO_ERROR(strBuf);
					}
				 
				 	break;
				 	
				case ENDINGtype:
					GetDlgWord(dlog, VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;
					
					GetDlgWord(dlog, PARAM1, &newval);
					if (newval<-999 || newval>4999) {					/* ??MAGIC NOS!! */
						GetIndCString(fmtStr, INFOERRS_STRS, 39);		/* "Right end position must be..." */
						sprintf(strBuf, fmtStr, -999, 4999);
						INFO_ERROR(strBuf);
					}

					GetDlgWord(dlog, PARAM2, &newval);
					if (newval<0 || newval>MAX_ENDING_STRINGS) {
						GetIndCString(fmtStr, INFOERRS_STRS, 40);		/* "Ending number must be..." */
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
					;
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
	
		case SPACERtype:
			GetDlgWord(dlog, PARAM1, &newval);
			pSpace = GetPSPACER(pL);
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
	if (graphicDirty)	{								/* Was anything graphic changed? */
		InvalMeasure(pL, 1);
	}
	if (graphicDirty || nodeDirty) {					/* Was anything at all changed? */
		doc->changed = True;							/* Yes. */
		LinkTWEAKED(pL) = True;
		LinkVALID(pL) = False;							/* Force recalc. objrect */
	}
	
	lastEditField = GetDialogKeyboardFocusItem(dlog);	/* Save itemNum of last edit field */
	lastObjType = ObjLType(pL);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);								/* Free heap space */
	return; 
}


/* ------------------------------------------------------------------ ExtendInfoDialog -- */
/* Handle Get Info dialog for "extended" object types. */

enum {
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
};

static void ExtendInfoDialog(Document *doc, LINK pL, char unitLabel[])
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	short		dialogOver;
	short		ditem, aShort;
	short		newval, userVoice;
	Handle		tHdl, twHdl, ulHdl, p1Hdl, p2Hdl;
	Rect		tRect, aRect;
	POTTAVA		ottavap;
	PTUPLET		pTuplet;
	Boolean		graphicDirty,									/* Anything graphic changed? */
				nodeDirty;										/* Anything non-graphic changed? */
	LINK		partL;
	char		fmtStr[256];
	PGRAPHIC	pGraphic;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButDragFilter);
	if (filterUPP == NULL) {
		MissingDialog(EXTINFO_DLOG);
		return;
	}
								
	GetPort(&oldPort);
	ArrowCursor();
	graphicDirty = nodeDirty = False;							/* Nothing changed yet */

/* --- 1. Create the dialog and initialize its contents. --- */

	PrepareUndo(doc, pL, U_GetInfo, 12);    					/* "Undo Get Info" */

	dlog = GetNewDialog(EXTINFO_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(EXTINFO_DLOG);
		return;
	}
	CenterWindow(GetDialogWindow(dlog), DIALOG_TOP);
	SetPort(GetDialogWindowPort(dlog));

	SetDlgFont(dlog, textFontNum, textFontSmallSize, 0);
	strcpy(strBuf, NameObjType(pL));
	if (GraphicTYPE(pL)) {
		strcat(strBuf, " / ");
		strcat(strBuf, NameGraphicType(pL, True));
	}

	PutDlgString(dlog, TYPE, CToPString(strBuf), False);
	PutDlgWord(dlog, LEFT_HORIZ, DD2I(LinkXD(pL)), False);
	
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
			PutDlgWord(dlog, STAFF_NO, pGraphic->staffn, False);
			if (pGraphic->voice>=0)
				Int2UserVoice(doc, pGraphic->voice, &userVoice, &partL);
			else
				userVoice = NOONE;
			PutDlgWord(dlog, VOICE_NO, userVoice, False);
			PutDlgWord(dlog, LEFT_VERT, DD2I(pGraphic->yd), False);

			GetDialogItem(dlog, LBL_EXPARAM1, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "Thickness");
			
			PutDlgWord(dlog, EXPARAM1, pGraphic->gu.thickness, False);
			PutDlgWord(dlog, RIGHT_HORIZ, DD2I(pGraphic->info), False);
			PutDlgWord(dlog, RIGHT_VERT, DD2I(pGraphic->info2), False);		/* for GRDraw only */

			PopLock(OBJheap);
			break;

		case OTTAVAtype:
			GetDialogItem(dlog, LBL_EXPARAM1, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "noCutoff");
			GetDialogItem(dlog, LBL_RIGHT_VERT, &aShort, &tHdl, &aRect);
			SetDialogItemCText(tHdl, "(unused)");

			ottavap = GetPOTTAVA(pL);
			PutDlgWord(dlog, STAFF_NO, ottavap->staffn, False);
			PutDlgWord(dlog, LEFT_HORIZ, DD2I(ottavap->xdFirst), False);
			PutDlgWord(dlog, LEFT_VERT, DD2I(ottavap->ydFirst), False);
			PutDlgWord(dlog, RIGHT_HORIZ, DD2I(ottavap->xdLast), False);
			PutDlgWord(dlog, EXPARAM1, ottavap->noCutoff, False);
			break;

		case TUPLETtype:
			GetDialogItem(dlog, LBL_EXPARAM1, &aShort, &p1Hdl, &aRect);
			SetDialogItemCText(p1Hdl, "accNum vis?");
			GetDialogItem(dlog, LBL_EXPARAM2, &aShort, &p2Hdl, &aRect);
			SetDialogItemCText(p2Hdl, "Bracket vis?");

			PushLock(OBJheap);
		 	pTuplet = GetPTUPLET(pL);
			PutDlgWord(dlog, LEFT_HORIZ, DD2I(pTuplet->xdFirst), False);
			PutDlgWord(dlog, LEFT_VERT, DD2I(pTuplet->ydFirst), False);
			PutDlgWord(dlog, RIGHT_HORIZ, DD2I(pTuplet->xdLast), False);
			PutDlgWord(dlog, RIGHT_VERT, DD2I(pTuplet->ydLast), False);

			PutDlgWord(dlog, STAFF_NO, pTuplet->staffn, False);
			Int2UserVoice(doc, pTuplet->voice, &userVoice, &partL);
			PutDlgWord(dlog, VOICE_NO, userVoice, False);
			PutDlgWord(dlog, EXPARAM1, pTuplet->numVis, False);
			PutDlgWord(dlog, EXPARAM2, pTuplet->brackVis, False);
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
	OutlineOKButton(dlog,True);

/*--- 2. Interact with user til they push OK or Cancel. --- */

	dialogOver = 0;
	while (dialogOver==0) {
		do {
			ModalDialog(filterUPP, &ditem);							/* Handle events in dialog window */
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

			switch (ObjLType(pL)) {									/* Check new values by type */
				case GRAPHICtype:									/* NB: only GRDraw for now */
					GetDlgWord(dlog, RIGHT_VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;

					GetDlgWord(dlog, EXPARAM1, &newval);
					if (newval<1 || newval>127) {
						GetIndCString(strBuf, INFOERRS_STRS, 45);	/* "Line thickness must be..." */
						INFO_ERROR(strBuf);
					}
					break;

				case OTTAVAtype:
					GetDlgWord(dlog, EXPARAM1, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 42);	/* "noCutoff must be..." */
						INFO_ERROR(strBuf);
					}

					break;

				case TUPLETtype:
					GetDlgWord(dlog, RIGHT_VERT, &newval);
					if (!LegalVert(doc, newval)) dialogOver = 0;

					GetDlgWord(dlog, EXPARAM1, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 43);	/* "accNum Visible must be..." */
						INFO_ERROR(strBuf);
					}
					
					GetDlgWord(dlog, EXPARAM2, &newval);
					if (newval<0 || newval>1) {
						GetIndCString(strBuf, INFOERRS_STRS, 44);	/* "Bracket Visible must be..." */
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

	switch (ObjLType(pL))										/* Get new values by type */
	{
		case GRAPHICtype:										/* NB: only GRDraw for now */
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

		case OTTAVAtype:
			PushLock(OBJheap);
			ottavap = GetPOTTAVA(pL);

			GetDlgWord(dlog, LEFT_HORIZ, &newval);
			DIST_ACCEPT(ottavap->xdFirst);

			GetDlgWord(dlog, LEFT_VERT, &newval);
			DIST_ACCEPT(ottavap->ydFirst);
			ottavap->ydLast = ottavap->ydFirst;				/* We don't support diagonal octave signs yet */

			GetDlgWord(dlog, EXPARAM1, &newval);
			GRAF_ACCEPT(ottavap->noCutoff);

			GetDlgWord(dlog, RIGHT_HORIZ, &newval);
			DIST_ACCEPT(ottavap->xdLast);

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
	if (graphicDirty)	{									/* Was anything graphic changed? */
		InvalMeasure(pL, 1);
	}
	if (graphicDirty || nodeDirty) {						/* Was anything at all changed? */
		doc->changed = True;								/* Yes. */
		LinkTWEAKED(pL) = True;
		LinkVALID(pL) = False;								/* Force recalc. objRect */
	}
	
	lastEditField = GetDialogKeyboardFocusItem(dlog);		/* Save itemNum of last edit field */
	lastObjType = ObjLType(pL);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	return; 
}


/* ======================================================= ModNRInfoDialog and friends == */

enum {
	HORIZ=4,
	MVERT=6,
	DATA=8,
	SHOW_MOD,
	PREV,
	NEXT
};

#define MAX_MODNRS 50		/* Max. modifiers per note/rest we can edit */

static DialogPtr mDlog;
static Rect modNRArea;
static short modCode, noteSubType;
static Boolean noteSmall;
static DDIST noteYD;
static Boolean above, center, dirty;
static short lastModEditField=0;

static void MIDrawSlashes(short, short, short, DDIST);
static void InfoDrawModNR(void);
static pascal void UserDrawModNR(DialogPtr, short);
static void PrepDrawModNR(AMODNR, PCONTEXT);
static Boolean GetModNRValues(PAMODNR);

void MIDrawSlashes(short xhead, short ystem, short nslashes, DDIST dEighthLn)
{
	DDIST	slashLeading, slashThick, slashWidth, slashHeight;
	short	i, xslash, yslash;
	
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

#define DISP_RASTRAL 2			/* Display rastral size: arbitrary but preferably big for readability */	

static void InfoDrawModNR()
{
	short oldtxFont, oldtxSize;
	short xOffset, yOffset, sizePercent, width, height, lnSpace, yoff;
	unsigned char glyph;
	Rect portRect;
	
	EraseRect(&modNRArea);
	ClipRect(&modNRArea);

	oldtxFont = GetPortTxFont();
	oldtxSize = GetPortTxSize();
	TextFont(sonataFontNum);									/* Use Sonata font */
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

	PutDlgWord(mDlog, HORIZ, modNR.xstd-XSTD_OFFSET, False);
	PutDlgWord(mDlog, DATA, modNR.data, False);
	PutDlgWord(mDlog, MVERT, modNR.ystdpit, True);		/* Last so it can start out selected */
}


static Boolean GetModNRValues(PAMODNR aModNR)
{
	short newval;
	Boolean okay=True;
	
	GetDlgWord(mDlog, HORIZ, &newval);
	if (newval<-16 || newval>15) {
		GetIndCString(strBuf, INFOERRS_STRS, 3);			/* "Horizontal must be..." */
		CParamText(strBuf, "", "", "");
		Inform(MODNRINFO_ALRT);
		okay = False;
	}
	aModNR->xstd = newval+XSTD_OFFSET;

	GetDlgWord(mDlog, MVERT, &newval);
	if (newval<-128 || newval>127) {
		GetIndCString(strBuf, INFOERRS_STRS, 4);			/* "Vertical must be..." */
		CParamText(strBuf, "", "", "");
		Inform(MODNRINFO_ALRT);
		okay = False;
	}
	aModNR->ystdpit = newval;

	GetDlgWord(mDlog, DATA, &newval);
	if (newval<-128 || newval>127) {
		GetIndCString(strBuf, INFOERRS_STRS, 5);			/* "Data must be..." */
		CParamText(strBuf, "", "", "");
		Inform(MODNRINFO_ALRT);
		okay = False;
	}
	aModNR->data = newval;
	
	return okay;
}


/* ------------------------------------------------------------------- ModNRInfoDialog -- */
/* Handler for note/rest modifier dialog. Assumes the first selected node is a Sync.
 Returns True if dialog is OK'ed, False if cancelled or an error occurs. */

Boolean ModNRInfoDialog(Document *doc)
{
	LINK pL, aNoteL, aModNRL;
	PANOTE aNote;
	PAMODNR aModNR;
	short i, modCount, dialogOver;
	short ditem, aShort;
	short curr;
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
		return False;
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
	dirty = False;												/* Nothing changed yet */

	userDrawModNRUPP = NewUserItemUPP(UserDrawModNR);
	if (userDrawModNRUPP==NULL) {
		MissingDialog(MODNRINFO_DLOG);
		return False;
	}
	filterUPP = NewModalFilterUPP(OKButDragFilter);
	if (filterUPP == NULL) {
		DisposeUserItemUPP(userDrawModNRUPP);
		MissingDialog(MODNRINFO_DLOG);
		return False;
	}
								
	GetPort(&oldPort);
	mDlog = GetNewDialog(MODNRINFO_DLOG, NULL, BRING_TO_FRONT);
	if (!mDlog) {
		DisposeUserItemUPP(userDrawModNRUPP);
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MODNRINFO_DLOG);
		return False;
	}
	SetPort(GetDialogWindowPort(mDlog));

	/* Prepare user item to display the note modifier. First get the rectangle and hide
	it so it doesn't cover up any other items. */
	
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
	while (dialogOver==0) {
		do {
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
					if (aModNR->xstd!=modNR[i].xstd) dirty = True;
					if (aModNR->ystdpit!=modNR[i].ystdpit) dirty = True;
					if (aModNR->data!=modNR[i].data) dirty = True;
					*aModNR = modNR[i];
				}
			}
			else
				dialogOver = 0;
		}
	}

	SetPort(oldPort);
	if (dirty)	{												/* Was anything changed? */
		InvalMeasure(pL, NoteSTAFF(aNoteL));
		doc->changed = True;									/* Yes. */
		LinkTWEAKED(pL) = True;									/* Flag to show node was edited */
	}

	if (dialogOver==OK)
		lastModEditField = GetDialogKeyboardFocusItem(mDlog);
	DisposeUserItemUPP(userDrawModNRUPP);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(mDlog);										/* Free heap space */
	return (dialogOver==OK);
}


/* ---------------------------------------------------------------- XLoadInfoDialogSeg -- */
/* Null function to allow loading or unloading InfoDialog.c's segment. */

void XLoadInfoDialogSeg()
{
}
