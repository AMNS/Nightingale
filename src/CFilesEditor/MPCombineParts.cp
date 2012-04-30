/* ExtractHighLevel.c for Nightingale - high-level routines for extracting parts */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean CombinePartsDlog(unsigned char *, Boolean *, Boolean *, Boolean *, short *);

/* ------------------------------------------------- CombinePartsDlog and DoExtract -- */

#ifdef VIEWER_VERSION

Boolean DoCombineParts(Document *doc, LINK *firstPartL, LINK *lastPartL)
	{
		return FALSE;
	}
	
#else

//static enum {
//	EXTRACTALL_DI=4,
//	EXTRACTONE_DI,
//	WHICHONE_DI,
//	SAVE_DI=8,
//	OPEN_DI,
//	REFORMAT_DI,
//	SPACE_DI=12,
//	SPACEBOX_DI=14
//} E_ExtractItems;

static enum {
	COMBINEINPLACE_DI=4,
	EXTRACTTOSCORE_DI,
	//WHICHONE_DI,
	SAVE_DI=8,
	OPEN_DI,
	REFORMAT_DI,
	SPACE_DI=12,
	SPACEBOX_DI=14
} E_CombineItems;

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


static Boolean CombinePartsDlog(unsigned char *partName, Boolean *pInPlace, Boolean *pSave,
										Boolean *pReformat, short *pSpacePercent)
{	
	DialogPtr dlog;
	short ditem; INT16 radio1, radio2;
	GrafPtr oldPort;
	short aShort; Handle aHdl; Rect spacePanelBox;
	short newSpace;
	Boolean dialogOver=FALSE;
	UserItemUPP userDimUPP;
	ModalFilterUPP filterUPP;
	
	userDimUPP = NewUserItemUPP(UserDimPanel);
	if (userDimUPP==NULL) {
		MissingDialog((long)COMBINEPARTS_DLOG);  /* Missleading, but this isn't likely to happen. */
		return FALSE;
	}
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		DisposeUserItemUPP(userDimUPP);
		MissingDialog((long)COMBINEPARTS_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(COMBINEPARTS_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeUserItemUPP(userDimUPP);
		DisposeModalFilterUPP(filterUPP);
		MissingDialog((long)COMBINEPARTS_DLOG);
		return FALSE;
	}
	
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

//	PutDlgString(dlog,WHICHONE_DI,partName, FALSE);

	radio1 = (*pInPlace ? COMBINEINPLACE_DI : EXTRACTTOSCORE_DI);
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
				GetDlgWord(dlog, SPACE_DI, (INT16 *)&newSpace);
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
			case COMBINEINPLACE_DI:
			case EXTRACTTOSCORE_DI:
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
		*pInPlace = GetDlgChkRadio(dlog, COMBINEINPLACE_DI);
		*pSave = GetDlgChkRadio(dlog, SAVE_DI);
		*pReformat = GetDlgChkRadio(dlog, REFORMAT_DI);
		GetDlgWord(dlog, SPACE_DI, (INT16 *)pSpacePercent);
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

static void ReformatPart(Document *, short, Boolean, Boolean, INT16);
static void ReformatPart(Document *doc, short spacePercent, Boolean changeSBreaks,
					Boolean careMeasPerSys, INT16 measPerSys)
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

/* Need one document to hold combined parts */

static Boolean EnoughFreeDocs()
	{
#ifdef NOTYET
		INT16 numDocs = NumFreeDocuments();
		
		if (numDocs < 1) return FALSE;
#endif

		return TRUE;		
	}
	
static INT16 MaxRelVoice(Document *doc, INT16 partn)
	{
		INT16 maxrv = -1;
		
		for (INT16 v = 0; v<=MAXVOICES; v++) 
		{
			INT16 vpartn = doc->voiceTab[v].partn;
			if (vpartn == partn) 
			{
				if (doc->voiceTab[v].relVoice > maxrv)
					maxrv = doc->voiceTab[v].relVoice;
			}
		}
	
		return maxrv;
	}

static void FixVoicesForPart(Document *doc, LINK destPartL, LINK partL, INT16 stfDiff)
	{
		INT16 partn = PartL2Partn(doc, partL);
		INT16 dpartn = PartL2Partn(doc, destPartL);
		
		INT16 firstStf = PartFirstSTAFF(partL);
		INT16 lastStf = PartLastSTAFF(partL);		
		INT16 v;
		
		/* Fix up all the base relvoices */
		for (v = firstStf; v<=lastStf; v++) {
			doc->voiceTab[v].relVoice += stfDiff;		
			doc->voiceTab[v].partn = dpartn;
		}
		
		/* Clear all other relvoices for this part */		
		for (v = 0; v<MAXVOICES+1; v++) 
		{
			if (v<firstStf || v>lastStf) 
			{
				INT16 vpartn = doc->voiceTab[v].partn;
				if (vpartn == partn) 
				{
					doc->voiceTab[v].relVoice = -1;
					doc->voiceTab[v].partn = dpartn;
				}				
			}
		}
		
		INT16 maxrv = MaxRelVoice(doc, dpartn) + 1;
		
		/* Fix up all the remaining relvoices */
		for (v = 0; v<MAXVOICES+1; v++) 
		{
			INT16 vpartn = doc->voiceTab[v].partn;
			if (vpartn == dpartn) 
			{
				if (doc->voiceTab[v].relVoice < 0)
				{
					doc->voiceTab[v].relVoice = maxrv++;
				}
			}
		}
	}
	
static Boolean CheckMultivoiceRoles(Document *doc, LINK firstPartL, LINK lastPartL)
	{
		Boolean ok = TRUE;
		Boolean hasUpperVoice = FALSE;
		Boolean hasLowerVoice = FALSE;
		
		LINK partL = firstPartL;
		while (ok && partL != lastPartL) 		/* handle all the parts after the first */
		{
			INT16 partn = PartL2Partn(doc, partL);
			for (INT16 v = 0; v<MAXVOICES+1; v++) 
			{
				if (doc->voiceTab[v].voiceRole == UPPER_DI) 
				{
					if (hasUpperVoice)
						ok = FALSE;
					else
						hasUpperVoice = TRUE;
				}
						
				if (doc->voiceTab[v].voiceRole == LOWER_DI) 
				{
					if (hasLowerVoice)
						ok = FALSE;
					else
						hasLowerVoice = TRUE;
				}
			}
			
			partL = NextPARTINFOL(partL);			
		}
	
		return ok;
	}

static INT16 FixStavesForPart(LINK firstPartL, LINK partL)
	{
		PartLastSTAFF(firstPartL) = PartLastSTAFF(partL);
		INT16 stfDiff = PartFirstSTAFF(partL) - PartFirstSTAFF(firstPartL);
		return stfDiff;
	}
	
static void DeletePartInfoList(Document *doc, LINK firstPartL, LINK lastPartL) 
	{
		LINK delPartL = NextPARTINFOL(firstPartL);
		NextPARTINFOL(firstPartL) = NextPARTINFOL(lastPartL);
		NextPARTINFOL(lastPartL) = NILINK;
		
		INT16 nparts = 0;
		LINK partL = delPartL;
		while (partL) 
		{
			nparts++; partL = NextPARTINFOL(partL);
		}
			
		HeapFree(doc->Heap+HEADERtype,delPartL);
		LinkNENTRIES(doc->masterHeadL) -= nparts;
	}
	
static void DeleteConnectSubObjs(LINK connectL) 
{
	INT16 nEntries = LinkNENTRIES(connectL);
	LINK pSubL = FirstSubLINK(connectL);
	for ( ; pSubL; pSubL=NextCONNECTL(pSubL))
		if (ConnectSEL(pSubL))
			nEntries--;
		
	if (nEntries == 0) {		
		MayErrMsg("Combine Parts: attempt to delete all connect subobjects");
		return;
	}
	else  {
		LinkNENTRIES(connectL) = nEntries;		
	}

	/* Traverse the subList and delete selected subObjects. */

	pSubL = FirstSubLINK(connectL);
	while (pSubL) {
		if (ConnectSEL(pSubL)) {
			LINK tempL = NextCONNECTL(pSubL);
			RemoveLink(connectL, CONNECTheap, FirstSubLINK(connectL), pSubL);
			HeapFree(CONNECTheap,pSubL);
			pSubL = tempL;
		}
		else pSubL = NextCONNECTL(pSubL);					
	}	
}

static Boolean UpdateConnect(LINK connectL, LINK partL)
	{
		Boolean connectSel = FALSE;
			
		INT16 firstStf = PartFirstSTAFF(partL);
		INT16 lastStf = PartLastSTAFF(partL);		
		
		LINK aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) 
		{
			PACONNECT aConnect = GetPACONNECT(aConnectL);
			if (aConnect->connLevel==PartLevel && aConnect->staffAbove==firstStf) {
				aConnect->staffBelow = lastStf;
			}
			else if (aConnect->connLevel==PartLevel && aConnect->staffAbove>firstStf && aConnect->staffBelow <= lastStf) 
			{
				aConnect->selected = TRUE;
				aConnect->staffBelow = lastStf;
				connectSel = TRUE;
			}		
		}
		
		return connectSel;
	}

static void FixConnectsForPart(Document *doc, LINK firstPartL)
	{
		LINK connectL,aConnectL;
		
		INT16 firstStf = PartFirstSTAFF(firstPartL);
		INT16 lastStf = PartLastSTAFF(firstPartL);		

		Boolean connectSel = FALSE;
		
		connectL = SSearch(doc->masterHeadL,CONNECTtype,GO_RIGHT);
		
		connectSel = UpdateConnect(connectL, firstPartL);
		if (connectSel) 
		{
			LinkSEL(connectL) = TRUE;
			DeleteConnectSubObjs(connectL);
		}
/*		
		connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
		
		for ( ; connectL; connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) {
			aConnectL = FirstSubLINK(connectL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) 
			{
				connectSel = UpdateConnect(connectL, firstPartL);
				if (connectSel) 
				{
					LinkSEL(connectL) = TRUE;
				}				
			}
		}	
		
		connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
		
		for ( ; connectL; connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) {
			aConnectL = FirstSubLINK(connectL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) 
			{
				connectSel = LinkSEL(connectL);
				if (connectSel) 
				{
					DeleteConnectSubObjs(connectL);
				}				
			}
		}	
*/
	}

static void CombineInPlace(Document *doc, LINK firstPartL, LINK lastPartL)
	{
		if (!CheckMultivoiceRoles(doc, firstPartL, lastPartL)) 
		{
			MayErrMsg("Combine Parts: Conflicting voice roles. Can't combine parts with more than one upper or lower voice");
			return;
		}
		
		LINK partL = firstPartL;
		while (partL != lastPartL) 		/* handle all the parts after the first */
		{
			partL = NextPARTINFOL(partL);
			
			INT16 stfDiff = FixStavesForPart(firstPartL, partL);
						
			FixVoicesForPart(doc, firstPartL, partL, stfDiff);
		}
		
		
		
		DeletePartInfoList(doc, firstPartL, lastPartL);		
	}


static void ExtractToScore(Document *doc, LINK firstPartL, LINK lastPartL)
	{
		if (!EnoughFreeDocs()) 
		{
			return;
		}
#ifdef NOTYET		
		
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
#endif

	}
	
static void MPGetSelPartsList(Document *doc, LINK partL[])
{
	INT16			selParts[MAXSTAVES+1];
	INT16			selStaves[MAXSTAVES+1];
	INT16			i, j;
	
	for (i = 0; i<=MAXSTAVES; i++) {
		selStaves[i] = FALSE;
		selParts[i] = FALSE;
		partL[i] = NILINK;
	}
	
	LINK staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	LINK aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) 
	{
		if (StaffSEL(aStaffL)) { 
			selStaves[StaffSTAFF(aStaffL)] = TRUE;
		}		
	}

	for (i = 1; i<=MAXSTAVES; i++)
		if (selStaves[i]) {
			j = Staff2Part(doc, i);
			selParts[j] = TRUE;
		}

	for (i = 1, j = 0; i<=MAXSTAVES; i++)
		if (selParts[i])  {
			partL[j++] = FindPartInfo(doc, i);			
		}
}

/* Extract parts into separate documents and optionally reformat and/or save them.
If all goes well, return TRUE; if there's a problem, return FALSE. */

Boolean DoCombineParts(Document *doc)
	{
		Str255 partName;
		//Str255 name;
		short spacePercent, numParts;
		LINK partL; PPARTINFO pPart;
		LINK selPartList[MAXSTAVES+1];
		//Document *partDoc;
		Boolean keepGoing=TRUE;
		static Boolean inPlace=TRUE, closeAndSave=FALSE, reformat=TRUE;
		static Boolean firstCall=TRUE, careMeasPerSys;
		static INT16 measPerSys;
		
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
		if (!CombinePartsDlog(partName, &inPlace, &closeAndSave, &reformat, &spacePercent))
			return FALSE;

		if (firstCall) {
			careMeasPerSys = FALSE; measPerSys = 4; firstCall = FALSE;
		}
		
		DeselAll(doc);

		/* MAS disabling
			Ask Don & Charlie: where is first & lastPartL coming from here?
		if (inPlace) {
			CombineInPlace(doc, firstPartL, lastPartL);
		}
		else {
			ExtractToScore(doc, firstPartL, lastPartL);
		}
		*/
Done:
		return TRUE;
	}

#endif /* VIEWER_VERSION */

