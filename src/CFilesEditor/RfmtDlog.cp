/***************************************************************************
	FILE:	RfmtDlog.c
	PROJ:	Nightingale, rev. for v.3.5
	DESC:	Routines for ReformatDialog.

	ReformatDialog					DimSysPanel				UserDimSysPanel
	DimTitleMarginPanel			UserDimTMPanel
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

Boolean reformatFirstSys;


/* -------------------------------------------------------------- ReformatDialog -- */
/* Handle Reformat dialog. Return TRUE if OK, FALSE if Cancel. */

static void DimSysPanel(DialogPtr dlog, short item);
static pascal void UserDimSysPanel(DialogPtr theWindow, short dItem);
static void DimTitleMarginPanel(DialogPtr dlog, short item);
static pascal void UserDimTMPanel(DialogPtr theWindow, short dItem);

static enum {								/* Dialog item numbers */
	BUT1_OK = 1,
	BUT2_Cancel,
	CHK4_ChangeSysBreaks=4,
	RAD5_AtMost,
	EDIT6_NMeasPerSys,
	CHK8_Justify=8,
	RAD10_SysPerPage=10,
	EDIT11_NSysPerPage,
	RAD13_AsManyAsFit=13,
	RAD14_Exactly,
	USER15_SysSubordRect,				/* Must follow items within its bounds */
	RAD16_AsManySysAsFit,
	EDIT18_TitleMargin=18,
	USER20_TitleMarginRect=20,			/* Must follow items within its bounds */	
	LASTITEM=USER20_TitleMarginRect
} E_RfmtItems;

#define MINPERSYS 1						/* Minimum legal measures per system: must be >0 */
#define MAXPERSYS 50						/* Maximum legal measures per system */

Boolean ReformatDialog(
				short startSys, short endSys,	/* Starting and ending system nos. (ending<0 => all systems) */
				Boolean *pSBreaks,				/* Change system breaks? */
				Boolean *pCareMPS,				/* Does user care about no. of meas. per system? */
				Boolean *pExactMPS,				/* Does user want exactly the spec. no. of meas./system? */
				short	*pNMPS,						/* Max. measures desired per system */
				Boolean *pJustify,				/* Justify afterwards? */
				Boolean *pCareSPP,				/* Does user care about no. of systems per page? */
				short	*pSPP, 						/* Max. systems desired per page */
				short	*pTitleMargin				/* Extra margin at top of 1st page (points) */
				)
{
	short			ditem, itype, newMPS, newJustify, newSPP, newTitleMargin,
					group1, group2;
	Handle		tHdl;
	Rect			tRect, sysPanelBox, titleMarginPanelBox;
	DialogPtr	dlog;
	GrafPtr		oldPort;
	Boolean		newSBreaks, newCareMPS, newCareSPP, newExactMPS,
					dialogOver;
	char			titleStr[256], fmtStr[256];
	double		titleMargInch, fTemp; long maxMargin;
	UserItemUPP userSysUPP, userTMUPP;
	ModalFilterUPP filterUPP;
	
	userSysUPP = NewUserItemUPP(UserDimSysPanel);
	userTMUPP = NewUserItemUPP(UserDimTMPanel);
	if (userSysUPP==NULL || userTMUPP==NULL) {
		MissingDialog(REFORMAT_DLOG);  /* Missleading, but this isn't likely to happen. */
		return FALSE;
	}
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP==NULL) {
		DisposeUserItemUPP(userSysUPP);
		DisposeUserItemUPP(userTMUPP);
		MissingDialog(REFORMAT_DLOG);
		return FALSE;
	}
	
	GetPort(&oldPort);
	dlog = GetNewDialog(REFORMAT_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		CenterWindow(GetDialogWindow(dlog), 65);
		
		/* Fill in dialog's values */
		
		if (endSys<0) {
			GetIndCString(titleStr, RFMTDLOG_STRS, 2);    	/* "Reformat all systems:" */
		}
		else if (startSys==endSys) {
			GetIndCString(fmtStr, RFMTDLOG_STRS, 3);    		/* "Reformat system %d:" */
			sprintf(titleStr, fmtStr, startSys); 
		}
		else {
			GetIndCString(fmtStr, RFMTDLOG_STRS, 4);    		/* "Reformat systems %d through %d:" */
			sprintf(titleStr, fmtStr, startSys, endSys); 
		}
		CParamText(titleStr, "", "", "");
		PutDlgChkRadio(dlog,CHK4_ChangeSysBreaks, *pSBreaks);
		
		reformatFirstSys = (startSys<=1 || endSys<0);

		if (!*pCareMPS)		group1 = RAD13_AsManyAsFit;
		else if (*pExactMPS) group1 = RAD14_Exactly;
		else						group1 = RAD5_AtMost;
		PutDlgChkRadio(dlog, group1, TRUE);

		group2 = (*pCareSPP? RAD10_SysPerPage : RAD16_AsManySysAsFit);
		PutDlgChkRadio(dlog, group2, TRUE);
		
		PutDlgChkRadio(dlog, CHK8_Justify, *pJustify);

		/* Stuff text items; select only if "owning" checkbox is checked. */
		PutDlgWord(dlog, EDIT6_NMeasPerSys, *pNMPS, FALSE);
		PutDlgWord(dlog, EDIT11_NSysPerPage, *pSPP, !*pSBreaks);

		titleMargInch = pt2in(*pTitleMargin);
		titleMargInch = RoundDouble(titleMargInch, .01);
		PutDlgDouble(dlog, EDIT18_TitleMargin, titleMargInch, FALSE);

		if (*pSBreaks) SelectDialogItemText(dlog, EDIT6_NMeasPerSys, 0, ENDTEXT);
		
		GetDialogItem(dlog, USER15_SysSubordRect, &itype, &tHdl, &sysPanelBox);
		GetDialogItem(dlog, USER20_TitleMarginRect, &itype, &tHdl, &titleMarginPanelBox);
		SetDialogItem(dlog, USER15_SysSubordRect, 0, (Handle)userSysUPP, &sysPanelBox);
		SetDialogItem(dlog, USER20_TitleMarginRect, 0, (Handle)userTMUPP, &titleMarginPanelBox);
		
		ShowWindow(GetDialogWindow(dlog));

		/* Handle initial dimming of both "panels". */

		DimSysPanel(dlog, USER15_SysSubordRect);
		DimTitleMarginPanel(dlog, USER20_TitleMarginRect);

		ArrowCursor();
				
		dialogOver = FALSE;
		do {
			while (TRUE) {
				ModalDialog(filterUPP, &ditem);
				if (ditem<1 || ditem>LASTITEM) continue;
				
				/* Get initial info on item, and if checkbox, toggle its value */		
				GetDialogItem(dlog, ditem, &itype, &tHdl, &tRect);
				if (itype == (ctrlItem+chkCtrl))
					SetControlValue((ControlHandle)tHdl, !GetControlValue((ControlHandle)tHdl));

				switch (ditem) {
					case OK:
					case Cancel:
						dialogOver = TRUE;
						goto Dismissed;
					case CHK4_ChangeSysBreaks:
						InvalWindowRect(GetDialogWindow(dlog),&sysPanelBox);	/* force filter to call DimSysPanel */
						break;
					case RAD5_AtMost:
					case RAD14_Exactly:
					case RAD13_AsManyAsFit:
						if (ditem!=group1)
							SwitchRadio(dlog, &group1, ditem);
						break;
						/* If user clicks in text item, hilite associated radio button */
					case EDIT6_NMeasPerSys:
						if (group1!=RAD14_Exactly && group1!=RAD5_AtMost)
							SwitchRadio(dlog, &group1, (*pExactMPS? RAD14_Exactly : RAD5_AtMost));
						break;
					case RAD10_SysPerPage:
					case RAD16_AsManySysAsFit:
						if (ditem!=group2)
							SwitchRadio(dlog, &group2, ditem);
						break;
						/* If user clicks in text item, hilite associated radio button */
					case EDIT11_NSysPerPage:
						if (group2!=RAD10_SysPerPage)
							SwitchRadio(dlog, &group2, RAD10_SysPerPage);
						break;
					default:
						;
				}
			}

Dismissed:
			if (ditem == OK) {
				GetDlgWord(dlog, EDIT6_NMeasPerSys, &newMPS);
				GetDlgWord(dlog, EDIT11_NSysPerPage, &newSPP);
				/* Complain about bad value, but only if MeasPerSys is relevant */
				if (GetDlgChkRadio(dlog, CHK4_ChangeSysBreaks)) {
					if (newMPS<MINPERSYS || newMPS>MAXPERSYS) {
						GetIndCString(fmtStr, RFMTDLOG_STRS, 1);    	/* "Number of meas per system must be..." */
						sprintf(strBuf, fmtStr, MINPERSYS, MAXPERSYS); 
						CParamText(strBuf,	"", "", "");
						StopInform(GENERIC_ALRT);
						dialogOver = FALSE;
						}
					}
				GetDlgDouble(dlog, EDIT18_TitleMargin, &fTemp);
				newTitleMargin = *pTitleMargin;							/* In case it's unchanged */				
				if (ABS(fTemp-titleMargInch)>.001) {
					newTitleMargin = in2pt(fTemp)+.5;
					if (newTitleMargin<0 || newTitleMargin>127) {
						maxMargin = 100L*pt2in(127);
						GetIndCString(fmtStr, PREFSERRS_STRS, 3);    /* "Extra margin must be from 0 to %ld.%02ld in." */
						sprintf(strBuf, fmtStr, maxMargin/100L, maxMargin%100L); 
						CParamText(strBuf,	"", "", "");
						StopInform(GENERIC_ALRT);
						dialogOver = FALSE;
						}
					}
				}
			 else
				break;
		
		} while (!dialogOver);
	
		newSBreaks = GetDlgChkRadio(dlog, CHK4_ChangeSysBreaks);
		newCareMPS = (group1!=RAD13_AsManyAsFit);
		newExactMPS = (group1==RAD14_Exactly);
		newCareSPP = (group2!=RAD16_AsManySysAsFit);
		
		newJustify = GetDlgChkRadio(dlog, CHK8_Justify);
		
		DisposeDialog(dlog);
	}
	 else {
		MissingDialog(REFORMAT_DLOG);
		ditem = Cancel;
	}

	DisposeUserItemUPP(userSysUPP);
	DisposeUserItemUPP(userTMUPP);
	DisposeModalFilterUPP(filterUPP);
	
	if (ditem==OK) {
		*pSBreaks = newSBreaks;
		*pCareMPS = newCareMPS;
		*pExactMPS = newExactMPS;
		*pNMPS = newMPS;
		*pJustify = newJustify;

		*pCareSPP = newCareSPP;
		*pSPP = newSPP;
		*pTitleMargin = newTitleMargin;
	}
	SetPort(oldPort);
	return (ditem==OK);
}


/* Maintain state of items subordinate to the Change System Breaks checkbox. If
that checkbox is not checked, un-hilite its subordinate radio buttons and checkbox,
hide the edit field, dim the whole area.  Otherwise, do the  opposite. The
userItem's handle should be set to a pointer to this procedure so it'll be called
automatically by the dialog filter. */
		
static void DimSysPanel(DialogPtr dlog, short item)
{
	short				type;
	Handle			hndl, hndl1, hndl2, hndl3, hndl4;
	Rect				box, tempR;
	Boolean			checked;
	
	checked = GetDlgChkRadio(dlog, CHK4_ChangeSysBreaks);

	GetDialogItem(dlog, RAD13_AsManyAsFit, &type, &hndl1, &box);
	GetDialogItem(dlog, RAD14_Exactly, &type, &hndl2, &box);
	GetDialogItem(dlog, RAD5_AtMost, &type, &hndl3, &box);
	GetDialogItem(dlog, CHK8_Justify, &type, &hndl4, &box);
	if (checked) {
		HiliteControl((ControlHandle)hndl1, CTL_ACTIVE);
		HiliteControl((ControlHandle)hndl2, CTL_ACTIVE);
		HiliteControl((ControlHandle)hndl3, CTL_ACTIVE);
		HiliteControl((ControlHandle)hndl4, CTL_ACTIVE);
		ShowDialogItem(dlog, EDIT6_NMeasPerSys);
	}
	else {
		HiliteControl((ControlHandle)hndl1, CTL_INACTIVE);
		HiliteControl((ControlHandle)hndl2, CTL_INACTIVE);
		HiliteControl((ControlHandle)hndl3, CTL_INACTIVE);
		HiliteControl((ControlHandle)hndl4, CTL_INACTIVE);
		HideDialogItem(dlog, EDIT6_NMeasPerSys);
		GetHiddenDItemBox(dlog, EDIT6_NMeasPerSys, &tempR);
		InsetRect(&tempR, -3, -3);
		FrameRect(&tempR);								
		
		PenPat(NGetQDGlobalsGray());	
		PenMode(patBic);	
		GetDialogItem(dlog, item, &type, &hndl, &box);
		PaintRect(&box);													/* Dim everything within userItem rect */
		PenNormal();
	}
}


/* User Item-drawing proc. to dim subordinate check boxes and text items */

static pascal void UserDimSysPanel(DialogPtr d, short dItem)
{
	DimSysPanel(d, dItem);
}


/* Maintain state of items for setting title margin: if we're not in the "not
reformatting the score's first system" state, just replace the editText box with an
empty rectange, and grey out the whole area. This is all the machinery necessary if
the state can't change during the dialog. */

static void DimTitleMarginPanel(DialogPtr dlog, short item)
{
	short				type;
	Handle			hndl;
	Rect				box, tempR;
	
	if (reformatFirstSys) return;
	
	HideDialogItem(dlog, EDIT18_TitleMargin);
	GetHiddenDItemBox(dlog, EDIT18_TitleMargin, &tempR);
	InsetRect(&tempR, -3, -3);
	FrameRect(&tempR);								
	
	PenPat(NGetQDGlobalsGray());	
	PenMode(patBic);	
	GetDialogItem(dlog, item, &type, &hndl, &box);
	PaintRect(&box);													/* Dim everything within userItem rect */
	PenNormal();
}

/* User Item-drawing proc. to dim title margin "panel". */

static pascal void UserDimTMPanel(DialogPtr d, short dItem)
{
	DimTitleMarginPanel(d, dItem);
}
