/***************************************************************************
*	FILE:	PrefsDialog.c
*	PROJ:	Nightingale
*	DESC:	Preferences dialog routines.
/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Local prototypes */

static void		InstallPrefsCard(DialogPtr, short, Boolean, Boolean);
static pascal	Boolean PrefsFilter(DialogPtr, EventRecord *, short *);
static Boolean	AnyBadPrefs(DialogPtr, short);
static short	BuildSpaceTblMenu(UserPopUp *);
static short	BuildMusFontMenu(UserPopUp *);
static void		ChangeDlogFont(DialogPtr, Boolean);

enum {								/* dialog item numbers */
	BUT1_OK = 1,
	BUT2_Cancel,
	BUT3_Save,
	POP4_SelectCard,
	USER5_Divider,
		/* General Prefs */
	STXT6_Extra,
	EDIT7_margin,
	STXT8_inches,
	STXT9_Mouse,
	EDIT10_ShakeTresh,
	STXT11_ticks,
	STXT12_DfltTS,
	EDIT13_numer,
	EDIT14_denom,
	STXT15_DfltRastral,
	EDIT16_rastral,
	CHK17_MakeBackup,
		/* File Prefs */
	CHK18_BeamRests,
	STXT19_Use,
	POP20_SpaceTbl,
	STXT21_Use,
	POP22_MusFont,
		/* Engraver Prefs */
	EDIT23_StfLineWid,
	EDIT24_LedgerWid,
	EDIT25_BarlineWid,
	EDIT26_SlurWid,
	EDIT27_StemWid,
	EDIT28_StemLenNorm,
	EDIT29_StemLenGr,
	EDIT30_StemLen2in,
	EDIT31_StemLen2out,
	EDIT32_BeamSlope,
	EDIT33_SpcAfterBar,
	EDIT34_AccsOffset,
	
	STXT35_Postscript,
	STXT36_Staff,
	STXT37_Ledger,
	STXT38_Barline,
	STXT39_Slur,
	STXT40_StemWid,
	STXT41_StemLen,
	STXT42_StemNorm,
	STXT43_StemGr,
	STXT44_Stem2in,
	STXT45_Stem2out,
	STXT46_Relative,
	STXT47_Percent,
	STXT48_SpaceAfter,
	STXT49_AccsOffset,
	LAST_ENGRAVER_ITEM = STXT49_AccsOffset,
	LASTITEM = LAST_ENGRAVER_ITEM
	};


/* Preferences sections (correspond to choices from the popup menu) */

enum {
		PM_FirstSection = 1,		/* used to dim arrow icons that we're not using */
		PM_General = PM_FirstSection,
		PM_File,
		PM_Engraver,		
		PM_LastSection				/* 1 more than last defined section number */
	};

static UserPopUp cardPopup;		/* Select preferences card */
static UserPopUp spaceTblPopup;	/* Select space table */
static UserPopUp musFontPopup;	/* Select music font */

static Rect	borderRect;				/* Store rect of USER5_Divider for drawing */
static short currEditField = 0;	/* Field selected when dlog last closed */


void PrefsDialog(
			Document *doc,
			Boolean	/*fileUsed*/,		/* (unused) Is there a score open that has any content? */
			short		*section			/* The card showing first */
			)
{
	register DialogPtr dlog;
	GrafPtr		oldPort;
	register short dialogOver;
	short			ditem, itype;
	short			tableID,oldSpaceTable, oldmShakeThresh, temp, saveResFile, musFontOrigChoice;
	Handle		beamrestHdl, tHdl;
	Rect			tRect;
	Boolean		oldAutoRespace, oldBeamRests;
	register short count;
	ResType		tType;
	Str255		tableName, mItemName;
	Handle		tableHndl;
	double		titleMargInch, fTemp;
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;
	
	WaitCursor();

/* --- 1. Create the dialog and initialize its contents. --- */

	filterUPP = NewModalFilterUPP(PrefsFilter);
	if (filterUPP == NULL) {
		MissingDialog(PREFS_DLOG);
		return;
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(PREFS_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(PREFS_DLOG);
		return;
	}
	
	SetPort(GetDialogWindowPort(dlog));
	
	/* Store rect for drawing border and hide the userItem */
	GetDialogItem(dlog, USER5_Divider, &itype, &tHdl, &borderRect);
	HideDialogItem(dlog, USER5_Divider);

	/* Install various item values and disable unimplemented ones */
	
	/* Don't allow saves for now */
	GetDialogItem(dlog, BUT3_Save, &itype, &tHdl, &tRect);
	HiliteControl((ControlHandle)tHdl, CTL_INACTIVE);
		
	/* Init card selection popup */
	if (!InitPopUp(dlog, &cardPopup, POP4_SelectCard, 0, CARD_MENU, *section))
		goto broken;
	if (ResizePopUp(&cardPopup)) ChangePopUpChoice(&cardPopup, *section);

	titleMargInch = pt2in(config.titleMargin);
	titleMargInch = RoundDouble(titleMargInch, .01);
	PutDlgDouble(dlog, EDIT7_margin, titleMargInch, False);
	PutDlgWord(dlog, EDIT10_ShakeTresh, config.mShakeThresh, False);
	PutDlgWord(dlog, EDIT13_numer, config.defaultTSNum, False);
	PutDlgWord(dlog, EDIT14_denom, config.defaultTSDenom, False);
	PutDlgWord(dlog, EDIT16_rastral, config.defaultRastral, False);
	tHdl = PutDlgChkRadio(dlog, CHK17_MakeBackup, config.makeBackup);
	
	beamrestHdl = PutDlgChkRadio(dlog, CHK18_BeamRests, doc->beamRests);
	
	if (!InitPopUp(dlog, &spaceTblPopup, POP20_SpaceTbl, 0,
				SPACETBL_MENU, 0 /* menuChoice dummy; changed below */))
		goto broken;
	BuildSpaceTblMenu(&spaceTblPopup);
	ResizePopUp(&spaceTblPopup);

	/* Convert sticky space table res ID to menu item number */
	if (doc->spaceTable == 0)					/* if Ngale's internal table */
		spaceTblPopup.currentChoice = 1;		/* first item */
	else {
		saveResFile = CurResFile();
		UseResFile(setupFileRefNum);
		tableHndl = GetResource('SPTB', doc->spaceTable);
		if (!GoodResource(tableHndl)) {
			GetIndCString(fmtStr, PREFSERRS_STRS, 2);    /* "Nightingale can't find the current spacing table (ID=%d): it will not appear in the menu of spacing tables." */
			sprintf(strBuf, fmtStr, doc->spaceTable); 
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			Pstrcpy((unsigned char *)tableName, "\p");
		}
		else
			GetResInfo(tableHndl, &tableID, &tType, tableName);
		/* Search for a match between doc->spaceTable and menuItem strings */
		for (count = 1; count <= CountMenuItems(spaceTblPopup.menu); count++) {
			GetMenuItemText(spaceTblPopup.menu, count, mItemName);
			if (EqualString(tableName, mItemName, True, True)) {
				spaceTblPopup.currentChoice = count;
				break;
			}
		}
		UseResFile(saveResFile);
	}
	/* Draw currentChoice string. */
	ChangePopUpChoice(&spaceTblPopup, spaceTblPopup.currentChoice);
		
	if (!InitPopUp(dlog, &musFontPopup, POP22_MusFont, 0,
				MUSFONT_MENU, 0 /* menuChoice dummy; changed below */))
		goto broken;
	BuildMusFontMenu(&musFontPopup);
	ResizePopUp(&musFontPopup);
	for (count = 1; count <= CountMenuItems(musFontPopup.menu); count++) {
		GetMenuItemText(musFontPopup.menu, count, mItemName);
		if (EqualString(doc->musFontName, mItemName, True, True)) {
			musFontPopup.currentChoice = count;
			break;
		}
	}
	ChangePopUpChoice(&musFontPopup, musFontPopup.currentChoice);
	musFontOrigChoice = musFontPopup.currentChoice;

	/* Engraver Prefs */
	PutDlgWord(dlog, EDIT23_StfLineWid, config.staffLW, False);
	PutDlgWord(dlog, EDIT24_LedgerWid, config.ledgerLW, False);
	PutDlgWord(dlog, EDIT25_BarlineWid, config.barlineLW, False);
	PutDlgWord(dlog, EDIT26_SlurWid, config.slurMidLW, False);
	PutDlgWord(dlog, EDIT27_StemWid, config.stemLW, False);
	PutDlgWord(dlog, EDIT28_StemLenNorm, config.stemLenNormal, False);
	PutDlgWord(dlog, EDIT29_StemLenGr, config.stemLenGrace, False);
	PutDlgWord(dlog, EDIT30_StemLen2in, config.stemLen2v, False);
	PutDlgWord(dlog, EDIT31_StemLen2out, config.stemLenOutside, False);
	PutDlgWord(dlog, EDIT32_BeamSlope, config.relBeamSlope, False);
	PutDlgWord(dlog, EDIT33_SpcAfterBar, config.spAfterBar, False);
	PutDlgWord(dlog, EDIT34_AccsOffset, config.hAccStep, False);
	
	oldAutoRespace = doc->autoRespace;
	oldBeamRests = doc->beamRests;
	oldSpaceTable = doc->spaceTable;
	
	oldmShakeThresh = config.mShakeThresh;

	CenterWindow(GetDialogWindow(dlog), 60);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	InstallPrefsCard(dlog, *section, True, False);
	if (currEditField) SelectDialogItemText(dlog, currEditField, 0, ENDTEXT);
	
/*--- 2. Interact with user til they push OK or Cancel. --- */

	dialogOver = 0;
	while (dialogOver==0)
	{
		do
		{
			ModalDialog(filterUPP, &ditem);
			if (ditem<1 || ditem>LASTITEM) continue;
	
			/* Get initial info on item, and if checkbox, swap its value */		
			GetDialogItem(dlog, ditem, &itype, &tHdl, &tRect);
			if (itype == (ctrlItem+chkCtrl))
				SetControlValue((ControlHandle)tHdl, !GetControlValue((ControlHandle)tHdl));
	
			switch (ditem) {
				case OK:
				case Cancel:
					dialogOver = ditem;
					break;
				case POP4_SelectCard:
					InstallPrefsCard(dlog, *section, False, True);
					InstallPrefsCard(dlog, *section = cardPopup.currentChoice, True, True);
					break;
				default:
					;
			}
		}	while (!dialogOver);

/* --- 3. If dialog was terminated with OK, check any new values. --- */
/* ---    If any are illegal, keep dialog on the screen to try again. - */

		if (dialogOver==Cancel) {
			DisposePopUp(&cardPopup);
			DisposePopUp(&spaceTblPopup);	
			DisposePopUp(&musFontPopup);	
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);									/* Free heap space */
			return;
		}
		else
			if (AnyBadPrefs(dlog, cardPopup.currentChoice)) dialogOver = 0;	/* Keep dialog on screen */
	}
	
/* --- 4. Dialog was terminated with OK and all values are legal. --- */
	
	GetDlgDouble(dlog, EDIT7_margin, &fTemp);
	if (ABS(fTemp-titleMargInch)>.001) {
		config.titleMargin = in2pt(fTemp)+.5;
		if (config.titleMargin<0) config.titleMargin = 0;
		if (config.titleMargin>127) config.titleMargin = 127;
	}

	GetDlgWord(dlog, EDIT10_ShakeTresh, &temp);
	config.mShakeThresh = temp;

	GetDlgWord(dlog, EDIT13_numer, &temp);
	config.defaultTSNum = temp;

	GetDlgWord(dlog, EDIT14_denom, &temp);
	config.defaultTSDenom = temp;

	GetDlgWord(dlog, EDIT16_rastral, &temp);
	config.defaultRastral = temp;

	config.makeBackup = GetDlgChkRadio(dlog, CHK17_MakeBackup);

	doc->beamRests = GetDlgChkRadio(dlog, CHK18_BeamRests);
			
	/* Get space table res ID for current choice */
	if (spaceTblPopup.currentChoice == 1)				/* if Ngale's internal table */
		doc->spaceTable = 0;
	else {
		saveResFile = CurResFile();
		UseResFile(setupFileRefNum);
		GetMenuItemText(spaceTblPopup.menu, spaceTblPopup.currentChoice, tableName);
		tableHndl = GetNamedResource ('SPTB',  tableName);
		if (!GoodResource(tableHndl)) {
			GetIndCString(strBuf, PREFSERRS_STRS, 1);    /* "Nightingale can't find the specified spacing table: will use the default table." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			tableID = 0;
		}
		else
			GetResInfo(tableHndl, &tableID, &tType, tableName);
		doc->spaceTable = tableID;
		UseResFile(saveResFile);
	}
	if (doc->spaceTable<0) doc->spaceTable = 0;

	if (musFontPopup.currentChoice!=musFontOrigChoice) {
		GetMenuItemText(musFontPopup.menu, musFontPopup.currentChoice, mItemName);
		Pstrcpy(doc->musFontName, mItemName);
		InitDocMusicFont(doc);	// FIXME: this gives dumb error msgs for prefs dlog context...
		InvalRange(doc->headL, doc->tailL);		/* Force recomputing of objRects. ??Doesn't work! */
		InvalWindow(doc);
		doc->changed = True;
	}

	/* Engraver Prefs */
	GetDlgWord(dlog, EDIT23_StfLineWid, &temp);
	config.staffLW = temp;
	GetDlgWord(dlog, EDIT24_LedgerWid, &temp);
	config.ledgerLW = temp;
	GetDlgWord(dlog, EDIT25_BarlineWid, &temp);
	config.barlineLW = temp;
	GetDlgWord(dlog, EDIT26_SlurWid, &temp);
	config.slurMidLW = temp;
	GetDlgWord(dlog, EDIT27_StemWid, &temp);
	config.stemLW = temp;
	GetDlgWord(dlog, EDIT28_StemLenNorm, &temp);
	config.stemLenNormal = temp;
	GetDlgWord(dlog, EDIT29_StemLenGr, &temp);
	config.stemLenGrace = temp;
	GetDlgWord(dlog, EDIT30_StemLen2in, &temp);
	config.stemLen2v = temp;
	GetDlgWord(dlog, EDIT31_StemLen2out, &temp);
	config.stemLenOutside = temp;
	GetDlgWord(dlog, EDIT32_BeamSlope, &temp);
	config.relBeamSlope = temp;
	GetDlgWord(dlog, EDIT33_SpcAfterBar, &temp);
	config.spAfterBar = temp;
	GetDlgWord(dlog, EDIT34_AccsOffset, &temp);
	config.hAccStep = temp;


	/* Check if any document-specific preferences changed (other than music font, checked above) */
	if (doc->autoRespace!=oldAutoRespace
	||  doc->beamRests!=oldBeamRests
	||  doc->spaceTable!=oldSpaceTable)
			doc->changed = True;

	/* Save current menu choice and text item for next invocation of dlog. */
	*section = cardPopup.currentChoice;
	currEditField = GetDialogKeyboardFocusItem(dlog);
	
	DisposePopUp(&cardPopup);
	DisposePopUp(&spaceTblPopup);	
	DisposePopUp(&musFontPopup);	
		
broken:
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return;
}


/* Given one of the choices from the Preference card popup menu, show or
hide (as specified by <show>) all items that belong to that choice. */

void InstallPrefsCard(DialogPtr dlog, short choice, Boolean show, Boolean sel) 
{
	short i;
	
	switch (choice) {
		case PM_General:
			for (i = STXT6_Extra; i<=CHK17_MakeBackup; i++)
				ShowHideItem(dlog, i, show);
			if (sel) SelectDialogItemText(dlog, EDIT16_rastral, 0, ENDTEXT);
			break;
		case PM_File:		
			ShowHideItem(dlog, CHK18_BeamRests, show);
			ShowHideItem(dlog, STXT19_Use, show);
			ShowHideItem(dlog, STXT21_Use, show);
			EraseRect(&spaceTblPopup.shadow);
			ShowPopUp(&spaceTblPopup, show);
			EraseRect(&musFontPopup.shadow);
			ShowPopUp(&musFontPopup, show);
			if (cardPopup.currentChoice == PM_File) {
				DrawPopUp(&spaceTblPopup);
				DrawPopUp(&musFontPopup);
			}
			break;
		case PM_Engraver:
			for (i = EDIT23_StfLineWid; i<=LAST_ENGRAVER_ITEM; i++)
				ShowHideItem(dlog, i, show);
			if (sel) SelectDialogItemText(dlog, EDIT23_StfLineWid, 0, ENDTEXT);
			break;
	}
}

/* Check values of edit fields on current card.  If any are bad, give an alert and
and return True. */

static Boolean AnyBadPrefs(DialogPtr dlog, short curCard)
{
	short newval;
	register short badField, titleMargin; double fTemp; long maxMargin;
	char fmtStr[256];
	
	switch (curCard) {
		case PM_General:
			GetDlgDouble(dlog, badField = EDIT7_margin, &fTemp);
			titleMargin = in2pt(fTemp)+.5;
			if (titleMargin<0 || titleMargin>127) {
				maxMargin = 100L*pt2in(127);
				GetIndCString(fmtStr, PREFSERRS_STRS, 3);    	/* "Extra margin must be from 0 to %ld.%02ld in." */
				sprintf(strBuf, fmtStr, maxMargin/100L, maxMargin%100L); 
				goto hadError;
			}
			GetDlgWord(dlog, badField = EDIT16_rastral, &newval);
			if (newval<0 || newval>MAXRASTRAL) {
				GetIndCString(fmtStr, PREFSERRS_STRS, 4);    	/* "Staff rastral size must be from 0 to %d." */
				sprintf(strBuf, fmtStr, MAXRASTRAL); 
				goto hadError;
			}
			GetDlgWord(dlog, badField = EDIT13_numer, &newval);
			if (TSNUM_BAD(newval)) {
				GetIndCString(strBuf, PREFSERRS_STRS, 5);    	/* "Time signature numerator must be from 1 to 99." */
				goto hadError;
			}
			GetDlgWord(dlog, badField = EDIT14_denom, &newval);
			if (TSDENOM_BAD(newval)) {
				GetIndCString(strBuf, PREFSERRS_STRS, 6);    	/* "Time signature denominator must be 1, 2, 4, 8, 16, 32, or 64." */
				goto hadError;
			}
			GetDlgWord(dlog, badField = EDIT10_ShakeTresh, &newval);
			if (newval<0 || newval>127) {
				GetIndCString(strBuf, PREFSERRS_STRS, 7);			/* "Mouse shake maximum time must be from 0 to 127." */
				goto hadError;
			}
			break;
		case PM_File:
			break;
		case PM_Engraver:
			GetDlgWord(dlog, badField = EDIT34_AccsOffset, &newval);
			if (newval<0 || newval>31) {
				GetIndCString(strBuf, PREFSERRS_STRS, 8);			/* "Accidental horizontal offset must be from 0 to 31." */
				goto hadError;				
			}

			/* For the stem lengths, have a generic msg and use badField to indicate which or none */
			badField = 0;
			GetDlgWord(dlog, EDIT31_StemLen2out, &newval);
			if (newval<1) badField = EDIT31_StemLen2out;
			GetDlgWord(dlog, EDIT30_StemLen2in, &newval);
			if (newval<1) badField = EDIT30_StemLen2in;
			GetDlgWord(dlog, EDIT29_StemLenGr, &newval);
			if (newval<1) badField = EDIT29_StemLenGr;
			GetDlgWord(dlog, EDIT28_StemLenNorm, &newval);
			if (newval<1) badField = EDIT28_StemLenNorm;
			if (badField!=0) {
				GetIndCString(strBuf, PREFSERRS_STRS, 9);    	/* "All stem lengths must be at least 1." */
				goto hadError;
			}

			/* For the line widths, the same thing. */
			badField = 0;
			GetDlgWord(dlog, EDIT27_StemWid, &newval);
			if (newval<0) badField = EDIT27_StemWid;
			GetDlgWord(dlog, EDIT26_SlurWid, &newval);
			if (newval<0) badField = EDIT26_SlurWid;
			GetDlgWord(dlog, EDIT25_BarlineWid, &newval);
			if (newval<0) badField = EDIT25_BarlineWid;
			GetDlgWord(dlog, EDIT24_LedgerWid, &newval);
			if (newval<0) badField = EDIT24_LedgerWid;
			GetDlgWord(dlog, EDIT23_StfLineWid, &newval);
			if (newval<0) badField = EDIT23_StfLineWid;
			if (badField!=0) {
				GetIndCString(strBuf, PREFSERRS_STRS, 10);    	/* "Line widths cannot be negative." */
				goto hadError;
			}

			GetDlgWord(dlog, badField = EDIT32_BeamSlope, &newval);
			if (newval<0) {
				GetIndCString(strBuf, PREFSERRS_STRS, 11);    	/* "Relative beam slope cannot be negative." */
				goto hadError;				
			}

			GetDlgWord(dlog, badField = EDIT33_SpcAfterBar, &newval);
			if (newval<0) {
				GetIndCString(strBuf, PREFSERRS_STRS, 12);    	/* "Space after barline cannot be negative." */
				goto hadError;				
			}

			break;
	}
	
	/* If we've gotten this far, there aren't any errors. */
	return False;
	
hadError:
	CParamText(strBuf, "", "", "");
	StopInform(MIDIBADVALUE_ALRT);
	SelectDialogItemText(dlog, badField, 0, ENDTEXT);					/* Select field so user knows which one is bad. */
	return True;
}


pascal Boolean PrefsFilter(DialogPtr dlog, EventRecord *theEvent, short *itemHit)
{
	Point		where;
	short		lastCard;
	Boolean	ans = False;

	switch (theEvent->what) {
		case updateEvt:
			BeginUpdate(GetDialogWindow(dlog));
			
			/* First draw border lines. */
	   	MoveTo(borderRect.left, borderRect.top);  	
			LineTo(cardPopup.shadow.left-7, borderRect.top);		/* Don't cover popup box */
			MoveTo(cardPopup.shadow.right+6, borderRect.top);	
			LineTo(borderRect.right, borderRect.top); 	   
			LineTo(borderRect.right, borderRect.bottom);
			LineTo(borderRect.left, borderRect.bottom);
			LineTo(borderRect.left, borderRect.top);
			
			ChangeDlogFont(dlog, False);						/* Use system font when drawing popup */
			DrawPopUp(&cardPopup);
			if (cardPopup.currentChoice == PM_Engraver)	/* Restore current font for drawing text items */
				ChangeDlogFont(dlog, True);				

			UpdateDialogVisRgn(dlog);

			FrameDefault(dlog, OK, True);
			if (cardPopup.currentChoice == PM_File) {
				DrawPopUp(&spaceTblPopup);
				DrawPopUp(&musFontPopup);
			}
			EndUpdate(GetDialogWindow(dlog));
			*itemHit = 0;
			ans = True;
			break;
		case mouseDown:
			where = theEvent->where;
			GlobalToLocal(&where);
			
			if (PtInRect(where, &cardPopup.shadow)) {
				lastCard = cardPopup.currentChoice;
				ChangeDlogFont(dlog, False);							/* Use system font when drawing popups */
				if (DoUserPopUp(&cardPopup)) {
					*itemHit = POP4_SelectCard;
					if (AnyBadPrefs(dlog, lastCard)) {
						ChangePopUpChoice(&cardPopup, lastCard);	/* Restore old menu choice */
						*itemHit = 0;
					}
					ans = True;
				}
				else { *itemHit = 0; ans = True; }
				if (cardPopup.currentChoice == PM_Engraver)		/* Restore current font for drawing text items */
					ChangeDlogFont(dlog, True);
				break;
			}
			if (PtInRect(where, &spaceTblPopup.shadow)) {
				ChangeDlogFont(dlog, False);							/* Use system font when drawing popups */
				*itemHit = DoUserPopUp(&spaceTblPopup) ? POP20_SpaceTbl : 0;
				ans =  True;
			}
			if (PtInRect(where, &musFontPopup.shadow)) {
				ChangeDlogFont(dlog, False);							/* Use system font when drawing popups */
				*itemHit = DoUserPopUp(&musFontPopup) ? POP22_MusFont : 0;
				ans =  True;
			}
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(dlog, theEvent, itemHit, False)) return True;
			break;
		default:
			;
	}

	return (ans);
}


/* BuildSpaceTblMenu() reads any SPTB resources in any open resource file and 
puts their names in the popup menu.  We really want the SPTB's from the Prefs
file, but if there aren't any there (unlikely), it shouldn't hurt to get
them from another file (very likely the application); anyway, there's no
Get1Menu to make it easy to look only at the Prefs file. It'd be nice to store
resource ID's in a struct to pass back, but we don't do that yet. */
	
static short BuildSpaceTblMenu(UserPopUp *popup)
{
	short		saveResFile;
		
	ReleaseResource((Handle)popup->menu);
	popup->menu = GetMenu(SPACETBL_MENU);
	
	saveResFile = CurResFile();
	UseResFile(setupFileRefNum);

	AppendResMenu(popup->menu, 'SPTB');

	UseResFile(saveResFile);
	
	return True;
}


/* BuildMusFontMenu() reads 'BBX#' resources in the application's rsrc fork and 
puts their names in the popup menu. */
	
static short BuildMusFontMenu(UserPopUp *popup)
{
	short		saveResFile;
		
	ReleaseResource((Handle)popup->menu);
	popup->menu = GetMenu(MUSFONT_MENU);

	saveResFile = CurResFile();
	UseResFile(setupFileRefNum);

	AppendResMenu(popup->menu, 'BBX#');

	UseResFile(saveResFile);
	
	return True;
}


static void ChangeDlogFont(DialogPtr dlog, Boolean useSmallFont)
{
	register TEHandle	teHndl;
	
	teHndl = GetDialogTextEditHandle(dlog);

	/* Show text in textFontNum-10 */
	if (useSmallFont) {
		SetDlgFont(dlog, textFontNum, textFontSize-2, 0);
		(**teHndl).lineHeight = 13;
		(**teHndl).fontAscent = 10;
	}
	/* Or show text in system font */
	else {
		SetDlgFont(dlog, systemFont, 12, 0);
		(**teHndl).lineHeight = 16;
		(**teHndl).fontAscent = 12;
	}
}
