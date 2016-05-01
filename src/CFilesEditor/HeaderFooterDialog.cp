/***************************************************************************
*	FILE:	HeaderFooterDialog.c
*	PROJ:	Nightingale
*	DESC:	Code for Header/footer dialog, which replaces the old Page Number
*			dialog. Written by John Gibson.
***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


static enum {
	EVERYBUT_DI = 4,												/* Item numbers */
	EVERY_DI,
	NEVER_DI,
	TOP_DI,
	BOTTOM_DI,
	LEFT_DI,
	CENTER_DI,
	RIGHT_DI,
	FIRSTNUM_DI = 13,
	ALTERNATE_DI,
	TOFFSET_DI = 20,
	BOFFSET_DI,
	LOFFSET_DI,
	ROFFSET_DI,
	MOREOPTIONS_DI,
	DIVIDER_DI,
	FIRST_HIDEABLE_ITEM = DIVIDER_DI,
	HDRLEFT_DI = 30,
	HDRCENTER_DI,
	HDRRIGHT_DI,
	FTRLEFT_DI = 37,
	FTRCENTER_DI,
	FTRRIGHT_DI,
	PNHINT_DI,
	LAST_HIDEABLE_ITEM = PNHINT_DI
} E_HeaderFooterItems;

#define PAGENUM_CHAR '%'				// FIXME: should read this from GetIndString((StringPtr)str, HEADERFOOTER_STRS, 3);
#define HEADERFOOTER_DELIM_CHAR	0x01
#define MAX_HEADERFOOTER_STRLEN	253		/* 255 for Pascal string minus 2 delimiter chars */

static short showMoreOptions;
static short hPosGroup, vPosGroup;
static short cancelButLeft;
static short okButLeft;
static short okButMoreOptionsTop;
static short okButFewerOptionsTop;

	
/* ------------------------------------------------------- GetHeaderFooterStrings -- */
/* Unpack the header or footer string stored in the data structure, and parse it
into left, center, and right strings.  <leftStr>, <centerStr>, and <rightStr> are
Pascal strings allocated by the caller.  (The data structure's Pascal string has
either zero chars or exactly two delimiter chars with other chars interspersed.) */

Boolean GetHeaderFooterStrings(
		Document		*doc,
		StringPtr	leftStr,
		StringPtr	centerStr,
		StringPtr	rightStr,
		short			pageNum,
		Boolean		usePageNum,
		Boolean		isHeader)
{
	short				i, j, count;
	StringPtr		pSrc, pDst;
	Str31				pageNumStr;
	STRINGOFFSET	offset;
	
	leftStr[0] = centerStr[0] = rightStr[0] = 0;
	
	offset = (isHeader? doc->headerStrOffset : doc->footerStrOffset);
	pSrc = PCopy(offset);
	if (pSrc[0] == 0)
		return TRUE;										/* not an error for string to be empty */

	if (usePageNum)
		NumToString(pageNum, pageNumStr);

	pDst = leftStr;
	count = 0;
	for (i = 1; i <= pSrc[0]; i++) {
		if (pSrc[i] == HEADERFOOTER_DELIM_CHAR) {
			pDst[0] = count;
			count = 0;
			if (pDst == leftStr)
				pDst = centerStr;
			else if (pDst == centerStr)
				pDst = rightStr;
			else
				return FALSE;								/* "can't happen" */
		}
		else if (pSrc[i] == PAGENUM_CHAR && usePageNum) {
			for (j = 1; j <= pageNumStr[0]; j++) {
				count++;
				pDst[count] = pageNumStr[j];
			}
		}
		else {
			count++;
			pDst[count] = pSrc[i];
		}
	}
	pDst[0] = count;

// say("retrieving: %p => %p | %p | %p\n", pSrc, leftStr, centerStr, rightStr);

	return TRUE;
}


/* ----------------------------------------------------- StoreHeaderFooterStrings -- */
/* Pack the given strings into a single Pascal string, separated by a delimiter char.
Store this string in the string pool, and store the resulting string offset into <doc>. */

static Boolean StoreHeaderFooterStrings(
		Document	*doc,
		Str255	leftStr,
		Str255	centerStr,
		Str255	rightStr,
		Boolean	isHeader)
{
	Str255			string, delim;
	STRINGOFFSET	offset, newOffset;
	
	/* Stuff the 3 strings, separated by a delimiter char, into one Pascal string. */
	if (leftStr[0] + centerStr[0] + rightStr[0] > MAX_HEADERFOOTER_STRLEN) {
		// program error
		return FALSE;
	}
	delim[0] = 1;
	delim[1] = HEADERFOOTER_DELIM_CHAR;
	string[0] = 0;
	PStrCat(string, leftStr);
	PStrCat(string, delim);
	PStrCat(string, centerStr);
	PStrCat(string, delim);
	PStrCat(string, rightStr);
//say("storing: %p\n", string);

	/* Store in string pool. */
	offset = (isHeader? doc->headerStrOffset : doc->footerStrOffset);
	if (offset > 0)
		newOffset = PReplace(offset, string);
	else
		newOffset = PStore(string);
	if (newOffset < 0L) {
		NoMoreMemory();
		return FALSE;
	}
	if (isHeader)
		doc->headerStrOffset = newOffset;
	else
		doc->footerStrOffset = newOffset;

	return TRUE;
}


/* --------------------------------------------------------------- SetOptionState -- */
/* Set appearance/visibility/etc. of dialog items depending on whether the user
wants fewer or more options (i.e., whether they want to see the header/footer
text fields).  The current state is held in a static global, <showMoreOptions>.
SetOptionState sets the dialog items to match <optionState>.

If user switches to full options, and all of the header/footer text fields
are empty, then set the appropriate field to contain the page number code
character (depending on the state of the left/center/right and top/bottom
radio buttons). */

static void SetOptionState(DialogPtr dlog)
{
	short		i, type, strID, top;
	Handle	hndl;
	Rect		box,portRect;
	Str255	str;
	Boolean	enableControl;

	/* Enable or disble control items in use only when using fewer options. */
	enableControl = !showMoreOptions;
	XableControl(dlog, TOP_DI, enableControl);
	XableControl(dlog, BOTTOM_DI, enableControl);
	XableControl(dlog, LEFT_DI, enableControl);
	XableControl(dlog, CENTER_DI, enableControl);
	XableControl(dlog, RIGHT_DI, enableControl);

	/* Change title of "More options" button. */
	strID = showMoreOptions? 2 : 1;				/* 2: "Fewer Options..."; 1: "More Options..." */
	GetIndString((StringPtr)str, HEADERFOOTER_STRS, strID);
	GetDialogItem(dlog, MOREOPTIONS_DI, &type, &hndl, &box);
	SetControlTitle((ControlHandle)hndl, str);

	/* Set visibility of Header/footer items. */
	if (showMoreOptions) {
		for (i = FIRST_HIDEABLE_ITEM; i <= LAST_HIDEABLE_ITEM; i++)
			ShowDialogItem(dlog, i);
	}
	else {
		for (i = FIRST_HIDEABLE_ITEM; i <= LAST_HIDEABLE_ITEM; i++)
			HideDialogItem(dlog, i);
	}
	
	/* Set position of OK and Cancel buttons. */
	top = showMoreOptions? okButMoreOptionsTop : okButFewerOptionsTop;
	OutlineOKButton(dlog, FALSE);
	MoveDialogControl(dlog, OK, okButLeft, top);
	OutlineOKButton(dlog, TRUE);
	MoveDialogControl(dlog, Cancel, cancelButLeft, top);

	/* Set size of dialog window. */
	GetDialogItem(dlog, OK, &type, &hndl, &box);
	GetWindowPortBounds(GetDialogWindow(dlog),&portRect);
	SizeWindow(GetDialogWindow(dlog), portRect.right, box.bottom + 10, TRUE);
}


/* ------------------------------------------------------------ FixTextFontAscent -- */
/* Adjust lineHeight and fontAscent of dlog's TextEdit handle, so that it works
well with our 10pt application font. */
	
static void FixTextFontAscent(DialogPtr dlog)
{
	TEHandle teHndl = GetDialogTextEditHandle(dlog);
	(**teHndl).lineHeight = 13;
	(**teHndl).fontAscent = 10;
}


/* ----------------------------------------------------------- HeaderFooterDialog -- */
/* Conduct dialog to get page header and footer info.  Delivers FALSE on
Cancel or error, TRUE on OK. */

Boolean HeaderFooterDialog(Document *doc)
{
	DialogPtr	dlog;
	short			showGroup, oldShowGroup, oldVPosGroup, oldHPosGroup,
					topOffset, bottomOffset, leftOffset, rightOffset,
					ditem, dialogOver, type;
	short			firstNumber, alternate;
	GrafPtr		oldPort;
	Handle		hndl;
	Rect			box;
	Str255		oldLeftHdrStr, oldCenterHdrStr, oldRightHdrStr,
					oldLeftFtrStr, oldCenterFtrStr, oldRightFtrStr,
					leftHdrStr, centerHdrStr, rightHdrStr,
					leftFtrStr, centerFtrStr, rightFtrStr;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(HEADERFOOTER_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(HEADERFOOTER_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(HEADERFOOTER_DLOG);
		return FALSE;
	}
		
	SetPort(GetDialogWindowPort(dlog));
	
	SetDlgFont(dlog, applFont, 10, 0);
	FixTextFontAscent(dlog);		

	/* Set up radio button groups. For now, we don't pay attention to the actual
	 *	value of doc->startPageNumber, we just distinguish 3 relationships between
	 *	it and doc->firstPageNumber.
	 */
	if (doc->firstPageNumber >= doc->startPageNumber)
		showGroup = EVERY_DI;
	else if (doc->firstPageNumber + 1 == doc->startPageNumber)
		showGroup = EVERYBUT_DI;
	else
		showGroup = NEVER_DI;
	PutDlgChkRadio(dlog, showGroup, TRUE);
	oldShowGroup = showGroup;
	
	vPosGroup = (doc->topPGN? TOP_DI : BOTTOM_DI);
	PutDlgChkRadio(dlog, vPosGroup, TRUE);
	oldVPosGroup = vPosGroup;
	
	if (doc->hPosPGN == LEFT_SIDE)
		hPosGroup = LEFT_DI;
	else if (doc->hPosPGN == CENTER)
		hPosGroup = CENTER_DI;
	else
		hPosGroup = RIGHT_DI;
	PutDlgChkRadio(dlog, hPosGroup, TRUE);
	oldHPosGroup = hPosGroup;	

	PutDlgChkRadio(dlog, ALTERNATE_DI, doc->alternatePGN);

	firstNumber = doc->firstPageNumber;
	PutDlgWord(dlog, FIRSTNUM_DI, firstNumber, FALSE);

	PutDlgWord(dlog, TOFFSET_DI, doc->headerFooterMargins.top, FALSE);
	PutDlgWord(dlog, BOFFSET_DI, doc->headerFooterMargins.bottom, FALSE);
	PutDlgWord(dlog, LOFFSET_DI, doc->headerFooterMargins.left, FALSE);
	PutDlgWord(dlog, ROFFSET_DI, doc->headerFooterMargins.right, FALSE);
	
	if (!GetHeaderFooterStrings(doc, oldLeftHdrStr, oldCenterHdrStr, oldRightHdrStr,
																						0, FALSE, TRUE)) {
		//what?
	}
	if (!GetHeaderFooterStrings(doc, oldLeftFtrStr, oldCenterFtrStr, oldRightFtrStr,
																						0, FALSE, FALSE)) {
		//what?
	}
	PutDlgString(dlog, HDRLEFT_DI, oldLeftHdrStr, FALSE);
	PutDlgString(dlog, HDRCENTER_DI, oldCenterHdrStr, FALSE);
	PutDlgString(dlog, HDRRIGHT_DI, oldRightHdrStr, FALSE);
	PutDlgString(dlog, FTRLEFT_DI, oldLeftFtrStr, FALSE);
	PutDlgString(dlog, FTRCENTER_DI, oldCenterFtrStr, FALSE);
	PutDlgString(dlog, FTRRIGHT_DI, oldRightFtrStr, FALSE);

	SelectDialogItemText(dlog, FIRSTNUM_DI, 0, ENDTEXT);

	/* Set globals that help us toggle more/fewer options state of dlog. */
	GetDialogItem(dlog, Cancel, &type, &hndl, &box);
	cancelButLeft = box.left;
	GetDialogItem(dlog, OK, &type, &hndl, &box);
	okButLeft = box.left;
	okButMoreOptionsTop = box.top;
	GetDialogItem(dlog, DIVIDER_DI, &type, &hndl, &box);
	okButFewerOptionsTop = box.bottom + 5;

	showMoreOptions = doc->useHeaderFooter;
	SetOptionState(dlog);

	CenterWindow(GetDialogWindow(dlog), 45);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	dialogOver = 0;
	while (dialogOver == 0) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
				GetDlgString(dlog, HDRLEFT_DI, leftHdrStr);
				GetDlgString(dlog, HDRCENTER_DI, centerHdrStr);
				GetDlgString(dlog, HDRRIGHT_DI, rightHdrStr);
				GetDlgString(dlog, FTRLEFT_DI, leftFtrStr);
				GetDlgString(dlog, FTRCENTER_DI, centerFtrStr);
				GetDlgString(dlog, FTRRIGHT_DI, rightFtrStr);

				if (!PStrCmp(oldLeftHdrStr, leftHdrStr)
						|| !PStrCmp(oldCenterHdrStr, centerHdrStr)
						|| !PStrCmp(oldRightHdrStr, rightHdrStr)
						|| !PStrCmp(oldLeftFtrStr, leftFtrStr)
						|| !PStrCmp(oldCenterFtrStr, centerFtrStr)
						|| !PStrCmp(oldRightFtrStr, rightFtrStr)) {
					if (leftHdrStr[0] + centerHdrStr[0] + rightHdrStr[0] > MAX_HEADERFOOTER_STRLEN) {
						char strBuf[256];
						GetIndCString(strBuf, HEADERFOOTER_STRS, 4);		/* "The three header text boxes together must not..." */
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, HDRLEFT_DI, 0, ENDTEXT);
						continue;
					}
					else if (leftFtrStr[0] + centerFtrStr[0] + rightFtrStr[0] > MAX_HEADERFOOTER_STRLEN) {
						char strBuf[256];
						GetIndCString(strBuf, HEADERFOOTER_STRS, 5);		/* "The three footer text boxes together must not..." */
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, FTRLEFT_DI, 0, ENDTEXT);
						continue;
					}
					else {
						StoreHeaderFooterStrings(doc, leftHdrStr, centerHdrStr, rightHdrStr, TRUE);
						StoreHeaderFooterStrings(doc, leftFtrStr, centerFtrStr, rightFtrStr, FALSE);
						doc->changed = TRUE;
					}
				}
				
				GetDlgWord(dlog, TOFFSET_DI, &topOffset);
				GetDlgWord(dlog, BOFFSET_DI, &bottomOffset);
				GetDlgWord(dlog, LOFFSET_DI, &leftOffset);
				GetDlgWord(dlog, ROFFSET_DI, &rightOffset);
				GetDlgWord(dlog, FIRSTNUM_DI, &firstNumber);

				if (showGroup!=oldShowGroup
						||	showMoreOptions!=doc->useHeaderFooter
						|| topOffset!=doc->headerFooterMargins.top
						|| bottomOffset!=doc->headerFooterMargins.bottom
						|| leftOffset!=doc->headerFooterMargins.left
						|| rightOffset!=doc->headerFooterMargins.right
						|| firstNumber!=doc->firstPageNumber
						|| vPosGroup!=oldVPosGroup
						|| hPosGroup!=oldHPosGroup
						|| GetDlgChkRadio(dlog, ALTERNATE_DI)!=doc->alternatePGN) {
					doc->useHeaderFooter = showMoreOptions;
					doc->headerFooterMargins.top = topOffset;
					doc->headerFooterMargins.bottom = bottomOffset;
					doc->headerFooterMargins.left = leftOffset;
					doc->headerFooterMargins.right = rightOffset;
					doc->firstPageNumber = firstNumber;
					if (doc->firstPageNumber>999) doc->firstPageNumber = 999;
					if (showGroup==EVERY_DI)
						doc->startPageNumber = doc->firstPageNumber;
					else if (showGroup==EVERYBUT_DI)
						doc->startPageNumber = doc->firstPageNumber + 1;
					else
						doc->startPageNumber = SHRT_MAX;

					doc->topPGN = (vPosGroup==TOP_DI);

					if (hPosGroup==LEFT_DI)			 doc->hPosPGN = LEFT_SIDE;
					else if (hPosGroup==CENTER_DI) doc->hPosPGN = CENTER;
					else									 doc->hPosPGN = RIGHT_SIDE;
					
					doc->alternatePGN = GetDlgChkRadio(dlog, ALTERNATE_DI);

					doc->changed = TRUE;
				}																		/* drop thru... */
			case Cancel:
				dialogOver = ditem;
				break;
			case EVERYBUT_DI:
			case EVERY_DI:
			case NEVER_DI:
				if (ditem!=showGroup) SwitchRadio(dlog, &showGroup, ditem);
				break;
			case TOP_DI:
			case BOTTOM_DI:
				if (ditem!=vPosGroup) SwitchRadio(dlog, &vPosGroup, ditem);
				break;
			case LEFT_DI:
			case CENTER_DI:
			case RIGHT_DI:
				if (ditem!=hPosGroup) SwitchRadio(dlog, &hPosGroup, ditem);
				if (GetDlgChkRadio(dlog,ALTERNATE_DI) && hPosGroup==CENTER_DI)
					PutDlgChkRadio(dlog,ALTERNATE_DI,FALSE);	
				break;
			case ALTERNATE_DI:
				alternate = !GetDlgChkRadio(dlog, ALTERNATE_DI);
				PutDlgChkRadio(dlog, ALTERNATE_DI, alternate);	
				if (alternate && hPosGroup==CENTER_DI)
					SwitchRadio(dlog, &hPosGroup, LEFT_DI);
				break;
			case MOREOPTIONS_DI:
				showMoreOptions = !showMoreOptions;
				SetOptionState(dlog);
				break;
			default:
				;
		}
	}
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);

	SetPort(oldPort);
	return (dialogOver==OK);
}

