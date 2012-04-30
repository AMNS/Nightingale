/* ResultList.c for Nightingale Search, by Donald Byrd. Incorporates modeless-dialog
handling code from Apple sample source code by Nitin Ganatra. */

// MAS
//#include "Nightingale.precomp.h"
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "SearchScorePrivate.h"

//#include <string.h>
//#include <Dialogs.h>
//#include <Lists.h>
//#include <Sound.h>
// MAS

#ifdef SEARCH_DBFORMAT_MEF

void DoDialogEvent(EventRecord *theEvent)
{
	AlwaysErrMsg("DoDialogEvent called in the MEF version!");
}

#else

extern Boolean gDone;
extern Boolean gInBackground;

typedef struct {
	ListHandle hndl;
	Rect bounds,scroll,content,dataBounds;
	Point cSize,cell;
	short nCells;
} UserList;

static INT16 itemCount, maxItemCount, sPatLen;
static UserList theList;
static UserList *gResUserList;


static char *resultStr = NULL;
static INT16 *docNumA = NULL;
static LINK *foundLA = NULL;
static INT16 *foundVoiceA = NULL;
static DB_LINK *matchedObjFA = NULL;
static DB_LINK *matchedSubobjFA = NULL;

#define MAX_LINELEN 100

static enum {
	BUT1_OK = 1,
	LIST2,
	LABEL_QMATCHESQ,
	LABEL_MATCHES,
	LASTITEM
} E_ResultListItems;

Boolean EventFilter(EventRecord *theEvent, WindowPtr theFrontWindow);

static pascal void DrawButtonBorder(DialogPtr, short);
static pascal void DrawList(DialogPtr, short);

static Boolean HandleListEvent(EventRecord *theEvent);
static pascal Boolean ResListFilter(DialogPtr, EventRecord *, short *);
static void ProcessModalEvents(void);

static void CloseToolPalette(void);

Boolean InitResultList(INT16 maxItems, INT16 patLen)
{
	long matchArraySize;

	if (resultStr) DisposePtr(resultStr);
	if (docNumA) DisposePtr((char *)docNumA);
	if (foundLA) DisposePtr((char *)foundLA);
	if (foundVoiceA) DisposePtr((char *)foundVoiceA);
	if (matchedObjFA) DisposePtr((char *)matchedObjFA);
	if (matchedSubobjFA) DisposePtr((char *)matchedSubobjFA);
	
	resultStr = NewPtr(maxItems*MAX_LINELEN);
	if (!GoodNewPtr((Ptr)resultStr))
		{ OutOfMemory(maxItems*MAX_LINELEN); return FALSE; }

	docNumA = (INT16 *)NewPtr(maxItems*sizeof(INT16));
	if (!GoodNewPtr((Ptr)docNumA))
		{ OutOfMemory(maxItems*sizeof(INT16)); return FALSE; }
	
	foundLA = (LINK *)NewPtr(maxItems*sizeof(LINK));
	if (!GoodNewPtr((Ptr)foundLA))
		{ OutOfMemory(maxItems*sizeof(LINK)); return FALSE; }
	
	foundVoiceA = (INT16 *)NewPtr(maxItems*sizeof(INT16));
	if (!GoodNewPtr((Ptr)foundVoiceA))
		{ OutOfMemory(maxItems*sizeof(INT16)); return FALSE; }
	
	matchArraySize = (MAX_HITS+1)*MAX_PATLEN;
	matchedObjFA = (DB_LINK *)NewPtr(matchArraySize*sizeof(DB_LINK));
	if (!GoodNewPtr((Ptr)matchedObjFA))
		{ OutOfMemory(matchArraySize*sizeof(DB_LINK)); return FALSE; }

	matchedSubobjFA = (DB_LINK *)NewPtr(matchArraySize*sizeof(DB_LINK));
	if (!GoodNewPtr((Ptr)matchedSubobjFA))
		{ OutOfMemory(matchArraySize*sizeof(DB_LINK)); return FALSE; }

	itemCount = 0;
	maxItemCount = maxItems;
	sPatLen = patLen;
	return TRUE;
}


Boolean AddToResultList(
				char str[],
				MATCHINFO matchInfo,
				DB_LINK matchedObjA[MAX_PATLEN],		/* Array of matched objects, by voice */
				DB_LINK matchedSubobjA[MAX_PATLEN]		/* Array of match subobjs, by voice */
)
{
	short k; long offset;
	
	if (itemCount>=maxItemCount) return FALSE;

	if (!resultStr) return FALSE;
	if (!docNumA) return FALSE;
	if (!foundLA) return FALSE;
	if (!foundVoiceA) return FALSE;
	if (!matchedObjFA) return FALSE;
	if (!matchedSubobjFA) return FALSE;
		
#ifdef NOMORE
{
	short m;

	DebugPrintf("AddToResultList: matchedObjA=");
	for (m = 0; m<sPatLen; m++)
		DebugPrintf("%d ", matchedObjA[m]);
	DebugPrintf("\n");

	DebugPrintf("AddToResultList: matchedSubobjA=");
	for (m = 0; m<sPatLen; m++)
		DebugPrintf("%d ", matchedSubobjA[m]);
	DebugPrintf("\n");
}
#endif
	GoodStrncpy(resultStr+(itemCount*MAX_LINELEN), str, MAX_LINELEN-1);
	docNumA[itemCount] = matchInfo.docNum;
	foundLA[itemCount] = matchInfo.foundL;
	foundVoiceA[itemCount] = matchInfo.foundVoice;
	for (k = 0; k<sPatLen; k++) {
		offset = (itemCount*MAX_PATLEN)+k;
		*(matchedObjFA+offset) = matchedObjA[k];
		*(matchedSubobjFA+offset) = matchedSubobjA[k];
	}
	itemCount++;
	return TRUE;
}


/* ------------------------------------------------------------------------------------- */
/*
	GetCtlHandle - Just a handy way to get a control's handle given only the item
	number and the dialog.
*/
static ControlHandle GetCtlHandle(DialogPtr theDialog, short theItem)
{
	Handle 	theControl;
	short	aType;
	Rect	aRect;
	
	GetDialogItem(theDialog, theItem, &aType, &theControl, &aRect);
	if (theControl==NULL)
		DebugStr((StringPtr)"\pGetDItem in GetCtlHandle failed");

	return (ControlHandle)theControl;
}


/* ------------------------------------------------------------------------------------- */

/* Convert internal voice number to staff number. For multi-staff parts, may not give
ideal results. ??Could be a useful utility in general; belongs elsewhere, maybe in
VoiceNumbers.c. */

INT16 VoiceNum2StaffNum(Document *doc, INT16 voice);
INT16 VoiceNum2StaffNum(Document *doc, INT16 voice)
{
	INT16 userVoice, nPartStaves; LINK aPartL;
	
	if (Int2UserVoice(doc, voice, &userVoice, &aPartL)) {
		/* For default voices, assume their default staves; for other voices, just
		 * assume the part's first staff.
		 */
		nPartStaves = PartLastSTAFF(aPartL)-PartFirstSTAFF(aPartL)+1;
		if (userVoice<=nPartStaves) return PartFirstSTAFF(aPartL)+userVoice-1;
		else return PartFirstSTAFF(aPartL);
	}
	else
		/* Something is wrong; oh well. */
		return 1;	
}


void DebugShowObjAndSubobj(char *label, DB_LINK matchedObjFA[][MAX_PATLEN],
							DB_LINK matchedSubobjFA[][MAX_PATLEN], INT16 patLen,
							INT16 nFound);

/* Make the given objects and subobjects the selection. For now, the objects should
all be Syncs: objects of other types are ignored. Deselect everything, then select
notes/rests in the given arrays and voice. Finally, update selection range and <setStaff>
accordingly, and Inval the window. NB: operates on Nightingale object list, not
DB_ stuff.

What if, after getting the result list, the user edited the score, or closed it and opened
another? We look for improper situations and simply return FALSE if we find one. However,
as of this writing, the checking is pretty weak; see below. */

Boolean SelectMatch(DB_Document *doc, LINK matchedObjA[MAX_PATLEN],
			LINK matchedSubobjA[MAX_PATLEN], INT16 count,
			Boolean matchTiedDur);
Boolean SelectMatch(
			Document *doc,
			LINK matchedObjA[MAX_PATLEN],
			LINK matchedSubobjA[MAX_PATLEN],
			INT16 count,
			Boolean matchTiedDur				/* Ignored for now */
			)
{
	INT16 n, staffn; LINK pL, aNoteL;
	
	/* Try to protect ourselves against "improper situations", but NB: this won't catch
	 * every problem case by a long shot! We're not checking that the subobjs exist, or
	 * that they match the objects they're supposed to belong to.
	 */
	for (n = 0; n<count; n++) {
		if (!InDataStruct(doc, matchedObjA[n], MAIN_DSTR))
			return FALSE;
	}
	
	DeselAll(doc);
	
	for (n = 0; n<count; n++) {
		pL = matchedObjA[n];
		if (!SyncTYPE(pL)) continue;
		
		aNoteL = matchedSubobjA[n];
		LinkSEL(pL) = TRUE;
		NoteSEL(aNoteL) = TRUE;
		//if (matchTiedDur) SelectTiedNotes(pL, aNoteL);
		staffn = NoteSTAFF(aNoteL);
	}

	doc->selStartL = matchedObjA[0];
	for (pL = LeftLINK(doc->tailL); pL!=LeftLINK(doc->selStartL); pL = LeftLINK(pL))
		if (LinkSEL(pL)) break;
	doc->selEndL = RightLINK(pL);
	doc->selStaff = staffn;

	InvalWindow(doc);
	return TRUE;
}

/* Show the given match by bringing the score to the front, selecting its notes/rests,
and scrolling to get them in view. Return TRUE if we succeed, FALSE if not. ??PROBABLY
NEED MORE INFO ON THE SCORE FOR THIS...FILE refNum, ETC. IS PROBABLY THE ONLY ROBUST
WAY, AND BEST TO ADD TO <MATCHINFO>! */

Boolean ShowMatch(INT16 matchNum);
Boolean ShowMatch(INT16 matchNum)
{
	extern Document *documentTable,			/* Ptr to head of score table */
						*topTable;					/* 1 slot past last entry in table */
	Document			*doc;
	long				offset;

	doc = documentTable+docNumA[matchNum];
	if (!doc->inUse) {
		CParamText("Can't show that match: the score is no longer open.", "",
						"", "");								// ??I18N BUG AND NOT USER-FRIENDLY
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	
	offset = matchNum*MAX_PATLEN;
	//DebugShowObjAndSubobj("ShowMatch", (DB_LINK (*)[MAX_PATLEN])(matchedObjFA+offset),
	//						(DB_LINK (*)[MAX_PATLEN])(matchedSubobjFA+offset), sPatLen, 1);
	InstallDoc(doc);
#if 111
	if (!SelectMatch(doc, (matchedObjFA+offset), (matchedSubobjFA+offset), sPatLen, FALSE)) {
		CParamText("Can't show that match: the score is no longer open, or the matching section has been edited.", "",
						"", "");								// ??I18N BUG AND NOT USER-FRIENDLY
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
#else
	DeselAll(doc);
	doc->selStartL = doc->selEndL = foundLA[matchNum];	/* Set insertion point */
	doc->selStaff = VoiceNum2StaffNum(doc, foundVoiceA[matchNum]);
	MEAdjustCaret(doc, TRUE);
#endif

	SelectWindow((WindowPtr)doc);
	GoToSel(doc);
	InvalWindow(doc);
	EraseAndInvalMessageBox(doc);
	return TRUE;
}


/* ------------------------------------------------------------------------------------- */
/*
	DoDialogEvent - After checking that EventFilter's result is TRUE, this calls 
	DialogSelect to check if any controls were hit, and if so this acts accordingly.
*/
// MAS #if 0
#if 1
void DoDialogEvent(EventRecord *theEvent)
{
	//DialogPtr	theDialog = (DialogPtr)FrontWindow();
	WindowPtr frontWindow = FrontWindow();
	short		theItem;
	GrafPtr		origPort;
	Point		where;
	INT16		modifiers;

	GetPort(&origPort);
	//SetPort(GetDialogWindowPort(theDialog));	
	SetPort(GetWindowPort(frontWindow));	

	if (EventFilter(theEvent, frontWindow)) {
		TextFont(0);									/* Set to the system font */
		TextSize(12);
		DialogPtr theDialog = GetDialogFromWindow(frontWindow);
		if (DialogSelect(theEvent, &theDialog, &theItem)) {
			if (theDialog!=NULL) {
				switch (theItem) {
					case LIST2:
						where = theEvent->where;
						GlobalToLocal(&where);
						modifiers = theEvent->modifiers;
						if (LClick(where, modifiers, theList.hndl)) {
							theList.cell.h = theList.cell.v = 0;
							if (LGetSelect(TRUE, &theList.cell, theList.hndl)) {
								short i;
				
								/* A cell was double-clicked: its number is in <theList.cell.v>.
								 * Show the corresponding match.
								 */
								for (i = 0; i<theList.nCells; i++) {
									if (i==theList.cell.v) {
										(void)ShowMatch(i);
										goto Finish;
									}
								}
								/* Something is wrong. */
								SysBeep(10);
							}
						}
						break;

					case BUT1_OK:
						/* Just throw out the dialog and return. */
						DisposeDialog(theDialog);
						break;
			
				}
			}
		}
	}

Finish:
	SetPort(origPort);
	return;

BailOut:
	DebugStr((StringPtr)"\pGetCtlHandle in DoDialogEvent failed");
}
#endif

/* ------------------------------------------------------------------------------------- */

short BuildList(DialogPtr dlog, short item, short fontSize, UserList *l);

static void SetListLength(UserList *l);

/* Set the length of the list to the number of matches. */

static void SetListLength(UserList *l)
{
	l->nCells = itemCount;
}

static INT16 GetListLength()
{
	return itemCount;
}

static void GetResultListData(UserList *l)
{
	INT16 i;
	static char cellstr[MAX_LINELEN];

	//SetListLength(l);
	//LAddRow(l->nCells,0,l->hndl);

	for (i = 0; i<l->nCells; i++) {
		l->cell.h = 0; l->cell.v = i;
		GoodStrncpy(cellstr, resultStr+(i*MAX_LINELEN), MAX_LINELEN-1);
		CToPString(cellstr);
		LSetCell(cellstr+1,*cellstr,l->cell,l->hndl);
	}
	LActivate(TRUE, l->hndl);
}


static void PrepareResultList(UserList *l) 
{
	INT16 count = GetListLength();
	INT16 delta = count - l->nCells;
	
	if (delta > 0) 
		LAddRow(delta, l->nCells,l->hndl);
	else
		LDelRow(count, delta,l->hndl);
	
	SetListLength(l);
	
	for (INT16 i = 0; i<l->nCells; i++) {
		l->cell.h = 0; l->cell.v = i;
		LClrCell(l->cell, l->hndl);
	}
}

/* Build a new list in given user item box of given dialog, with cell height
determined by the given font size.  If success, delivers TRUE; if couldn't allocate
ListMgr list (no more memory or whatever), delivers FALSE. */

short BuildList(DialogPtr dlog, short item, short fontSize, UserList *l)
{
	short type, csize; Rect box; Handle hndl;

	csize = fontSize+4;
	
	/* Content area (plus scroll bar) of list corresponds to user item box */
	
	GetDialogItem(dlog,item,&type,&hndl,&box);
	l->bounds = box; InsetRect(&l->bounds,-1,-1);
	SetDialogItem(dlog,item,userItem,NULL,&l->bounds);

	l->scroll = box; l->scroll.left = l->scroll.right - 15;		/* Scrollbar width */
	l->content = box; 
	l->content.right = l->scroll.left;
	l->content.bottom = box.bottom - 15;

	SetRect(&l->dataBounds,0,0,1,0);
	l->cSize.v = csize > 0 ? csize : 1;
	l->cSize.h = l->content.right - l->content.left;

	/* Set font for list */
	TextFont(1);			// ??OR SYSFONTID_SANSSERIF
	TextSize(fontSize);
	TextFace(0);											/* Plain */

	l->hndl = LNew(&l->content,&l->dataBounds,l->cSize,0,GetDialogWindow(dlog),FALSE,FALSE,FALSE,TRUE);
	if (!l->hndl) return FALSE;

	(*l->hndl)->selFlags = lOnlyOne;		/* Or whatever */
	
	GetResultListData(l);

	l->cell.v = 0;
	LSetSelect(TRUE,l->cell,l->hndl);
	EraseRect(&l->content);
	InvalWindowRect(GetDialogWindow(dlog),&l->bounds);
	//InvalRect(&l->bounds);
	LSetDrawingMode(TRUE,l->hndl);

	return TRUE;
}



#define RESULTLIST_DLOG 966

/* ------------------------------------------------------------------------------------- */
/* DoResultList - Creates the modeless dialog (from a DLOG resource) and sets
up the initial state of the controls and user items.  Note that it also allocates
two globals that will always point to the user item proc routines.  This is needed
only once and can be called by all sebsequent calls to DoResultList, because
the routines won't change.  In the 68K world, these routines are redefined to just 
be plain old proc pointers; on PowerPC builds they use real UPPs.
*/

/* Display the Result List modeless dialog.  Return TRUE if all OK, FALSE on error. */

Boolean DoResultList(char label[])
{
	short 					tempType;
	Handle					tempHandle;
	Rect						tempRect;
	DialogPtr				mlDlog;
	static UserItemUPP	procForBorderUserItem = NULL;
	static UserItemUPP	procForListUserItem = NULL;

	mlDlog = GetNewDialog(RESULTLIST_DLOG, NULL, BRING_TO_FRONT);
	if (mlDlog==NULL) { MissingDialog(RESULTLIST_DLOG); return FALSE; }
	
	gResultListDlog = mlDlog;
	
	if (procForBorderUserItem==NULL)
		procForBorderUserItem = NewUserItemUPP(DrawButtonBorder);	

	if (procForListUserItem==NULL)
		procForListUserItem = NewUserItemUPP(DrawList);	
		
	PutDlgString(mlDlog, LABEL_MATCHES, CToPString(label), FALSE);

	if (!BuildList(mlDlog, LIST2, 10, &theList)) {
		CParamText("DoResultList: couldn't build list; probably out of memory.", "",
						"", "");								// ??I18N BUG AND NOT USER-FRIENDLY
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	/* set up the user item that will draw the list */
	GetDialogItem(mlDlog, LIST2, &tempType, &tempHandle, &tempRect);
	SetDialogItem(mlDlog, LIST2, tempType, (Handle)procForListUserItem, &tempRect);

	WindowPtr w = GetDialogWindow(mlDlog);
	ShowWindow(w);
	SelectWindow(w);
	
	//ProcessModalEvents();
	
	/* Just throw out the dialog and return. */
	//DisposeDialog(mlDlog);
	
	//CloseToolPalette();

	return TRUE;
}

/* HiliteAllControls - This is used in response to activate events for the dialog.  If
an activate event is sent, hilite all controls to be active, and if its a deactivate
event, unhilite all controls. */

static void HiliteAllControls(DialogPtr theDialog, short hiliteCode)
{
	ControlHandle theControl;

	theControl = GetCtlHandle(theDialog, BUT1_OK);
	if (theControl!=NULL)
		HiliteControl(theControl, hiliteCode);

}


/* DrawButtonBorder - Standard way to draw a border around the default button. */

static pascal void DrawButtonBorder(DialogPtr theDialog, short theItem)
{
#pragma unused (theItem)
	Handle			theControl;
	short			aType;
	Rect			aRect;

	GetDialogItem((DialogPtr)theDialog, BUT1_OK, &aType, &theControl, &aRect);
	
	PenSize(3,3);
	InsetRect(&aRect, -4, -4);
	FrameRoundRect(&aRect, 16,16);
}


/* DrawList - ?????????. */

static pascal void DrawList(DialogPtr theDialog, short theItem)
{
#pragma unused (theItem)
	FrameRect(&theList.bounds);
	//LUpdate(theDialog->visRgn, theList.hndl);
	
	LUpdateDVisRgn(theDialog, theList.hndl);
}

static void CloseToolPalette() 
{
	if (palettesVisible[TOOL_PALETTE] ) {
		PaletteGlobals *pg = *paletteGlobals[TOOL_PALETTE];
		palettesVisible[TOOL_PALETTE] = FALSE;
		DoCloseWindow(pg->paletteWindow);				
	}			
}


/* EventFilter - This is the first thing done if a dialog event is received, giving 
you a chance to perform any special stuff before passing control on to 
DialogSelect.

If this routine returns TRUE, the event processing will continue, and DialogSelect
will be called to perform hit detection on the controls.  If FALSE is returned,
it means the event is already handled and the main event loop will continue. */
	
Boolean EventFilter(EventRecord *theEvent, WindowPtr theFrontWindow)
{
	char 		theKey;
	short 		theHiliteCode;
	
	switch (theEvent->what)
	{	
		case keyDown:
		case autoKey:	
				theKey = theEvent->message & charCodeMask;

				if ((theEvent->modifiers & cmdKey)!=0) {	
#if 0
					long menuResult;
				
					AdjustMenus();
					menuResult = MenuKey(theKey);
			
					if ((menuResult >> 16) != 0) {
						Boolean editOpPerformed = MenuCommand(menuResult);
						
						if (editOpPerformed)
							/* You may ask yourself, "Why are we exiting when an 	*/
							/* edit operation is performed?"  Well, DialogSelect 	*/
							/* performs some automatic handling for any editText 	*/
							/* items that may be in the Dialog, and since we've 	*/
							/* already handled them in MenuCommand() we don't 		*/
							/* want  DialogSelect to do anything		 			*/
							return FALSE;
					}
#endif
				}
				
				break;
				
		case mouseDown:
			WindowPtr w = (WindowPtr)theEvent->message;
			WindowPartCode part = FindWindow(theEvent->where,&w);
			//w==GetDialogWindow(dlog);
			Rect bounds = GetQDScreenBitsBounds();
			switch (part) {
				case inContent:
					if (HandleListEvent(theEvent)) {
						return TRUE;
					}
					break;
				case inDrag:
					DragWindow(w,theEvent->where,&bounds);
					return TRUE;
					break;
					
				default:
					break;					
			}
			return  FALSE;
			break;

		case activateEvt:				
				/* This is where we take care of hiliting the		*/
				/* controls according to whether or not the dialog 	*/
				/* is frontmost. 									*/
				theFrontWindow = (WindowPtr)theEvent->message;
				if (theEvent->modifiers & activeFlag)
					theHiliteCode = CTL_ACTIVE;
				else
					theHiliteCode = CTL_INACTIVE;

				HiliteAllControls(GetDialogFromWindow(theFrontWindow), theHiliteCode);
				
				//if (theHiliteCode == CTL_ACTIVE)
				//	CloseToolPalette();
				
				return FALSE;

	}

	/* If we haven't returned FALSE by now, go ahead	*/
	/* and return TRUE so DialogSelect can do its 		*/
	/* thing, like update the window, deal with an 		*/
	/* item hit, etc.									*/
	return TRUE;
}

static Boolean HandleListEvent(EventRecord *theEvent) 
{
	Point where = theEvent->where;
	GlobalToLocal(&where);
	int modifiers = theEvent->modifiers;
	if (LClick(where, modifiers, theList.hndl)) {
		theList.cell.h = theList.cell.v = 0;
		if (LGetSelect(TRUE, &theList.cell, theList.hndl)) {
			short i;

			/* A cell was double-clicked: its number is in <theList.cell.v>.
			 * Show the corresponding match.
			 */
			for (i = 0; i<theList.nCells; i++) {
				if (i==theList.cell.v) {
					(void)ShowMatch(i);
					return TRUE;
				}
			}
			/* Something is wrong. */
			SysBeep(10);
		}
		
		return TRUE;
	}
	
	return FALSE;
}

static pascal Boolean ResListFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr		w;
	//register short	ch, ans;
	//Point				where;
	GrafPtr			oldPort;
	WindowPtr theFrontWindow;
	short 		theHiliteCode;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));			
				UpdateDialogVisRgn(dlog);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return TRUE;
			}
			break;
		case activateEvt:
				/* This is where we take care of hiliting the		*/
				/* controls according to whether or not the dialog 	*/
				/* is frontmost. 									*/
				theFrontWindow = (WindowPtr)evt->message;
				if (evt->modifiers & activeFlag)
					theHiliteCode = CTL_ACTIVE;
				else
					theHiliteCode = CTL_INACTIVE;

				HiliteAllControls(GetDialogFromWindow(theFrontWindow), theHiliteCode);
			break;
		case mouseDown:
			w = (WindowPtr)evt->message;
			WindowPartCode part = FindWindow(evt->where,&w);
			//w==GetDialogWindow(dlog);
			Rect bounds = GetQDScreenBitsBounds();
			switch (part) {
				case inContent:
					if (HandleListEvent(evt)) {
						return TRUE;
					}
					break;
				case inDrag:
					DragWindow(w,evt->where,&bounds);
					break;
					
				default:
					break;					
			}
			break;
		case mouseUp:
			break;
		case keyDown:
			break;
	}

	return FALSE;
}

static void ProcessModalEvents() 
{
	ModalFilterUPP	filterUPP;
	Boolean done = FALSE;
	short ditem;
	
	WindowPtr frontWindow = FrontWindow();
	GrafPtr		origPort;
	GetPort(&origPort);
	SetPort(GetWindowPort(frontWindow));	
	
	filterUPP = NewModalFilterUPP(ResListFilter);
	if (filterUPP == NULL) { return; }
	
	while (!done) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
				done = TRUE;
				break;
		}
	}
	
	if (filterUPP != NULL)
		DisposeModalFilterUPP(filterUPP);
	SetPort(origPort);
}

/* ------------------------------------------------------------------------------------- */

void InitList(UserList *l) 
{
	
}

Boolean BuildDocList(Document *doc, short fontSize)
{
	short csize; Rect box;
	WindowPtr w = doc->theWindow;
	UserList *l = &theList;
		
	csize = fontSize+4;
	
	/* Content area (plus scroll bar) of list corresponds to user item box */
	
	GetWindowPortBounds(w,&box);
	//InsetRect(&box, 4, 4);
	l->bounds = box; InsetRect(&l->bounds,-1,-1);

	l->scroll = box; l->scroll.left = l->scroll.right - 15;		/* Scrollbar width */
	l->content = box;
	l->content.right = l->scroll.left;
	l->content.bottom = box.bottom - 15;

	SetRect(&l->dataBounds,0,0,50,1);
	l->cSize.v = csize > 0 ? csize : 1;
	l->cSize.h = l->content.right - l->content.left;

	/* Set font for list */
	TextFont(1);			// ??OR SYSFONTID_SANSSERIF
	TextSize(fontSize);
	TextFace(0);											/* Plain */

	l->hndl = LNew(&l->content,&l->dataBounds,l->cSize,0,w,FALSE,FALSE,FALSE,TRUE);
	if (!l->hndl) return FALSE;

	(*l->hndl)->selFlags = lOnlyOne;		/* Or whatever */
	
	l->cell.v = 0;
	LSetSelect(TRUE,l->cell,l->hndl);
	EraseRect(&l->content);
	InvalWindowRect(w,&l->bounds);
	LSetDrawingMode(TRUE,l->hndl);
	
	gResUserList = l;

	return TRUE;
}

Boolean HandleResListUpdate()
{
	GrafPtr			oldPort;
	Rect bounds = theList.bounds;
	InsetRect(&bounds,-1,-1);
	FrameRect(&bounds);
	
	WindowPtr w = gResListDocument->theWindow;
	GetPort(&oldPort);
	SetPort(GetWindowPort(w));
	
	BeginUpdate(w);
	
	if (w != NULL && theList.hndl != NULL)
		LUpdateWVisRgn(w, theList.hndl);
	
	EndUpdate(w);
	SetPort(oldPort);
	return TRUE;
}

Boolean HandleResListActivate(Boolean activ)
{
	WindowPtr w = gResListDocument->theWindow;
	
	SetPort(GetWindowPort(w));
	if (w != NULL && theList.hndl != NULL) {
		LActivate(activ, theList.hndl);
		ControlRef ctl = GetListVerticalScrollBar(theList.hndl);
		if (activ)
			ActivateControl(ctl);
		else
			DeactivateControl(ctl);
	}
	return TRUE;
}

static void PenBlack()
	{
		PenPat(NGetQDGlobalsBlack());
	}

static void FillUpdateRgn(WindowPtr w)
	{
		GrafPtr oldPort; long soon;
		RgnHandle updateRgn=NewRgn();
		PenState pen; Point pt;
		Boolean eraseUpdates = TRUE;
		
		GetPort(&oldPort); SetPort(GetWindowPort(w));
		pt.h = pt.v = 0;
		LocalToGlobal(&pt);		
		GetWindowRegion(w, kWindowUpdateRgn, updateRgn);
		OffsetRgn(updateRgn,-pt.h,-pt.v);
		GetPenState(&pen); PenBlack();
		PaintRgn(updateRgn);
		/* Wait a half second or until Shift key is no longer down */
		soon = TickCount() + 30;
		while (TickCount()<soon || ShiftKeyDown()) ;
		if (eraseUpdates) EraseRgn(updateRgn);
		OffsetRgn(updateRgn,pt.h,pt.v);
		SetPenState(&pen);
	}
	
static void InvalWindowPort(WindowPtr w) 
{
	Rect box;
	GetWindowPortBounds(w,&box);
	InvalWindowRect(w,&box);
}

Boolean HandleResListMouseDown(Point where, int modifiers) 
{
	WindowPtr w = gResListDocument->theWindow;
#if 0
	InvalWindowPort(w);
	FillUpdateRgn(w);
#endif
	
	//GlobalToLocal(&where);
	if (LClick(where, modifiers, theList.hndl)) {
		theList.cell.h = theList.cell.v = 0;
		if (LGetSelect(TRUE, &theList.cell, theList.hndl)) {
			short i;

			/* A cell was double-clicked: its number is in <theList.cell.v>.
			 * Show the corresponding match.
			 */
			for (i = 0; i<theList.nCells; i++) {
				if (i==theList.cell.v) {
					(void)ShowMatch(i);
					return TRUE;
				}
			}
			/* Something is wrong. */
			SysBeep(10);
		}
		
		return TRUE;
	}
	
	return FALSE;
}

Boolean HandleResListDragWindow() 
{
	return TRUE;
}

/* ------------------------------------------------------------------------------------- */
/* DoResultListDoc - Document replacement for DoResultList */

Boolean DoResultListDoc(char label[]) 
{
	Document *resListDoc = NULL;
	Boolean ok = FALSE;
	UserList *l = gResUserList;
	
	ok = DoOpenResultListDocument(&resListDoc);

	//LDelRow(0,0,l->hndl);
	PrepareResultList(l);
	GetResultListData(l);
#if 0
	l->cell.v = 0;
	LSetSelect(TRUE,l->cell,l->hndl);
	EraseRect(&l->content);
	WindowPtr w = gResListDocument->theWindow;
	InvalWindowRect(w,&l->bounds);
	LSetDrawingMode(TRUE,l->hndl);
#endif
	return ok;
}


#endif /* !SEARCH_DBFORMAT_MEF */
