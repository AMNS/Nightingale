/* ExtractHighLevel.c for Nightingale - high-level routines for extracting parts */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Copyright © 2016 by Avian Music Notation Foundation.
 * All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void DimSpacePanel(DialogPtr dlog, short item);
static pascal void UserDimPanel(DialogPtr d, short dItem);
static Boolean ExtractDialog(unsigned char *, Boolean *, Boolean *, Boolean *, short *);
static void NormalizePartFormat(Document *doc);
static void ReformatPart(Document *, short, Boolean, Boolean, short);
static Boolean BuildPartFilename(Document *doc, LINK partL, unsigned char *name);
static Boolean CopyHeaderFooter(Document *dstDoc, char headerStr[], char footerStr[], Boolean isHeader);

/* ------------------------------------------------- ExtractDialog and DoExtract -- */

static enum {
	EXTRACTALL_DI=4,
	EXTRACTONE_DI,
	WHICHONE_DI,
	SAVE_DI=8,
	OPEN_DI,
	REFORMAT_DI,
	SPACE_DI=12,
	SPACEBOX_DI=14
} E_ExtractItems;


/* Maintain state of items subordinate to the Respace and Reformat checkbox. If that
checkbox is not checked, hide its subordinate edit field, and dim the whole area.
Otherwise, do the  opposite. We should set the userItems' handle to be a pointer to
this procedure so it'll be called automatically by the dialog filter. */
		
static void DimSpacePanel(DialogPtr dlog,
							short item)		/* the userItem number */
{
	short		type;
	Handle		hndl;
	Rect		box, tempR;
	Boolean		checked;
	
	checked = GetDlgChkRadio(dlog, REFORMAT_DI);

	if (checked) {
		ShowDialogItem(dlog, SPACE_DI);
	}
	else {
		HideDialogItem(dlog, SPACE_DI);
		GetHiddenDItemBox(dlog, SPACE_DI, &tempR);
		InsetRect(&tempR, -3, -3);
		FrameRect(&tempR);								
		
		PenPat(NGetQDGlobalsGray());
		PenMode(patBic);	
		GetDialogItem(dlog, item, &type, &hndl, &box);
		PaintRect(&box);								/* Dim everything within userItem rect */
		PenNormal();
	}
}


/* User Item-drawing procedure to dim subordinate check boxes and text items */

static pascal void UserDimPanel(DialogPtr d, short dItem)
{
	DimSpacePanel(d, dItem);
}


static Boolean ExtractDialog(unsigned char *partName, Boolean *pAll, Boolean *pSave,
									Boolean *pReformat, short *pSpacePercent)
{	
	DialogPtr dlog;
	short ditem;  short radio1, radio2;
	GrafPtr oldPort;
	short aShort;  Handle aHdl;  Rect spacePanelBox;
	short newSpace;
	Boolean dialogOver=FALSE;
	UserItemUPP userDimUPP;
	ModalFilterUPP filterUPP;
	
	userDimUPP = NewUserItemUPP(UserDimPanel);
	if (userDimUPP==NULL) {
		MissingDialog((long)EXTRACT_DLOG);  /* Misleading, but this isn't likely to happen. */
		return FALSE;
	}
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		DisposeUserItemUPP(userDimUPP);
		MissingDialog((long)EXTRACT_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(EXTRACT_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeUserItemUPP(userDimUPP);
		DisposeModalFilterUPP(filterUPP);
		MissingDialog((long)EXTRACT_DLOG);
		return FALSE;
	}
	
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	PutDlgString(dlog,WHICHONE_DI,partName, FALSE);

	radio1 = (*pAll ? EXTRACTALL_DI : EXTRACTONE_DI);
	PutDlgChkRadio(dlog, radio1, TRUE);
	radio2 = (*pSave ? SAVE_DI : OPEN_DI);
	PutDlgChkRadio(dlog, radio2, TRUE);
	PutDlgChkRadio(dlog, REFORMAT_DI, *pReformat);
	GetDialogItem(dlog, SPACEBOX_DI, &aShort, &aHdl, &spacePanelBox);
	SetDialogItem(dlog, SPACEBOX_DI, userItem, (Handle)userDimUPP, &spacePanelBox);
	PutDlgWord(dlog, SPACE_DI, *pSpacePercent, TRUE);

	CenterWindow(GetDialogWindow(dlog), 55);
	ShowWindow(GetDialogWindow(dlog));
	DimSpacePanel(dlog, SPACEBOX_DI);			/* Prevent flashing if subordinates need to be dimmed right away */
	ArrowCursor();

	dialogOver = FALSE;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
				GetDlgWord(dlog, SPACE_DI, (short *)&newSpace);
				if (newSpace<MINSPACE || newSpace>MAXSPACE)
					Inform(SPACE_ALRT);
				else
					dialogOver = TRUE;
				break;
			case Cancel:
				dialogOver = TRUE;
				break;
			case REFORMAT_DI:
				PutDlgChkRadio(dlog, REFORMAT_DI, !GetDlgChkRadio(dlog, REFORMAT_DI));
				InvalWindowRect(GetDialogWindow(dlog), &spacePanelBox);	/* Force filter to call DimSpacePanel */
				break;
			case EXTRACTALL_DI:
			case EXTRACTONE_DI:
				SwitchRadio(dlog, &radio1, ditem);
				break;
			case SAVE_DI:
			case OPEN_DI:
				SwitchRadio(dlog, &radio2, ditem);
				break;
			default:
				;
		}
	}
		
	/* The dialog is over and everything's legal. If it was OKed, pick up new values. */
	
	if (ditem==OK) {
		*pAll = GetDlgChkRadio(dlog, EXTRACTALL_DI);
		*pSave = GetDlgChkRadio(dlog, SAVE_DI);
		*pReformat = GetDlgChkRadio(dlog, REFORMAT_DI);
		GetDlgWord(dlog, SPACE_DI, (short *)pSpacePercent);
	}
	
	DisposeDialog(dlog);
	DisposeUserItemUPP(userDimUPP);
	DisposeModalFilterUPP(filterUPP);
	SetPort(oldPort);
	
	return (ditem==OK);
}


/* ----------------------------------------------------------------------------------- */

/* Normalize the part's format by making all staves visible and giving all systems
the same dimensions. For details of what we do to measure and system positions and
bounding boxes, see comments in ExFixMeasAndSysRects. */

static void NormalizePartFormat(Document *doc)
{
	LINK pL;
	
	/*
	 * Show all hidden staves in <doc>. This is much simpler than the similar routine
	 * for the Show All Staves command because here we know that a single system
	 * can't overflow a page. However, NB: we're still dealing with several systems per
	 * page, and the lowest may well overflow. Reformatting after this will be all
	 * that's needed in the vast majority of cases. In extreme cases, we may produce
	 * coordinates before we reformat that overflow DDISTs and signed ints, in which
	 * case Reformat will fail. It seems that this will be very rare, though.  
	 */
	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL))
		if (StaffTYPE(pL)) {
			VisifyAllStaves(doc, pL);
		}

	/*
	 * VisifyAllStaves set measureRects from the systemRects, so these are still set
	 * for the original score. Now we need to correct staff positions, measureRects
	 * and systemRects.
	 */
	ExFixMeasAndSysRects(doc);								/* OK since all staves are visible */
}
					

/* Reformat the newly-extracted part: show all hidden staves, adjust measure and
system positions and bounding boxes, reformat to update page breaks, and optionally
reformat to update system breaks. */

static void ReformatPart(Document *doc, short spacePercent, Boolean changeSBreaks,
					Boolean careMeasPerSys, short measPerSys)
{
	LINK pL, firstDelL, lastDelL;
	
	NormalizePartFormat(doc);

	InitAntikink(doc, doc->headL, doc->tailL);
	pL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, FALSE);		/* Start at first measure */
	RespaceBars(doc, pL, doc->tailL,
					RESFACTOR*(long)spacePercent, FALSE, FALSE);	/* Don't reformat! */
	doc->spacePercent = spacePercent;
	Antikink();														/* FIXME: SHOULD DO AFTER Reformat! */

	Reformat(doc, RightLINK(doc->headL), doc->tailL,
				changeSBreaks, (careMeasPerSys? measPerSys : 9999),
				FALSE, 999, config.titleMargin);

	(void)DelRedTimeSigs(doc, TRUE, &firstDelL, &lastDelL);
}


static Boolean BuildPartFilename(Document *doc, LINK partL, unsigned char *partFName)
{
	int wantLen;
	char tmpStr[256], nameStr[256], extStr[32];
	Boolean okay, tooLong;

	/* Convert Pascal to C strings; manipulate C strings, and convert back at the end. */
	PToCString(Pstrcpy((unsigned char *)tmpStr, doc->name));

	GetFinalSubstring(tmpStr, extStr, '.');
    wantLen = strlen(tmpStr)-strlen(extStr)-1;				/* -1 to leave out the "." */
    okay = GetInitialSubstring(tmpStr, nameStr, wantLen);
    if (okay) {
        //LogPrintf(LOG_DEBUG, "BuildPartFilename: OK. tmpStr='%s' wantLen=%d name='%s' ext='%s'\n",
		//			tmpStr, wantLen, nameStr, extStr);
    } else {
        //LogPrintf(LOG_DEBUG, "BuildPartFilename: Not OK. tmpStr=%s wantLen=%d.\n", tmpStr, wantLen);
		return FALSE;
	}
	strcat(nameStr, "-");
	
	/* With the Carbon toolkit, filenames can't be longer than the modest length MacOS 9
		allows; if we're going to exceed it, shorten the part name. NB: This won't handle
		every case: in fact, the score filename might already have the maximum length.
		But if worst comes to worst, the file I/O library and/or OS will shorten it to
		the legal limit. And since we might exceed the limit regardless, always keep at
		least 2 chars. of the part name to increase the odds of getting a meaningful
		default name. FIXME: But there's still a good chance of getting meaningless
		default names, or, worse, two parts getting the same default name! It'd be
		better to shorten both the score filename _and_ the part name, or let the user
		replace the score filename with a (presumably shorter) name, e.g., in the Extract
		dialog. */
	short maxNameLen = FILENAME_MAXLEN-strlen(nameStr)-strlen(extStr)-1;
	if (maxNameLen<2) maxNameLen = 2;
	strncat(nameStr, PartNAME(partL), maxNameLen);
	strcat(nameStr, ".");
	strcat(nameStr, extStr);
	//LogPrintf(LOG_DEBUG, "BuildPartFilename: tmpStr='%s' extStr='%s' maxNameLen=%d nameStr='%s' len=%d\n",
	//	tmpStr, extStr, maxNameLen, nameStr, strlen(nameStr));

	tooLong = (strlen(nameStr)>FILENAME_MAXLEN);
	CToPString(nameStr);
	Pstrcpy(partFName, (unsigned char *)nameStr);
	
	return tooLong;
}


/* Copy the header or footer strings from one document to another. */

static Boolean CopyHeaderFooter(Document *dstDoc, char headerStr[], char footerStr[], Boolean isHeader)
{
	STRINGOFFSET hOffset, fOffset, newOffset;
	Str255 string;
	
	if (isHeader) {
		strcpy((char *)string, headerStr);
		CToPString((char *)string);
		hOffset = dstDoc->headerStrOffset;
		if (hOffset > 0)
			newOffset = PReplace(hOffset, string);
		else
			newOffset = PStore(string);
		if (newOffset < 0L) {
			NoMoreMemory();
			return FALSE;
		}
		dstDoc->headerStrOffset = newOffset;
	}
	else {
		strcpy((char *)string, footerStr);
		CToPString((char *)string);
		fOffset = dstDoc->footerStrOffset;
		if (fOffset > 0)
			newOffset = PReplace(fOffset, string);
		else
			newOffset = PStore(string);
		if (newOffset < 0L) {
			NoMoreMemory();
			return FALSE;
		}
		dstDoc->footerStrOffset = newOffset;
	}

	return TRUE;
}


/* Extract part(s) into separate document(s) and optionally reformat and/or save
it/them. If all goes well, return TRUE; if there's a problem, return FALSE. */

Boolean DoExtract(Document *doc)
{
	Str255 partName, name, str;
	short spacePercent, numParts;
	LINK partL;  PPARTINFO pPart;
	LINK selPartList[MAXSTAVES+1];
	Document *partDoc;
	Boolean useHeaderFooter;
	Boolean keepGoing=TRUE;
	static Boolean allParts=TRUE, closeAndSave=FALSE, reformat=TRUE;
	static Boolean firstCall=TRUE, careMeasPerSys;
	static short measPerSys;

	/* Prepare and run the Extract dialog to find out what user wants */
	GetSelPartList(doc, selPartList);
	numParts = CountSelParts(selPartList);
	if (numParts==1) {
		partL = selPartList[0];
		pPart = GetPPARTINFO(partL);
		strcpy((char *)partName, (strlen(pPart->name)>16? pPart->shortName : pPart->name));
		CToPString((char *)partName);
	}
	else {
		GetIndString(str, MiscStringsID, 18);				/* "selected parts" */
		Pstrcpy(partName, str);
	}

	/* It's not easy to guess what a reasonable amount of squeezing is, but we'll try. */
	spacePercent = doc->spacePercent;
	if (spacePercent>100) spacePercent = 100;
	if (!ExtractDialog(partName, &allParts, &closeAndSave, &reformat, &spacePercent))
		return FALSE;

	/* Dialog was OK'd: the user really wants one or more parts exxtracted. */
	
	if (firstCall) {
		careMeasPerSys = FALSE;  measPerSys = 4;  firstCall = FALSE;
	}
	
	char headerStr[256], footerStr[256];
	STRINGOFFSET hOffset, fOffset;
	StringPtr pStr;

	/* Save the score's header and footer strings (as C strings), if any, for later use. */
	useHeaderFooter = doc->useHeaderFooter;
	if (doc->useHeaderFooter) {
		hOffset = doc->headerStrOffset;
		pStr = PCopy(hOffset);
		if (pStr[0]!=0) {
			Pstrcpy((StringPtr)headerStr, (StringPtr)pStr);
			PToCString((StringPtr)headerStr);
			//LogPrintf(LOG_DEBUG, "page header='%s'\n", headerStr);
		}

		fOffset = doc->footerStrOffset;
		pStr = PCopy(fOffset);
		if (pStr[0]!=0) {
			Pstrcpy((StringPtr)footerStr, (StringPtr)pStr);
			PToCString((StringPtr)footerStr);
			//LogPrintf(LOG_DEBUG, "page footer='%s'\n", footerStr);
		}
	}

	DeselAll(doc);
	
	partL = FirstSubLINK(doc->headL);
	for (partL=NextPARTINFOL(partL); partL && keepGoing; partL=NextPARTINFOL(partL)) {
		if (allParts || IsSelPart(partL, selPartList)) {
			if (CheckAbort()) {
				ProgressMsg(SKIPPARTS_PMSTR, "");
				SleepTicks(150L);								/* So user can read the msg */
				ProgressMsg(0, "");
				goto Done;
			}
			
			/* Construct filename for the part. */
			BuildPartFilename(doc, partL, name);

			WaitCursor();
			/* FIXME: If <doc> has been saved, <doc->vrefnum> seems to be correct, but if
			it hasn't been, <doc->vrefnum> seems to be zero; I'm not sure if that's safe. */
			partDoc = CreatePartDoc(doc, name, doc->vrefnum, &doc->fsSpec, partL);
			if (partDoc) {
				if (reformat) {
					ReformatPart(partDoc, spacePercent, TRUE, careMeasPerSys,
										measPerSys);
				}
				else
					NormalizePartFormat(partDoc);

				if (useHeaderFooter) {
					CopyHeaderFooter(partDoc, headerStr, footerStr, TRUE);
					CopyHeaderFooter(partDoc, headerStr, footerStr, FALSE);
					partDoc->useHeaderFooter = TRUE;
				}

				/* Finally, set empty selection just after the first Measure, and put
					caret there, on the top staff if the part has multiple staves. */
				
				SetDefaultSelection(partDoc);
				partDoc->selStaff = 1;
				
				MEAdjustCaret(partDoc, FALSE);		// FIXME: AND REMOVE CALL IN CreatePartDoc!
				AnalyzeWindows();
				if (closeAndSave)
					{ if (!DoCloseDocument(partDoc)) keepGoing = FALSE; }
				 else
					DoUpdate(partDoc->theWindow);
				}
			 else
				keepGoing = FALSE;

			/*
			 *	Installation of correct heaps here is not yet understood. This
			 *	seems always to leave the heaps in the correct state, yet a call to
			 *	InstallDoc in DoCloseDocument when deleting the undo data
			 *	structure is required as of implementing this to prevent crashing.
			 *	It is totally unclear what this code has to do with deleting the undo
			 *	data structure when closing documents, or how the incorrect state of
			 *	heap installation is connected with this.
			 *
			 *	We require this call to insure that the for (partL= ... ) loop
			 *	correctly traverses the score's partinfo list, since SaveParts
			 *	must leave the parts heaps installed in order to display the
			 *	part document.
			 */
			InstallDoc(doc);
		}
	}

Done:
	return TRUE;
}

