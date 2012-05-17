/* ExtractHighLevel.c for Nightingale - high-level routines for extracting parts */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE� PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright � 1989-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean CombinePartsDlog(unsigned char *, Boolean *, Boolean *, Boolean *, short *);
static INT16 NumPartStaves(LINK partL);

/* ------------------------------------------------- CombinePartsDlog and DoExtract -- */

#ifdef VIEWER_VERSION

Boolean DoCombineParts(Document *doc, LINK *firstPartL, LINK *lastPartL)
	{
		return FALSE;
	}
	
#else

static enum {
	COMBINEINPLACE_DI=4,
	EXTRACTTOSCORE_DI,
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
	
static Boolean WithinStfRange(INT16 stf, INT16 start, INT16 end) 
	{
		return (stf >= start && stf <= end);
	}
	
static Boolean HasGroupConnect(LINK connectL) 
	{
		LINK aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) 
		{
			PACONNECT aConnect = GetPACONNECT(aConnectL);
			if (aConnect->connLevel==GroupLevel) {
				return TRUE;				
			}
		}
		
		return FALSE;
	}
	
static Boolean HasSpanningGroup(LINK connectL, INT16 firstStf, INT16 lastStf) 
	{
		LINK aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) 
		{
			INT16 stfAbove = ConnectSTAFFABOVE(aConnectL);
			INT16 stfBelow = ConnectSTAFFBELOW(aConnectL);
			if (ConnectCONNLEVEL(aConnectL)==GroupLevel) {
				if (stfAbove <= firstStf && stfBelow >= lastStf) {
					return TRUE;
				}
			}
		}
		
		return FALSE;
	}

static Boolean HasContainedGroup(LINK connectL, INT16 firstStf, INT16 lastStf) 
	{
		LINK aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) 
		{
			INT16 stfAbove = ConnectSTAFFABOVE(aConnectL);
			INT16 stfBelow = ConnectSTAFFBELOW(aConnectL);
			if (ConnectCONNLEVEL(aConnectL)==GroupLevel) {
				if (stfAbove >= firstStf && stfBelow <= lastStf) {
					return TRUE;
				}
			}
		}
		
		return FALSE;
	}

static Boolean HasOverlappingGroup(LINK connectL, INT16 firstStf, INT16 lastStf) 
	{
		LINK aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) 
		{
			INT16 stfAbove = ConnectSTAFFABOVE(aConnectL);
			INT16 stfBelow = ConnectSTAFFBELOW(aConnectL);
			if (ConnectCONNLEVEL(aConnectL)==GroupLevel) {
				if (WithinStfRange(stfAbove, firstStf, lastStf) ||
					 WithinStfRange(stfBelow, firstStf, lastStf)) 
				{
					return TRUE;
				}
			}
		}
		
		return FALSE;
	}

static Boolean CheckGroupLevelConnects(Document *doc, LINK firstPartL, LINK lastPartL)
	{
		LINK connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
				
		if (!HasGroupConnect(connectL)) 
			return TRUE;
		
		INT16 firstStf = PartFirstSTAFF(firstPartL);
		INT16 lastStf = PartLastSTAFF(lastPartL);
		
		if (HasSpanningGroup(connectL, firstStf, lastStf))
			return FALSE;
		
		if (HasSpanningGroup(connectL, firstStf, lastStf))
			return FALSE;
		
		if (HasSpanningGroup(connectL, firstStf, lastStf))
			return FALSE;
		
		return TRUE;
	}

static INT16 FixStavesForPart(LINK firstPartL, LINK partL)
	{
		PartLastSTAFF(firstPartL) = PartLastSTAFF(partL);
		INT16 stfDiff = PartFirstSTAFF(partL) - PartFirstSTAFF(firstPartL);
		return stfDiff;
	}
	
static void DeletePartInfoList(Document *doc, LINK headL, LINK firstPartL, LINK lastPartL) 
	{
		if (firstPartL == lastPartL)
			return;
		
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
		LinkNENTRIES(headL) -= nparts;
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

static Boolean UpdateConnect(LINK connectL, LINK firstPartL, LINK partL)
	{
		Boolean connectSel = FALSE;
			
		INT16 firstStf = PartFirstSTAFF(firstPartL);
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

static LINK GetConnectForFirstPart(Document *doc, LINK firstPartL, LINK connectL) 
	{
		INT16 connStf = PartFirstSTAFF(firstPartL);
		LINK aConnectL = FirstSubLINK(connectL);
		
		while (aConnectL) {
			if (ConnectSTAFFABOVE(aConnectL) == connStf) {
				return aConnectL;
			}		
			aConnectL = NextCONNECTL(aConnectL);
		}
		
		aConnectL = FirstSubLINK(connectL);
		
		LINK prevConnectL = aConnectL;
		
		while (aConnectL) {
			if (ConnectSTAFFABOVE(aConnectL) > connStf) {
				break;
			}
			
			prevConnectL = aConnectL;
			aConnectL = NextCONNECTL(aConnectL);
		}
	
		LINK connList = HeapAlloc(CONNECTheap,1);
		if (!connList) { NoMoreMemory(); return NILINK; }		/* ??not good--leaves doc in inconsistent state! */

		if (FirstSubLINK(connectL) == NILINK) {
			FirstSubLINK(connectL) = connList;
		}
		else {
			FirstSubLINK(connectL) = InsertLink(CONNECTheap,FirstSubLINK(connectL),prevConnectL,connList);			
		}
		LinkNENTRIES(connectL)++;
	
		aConnectL = connList;
		PACONNECT aConnect = GetPACONNECT(aConnectL);
		aConnect->connLevel = PartLevel;
		aConnect->connectType = CONNECTCURLY;
		DDIST dLineSp = STHEIGHT/(STFLINES-1);								/* Space between staff lines */
		aConnect->xd = -ConnectDWidth(doc->srastralMP, CONNECTCURLY);
		aConnect->staffAbove = connStf;
		aConnect->staffBelow = connStf;
		return aConnectL;
	}

static void DeselectCONNECT(LINK pL)
{
	PACONNECT	aConnect;
	LINK		aConnectL;
	
	aConnectL = FirstSubLINK(pL);
	for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		aConnect->selected = FALSE;
	}
	
	LinkSEL(pL) = FALSE;
}

static void ViewConnect(LINK connectL) {
	
	int i = 0;
	PCONNECT pConn = GetPCONNECT(connectL);
	LINK aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
		PACONNECT aConnect = GetPACONNECT(aConnectL);
		i++;
	}
}

static void FixMasterConnectForPart(Document *doc, LINK firstPartL, LINK partL)
	{
		LINK connectL;
//		LINK aConnectL;
		
		INT16 firstStf = PartFirstSTAFF(partL);
		INT16 lastStf = PartLastSTAFF(partL);		

		Boolean connectSel = FALSE;
		
		connectL = SSearch(doc->masterHeadL,CONNECTtype,GO_RIGHT);
		LINK firstConnL = GetConnectForFirstPart(doc, firstPartL, connectL);
		
		connectSel = UpdateConnect(connectL, firstPartL, partL);
		if (connectSel) 
		{
			LinkSEL(connectL) = TRUE;
			DeleteConnectSubObjs(connectL);
		}
		
		// Why are the connects still selected????
			DeselectCONNECT(connectL);
		
//		ViewConnect(connectL);
	}

static LINK GetMeasureForStaff(LINK measL, INT16 staffn)
	{
		LINK aMeasL = FirstSubLINK(measL);
		for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
			if (MeasureSTAFF(aMeasL) == staffn) {
				return aMeasL;
			}
		}
		
		return NILINK;
	}
	
static void ClearConnFieldsForMeasure(LINK measL, LINK aConnMeasL) 
	{
		if (MeasCONNSTAFF(aConnMeasL) > 0) {
			for (int i = MeasureSTAFF(aConnMeasL); i <= MeasCONNSTAFF(aConnMeasL); i++) {
				LINK aMeasL = GetMeasureForStaff(measL, i);
				
				if (MeasCONNABOVE(aMeasL))
					MeasCONNABOVE(aMeasL) = FALSE;
			}
		}		
	}
	
static void FixMeasureForPart(LINK firstPartL, LINK measL) 
	{
		INT16 firstStf = PartFirstSTAFF(firstPartL);
		INT16 lastStf = PartLastSTAFF(firstPartL);		
	
		for (int i = firstStf; i<=lastStf; i++) {
			LINK aMeasL = GetMeasureForStaff(measL, i);
			
			ClearConnFieldsForMeasure(measL, aMeasL);
		}
		
		LINK aFirstMeasL = GetMeasureForStaff(measL, firstStf);
		MeasCONNSTAFF(aFirstMeasL) = lastStf;
		
		for (int i = firstStf+1; i<=lastStf; i++) {
			LINK aMeasL = GetMeasureForStaff(measL, i);
			
			MeasCONNABOVE(aMeasL) = TRUE;
		}		
	}
	
		
static void FixMasterMeasureForPart(Document *doc, LINK firstPartL)
	{
		INT16 firstStf = PartFirstSTAFF(firstPartL);
		INT16 lastStf = PartLastSTAFF(firstPartL);	
			
		LINK measL = SSearch(doc->masterHeadL,MEASUREtype,GO_RIGHT);
		FixMeasureForPart(firstPartL, measL);		
	}

/*
static void FixConnectsForPart(Document *doc, LINK firstPartL, LINK partL)
	{
		LINK connectL;
		LINK aConnectL;
		
		INT16 firstStf = PartFirstSTAFF(partL);
		INT16 lastStf = PartLastSTAFF(partL);		

		Boolean connectSel = FALSE;
		
		connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
		
		for ( ; connectL; connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) 
		{
			LINK firstConnL = GetConnectForFirstPart(doc, firstPartL, connectL);
		
			aConnectL = FirstSubLINK(connectL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) 
			{
				connectSel = UpdateConnect(connectL, firstPartL, partL);
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
					DeselectCONNECT(connectL);
				}				
			}
		}	
	}
*/	

static void FixConnectsForPart(Document *doc, LINK firstPartL, LINK partL)
	{
		LINK connectL;
//		LINK aConnectL;
		
		INT16 firstStf = PartFirstSTAFF(partL);
		INT16 lastStf = PartLastSTAFF(partL);		

		Boolean connectSel = FALSE;
		
		connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
		
		for ( ; connectL; connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) 
		{
			LINK aConnectL = GetConnectForFirstPart(doc, firstPartL, connectL);
		
			connectSel = UpdateConnect(connectL, firstPartL, partL);
			if (connectSel) 
			{
				LinkSEL(connectL) = TRUE;
			}				
		}	
		
		connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
		
		for ( ; connectL; connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) 
		{
			connectSel = LinkSEL(connectL);
			if (connectSel) 
			{
				DeleteConnectSubObjs(connectL);
			}	
						
			// Why are the connects still selected????
				DeselectCONNECT(connectL);	
		}	
	}
	
static void FixMeasuresForPart(Document *doc, LINK firstPartL) 
	{
		LINK measL = SSearch(doc->headL,MEASUREtype,GO_RIGHT);
		
		for ( ; measL; measL=LSSearch(RightLINK(measL),MEASUREtype,ANYONE,GO_RIGHT,FALSE)) 
		{
			FixMeasureForPart(firstPartL, measL);		
		}	
	}

static void MPGetPartList(Document *doc, LINK firstPartL, LINK lastPartL, 
													  LINK *mpFirstPartL, LINK *mpLastPartL) 
	{
		*mpFirstPartL = *mpLastPartL = NILINK;
		
		LINK partL = FirstSubLINK(doc->masterHeadL);
		
		INT16 firstStf = PartFirstSTAFF(firstPartL);
		INT16 lastStf = PartLastSTAFF(lastPartL);
		
		while (partL) 
		{
			if (PartFirstSTAFF(partL) == firstStf) {
				*mpFirstPartL = partL; break;			
			}
			partL = NextPARTINFOL(partL);
		}
		
		partL = FirstSubLINK(doc->masterHeadL);
		while (partL) 
		{
			if (PartLastSTAFF(partL) == lastStf) {
				*mpLastPartL = partL; break;			
			}
			partL = NextPARTINFOL(partL);
		}
		
		
	}

static INT16 NumVoices(Document *doc) 
{
	INT16 nv = 0;
	
	for (int i = 0; i <= MAXVOICES; i++) {
		if (doc->voiceTab[i].partn != 0) nv++;
	}
	
	return nv;
}

static void CopyVoiceTable(VOICEINFO *srcTab, VOICEINFO *dstTab) 
{
	for (int i = 0; i <= MAXVOICES; i++) {
		dstTab[i] = srcTab[i];
	}
}

static INT16 TotalStfDiff(LINK firstPartL, LINK lastPartL) 
{
	INT16 totalDiff = 0;
	
	LINK partL = firstPartL;
	while (partL != NILINK && partL != lastPartL) 		// handle all the parts after the first
	{
		partL = NextPARTINFOL(partL);
		
		INT16 stfDiff = NumPartStaves(partL);
		totalDiff += stfDiff;
	}
	
	return totalDiff;
}

static INT16 TotalPartDiff(LINK firstPartL, LINK lastPartL) 
{
	INT16 totalDiff = 0;
	
	LINK partL = firstPartL;
	while (partL != lastPartL) 		// handle all the parts after the first
	{
		totalDiff ++;
		partL = NextPARTINFOL(partL);		
	}
	
	return totalDiff;
}

static void UpdatePartNumber(VOICEINFO *vTable, LINK partL, INT16 firstpartn) 
{
	INT16 firstStf = PartFirstSTAFF(partL);
	INT16 lastStf = PartLastSTAFF(partL);
	
	for (INT16 i = firstStf; i<=lastStf; i++) {
		vTable[i].partn = firstpartn;
	}
}

static void UpdatePartNums(Document *doc, VOICEINFO *vTable, LINK firstPartL, LINK lastPartL, INT16 numDiff) 
{
	INT16 firstpartn = PartL2Partn(doc, firstPartL);
	
	LINK partL = firstPartL;
	while (partL != NILINK && partL != lastPartL) 		// handle all the parts after the first
	{
		partL = NextPARTINFOL(partL);
		
		UpdatePartNumber(vTable, partL, firstpartn);
	}
	
	partL = NextPARTINFOL(lastPartL);		
	while (partL) 
	{			
		INT16 partn = PartL2Partn(doc, partL);
		UpdatePartNumber(vTable, partL, partn - numDiff);
		
		partL = NextPARTINFOL(partL);
	}
}

static void RenumberRelVoices(Document *doc, VOICEINFO *vTable, LINK firstPartL) 
{
	int firstpartn = PartL2Partn(doc, firstPartL);
	int partFirstStf = PartFirstSTAFF(firstPartL);
	INT16 i = partFirstStf + 1;	
	INT16 rv = 2;
	
	for ( ; i<=MAXVOICES; i++) {
		if (vTable[i].partn == firstpartn)
			vTable[i].relVoice = rv++;
	}
}

static void MoveVoicesDown(Document *doc, VOICEINFO *voiceTab, LINK firstPartL, LINK lastPartL) 
{
		VOICEINFO tempTab[2*MAXVOICES+2];	/* Descriptions of voices in use */
		INT16 numVoices = NumVoices(doc);

		// Duplicate the voice table
		CopyVoiceTable(voiceTab, tempTab);
		
		// Copy all the parts to be combined to the end of the duplicate
		INT16 firstpartn = PartL2Partn(doc, firstPartL);
		INT16 lastpartn = PartL2Partn(doc, lastPartL);
		
		int i = firstpartn + 1;
		int j = numVoices + 1;
		
		for ( ; i <= lastpartn; i++, j++) {
			tempTab[j] = voiceTab[i];
		}
		
		// Move everything back down
		
		i = firstpartn + 1;
		j = lastpartn + 1;
				
		int k = 0;
		int num2move = numVoices - firstpartn + 1;
		
		for (k = 0; k < num2move; k++) {
			voiceTab[i + k] = tempTab[j + k];
		}		
}

static void FixPartStaffNumbers(Document *doc, LINK firstPartL, LINK lastPartL) 
{
 		LINK partL = firstPartL;
		while (partL != NILINK && partL != lastPartL) 		// handle all the parts after the first
		{
			partL = NextPARTINFOL(partL);
			
			INT16 stfDiff = FixStavesForPart(firstPartL, partL);
			FixConnectsForPart(doc, firstPartL, partL);
		}
		FixMeasuresForPart(doc, firstPartL);
}

static void CombinePartsInPlace(Document *doc, LINK firstPartL, LINK lastPartL) 
{		
		if (firstPartL == lastPartL)
			return;
		
		VOICEINFO voiceTab[MAXVOICES+1];							// Descriptions of voices in use
		INT16 numVoices = NumVoices(doc);
		
		CopyVoiceTable(doc->voiceTab, voiceTab);
		
		INT16 totalDiff = TotalPartDiff(firstPartL, lastPartL);
		UpdatePartNums(doc, voiceTab, firstPartL, lastPartL, totalDiff);
		RenumberRelVoices(doc, voiceTab, firstPartL);
		//MoveVoicesDown(doc, voiceTab, firstPartL, lastPartL);
		CopyVoiceTable(voiceTab, doc->voiceTab);
		FixPartStaffNumbers(doc, firstPartL, lastPartL);
		
		/*
 		LINK partL = firstPartL;
		while (partL != NILINK && partL != lastPartL) 		// handle all the parts after the first
		{
			partL = NextPARTINFOL(partL);
			
			INT16 stfDiff = FixStavesForPart(firstPartL, partL);
									
			FixVoicesForPart(doc, firstPartL, partL, stfDiff);
			FixConnectsForPart(doc, firstPartL, partL);
		}
		*/		
		
		DeletePartInfoList(doc, doc->headL, firstPartL, lastPartL);			
}

static void FixMasterPartStaffNumbers(Document *doc, LINK firstPartL, LINK lastPartL) 
{
		if (firstPartL == lastPartL)
			return;
		
 		LINK partL = firstPartL;
		while (partL != NILINK && partL != lastPartL) 		// handle all the parts after the first
		{
			partL = NextPARTINFOL(partL);
			
			INT16 stfDiff = FixStavesForPart(firstPartL, partL);
			FixMasterConnectForPart(doc, firstPartL, partL);
		}
		FixMasterMeasureForPart(doc, firstPartL);
}

static void CombineMasterPartsInPlace(Document *doc, LINK firstPartL, LINK lastPartL) 
{
		/*
 		LINK partL = firstPartL;
		while (partL != lastPartL) 		// handle all the parts after the first
		{
			partL = NextPARTINFOL(partL);
			
			INT16 stfDiff = FixStavesForPart(firstPartL, partL);
			
			FixMasterConnectForPart(doc, firstPartL, partL);
		}	
		*/	
		
		FixMasterPartStaffNumbers(doc, firstPartL, lastPartL);
		DeletePartInfoList(doc, doc->masterHeadL, firstPartL, lastPartL);			
}

static void CombineInPlace(Document *doc, LINK firstPartL, LINK lastPartL)
	{
		if (!CheckMultivoiceRoles(doc, firstPartL, lastPartL)) 
		{
			MayErrMsg("Combine Parts: Conflicting voice roles. Can't combine parts with more than one upper or lower voice");
			return;
		}
		
		if (!CheckGroupLevelConnects(doc, firstPartL, lastPartL)) 
		{
			MayErrMsg("Combine Parts: Please remove all groups containing parts to be combined and then retry the operation");
			return;
		}
		
		LINK mpFirstPartL, mpLastPartL;
		
		MPGetPartList(doc, firstPartL, lastPartL, &mpFirstPartL,&mpLastPartL);
		
		if (mpFirstPartL == NILINK || mpLastPartL == NILINK) {
			MayErrMsg("Combine in Place: unable to get parts in master page");
			return;
		}
		
		LINK selStartL = doc->selStartL;
		
		CombinePartsInPlace(doc, firstPartL, lastPartL);
		CombineMasterPartsInPlace(doc, mpFirstPartL, mpLastPartL);
		
		DeselAll(doc);
		
	}

static Document *CreateFirstPartDoc(Document *doc, LINK firstPartL) 
	{
		Str255 name;
		
		PToCString(Pstrcpy(name,doc->name));
		strcat((char *)name,"-");
		PPARTINFO pPart = GetPPARTINFO(firstPartL);
		strcat((char *)name,pPart->name);
		CToPString((char *)name);
		
		Document *partDoc = CreatePartDoc(doc,name,doc->vrefnum,&doc->fsSpec,firstPartL);
		return partDoc;
	}
	
static INT16 NumPartStaves(LINK partL) 
	{
		return PartLastSTAFF(partL) - PartFirstSTAFF(partL) + 1;
	}
	
static void CopyPart(Document *doc, LINK partL) 
	{
		
		SelectRange(doc, doc->headL, doc->tailL, PartFirstSTAFF(partL), PartLastSTAFF(partL));
		DoCopy(doc);
	}
	
static void AddPart2Doc(Document *doc, Document *partDoc, LINK partL) 
	{
		LINK newPartL = AddPart(partDoc, doc->nstaves, NumPartStaves(partL), SHOW_ALL_LINES);

		CopyPart(doc, partL);
		DoMerge(partDoc);	
	}
	
static Boolean cpReformat;
static Boolean cpSpacePercent;
static Boolean cpCareMeasPerSys;
static INT16 cpMeasPerSys;
static Boolean cpCloseAndSave;

static void ExtractToScore(Document *doc, LINK firstPartL, LINK lastPartL)
	{
		if (!EnoughFreeDocs()) {
			return;
		}
		WaitCursor();
		Document *partDoc = CreateFirstPartDoc(doc, firstPartL);
		if (partDoc == NULL) {
			return;
		}
		
 		LINK partL = firstPartL;
		while (partL != NILINK && partL != lastPartL) 		/* handle all the parts after the first */
		{
			partL = NextPARTINFOL(partL);
			AddPart2Doc(doc, partDoc, partL);
		}
			
		if (cpReformat)
			ReformatPart(partDoc, cpSpacePercent, TRUE, cpCareMeasPerSys, cpMeasPerSys);
		else
			NormalizePartFormat(partDoc);
		
		/* Finally, set empty selection just after the first Measure and put caret there.
			??Will this necessarily be on the screen? */
		
		SetDefaultSelection(partDoc);
		partDoc->selStaff = 1;
		
		MEAdjustCaret(partDoc, FALSE);		// ??AND REMOVE CALL IN CreatePartDoc!
		AnalyzeWindows();
		if (cpCloseAndSave)
			DoCloseDocument(partDoc);
	}
	
static void RedrawDocument(Document *doc) 
	{
		doc->changed = TRUE;
		EraseAndInval(&doc->viewRect);
		DoUpdate(doc->theWindow);		
	}
	
/* 
 * Extract parts into separate documents and optionally reformat and/or save them.
 * If all goes well, return TRUE; if theres a problem, return FALSE. 
 */
Boolean DoCombineParts(Document *doc)
	{
		Str255 partName;
		short spacePercent, numParts;
		LINK firstPartL, lastPartL;
		LINK partL; PPARTINFO pPart;
		LINK selPartList[MAXSTAVES+1];
		Boolean keepGoing=TRUE;
		static Boolean inPlace=TRUE, closeAndSave=FALSE, reformat=TRUE;
		static Boolean firstCall=TRUE, careMeasPerSys;
		static INT16 measPerSys;
		INT16 i;
		
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
		
		for (i = 0; i<=MAXSTAVES; i++)  {
			if (selPartList[i] != NILINK) {
				firstPartL = selPartList[i]; break;
			}
		}
		
		for (i = MAXSTAVES; i>=0; i--) {
			if (selPartList[i] != NILINK) {
				lastPartL = selPartList[i]; break;
			}
		}

		spacePercent = doc->spacePercent;
		if (!CombinePartsDlog(partName, &inPlace, &closeAndSave, &reformat, &spacePercent))
			return FALSE;

		if (firstCall) {
			careMeasPerSys = FALSE; measPerSys = 4; firstCall = FALSE;
		}
		
		DeselAll(doc);

		cpReformat = reformat;
		cpSpacePercent = spacePercent;
		cpCareMeasPerSys = careMeasPerSys;
		cpMeasPerSys = measPerSys;
		cpCloseAndSave = closeAndSave;

		if (inPlace) {
			if (firstPartL == lastPartL)
				MayErrMsg("CombineParts: selection already a single part");
			else
				CombineInPlace(doc, firstPartL, lastPartL);
		}
		else {
			if (0) 
				ExtractToScore(doc, firstPartL, lastPartL);
			else
				MayErrMsg("CombineParts: feature currently under construction");
		}
		RedrawDocument(doc);
		
Done:
		return TRUE;
	}

#endif /* VIEWER_VERSION */
