/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/* DebugHighLevel.c - Debug command and high-level functions - rev. for Nightingale 3.5:
	DCheckEverything			DebugDialog				DErrLimit
	DoDebug
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#ifndef PUBLIC_VERSION		/* If public, skip this file completely! */

#define DDB

static short DebugDialog(char *, short *, short *, short *,
					Boolean *, Boolean *, Boolean *, Boolean *);

short nerr, errLim;
Boolean minDebugCheck;			/* Do only Most important checks? */


/* -------------------------------------------------------------- DCheckEverything -- */
/* Do many or all available consistency checks on everything. Unfortunately, "all
available consistency checks" don't come anywhere near checking everything that's
checkable. Someday... Anyway, return FALSE normally, TRUE if things are dangerously
f*cked up.

There are four levels of checks:
	Most important: messages about problems are prefixed with '•'
	More important: messages about problems are prefixed with '*'
	Less & Least important: messages about problems have no prefix
*/

Boolean DCheckEverything(Document *doc,
			Boolean maxCheck,	/* TRUE=do even Least important checks */
			Boolean minCheck	/* TRUE=print only results of More & Most important checks */
			)
{
	LINK	pL;
	short	nInRange, nSel, nTotal, nvUsed;
	Boolean	strictCont, looseCont;

	minDebugCheck = minCheck;
	 
#ifdef DDB
	LogPrintf(LOG_WARNING, "CHK MAIN:");
#endif
	if (DCheckHeaps(doc)) return TRUE;
	/*
	 * First check individually all nodes in the main data structure (the object
	 * list), including their links, since the "global" checks must assume they can
	 * successfully traverse the data structure.
	 */
	ResetDErrLimit();
	nTotal = 0;
	
	/*
	 * The "for" loop and single separate iteration below could have been written with
	 * just the "for" by making the termination condition "pL!=RightLINK(doc->tailL)";
	 * however, if the data structure's links are bad, that would be more likely to stop
	 * in the wrong place.
	 */
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		nTotal++;
		if (DCheckNode(doc, pL, MAIN_DSTR, maxCheck)<0) return TRUE;
		if (UserInterrupt()) return FALSE;

		DCheckNodeSel(doc, pL);		if (DErrLimit() || UserInterrupt()) return FALSE;
	}
	nTotal++;																/* Count tailL */
	
	if (DCheckNode(doc, doc->tailL, MAIN_DSTR, maxCheck)<0) return TRUE;
	
	DCheckNodeSel(doc, doc->tailL); if (DErrLimit() || UserInterrupt()) return FALSE;

	if (doc!=clipboard) {
		DCheckVoiceTable(doc, maxCheck, &nvUsed); if (DErrLimit()) return FALSE;
	}

	(void)DCheckHeirarchy(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckJDOrder(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;

	(void)DCheckBeams(doc);					if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckOctaves(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckSlurs(doc);					if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckTuplets(doc, maxCheck);		if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckPlayDurs(doc, maxCheck);	if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckHairpins(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;

	(void)DCheckContext(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckRedundKS(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckRedundTS(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;
	if (maxCheck) {
		(void)DCheckMeasDur(doc);			if (DErrLimit() || UserInterrupt()) return FALSE;
	}
	(void)DCheckUnisons(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;
	(void)DCheckNoteNums(doc);				if (DErrLimit() || UserInterrupt()) return FALSE;

	if (DCheckSel(doc, &nInRange, &nSel)) return TRUE;
	
#ifdef DDB
	strictCont = ContinSelection(doc, TRUE);
	if (!strictCont) looseCont = ContinSelection(doc, FALSE);
	LogPrintf(LOG_WARNING, "%s sel, %d objs in range, %d sel, %d total. %c%d voices.\n",
					 (strictCont? "Strict contin." : (looseCont? "Loose contin." : "Discont.")),
					 nInRange, nSel, nTotal,
					 (nvUsed>doc->nstaves? '*' : ' '), nvUsed);
	
	/* Now check all nodes in the Master Page, Clipboard and the Undo Clipboard. */

	LogPrintf(LOG_WARNING, "    CHK MASTER: ");
#endif
	for (pL = doc->masterHeadL; pL!=doc->masterTailL; pL = RightLINK(pL)) {
		if (DCheckNode(doc, pL, MP_DSTR, maxCheck)<0) return TRUE;
		DCheckNodeSel(doc, pL);		if (DErrLimit() || UserInterrupt()) return FALSE;
	}
#ifdef DDB
	LogPrintf(LOG_WARNING, " Done.");
#endif
	
	LogPrintf(LOG_WARNING, "    CHK CLIP: ");
	InstallDoc(clipboard);
	for (pL = clipboard->headL; pL!=clipboard->tailL; pL = RightLINK(pL))
		if (DCheckNode(clipboard, pL, CLIP_DSTR, maxCheck)<0) {
			InstallDoc(doc);
			return TRUE;
		}
	InstallDoc(doc);

#ifdef DDB
	LogPrintf(LOG_WARNING, " Done.");
	if (nerr>0) LogPrintf(LOG_WARNING, " %d ERRORS. ", nerr); 	
	LogPrintf(LOG_WARNING, "\n");
#endif

	return FALSE;
}


/* ------------------------------------------------------------------- DebugDialog -- */
/* Let user say what debug information they want checked and/or displayed. */

#define DISABLE_CONTROLS {	\
		HiliteControl(dispHdl, CTL_INACTIVE);	\
		HiliteControl(checkHdl, CTL_INACTIVE);	\
		HiliteControl(linksHdl, CTL_INACTIVE);	\
		HiliteControl(subsHdl, CTL_INACTIVE);	\
	}
#define ENABLE_CONTROLS	{ \
		HiliteControl(dispHdl, CTL_ACTIVE);		\
		HiliteControl(checkHdl, CTL_ACTIVE);	\
		HiliteControl(linksHdl, CTL_ACTIVE);	\
		HiliteControl(subsHdl, CTL_ACTIVE);		\
	}

enum
{
	FULL=3,												/* Item numbers */
	SELECTION,
	CLIPBOARD,
	UNDODSTR,
	EVERYTHING,
	MOST_THINGS,
	MIN_THINGS,
	INDEX,
	MEMORY,
	START=13,
	STOP=15,
	DISPLAY,
	CHECK,
	LINKS,
	SUBENTRIES,
	LABEL_DI=21
};

#define LAST_RBUTTON MEMORY

static short DebugDialog(char *label, short *what, short *istart, short *istop,
							Boolean *disp, Boolean *check, Boolean *links, Boolean *subs)
{	
	DialogPtr		dialogp;
	GrafPtr			oldPort;
	Boolean			dialogOver;
	short			whatval;
	short			ditem, itype, j;
	ControlHandle	whatHdl[LAST_RBUTTON+1],
					dispHdl, checkHdl, linksHdl, subsHdl;
	Rect			tRect;
	ModalFilterUPP	filterUPP;
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(DEBUG_DLOG);
		return Cancel;
	}
	dialogp = GetNewDialog(DEBUG_DLOG, NULL, BRING_TO_FRONT);
	if (!dialogp) {
		DisposeModalFilterUPP(filterUPP);
		MayErrMsg("DebugDialog: unable to get dialog %ld", (long)DEBUG_DLOG);
		return Cancel;
	}
	ArrowCursor();

	strcpy(strBuf, "\"");
	strcat(strBuf, (char *)label);
	strcat(strBuf, "\"");
	CToPString(strBuf);
	PutDlgString(dialogp,LABEL_DI,(unsigned char *)strBuf,FALSE);

	/* Initialize: get Handles to controls, set initial values, etc. */
	for (j=FULL; j<=LAST_RBUTTON; j++)						/* Get Hdls to radio btns */
		GetDialogItem(dialogp, j, &itype, (Handle *)&whatHdl[j], &tRect);
	if (*what<FULL || *what>LAST_RBUTTON) {
		SetControlValue(whatHdl[FULL], 1);						/* Set default */		  	
		whatval = FULL;
	}
	else {
		SetControlValue(whatHdl[*what], 1);		  	
		whatval = *what;
	}
	PutDlgWord(dialogp, START, *istart, FALSE);
	PutDlgWord(dialogp, STOP, *istop, FALSE);
	GetDialogItem(dialogp, DISPLAY, &itype, (Handle *)&dispHdl, &tRect);
	SetControlValue(dispHdl, (*disp? 1 : 0));
	GetDialogItem(dialogp, CHECK, &itype, (Handle *)&checkHdl, &tRect);
	SetControlValue(checkHdl, (*check? 1 : 0));
	GetDialogItem(dialogp, LINKS, &itype, (Handle *)&linksHdl, &tRect);
	SetControlValue(linksHdl, (*links? 1 : 0));
	GetDialogItem(dialogp, SUBENTRIES, &itype, (Handle *)&subsHdl, &tRect);
	SetControlValue(subsHdl, (*subs? 1 : 0));
	SelectDialogItemText(dialogp, STOP, 0, ENDTEXT);					/* Hilite stop no. */
	if (*what>=EVERYTHING) DISABLE_CONTROLS
	else				   ENABLE_CONTROLS;
	dialogOver = FALSE;
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dialogp));
	
	do
	{
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = TRUE;
				break;
			case FULL:
			case SELECTION:
			case CLIPBOARD:
			case UNDODSTR:
				SetControlValue(whatHdl[whatval], 0);				/* Clear old radio button, */
				SetControlValue(whatHdl[ditem], 1);					/*   set this one */		  	
				ENABLE_CONTROLS;
				whatval = ditem;
				break;
			case EVERYTHING:
			case MOST_THINGS:
			case MIN_THINGS:
			case INDEX:
			case MEMORY:
				SetControlValue(whatHdl[whatval], 0);				/* Clear old radio button, */
				SetControlValue(whatHdl[ditem], 1);					/*   set this one */		  	
				DISABLE_CONTROLS;
				whatval = ditem;
				break;
			case DISPLAY:
				SetControlValue(dispHdl, 1-GetControlValue(dispHdl));
				break;
			case CHECK:
				SetControlValue(checkHdl, 1-GetControlValue(checkHdl));
				break;
			case LINKS:
				SetControlValue(linksHdl, 1-GetControlValue(linksHdl));
				break;
			case SUBENTRIES:
				SetControlValue(subsHdl, 1-GetControlValue(subsHdl));
				break;
			default:
				;
		}
	} while (!dialogOver);

	if (ditem==OK)
	{
		*what = whatval;
		GetDlgWord(dialogp,START,istart);
		GetDlgWord(dialogp,STOP,istop);
		GetDialogItem(dialogp, DISPLAY, &itype, (Handle *)&dispHdl, &tRect);
		*disp = GetControlValue(dispHdl);
		GetDialogItem(dialogp, CHECK, &itype, (Handle *)&checkHdl, &tRect);
		*check = GetControlValue(checkHdl);
		GetDialogItem(dialogp, LINKS, &itype, (Handle *)&linksHdl, &tRect);
		*links = GetControlValue(linksHdl);
		GetDialogItem(dialogp, SUBENTRIES, &itype, (Handle *)&subsHdl, &tRect);
		*subs = GetControlValue(subsHdl);
	}
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dialogp);										/* Free heap space */
	SetPort(oldPort);
	return ditem;
}


/* ---------------------------------------------------------- DErrLimit and friend -- */
/* DErrLimit returns TRUE if Debug should quit because the maximum no. of errors has
been exceeded. */

static Boolean alreadyAsked, userSaidQuit;

#define FIRST_ERRLIM 20
#define SECOND_ERRLIM 100

void ResetDErrLimit()
{
	nerr = 0;
	errLim = FIRST_ERRLIM;
	alreadyAsked = userSaidQuit = FALSE;
}


Boolean DErrLimit()
{
	
	if (nerr>errLim) {
		if (alreadyAsked && userSaidQuit) return TRUE;
		
		alreadyAsked = TRUE;
		sprintf(strBuf, "%d", errLim);
		CParamText(strBuf, "", "", "");
		if (CautionAdvise(DEBUGERR_ALRT)==OK) {
			alreadyAsked = userSaidQuit = FALSE;
			if (errLim==FIRST_ERRLIM) errLim = SECOND_ERRLIM;
			else							  errLim = 31999;		/* To disable limit checking */
			return FALSE;
		}
		else {
			userSaidQuit = TRUE;			
			return TRUE;
		}
	}
	return FALSE;
}


/* ----------------------------------------------------------------------- DoDebug -- */
/* Ask user what info they want and display/check it.  Returns FALSE normally,
TRUE if things are so badly screwed up that quitting immediately is desirable. */

Boolean DoDebug(
			char *label			/* Identifying string to display */
			)
{
	LINK			pL, startL, stopL;
	static short	what, istart, istop;
	short			kount, inLine, nInRange, nSel, objList;
	static Boolean	disp, check, showLinks, showSubs;
	static Boolean firstCall = TRUE;
	Boolean			selLinksBad;
	Document 		*doc=GetDocumentFromWindow(TopDocument);
	char			fullLabel[256];			/* Starting with close single quote */
	
	if (firstCall) {
		what = 0;
		istart = 7;
		istop = 15;
		disp = check = TRUE;
		showLinks = showSubs = FALSE;
		ResetDErrLimit();
		firstCall = FALSE;
	}
	
	if (doc==NULL) {
		LogPrintf(LOG_WARNING, "•DoDebug: doc NULL");
		return TRUE;
	}
	if (DebugDialog(label, &what, &istart, &istop, &disp, &check,	/* What does the */
							&showLinks, &showSubs)==Cancel)				/*   user want? */
		return FALSE;														/* Nothing? Quit */

	WaitCursor();

	strcpy(fullLabel, label);
	switch (what) {
		case EVERYTHING:
			strcat(fullLabel, "'-All");
			break;
		case MOST_THINGS:
			strcat(fullLabel, "'-Std");
			break;
		case MIN_THINGS:
			strcat(fullLabel, "'-Min");
			break;
		default:
			strcat(fullLabel, "'");
	}
	LogPrintf(LOG_WARNING, "DEBUG '%s: ", fullLabel);

	switch (what) {
		case EVERYTHING:
		case MOST_THINGS:
		case MIN_THINGS:
			if (DCheckEverything(doc, what==EVERYTHING, what==MIN_THINGS)) {
					/* Things are in a disasterous state. Quit before we crash! */
					LogPrintf(LOG_WARNING, "•DoDebug: CAN'T FINISH CHECKING.\n"); 					
					return TRUE;							
			}
			else
				return FALSE;
				
#ifdef DDB
		case INDEX:
			LogPrintf(LOG_WARNING, "INDEX: doc->headL=%d tailL=%d\n",
						doc->headL, doc->tailL);
			kount = inLine = 0;
			ResetDErrLimit();
			for (pL=doc->headL; pL; pL=RightLINK(pL)) {				
				if (ObjLType(pL)==HEADERtype
				||  ObjLType(pL)==TAILtype
				||  ObjLType(pL)==PAGEtype
				||  ObjLType(pL)==SYSTEMtype
				||  ObjLType(pL)==MEASUREtype) {
					DisplayIndexNode(doc, pL, kount, &inLine);
					if (DErrLimit()) return FALSE;
				}
				kount++;
			}
			return FALSE;
		
		case MEMORY:
			MemUsageStats(doc);
			return FALSE;
			
		case FULL:
			LogPrintf(LOG_WARNING, "FULL%s: headL=%d tailL=%d",
						(check? "/CHK" : " "),
						doc->headL, doc->tailL);
			startL = doc->headL;
			stopL = NILINK;
			break;
			
		case CLIPBOARD:
			LogPrintf(LOG_WARNING, "CLIP%s: clipboard->headL=%d clipboard->tailL=%d",
						(check? "/CHK" : " "),
						clipboard->headL, clipboard->tailL);
			startL = clipboard->headL;
			stopL = NILINK;
			break;
			
		case UNDODSTR:
			LogPrintf(LOG_WARNING, "UNDO%s: undo.headL=%d undo.tailL=%d",
						(check? "/CHK" : " "),
						doc->undo.headL, doc->undo.headL);
			startL = doc->undo.headL;
			stopL = NILINK;
			break;
			
		default:
			LogPrintf(LOG_WARNING, "SELECT%s: selStartL=%d End=%d ",
						(check? "/CHK" : " "),
						doc->selStartL, doc->selEndL);
			if (check) {
				if (DCheckHeaps(doc)) return TRUE;
				selLinksBad = DCheckSel(doc, &nInRange, &nSel);
				DCheckHeirarchy(doc);
				if (selLinksBad) {
					/* Things are in a disasterous state. Quit before we crash! */
					LogPrintf(LOG_WARNING, "•DoDebug: CAN'T DISPLAY THE SELECTION.\n"); 	
					return TRUE;
				}
			}
			startL = doc->selStartL;
			stopL = doc->selEndL;
	}
	
	if (disp && startL!=stopL) LogPrintf(LOG_WARNING, " (Obj flags: SelVisSoftValidTwkd)");
	LogPrintf(LOG_WARNING, "\n");
	kount = 0;
	ResetDErrLimit();

	if (DCheckHeaps(doc)) return FALSE;							/* Situation is bad but maybe not hopeless */				

	switch (what) {
		case CLIPBOARD:
			objList = CLIP_DSTR; break;
		case UNDODSTR:
			objList = UNDO_DSTR; break;
		default:
			objList = MAIN_DSTR;
	} 

	if (what==CLIPBOARD) InstallDoc(clipboard);

	for (pL=startL; pL!=stopL; pL=RightLINK(pL)) {				/* Main display/check loop */
		if (kount>=istart && kount<=istop) {
			if (disp)  DisplayNode(doc, pL, kount, showLinks, showSubs,
											(what==CLIPBOARD || what==UNDODSTR));
			if (check) DCheckNode(doc, pL, objList, FALSE);
			if (check && objList==MAIN_DSTR) DCheckNodeSel(doc, pL);
			if (DErrLimit()) {
				if (what==CLIPBOARD) InstallDoc(doc);
				return FALSE;
			}
		}
		kount++;
	}

	if (nerr>0) LogPrintf(LOG_WARNING, "%d ERRORS. ", nerr); 	

	if (what==CLIPBOARD) InstallDoc(doc);
	return FALSE;
#endif

}

//#endif /* PUBLIC_VERSION */
