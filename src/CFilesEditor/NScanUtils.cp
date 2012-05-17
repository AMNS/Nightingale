/***************************************************************************
*	FILE:	NScanUtils.c																		*
*	PROJ:	Nightingale, minor rev. for v 3.1											*
*	DESC:	Utilities for Nightingale scanner interface.								*
/***************************************************************************/

/*											NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROPERTY OF
 *	ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE SECRET AND IS
 *	NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED WRITTEN
 *	AUTHORIZATION FROM THE OWNER.
 * Copyright © 1994-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/*
	Page sizes are handled as follows (all numbers are in inches):
	
	US Letter	8.5	11.0		8.0	10.5
	US Legal		8.5	14.0		8.0	13.5
	A4 Letter	8.5	11.68		x
	B5 Legal		6.93	9.83		x
	Tabloid		11.0	17.0		10.5	16.5
	A3 Tabloid	11.68	16.54		x
	
	where the last two columns indicate max width and ht of doc which can fit
	into that page, given 1/4 inch margins all around; 'x' indicate an intermediate
	size which is handled by defaulting to next larger handled size.
	This works for all except A3 Tabloid, which provides an extra .68 inch width
	that we are neglecting to take advantage of here. 
*/

enum {
	NO_PAGE = 0,
	LETTER_PAGE,
	LEGAL_PAGE,
	TABLOID_PAGE
};

static ModalFilterUPP	dlogFilterUPP;

/* --------------------------------------------------------------------------------- */
/* Local function prototypes */

static short NSSyncProblems(LINK syncL, short *pFirstErrStaff);

/* ----------------------------------------------------------- BuildNightScanDoc -- */
/*
 *	Initialise a Document.  If <isNew>, make a new Document, untitled and empty;
 *	otherwise, read the given file, decode it, and attach it to this Document.
 *	In any case, make the Document the current grafPort and install it, including
 *	magnification. Return TRUE normally, FALSE in case of error.
 */

Boolean BuildNightScanDoc(
					register Document *doc,
					unsigned char *fileName,
					short vRefNum,
					FSSpec *pfsSpec,
					long *fileVersion,
					Boolean isNew,						/* TRUE=new file, FALSE=open existing file */
					short pageWidth, short pageHt,
					short /*nStaves*/,						/* unused */
					short rastral,
					short FILEXResolution
					)
	{
		WindowPtr w = doc->theWindow; Rect r;
		DDIST sysTop;
		short pageType=NO_PAGE;
		FASTFLOAT pageWidthIN,pageHtIN,bottom,right;

		SetPort(GetWindowPort(w));
		
		/*
		 *	About coordinate systems:
		 *	The point (0,0) is placed at the upper left corner of the initial paperRect.
		 * The viewRect is the same as the window portRect, but without the scroll bar
		 *	areas. The display of the document's contents should always be in this
		 *	coordinate system, so that printing will be in the same coordinate system.
		 */
		
		/* Set the initial paper size, margins, etc. from the config and from
			params passed in from the NoteScan file */
		
		pageWidthIN = (FASTFLOAT)pageWidth/(FASTFLOAT)FILEXResolution;
		pageHtIN = (FASTFLOAT)pageHt/(FASTFLOAT)FILEXResolution;

		if (pageWidthIN>8.0 || pageHtIN>13.5)
			pageType = TABLOID_PAGE;
		else if (pageHtIN>10.5)
			pageType = LEGAL_PAGE;
		else
			pageType = LETTER_PAGE;
		
		doc->paperRect.top = doc->paperRect.left = 0;

		if (pageType==LETTER_PAGE) {
			right = 8.5 * (FASTFLOAT)POINTSPERIN;
			bottom = 11.0 * (FASTFLOAT)POINTSPERIN;
			
			doc->paperRect.bottom = bottom;
			doc->paperRect.right = right;
		}
		else if (pageType==LEGAL_PAGE) {
			right = 8.5 * (FASTFLOAT)POINTSPERIN;
			bottom = 14.0 * (FASTFLOAT)POINTSPERIN;
			
			doc->paperRect.bottom = bottom;
			doc->paperRect.right = right;
		}
		else {																	/* tabloid page */
			right = 11.0 * (FASTFLOAT)POINTSPERIN;
			bottom = 17.0 * (FASTFLOAT)POINTSPERIN;
			
			doc->paperRect.bottom = bottom;
			doc->paperRect.right = right;
		}

		doc->pageType = 2;
		doc->measSystem = 0;
		doc->origPaperRect = doc->paperRect;
#if 1
		/* For now, always use margin of 18 points = 1/4 inch */
		doc->marginRect.top = doc->paperRect.top+18;
		doc->marginRect.left = doc->paperRect.left+18;
		doc->marginRect.bottom = doc->paperRect.bottom-18;
		doc->marginRect.right = doc->paperRect.right-18;
#else
		doc->marginRect.top = doc->paperRect.top+config.pageMarg.top;
		doc->marginRect.left = doc->paperRect.left+config.pageMarg.left;
		doc->marginRect.bottom = doc->paperRect.bottom-config.pageMarg.bottom;
		doc->marginRect.right = doc->paperRect.right-config.pageMarg.right;
#endif

		InitDocFields(doc);
		doc->srastral = rastral;								/* Override staff size, from parameter */

		FillSpaceMap(doc, 0);

		/*
		 *	Add the standard scroll bar controls to Document's window. The scroll
		 *	bars are created with a maximum value of 0 here, but this will have no
		 *	effect if we call RecomputeView, since it resets the max.
		 */
		GetWindowPortBounds(w,&r);
		r.left = r.right - (SCROLLBAR_WIDTH+1);
		r.bottom -= SCROLLBAR_WIDTH;
		r.top--;
		
		doc->vScroll = NewControl(w,&r,"\p",TRUE,
									doc->origin.h,doc->origin.h,0,scrollBarProc,0L);
		GetWindowPortBounds(w,&r);
		r.top = r.bottom - (SCROLLBAR_WIDTH+1);
		r.right -= SCROLLBAR_WIDTH;
		r.left += MESSAGEBOX_WIDTH;
		
		doc->hScroll = NewControl(w,&r,"\p",TRUE,
									doc->origin.v,doc->origin.v,0,scrollBarProc,0L);
		if (!InitAllHeaps(doc)) { NoMoreMemory(); return FALSE; }
		InstallDoc(doc);
		BuildEmptyList(doc,&doc->headL,&doc->tailL);
		
		doc->selStartL = doc->selEndL = doc->tailL;			/* Empty selection  */
		
		/*
		 * Set part name showing and corresponding system indents. We'd like to use
		 * PartNameMargin() to get the appropriate indents, but not enough of the data
		 * structure is set up, so do something cruder.
		 */
		doc->firstNames = FULLNAMES;								/* 1st system: full part names */
		doc->firstIndent = qd2d(config.indent1st, drSize[doc->srastral], STFLINES);
		doc->otherNames = NONAMES;									/* Other systems: no part names */
		doc->otherIndent = 0;
		
		if (isNew) {
			*fileVersion = THIS_VERSION;
			NewDocScore(doc);										/* Set up initial staves, clefs, etc. */
			doc->firstSheet = 0;
			doc->currentSheet = 0;
			doc->origin = doc->sheetOrigin;					/* Ignore position recorded in file */
			if (useWhichMIDI==MIDIDR_FMS) {
				/* These won't pop any alerts. */
				(void)FMSCheckPartDestinations(doc, TRUE);
				FMSSetNewDocRecordDevice(doc);
			}
		}
		else {														/* Finally READ THE FILE! */
			if (OpenFile(doc,(unsigned char *)fileName,vRefNum,pfsSpec,fileVersion)!=noErr) {
				NewDocScore(doc);
				return FALSE;
				}
#ifndef TARGET_API_MAC_CARBON
			UnloadSeg(OpenFile);
#endif
			doc->firstSheet = 0 /* Or whatever; may be document specific */;
			doc->currentSheet = 0;
			doc->lastGlobalFont = 4;							/* Default is Regular1 */
			InstallMagnify(doc);									/* Set for file-specified magnification */
			if (doc->masterHeadL == NILINK) {
				/* This is an old file without Master Page object list: make default one */
				sysTop = SYS_TOP(doc);
				NewMasterPage(doc,sysTop,TRUE);
				}
			}

		if (!InitDocUndo(doc))
			return FALSE;
		
		doc->yBetweenSysMP = doc->yBetweenSys = 0;
		SetOrigin(doc->origin.h,doc->origin.v);
		GetAllSheets(doc);

		/* Finally, set empty selection just after the first Measure and put caret there.
			N.B. This will not necessarily be on the screen! We should eventually make
			the initial selection agree with the doc's scrolled position. */

		SetDefaultSelection(doc);
		doc->selStaff = 1;
		
		MEAdjustCaret(doc,FALSE);
		
		return TRUE;
	}


/* ------------------------------------------------------------------ NSInvisify -- */
/* Invisify selected staves. If there are any cross-staff objects on the staves
being invisified, this will cause problems: we don't check, since this should
be called only from Open NoteScan File routines and in that situation there are
never any cross-staff objects. ??NB: this could probably just call InvisifySelStaves
--is anything else necessary? */

void NSInvisify(Document *doc)
{	
	InvisifySelStaves(doc);

	InvalRange(doc->headL,doc->tailL);
	InvalWindow(doc);
}


/* ------------------------------------------------------- NSProblems and helper -- */
/* Do voice-by-voice and other simple consistency checks on the given Sync; display
information about any problems found. */

typedef struct {
	ListHandle hndl;
	Rect bounds,scroll,content,dataBounds;
	Point cSize,cell;
	short nCells;
} UserList;

/* Symbolic Dialog Item Numbers */

static enum {
	BUT1_OK = 1,
	LIST5=5,
	LASTITEM = 6
} E_NSProblemsItems;

#define OK_ITEM 		BUT1_OK
#define CANCEL_ITEM 	OK_ITEM

#define MAX_ERRLINES 30
#define MAX_LINECHARS 72

/* Local Prototypes */

static Boolean	NSErrListDialog(Document *doc, char errList[MAX_ERRLINES][MAX_LINECHARS], short nErrors);
static DialogPtr OpenThisDialog(char errList[MAX_ERRLINES][MAX_LINECHARS], short nErrors);
static void		CloseThisDialog(DialogPtr dlog);
static void		DoDialogUpdate(DialogPtr dlog);
static void		DoDialogActivate(DialogPtr dlog, INT16 activ);
static void		DoDialogContent(DialogPtr dlog, EventRecord *evt);
static Boolean	DoDialogItem(DialogPtr dlog, short itemHit);

static pascal  	Boolean MyFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static Boolean  AnyBadValues(DialogPtr dlog);

static Boolean	BuildList(char errList[MAX_ERRLINES][MAX_LINECHARS], short nErrors, DialogPtr dlog,
							INT16 item, INT16 csize, UserList *l);

static enum {
	ERR_VARYING_DUR,
	ERR_BEAMED_LONG_DUR,
	ERR_BADCHORD,
	ERR_RESTINCHORD
} E_NSErrs;

void BuildNSErrorMsg(Document *doc, LINK syncL, LINK noteL, short errType, char str[]);
void BuildNSErrorMsg(Document *doc, LINK syncL, LINK noteL, short errType, char str[])
{
	short measNum;
	char fmtStr[256];
	
	measNum = GetMeasNum(doc, syncL);
	GetIndCString(fmtStr, NOTESCANERRS_STRS, 16);    		/* "Measure %d, staff %d: " */
	sprintf(str, fmtStr, measNum, NoteSTAFF(noteL)); 
	switch (errType) {
		case ERR_VARYING_DUR:
			GetIndCString(strBuf, NOTESCANERRS_STRS, 17);    /* "chord has varying durations" */
			break;
		case ERR_BEAMED_LONG_DUR:
			GetIndCString(strBuf, NOTESCANERRS_STRS, 18);    /* "beamed duration of quarter note or longer" */
			break;
		case ERR_BADCHORD:
			GetIndCString(strBuf, NOTESCANERRS_STRS, 19);    /* "chord of one note, or non-chord of more than one" */
			break;
		case ERR_RESTINCHORD:
			GetIndCString(strBuf, NOTESCANERRS_STRS, 20);    /* "chord contains a rest" */
			break;
		default:
			NULL;
	}
	strcat(str, strBuf);
}

void Get1SyncProblemList(Document *doc, LINK syncL,
									char errList[MAX_ERRLINES][MAX_LINECHARS], short *pnErrors);
void Get1SyncProblemList(Document *doc, LINK syncL,
									char errList[MAX_ERRLINES][MAX_LINECHARS], short *pnErrors)
{
	char str[256];
	short v;
	short	vNotes[MAXVOICES+1],							/* No. of notes in sync in voice */
			vMainNotes[MAXVOICES+1],					/* UNUSED No. of stemmed notes in sync in voice */
			vlDur[MAXVOICES+1],							/* l_dur of voice's notes in sync */
			vlDots[MAXVOICES+1];							/* Number of dots on voice's notes in sync */
	LINK aNoteL; PANOTE aNote;
	
	for (v = 0; v<=MAXVOICES; v++) {
		vNotes[v] = vMainNotes[v] = 0;
	}
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (*pnErrors>=MAX_ERRLINES) return;
		
		aNote = GetPANOTE(aNoteL);
		vNotes[aNote->voice]++;
		if (MainNote(aNoteL)) vMainNotes[aNote->voice]++;
		vlDur[aNote->voice] = aNote->subType;
		vlDots[aNote->voice] = aNote->ndots;
	}

	/* At this point everything is initialized that needs to be: in particular,
		every vlDur[] and vlDots[] entry that will be actually be used is. */
		
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);

		/* Check consistency of durations in chords, but complain about each chord only once. */
		if ( (vlDur[aNote->voice]!=-999 && aNote->subType!=vlDur[aNote->voice])
		||   (vlDots[aNote->voice]!=-999 && aNote->ndots!=vlDots[aNote->voice]) ) {
			BuildNSErrorMsg(doc, syncL, aNoteL, ERR_VARYING_DUR, str);
			strncpy(errList[*pnErrors], str, MAX_LINECHARS-1);
			(*pnErrors)++;
			vlDur[aNote->voice] = vlDots[aNote->voice] = -999;
		}

		if (aNote->beamed)
			if (aNote->subType<=QTR_L_DUR) {
			BuildNSErrorMsg(doc, syncL, aNoteL, ERR_BEAMED_LONG_DUR, str);
			strncpy(errList[*pnErrors], str, MAX_LINECHARS-1);
			(*pnErrors)++;
			}

		if ((aNote->inChord && vNotes[aNote->voice]<2)
		||  (!aNote->inChord && vNotes[aNote->voice]>=2)) {
			BuildNSErrorMsg(doc, syncL, aNoteL, ERR_BADCHORD, str);
			strncpy(errList[*pnErrors], str, MAX_LINECHARS-1);
			(*pnErrors)++;
		}
		if (aNote->inChord && aNote->rest) {
			BuildNSErrorMsg(doc, syncL, aNoteL, ERR_RESTINCHORD, str);
			strncpy(errList[*pnErrors], str, MAX_LINECHARS-1);
			(*pnErrors)++;
		}
	}
}

short GetSyncProblemList(Document *doc, char errList[MAX_ERRLINES][MAX_LINECHARS]);
short GetSyncProblemList(Document *doc, char errList[MAX_ERRLINES][MAX_LINECHARS])
{
	short nErrors; LINK pL;
	
	for (nErrors = 0, pL = doc->headL; pL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			Get1SyncProblemList(doc, pL, errList, &nErrors);
		}
		
	return nErrors;
}

/* Get a list of problems with Syncs in the score object list and, if any are found,
display them in a dialog with a scrolling list. Return TRUE if errors were found.
NB: this function can use a lot of stack space!*/

Boolean NSProblems(Document *doc, short *pTotalErrors,
							short */*pFirstErrMeas*/, short */*pFirstErrStaff*/)	/* UNDEFINED for now! */
{
	char errList[MAX_ERRLINES][MAX_LINECHARS];
	short nErrors;
	
	nErrors = GetSyncProblemList(doc, errList);
	if (nErrors) (void)NSErrListDialog(doc, errList, nErrors);
	*pTotalErrors = nErrors;
	return (nErrors!=0);
}


static Point where;
static INT16 modifiers;

/* Lists and/or popups */

static UserList theList;

/* Display the NoteScan error list dialog.  Return TRUE if OK'd, FALSE if Cancelled
or there's an error. */

Boolean NSErrListDialog(Document */*doc*/, char errList[MAX_ERRLINES][MAX_LINECHARS],
							short nErrors)
{
	short itemHit,keepGoing=TRUE;
	register DialogPtr dlog; GrafPtr oldPort;

	/* Build dialog window and install its item values */
	
	GetPort(&oldPort);
	dlog = OpenThisDialog(errList, nErrors);
	if (dlog==NULL) return FALSE;

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(dlogFilterUPP,&itemHit);
		keepGoing = DoDialogItem(dlog,itemHit);
	}
		
	CloseThisDialog(dlog);
	SetPort(oldPort);
	
	return (itemHit==OK);
}

static pascal Boolean MyFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
	{
		Boolean ans=FALSE,doHilite=FALSE; WindowPtr w;
		short type,ch; Handle hndl; Rect box;
		static long then; static Point clickPt;
		
		w = (WindowPtr)(evt->message);
		switch(evt->what) {
			case updateEvt:
				if (w == GetDialogWindow(dlog)) {
					/* Update our dialog contents */
					DoDialogUpdate(dlog);
					ans = TRUE; *itemHit = 0;
					}
				 else {
					}
				break;
			case activateEvt:
				if (w == GetDialogWindow(dlog)) {
					DoDialogActivate(dlog,(evt->modifiers & activeFlag)!=0);
					*itemHit = 0;
					}
				 else {
					}
				break;
			case mouseDown:
			case mouseUp:
				where = evt->where;		/* Make info available to DoDialog() */
				GlobalToLocal(&where);
				modifiers = evt->modifiers;
				ans = FALSE;
				break;
			case keyDown:
				if ((ch=(unsigned char)evt->message)=='\r' || ch==CH_ENTER) {
					*itemHit = OK_ITEM;
					doHilite = ans = TRUE;
					}
				 else if (evt->modifiers & cmdKey) {
					ch = (unsigned char)evt->message;
					switch(ch) {
						case 'x':
						case 'X':
							if (TextSelected(dlog))
								{ ClearCurrentScrap(); DialogCut(dlog); TEToScrap(); }
							 else {
								}
							break;
						case 'c':
						case 'C':
							if (TextSelected(dlog))
								{ ClearCurrentScrap(); DialogCopy(dlog); TEToScrap(); }
							 else {
								}
							break;
						case 'v':
						case 'V':
							if (CanPaste(1,'TEXT'))
								{ TEFromScrap(); DialogPaste(dlog); }
							 else {
								}
							break;
						case 'a':
						case 'A':
							INT16 keyFocus = GetDialogKeyboardFocusItem(dlog);
							if (keyFocus > 0) {
								/* Dialog has text edit item: select all */
								SelectDialogItemText(dlog,keyFocus,0,ENDTEXT);
								}
							 else {
								}
							*itemHit = 0;
							break;
						case '.':
							*itemHit = CANCEL_ITEM;
							doHilite = TRUE;
							break;
						}
					ans = TRUE;		/* Other cmd-chars ignored */
					}
				break;
			}
		if (doHilite) {
			GetDialogItem(dlog,*itemHit,&type,&hndl,&box);
			/* Reality check */
			if (type == (btnCtrl+ctrlItem)) {
				long soon = TickCount() + 7;		/* Or whatever feels right */
				HiliteControl((ControlHandle)hndl,1);
				while (TickCount() < soon) ;		/* Leave hilited for a bit */
				}
			}
		return(ans);
	}

/*
 * Redraw the contents of this dialog due to update event.
 */

static void DoDialogUpdate(DialogPtr dlog)
	{
		GrafPtr oldPort;

		GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
		BeginUpdate(GetDialogWindow(dlog));

		UpdateDialogVisRgn(dlog);
		FrameDefault(dlog,BUT1_OK,TRUE);
		FrameRect(&theList.bounds);
		LUpdateDVisRgn(dlog,theList.hndl);
//		LUpdate(((GrafPtr)dlog)->visRgn,theList.hndl);

		EndUpdate(GetDialogWindow(dlog));
		SetPort(oldPort);
	}

/*
 * Activate event: Activate or deactivate this dialog and any items in it
 */

static void DoDialogActivate(DialogPtr dlog, INT16 activ)
	{
		SetPort(GetDialogWindowPort(dlog));
		LActivate(activ,theList.hndl);
	}


/*
 * Build this dialog's window on desktop, and install initial item values.
 * Return the dlog opened, or NULL if error (no resource, no memory).
 */

static DialogPtr OpenThisDialog(char errList[MAX_ERRLINES][MAX_LINECHARS], short nErrors)
	{
		GrafPtr oldPort;
		DialogPtr dlog; char str1[256], str2[256], fmtStr[256];

		dlogFilterUPP = NewModalFilterUPP(MyFilter);
		if (dlogFilterUPP == NULL) {
			MissingDialog(NSERRLIST_DLOG);
			return NULL;
		}
		GetPort(&oldPort);
		dlog = GetNewDialog(NSERRLIST_DLOG,NULL,BRING_TO_FRONT);
		if (dlog==NULL) {
			DisposeModalFilterUPP(dlogFilterUPP);
			MissingDialog(NSERRLIST_DLOG);
			return NULL;
		}

		CenterWindow(GetDialogWindow(dlog), 50);
		SetPort(GetDialogWindowPort(dlog));

		/* Fill in dialog's values here */

		sprintf(str1, "%d", nErrors);
		GetIndCString(fmtStr, NOTESCANERRS_STRS, 8);		/* "(only the first n are listed)" */
		sprintf(str2, (nErrors>MAX_ERRLINES? (char *)fmtStr : ""), MAX_ERRLINES);
		CParamText(str1, str2, "", "");
		if (!BuildList(errList,nErrors,dlog,LIST5,16,&theList)) goto broken;

		ShowWindow(GetDialogWindow(dlog));
		return(dlog);

		/* Error return */
broken:
		CloseThisDialog(dlog);
		SetPort(oldPort);
		return(NULL);
	}

/*
 * Clean up any allocated stuff, and return dialog to primordial mists
 */

static void CloseThisDialog(DialogPtr dlog)
	{
		if (theList.hndl) {
			LDispose(theList.hndl);
			theList.hndl = NULL;
		}
		DisposeModalFilterUPP(dlogFilterUPP);
		DisposeDialog(dlog);
	}

/*
 * Handle OK button, and possibly handle a click in the list.
 * The local point is in where; modifiers in modifiers.
 * Returns whether or not the dialog should be closed (keepGoing).
 */

static Boolean DoDialogItem(DialogPtr dlog, short itemHit)
	{
		Boolean okay=FALSE,keepGoing=TRUE;

		if (itemHit<1 || itemHit>=LASTITEM)
			return(keepGoing);										/* Only legal items, please */

		switch(itemHit) {
			case BUT1_OK:
				keepGoing = FALSE; okay = TRUE;
				break;
			case LIST5:
				/* Handle click in the list: scroll if necessary. */
				if (LClick(where,modifiers,theList.hndl)) {
					/* Handle double-clicking here */
					}
				break;
			}

		if (okay) keepGoing = AnyBadValues(dlog);
		return(keepGoing);
	}

/*
 * Pull values out of dialog items and deliver TRUE if any of them are illegal
 * or inconsistent; otherwise deliver FALSE.
 */

static Boolean AnyBadValues(DialogPtr /*dlog*/)
	{
		return FALSE;
	}

/* --------------------------------------------------------------------------------- */

INT16 GetNSErrorList(UserList *l, INT16 nErrors);

static void ErrorData2List(char errList[MAX_ERRLINES][MAX_LINECHARS], short nErrors,
									UserList *l);
static void ErrorData2List(char errList[MAX_ERRLINES][MAX_LINECHARS], short nErrors,
									UserList *l)
	{
		char cellstr[256]; short i;

		/*
		 * Set the length of the list to the number of errors, and add a
		 * single row with that many cells.
		 */
		l->nCells = nErrors;
		LAddRow(l->nCells,0,l->hndl);
			
		for (i = 0; i<nErrors; i++) {
			l->cell.h = 0; l->cell.v = i;
			strcpy(cellstr, errList[i]);
			CtoPstr((StringPtr)cellstr);
			LSetCell(cellstr+1,*cellstr,l->cell,l->hndl);
		}
	}
	
/*
 *	Build a new list in given user item box of dialog, dlog,
 *	with cell height, csize.  If success, delivers TRUE; if couldn't
 *	allocate ListMgr list (no more memory or whatever), delivers FALSE.
 */

static Boolean BuildList(char errList[MAX_ERRLINES][MAX_LINECHARS], short nErrors,
							DialogPtr dlog, INT16 item, INT16 csize, UserList *l)
	{
		short type; Rect box; Handle hndl;

		/* Content area (plus scroll bar) of list corresponds to user item box */
		
		GetDialogItem(dlog,item,&type,&hndl,&box);
		l->bounds = box; InsetRect(&l->bounds,-1,-1);
		SetDialogItem(dlog,item,userItem,NULL,&l->bounds);

		l->scroll = box; l->scroll.left = l->scroll.right - 15;		/* Scrollbar width */
		l->content = box; l->content.right = l->scroll.left;

		SetRect(&l->dataBounds,0,0,1,0);
		l->cSize.v = csize > 0 ? csize : 1;
		l->cSize.h = l->content.right - l->content.left;

		l->hndl = LNew(&l->content,&l->dataBounds,l->cSize,0,GetDialogWindow(dlog),FALSE,FALSE,FALSE,TRUE);
		if (!l->hndl) return FALSE;

		(*l->hndl)->selFlags = lOnlyOne;		/* Or whatever */
		
		ErrorData2List(errList,nErrors,l);

		l->cell.v = 0;
		LSetSelect(TRUE,l->cell,l->hndl);
		EraseRect(&l->content);
		InvalWindowRect(GetDialogWindow(dlog),&l->bounds);
		LSetDrawingMode(TRUE,l->hndl);

		return TRUE;
	}


/* ================================================================================= */

/* As of v. 3.5, NoteScan is always enabled. The code to check a user enabling code
could certainly be valuable in the future, so I'll leave it here, commented out,
for a while, though it should really be archived elsewhere--and is, as part of the
source code for Nightingale 3.0: is that sufficient? Also, the resources involved
in this should eventually be discarded. */

#if 1

Boolean NSEnabled()
{
	return TRUE;
}

#else

Boolean GoodEnableNSCode(unsigned long);
Boolean GoodEnableNSCode(unsigned long code)
{
	Handle snRsrc;
	unsigned long serialNum, enableCode;
		
	snRsrc = GetResource('twst', 1000);
	if (!snRsrc) {
		GetIndCString(strBuf, NOTESCANERRS_STRS, 9);		/* "Can't get or check serial no." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	
	LoadResource(snRsrc);											/* Just to be safe */
	serialNum = *(unsigned long *)(*snRsrc);
	serialNum = ~serialNum;
	enableCode = serialNum % 10000L;								/* Only the bottom 4 digits */

#ifndef PUBLIC_VERSION
	if (!CapsLockKeyDown() || !ShiftKeyDown())				/* So developers can test this */
		return TRUE;
#endif
	return (code==enableCode);
}

#define MAX_TRIES 3

#define ENABLENUM_DI 6

/* Return TRUE if OK, FALSE if Cancel. In either case, update *pnTries, but if
Cancel, *pEnabled and *pCode may not be meaningful. */

Boolean EnableNSDialog(INT16 *, Boolean *, unsigned long *);
Boolean EnableNSDialog(
				INT16 *pnTries,
				Boolean *pEnabled,
				unsigned long *pCode 		/* if dialog OKed, number the user entered */
				)
{
	DialogPtr	dlog;
	INT16			ditem;
	GrafPtr		oldPort;
	Boolean		keepGoing=TRUE;
	unsigned long code;
	Boolean		enabled=FALSE;
		
	GetPort(&oldPort);
	dlog = GetNewDialog(ENABLENS_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) { MissingDialog(ENABLENS_DLOG); return FALSE; }

	SetPort(GetDialogWindowPort(dlog));
	PutDlgWord(dlog, ENABLENUM_DI, 0, TRUE);

	ShowWindow(dlog);
	ArrowCursor();

	while (keepGoing) {
		ModalDialog(OKButFilter, &ditem);
		if (ditem==OK) {
			GetDlgLong(dlog, ENABLENUM_DI, (long *)&code);
			(*pnTries)++;
			keepGoing = FALSE;
			/* If not too many tries yet, let them try to type the correct code. */
			 
			if (GoodEnableNSCode(code)) {
				enabled = TRUE;
			}
			else {
				WaitCursor();
				SleepTicks(60L*5L);										/* Long delay to foil automated cracking */
				if (*pnTries<MAX_TRIES) {
					keepGoing = TRUE;										/* Give them a total of a few tries per run */
					GetIndCString(strBuf, NOTESCANERRS_STRS, 12);	/* "Wrong code" */
					CParamText(strBuf, "", "", "");
				}
				else {
					GetIndCString(strBuf, NOTESCANERRS_STRS, 13);	/* "too many wrong tries" */
					CParamText(strBuf, "", "", "");
				}
				StopInform(GENERIC_ALRT);
			}
		}
		else if (ditem==Cancel)
			keepGoing = FALSE;
	}
		
	DisposeDialog(dlog);
	SetPort(oldPort);
	
	/* *pnTries has already been updated, regardless of Cancel--as it should. */
	
	*pEnabled = enabled;
	*pCode = code; 
	return (ditem==OK);
}

/* Handle everything related to enabling and checking for enabling of the Open
NoteScan command, including a dialog for entering the enabling code, if needed.
Return TRUE if Open NoteScan is now (when we return) enabled. */

#define ENABLED_RES_SIZE (sizeof(unsigned long) + 4)		/* extra bytes "just in case" */
#define ENABLED_RES_TYPE 'here'
#define ENABLED_RES_ID 1000

Boolean NSEnabled()
{
	unsigned long code;
	static INT16 nTries;
	static Boolean enabled, firstCall=TRUE;
	Handle enabledResH, oldResH;
	short oldResFile;
	Boolean dialogOKed=TRUE;
		
#ifdef DEMO_VERSION
	return TRUE;
#endif

	if (firstCall) {
		firstCall = FALSE;
		enabled = FALSE;
		nTries = 0;

		/*
		 * If the enabling resource exists, it's easy: if it contains the correct code
		 * for this copy of Nightingale, enable Open NoteScan for this run; otherwise
		 * just give user an explanation, and set the number of tries to "too many"
		 * so this routine won't give them any more chances to guess they code--anyway,
		 * unless the resource is damaged, they really have had too many tries, but in a
		 * previous run of Nightingale.
		 */
		oldResFile = CurResFile();
		UseResFile(appRFRefNum);
		enabledResH = Get1Resource(ENABLED_RES_TYPE, ENABLED_RES_ID);
		UseResFile(oldResFile);		
		if (enabledResH) {
			code = **(unsigned long **)enabledResH;
			if (GoodEnableNSCode(code)) {
				enabled = TRUE;
				return TRUE;
			}
			else {
				GetIndCString(strBuf, NOTESCANERRS_STRS, 10);		/* "Check of enabling code failed" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				nTries = MAX_TRIES;
				return FALSE;
			}
		}
	}
	else if (enabled)
		return TRUE;
	
	if (nTries>=MAX_TRIES) {
		GetIndCString(strBuf, NOTESCANERRS_STRS, 13);				/* "too many wrong tries" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return enabled;
	}
	
	/*
	 * No enabling resource, and they have one or more tries left. Go thru the dialog
	 * until either the user gives up, runs out of tries, or gets the code right. We
	 * want to make things hard for would-be automated code crackers, so we waste a
	 * few seconds after each wrong try, and after a few wrong tries, prevent them from
	 * trying again by writing an enabling resource with the wrong code.
	 */
	
	while (dialogOKed && nTries<MAX_TRIES &&!enabled) {
		dialogOKed = EnableNSDialog(&nTries, &enabled, &code);

		/* Write resource if dialog OKed and either got correct code or exceeded MAX_TRIES */

		if (dialogOKed && (enabled || nTries>=MAX_TRIES)) {
			enabledResH = NewHandle(ENABLED_RES_SIZE);
			if (!GoodNewHandle(enabledResH)) {
				GetIndCString(strBuf, NOTESCANERRS_STRS, 11);		/* "Can't enable Open NS" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				break;				
			}
			**(unsigned long **)enabledResH = code;
			oldResFile = CurResFile();
			UseResFile(appRFRefNum);
			oldResH = Get1Resource(ENABLED_RES_TYPE, ENABLED_RES_ID);
			if (GoodNewHandle(oldResH)) RemoveResource(oldResH);	/* Ignore possible error */
			AddResource(enabledResH, ENABLED_RES_TYPE, ENABLED_RES_ID, "\p");
			if (!ReportResError())
				WriteResource(enabledResH);							/* Force updating the file */
			UseResFile(oldResFile);		
		}
	}

	return enabled;
}

#endif
