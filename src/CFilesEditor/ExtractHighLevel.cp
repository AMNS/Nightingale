/* ExtractHighLevel.c for Nightingale - high-level routines for extracting parts */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean ExtractDialog(unsigned char *, Boolean *, Boolean *, Boolean *, short *);

/* ------------------------------------------------- ExtractDialog and DoExtract -- */

#ifdef VIEWER_VERSION

Boolean DoExtract(doc)
	Document *doc;
	{
		return FALSE;
	}
	
#else

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
	short			type;
	Handle		hndl;
	Rect			box, tempR;
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
		PaintRect(&box);													/* Dim everything within userItem rect */
		PenNormal();
	}
}

/* User Item-drawing procedure to dim subordinate check boxes and text items */

static pascal void UserDimPanel(DialogPtr d, short dItem);
static pascal void UserDimPanel(DialogPtr d, short dItem)
{
	DimSpacePanel(d, dItem);
}


static Boolean ExtractDialog(unsigned char *partName, Boolean *pAll, Boolean *pSave,
										Boolean *pReformat, short *pSpacePercent)
{	
	DialogPtr dlog;
	short ditem; short radio1, radio2;
	GrafPtr oldPort;
	short aShort; Handle aHdl; Rect spacePanelBox;
	short newSpace;
	Boolean dialogOver=FALSE;
	UserItemUPP userDimUPP;
	ModalFilterUPP filterUPP;
	
	userDimUPP = NewUserItemUPP(UserDimPanel);
	if (userDimUPP==NULL) {
		MissingDialog((long)EXTRACT_DLOG);  /* Missleading, but this isn't likely to happen. */
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
				PutDlgChkRadio(dlog,REFORMAT_DI,!GetDlgChkRadio(dlog,REFORMAT_DI));
				InvalWindowRect(GetDialogWindow(dlog),&spacePanelBox);					/* force filter to call DimSpacePanel */
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


/* ------------------------------------------------------------------------------ */

/* Normalize the part's format by making all staves visible and giving all systems
the same dimensions. For details of what we do to measure and system positions and
bounding boxes, see comments in ExFixMeasAndSysRects. */

static void NormalizePartFormat(Document *doc);
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
	ExFixMeasAndSysRects(doc);												/* OK since all staves are visible */
}
					

/* Reformat the newly-extracted part: show all hidden staves, adjust measure and
system positions and bounding boxes, reformat to update page breaks, and optionally
reformat to update system breaks. */

static void ReformatPart(Document *, short, Boolean, Boolean, short);
static void ReformatPart(Document *doc, short spacePercent, Boolean changeSBreaks,
					Boolean careMeasPerSys, short measPerSys)
{
	LINK pL, firstDelL, lastDelL;
	
	NormalizePartFormat(doc);

	InitAntikink(doc, doc->headL, doc->tailL);
	pL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, FALSE); /* Start at first measure */
	RespaceBars(doc, pL, doc->tailL,
					RESFACTOR*(long)spacePercent, FALSE, FALSE);		/* Don't reformat! */
	doc->spacePercent = spacePercent;
	Antikink();															/* ??SHOULD BE AFTER Reformat! */

	Reformat(doc, RightLINK(doc->headL), doc->tailL,
				changeSBreaks, (careMeasPerSys? measPerSys : 9999),
				FALSE, 999, config.titleMargin);

#ifdef LIGHT_VERSION
	EnforcePageLimit(doc);
#endif

	(void)DelRedTimeSigs(doc, TRUE, &firstDelL, &lastDelL);
}


/* Extract parts into separate documents and optionally reformat and/or save them.
If all goes well, return TRUE; if there's a problem, return FALSE. */

Boolean DoExtract(Document *doc)
	{
		Str255 partName, name;
		short spacePercent, numParts;
		LINK partL; PPARTINFO pPart;
		LINK selPartList[MAXSTAVES+1];
		Document *partDoc;
		Boolean keepGoing=TRUE;
		static Boolean allParts=TRUE, closeAndSave=FALSE, reformat=TRUE;
		static Boolean firstCall=TRUE, careMeasPerSys;
		static short measPerSys;
		
		GetSelPartList(doc, selPartList);

		numParts = CountSelParts(selPartList);
		if (numParts==1) {
			partL = selPartList[0];
			pPart = GetPPARTINFO(partL);
			strcpy((char *)partName, (strlen(pPart->name)>16? pPart->shortName : pPart->name));
		}
		else
			strcpy((char *)partName, "selected parts");		// FIXME: put in STR# rsrc
		CToPString((char *)partName);

		spacePercent = doc->spacePercent;
		if (!ExtractDialog(partName, &allParts, &closeAndSave, &reformat, &spacePercent))
			return FALSE;

		if (firstCall) {
			careMeasPerSys = FALSE; measPerSys = 4; firstCall = FALSE;
		}
		
		DeselAll(doc);

#ifdef NOTYET
		/* If not enough free documents to hold the new parts, consult with user */
		
		numDocs = NumFreeDocuments();
		partL = FirstSubLINK(doc->headL);
		if (allParts)
			for (numParts=0,partL=NextPARTINFOL(partL); partL; partL=NextPARTINFOL(partL))
				numParts++;
		if (numDocs<numParts)
			give caution alert("Not enough free documents for all parts; go ahead anyway?")
#endif
		
		partL = FirstSubLINK(doc->headL);
		for (partL=NextPARTINFOL(partL); partL && keepGoing; partL=NextPARTINFOL(partL)) {
			
			if (allParts || IsSelPart(partL, selPartList)) {
				if (CheckAbort()) {
					ProgressMsg(SKIPPARTS_PMSTR, "");
					SleepTicks(90L);										/* So user can read the msg */
					ProgressMsg(0, "");
					goto Done;
				}
				PToCString(Pstrcpy(name,doc->name));
				strcat((char *)name,"-");
				pPart = GetPPARTINFO(partL);
				strcat((char *)name,pPart->name);
				CToPString((char *)name);
	
				WaitCursor();
				/* ??If <doc> has been saved, <doc->vrefnum> seems to be correct, but if
				it hasn't been, <doc->vrefnum> seems to be zero; I'm not sure if that's safe. */
				partDoc = CreatePartDoc(doc,name,doc->vrefnum,&doc->fsSpec,partL);
				if (partDoc) {
					if (reformat) {
						ReformatPart(partDoc, spacePercent, TRUE, careMeasPerSys,
											measPerSys);
#ifdef LIGHT_VERSION
						if (partDoc->numSheets>MAXPAGES) {
							/* Already given LIGHTVERS_MAXPAGES_ALRT in EnforcePageLimit. Unlikely
								that we'll get a part that needs more pages than the score, but you
								never know, if spacePercent is huge. */
							partDoc->changed = FALSE;
							DoCloseDocument(partDoc);
							InstallDoc(doc);					/* See comment below about this. */
							continue;
						}
#endif
					}
					else
						NormalizePartFormat(partDoc);

					/* Finally, set empty selection just after the first Measure and put caret there.
						??Will this necessarily be on the screen? */
					
					SetDefaultSelection(partDoc);
					partDoc->selStaff = 1;
					
					MEAdjustCaret(partDoc, FALSE);		// ??AND REMOVE CALL IN CreatePartDoc!
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
				 *	seems to leave the heaps in the correct state, yet a call to
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

#endif /* VIEWER_VERSION */

