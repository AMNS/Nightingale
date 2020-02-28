/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* DebugHighLevel.c - Debug command and high-level functions:
	DCheckEverything			DebugDialog				DErrLimit
	DoDebug
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


#define DDB

static short DebugDialog(char *, short *, short *, short *, Boolean *, Boolean *,
						Boolean *, Boolean *);

short nerr, errLim;
Boolean minDebugCheck;			/* Do only Most important checks? */


/* ------------------------------------------------------------------ DCheckEverything -- */
/* Do many or all available consistency checks on everything. Unfortunately, "all
available consistency checks" don't come anywhere near checking everything that's
checkable. Someday... Anyway, return False normally, True if things are dangerously
f*cked up.

There are four levels of checks:
	Most important: messages about problems are prefixed with '•'
	More important: messages about problems are prefixed with '*'
	Less & Least important: messages about problems have no prefix
*/

Boolean DCheckEverything(Document *doc,
			Boolean maxCheck,	/* True=do even Least important checks */
			Boolean minCheck	/* True=print only results of More & Most important checks */
			)
{
	LINK	pL;
	short	nInRange, nSel, nTotal, nvUsed;
	Boolean	strictCont, looseCont;

	minDebugCheck = minCheck;
	 
#ifdef DDB
	LogPrintf(LOG_INFO, "--CHECK MAIN:\n");
#endif
	if (DCheckHeaps(doc)) return True;
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
		if (DCheckNode(doc, pL, MAIN_DSTR, maxCheck)<0) return True;
		if (UserInterrupt()) return False;

		DCheckNodeSel(doc, pL);		if (DErrLimit() || UserInterrupt()) return False;
	}
	nTotal++;														/* Count tailL */
	
	/* PART 1. Check that data structures -- mostly the object list -- are legal. */
	
	if (DCheckNode(doc, doc->tailL, MAIN_DSTR, maxCheck)<0) return True;
	
	DCheckNodeSel(doc, doc->tailL); if (DErrLimit() || UserInterrupt()) return False;

	if (doc!=clipboard) {
		DCheckVoiceTable(doc, maxCheck, &nvUsed); if (DErrLimit()) return False;
	}

	(void)DCheckHeirarchy(doc);				if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckJDOrder(doc);				if (DErrLimit() || UserInterrupt()) return False;

	(void)DCheckBeams(doc, maxCheck);		if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckOttavas(doc);				if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckSlurs(doc);					if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckTuplets(doc, maxCheck);		if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckHairpins(doc);				if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckContext(doc);				if (DErrLimit() || UserInterrupt()) return False;

	/* PART 2. Check that musical and music-notation constraints are obeyed. */
	
	(void)DCheckPlayDurs(doc, maxCheck);	if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckTempi(doc);					if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckRedundantKS(doc);			if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckExtraTS(doc);				if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckCautionaryTS(doc);			if (DErrLimit() || UserInterrupt()) return False;
	if (maxCheck) {
		(void)DCheckMeasDur(doc);			if (DErrLimit() || UserInterrupt()) return False;
	}
	(void)DCheckUnisons(doc);				if (DErrLimit() || UserInterrupt()) return False;
	(void)DCheckNoteNums(doc);				if (DErrLimit() || UserInterrupt()) return False;

	if (DCheckSel(doc, &nInRange, &nSel)) return True;
	
#ifdef DDB
	strictCont = ContinSelection(doc, True);
	if (!strictCont) looseCont = ContinSelection(doc, False);
	LogPrintf(LOG_INFO, "%s sel, %d objs in range, %d sel, %d total. %c%d voices.\n",
					 (strictCont? "Strict contin." : (looseCont? "Loose contin." : "Discont.")),
					 nInRange, nSel, nTotal,
					 (nvUsed>doc->nstaves? '*' : ' '), nvUsed);
	
	/* Now check all nodes in the Master Page, Clipboard and the Undo Clipboard. */

	LogPrintf(LOG_INFO, "--CHECK MASTER: ");
#endif
	for (pL = doc->masterHeadL; pL!=doc->masterTailL; pL = RightLINK(pL)) {
		if (DCheckNode(doc, pL, MP_DSTR, maxCheck)<0) return True;
		DCheckNodeSel(doc, pL);			if (DErrLimit() || UserInterrupt()) return False;
	}
#ifdef DDB
	LogPrintf(LOG_INFO, " Done.\n");
#endif
	
	LogPrintf(LOG_INFO, "--CHECK CLIPBOARD: ");
	InstallDoc(clipboard);
	for (pL = clipboard->headL; pL!=clipboard->tailL; pL = RightLINK(pL))
		if (DCheckNode(clipboard, pL, CLIP_DSTR, maxCheck)<0) {
			InstallDoc(doc);
			return True;
		}
	InstallDoc(doc);

#ifdef DDB
	LogPrintf(LOG_INFO, " Done.");
	if (nerr>0) LogPrintf(LOG_INFO, " %d ERROR(S). ", nerr); 	
	LogPrintf(LOG_INFO, "\n");
#endif

	return False;
}


/* ----------------------------------------------------------------------- DebugDialog -- */
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
	VOICETBL_DI,
	STARTNUM_DI=14,
	STOPNUM_DI=16,
	DISPLAY,
	CHECK,
	LINKS,
	SUBENTRIES,
	LABEL_DI=22
};

#define LAST_RBUTTON VOICETBL_DI

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
	PutDlgString(dialogp,LABEL_DI, (unsigned char *)strBuf, False);

	/* Initialize: get Handles to controls, set initial values, etc. */
	for (j=FULL; j<=LAST_RBUTTON; j++)							/* Get Hdls to radio btns */
		GetDialogItem(dialogp, j, &itype, (Handle *)&whatHdl[j], &tRect);
	if (*what<FULL || *what>LAST_RBUTTON) {
		SetControlValue(whatHdl[FULL], 1);						/* Set default */		  	
		whatval = FULL;
	}
	else {
		SetControlValue(whatHdl[*what], 1);		  	
		whatval = *what;
	}
	PutDlgWord(dialogp, STARTNUM_DI, *istart, False);
	PutDlgWord(dialogp, STOPNUM_DI, *istop, False);
	GetDialogItem(dialogp, DISPLAY, &itype, (Handle *)&dispHdl, &tRect);
	SetControlValue(dispHdl, (*disp? 1 : 0));
	GetDialogItem(dialogp, CHECK, &itype, (Handle *)&checkHdl, &tRect);
	SetControlValue(checkHdl, (*check? 1 : 0));
	GetDialogItem(dialogp, LINKS, &itype, (Handle *)&linksHdl, &tRect);
	SetControlValue(linksHdl, (*links? 1 : 0));
	GetDialogItem(dialogp, SUBENTRIES, &itype, (Handle *)&subsHdl, &tRect);
	SetControlValue(subsHdl, (*subs? 1 : 0));
	SelectDialogItemText(dialogp, STOPNUM_DI, 0, ENDTEXT);			/* Hilite stop no. */
	if (*what>=EVERYTHING) DISABLE_CONTROLS
	else				   ENABLE_CONTROLS;
	dialogOver = False;
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dialogp));
	
	do
	{
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = True;
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
			case VOICETBL_DI:
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
		GetDlgWord(dialogp,STARTNUM_DI,istart);
		GetDlgWord(dialogp,STOPNUM_DI,istop);
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


/* -------------------------------------------------------------- DErrLimit and friend -- */
/* DErrLimit returns True if Debug should quit because the maximum no. of errors has
been exceeded. */

static Boolean alreadyAsked, userSaidQuit;

#define FIRST_ERRLIM 20
#define SECOND_ERRLIM 100

void ResetDErrLimit()
{
	nerr = 0;
	errLim = FIRST_ERRLIM;
	alreadyAsked = userSaidQuit = False;
}


Boolean DErrLimit()
{
	
	if (nerr>errLim) {
		if (alreadyAsked && userSaidQuit) return True;
		
		alreadyAsked = True;
		sprintf(strBuf, "%d", errLim);
		CParamText(strBuf, "", "", "");
		if (CautionAdvise(DEBUGERR_ALRT)==OK) {
			alreadyAsked = userSaidQuit = False;
			if (errLim==FIRST_ERRLIM) errLim = SECOND_ERRLIM;
			else							  errLim = 31999;		/* To disable limit checking */
			return False;
		}
		else {
			userSaidQuit = True;			
			return True;
		}
	}
	return False;
}


/* --------------------------------------------------------------------------- DoDebug -- */
/* Ask user what info they want and display/check it.  Returns False normally,
True if things are so badly screwed up that quitting immediately is desirable. */

Boolean DoDebug(
			char *label			/* Identifying string to display */
			)
{
	LINK			pL, startL, stopL;
	static short	what, istart, istop;
	short			kount, inLine, nInRange, nSel, objList, status;
	static Boolean	disp, check, showLinks, showSubs;
	static Boolean	firstCall = True;
	Boolean			selLinksBad;
	Document 		*doc=GetDocumentFromWindow(TopDocument);
	char			fullLabel[256];			/* Starting with close single quote */
	
	if (firstCall) {
		what = 0;
		istart = 7;
		istop = 15;
		disp = check = True;
		showLinks = showSubs = False;
		ResetDErrLimit();
		firstCall = False;
	}
	
	if (doc==NULL) {
		LogPrintf(LOG_ERR, "•DoDebug: doc is NULL!\n");
		return True;
	}
	if (DebugDialog(label, &what, &istart, &istop, &disp, &check,	/* What does the */
							&showLinks, &showSubs)==Cancel)			/*   user want? */
		return False;												/* If nothing, quit */

	WaitCursor();

	strcpy(fullLabel, label);
	switch (what) {
		case EVERYTHING:
			strcat(fullLabel, "'-Everything");
			break;
		case MOST_THINGS:
			strcat(fullLabel, "'-Standard");
			break;
		case MIN_THINGS:
			strcat(fullLabel, "'-Min. things");
			break;
		default:
			strcat(fullLabel, "'");
	}
	LogPrintf(LOG_INFO, "DEBUG '%s: ", fullLabel);

	switch (what) {
		case EVERYTHING:
		case MOST_THINGS:
		case MIN_THINGS:
			if (DCheckEverything(doc, what==EVERYTHING, what==MIN_THINGS)) {
					/* Things are in a disastrous state. Quit before we crash or hang! */
					LogPrintf(LOG_ERR, "•DoDebug: CAN'T FINISH CHECKING.\n"); 					
					return True;							
			}
			else
				return False;
				
#ifdef DDB
		case INDEX:
			LogPrintf(LOG_INFO, "INDEX: doc->headL=%d tailL=%d\n",
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
					if (DErrLimit()) return False;
				}
				kount++;
			}
			return False;
		
		case MEMORY:
			MemUsageStats(doc);
			return False;
			
		case VOICETBL_DI:
			short v; char str[300];

			LogPrintf(LOG_INFO, "VOICE TABLE: there are %d voices.\n", CountVoices(doc));
			for (v = 1; v<=CountVoices(doc); v++) {
				GetVoiceTableLine(doc, v, str);
				strcat(str, "\n");
				LogPrintf(LOG_INFO, str);
			}
			return False;
			
		case FULL:
			LogPrintf(LOG_INFO, "FULL%s: headL=%d tailL=%d", (check? "/CHECK" : " "),
						doc->headL, doc->tailL);
			startL = doc->headL;
			stopL = NILINK;
			break;
			
		case CLIPBOARD:
			LogPrintf(LOG_INFO, "CLIP%s: clipboard->headL=%d clipboard->tailL=%d",
						(check? "/CHECK" : " "),
						clipboard->headL, clipboard->tailL);
			startL = clipboard->headL;
			stopL = NILINK;
			break;
			
		case UNDODSTR:
			LogPrintf(LOG_INFO, "UNDO%s: undo.headL=%d undo.tailL=%d",
						(check? "/CHECK" : " "),
						doc->undo.headL, doc->undo.headL);
			startL = doc->undo.headL;
			stopL = NILINK;
			break;
			
		default:
			LogPrintf(LOG_INFO, "SELECT%s: selStartL=%d End=%d ",
						(check? "/CHECK" : " "),
						doc->selStartL, doc->selEndL);
			if (check) {
				if (DCheckHeaps(doc)) return True;
				selLinksBad = DCheckSel(doc, &nInRange, &nSel);
				DCheckHeirarchy(doc);
				if (selLinksBad) {
					/* Things are in a disastrous state. Quit before we crash or hang! */
					LogPrintf(LOG_ERR, "•DoDebug: CAN'T DISPLAY THE SELECTION.\n"); 	
					return True;
				}
			}
			startL = doc->selStartL;
			stopL = doc->selEndL;
	}
	
	if (disp && startL!=stopL) LogPrintf(LOG_INFO, " (Obj flags: SelVisSoftValidTwkdSpare)");
	LogPrintf(LOG_INFO, "\n");
	kount = 0;
	ResetDErrLimit();

	if (DCheckHeaps(doc)) return False;							/* Situation is bad but maybe not hopeless */				

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
			if (check) {
				status = DCheckNode(doc, pL, objList, False);
				if (status<0) {
					/* Things are in a disastrous state. Quit before we crash or hang! */
					LogPrintf(LOG_ERR, "•DoDebug: CAN'T FINISH CHECKING.\n"); 					
					return True;							
				}
			}
			if (check && objList==MAIN_DSTR) DCheckNodeSel(doc, pL);
			if (DErrLimit()) {
				if (what==CLIPBOARD) InstallDoc(doc);
				return False;
			}
		}
		kount++;
	}

	if (nerr>0) LogPrintf(LOG_WARNING, "%d ERROR(S).\n", nerr); 	

	if (what==CLIPBOARD) InstallDoc(doc);
	return False;
#endif

}
