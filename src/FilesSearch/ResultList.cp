/* ResultList.c for Nightingale Search, by Donald Byrd. Incorporates modeless-dialog
handling code from Apple sample source code by Nitin Ganatra. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "SearchScorePrivate.h"

#include <string.h>

#ifdef SEARCH_DBFORMAT_MEF

void DoDialogEvent(EventRecord *theEvt)
{
	AlwaysErrMsg("DoDialogEvent called in the MEF version!");
}

#else

extern Boolean gDone;
extern Boolean gInBackground;

typedef struct {
	ListHandle hndl;
	Rect bounds, scroll, content, dataBounds;
	Point cSize, cell;
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

enum {
	BUT1_OK = 1,
	LIST2,
	LABEL_QMATCHESQ,
	LABEL_MATCHES,
	LASTITEM
};

Boolean EventFilter(EventRecord *theEvt, WindowPtr theFrontWindow);

static pascal void DrawButtonBorder(DialogPtr, short);
static pascal void DrawList(DialogPtr, short);

static Boolean HandleListEvent(EventRecord *theEvt);
static pascal Boolean ResultListFilter(DialogPtr, EventRecord *, short *);
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
		{ OutOfMemory(maxItems*MAX_LINELEN); return False; }

	docNumA = (INT16 *)NewPtr(maxItems*sizeof(INT16));
	if (!GoodNewPtr((Ptr)docNumA))
		{ OutOfMemory(maxItems*sizeof(INT16)); return False; }
	
	foundLA = (LINK *)NewPtr(maxItems*sizeof(LINK));
	if (!GoodNewPtr((Ptr)foundLA))
		{ OutOfMemory(maxItems*sizeof(LINK)); return False; }
	
	foundVoiceA = (INT16 *)NewPtr(maxItems*sizeof(INT16));
	if (!GoodNewPtr((Ptr)foundVoiceA))
		{ OutOfMemory(maxItems*sizeof(INT16)); return False; }
	
	matchArraySize = (MAX_HITS+1)*MAX_PATLEN;
	matchedObjFA = (DB_LINK *)NewPtr(matchArraySize*sizeof(DB_LINK));
	if (!GoodNewPtr((Ptr)matchedObjFA))
		{ OutOfMemory(matchArraySize*sizeof(DB_LINK)); return False; }

	matchedSubobjFA = (DB_LINK *)NewPtr(matchArraySize*sizeof(DB_LINK));
	if (!GoodNewPtr((Ptr)matchedSubobjFA))
		{ OutOfMemory(matchArraySize*sizeof(DB_LINK)); return False; }

	itemCount = 0;
	maxItemCount = maxItems;
	sPatLen = patLen;
	return True;
}


Boolean AddToResultList(
				char str[],
				MATCHINFO matchInfo,
				DB_LINK matchedObjA[MAX_PATLEN],		/* Array of matched objects, by voice */
				DB_LINK matchedSubobjA[MAX_PATLEN]		/* Array of match subobjs, by voice */
)
{
	short k;  long offset;
	
//LogPrintf(LOG_DEBUG, "AddToResultList: itemCount=%d maxItemCount=%d\n", itemCount, maxItemCount);
	if (itemCount>=maxItemCount) return False;

	if (!resultStr) return False;
	if (!docNumA) return False;
	if (!foundLA || !foundVoiceA) return False;
	if (!matchedObjFA || !matchedSubobjFA) return False;
		
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
	return True;
}


/* -------------------------------------------------------------------------------------- */
/* GetCtlHandle - Just a handy way to get a control's handle given only the item number
and the dialog. */

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


/* -------------------------------------------------------------------------------------- */

/* Convert internal voice number to staff number. For multi-staff parts, may not give
ideal results. ??Could be a useful utility in general; belongs elsewhere, maybe in
VoiceTable.c. */

INT16 VoiceNum2StaffNum(Document *doc, INT16 voice);
INT16 VoiceNum2StaffNum(Document *doc, INT16 voice)
{
	INT16 userVoice, nPartStaves;  LINK aPartL;
	
	if (Int2UserVoice(doc, voice, &userVoice, &aPartL)) {
	
		/* For default voices, assume their default staves; for other voices, just
		   assume the part's first staff. */
		   
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
another? We look for improper situations and simply return False if we find one. However,
as of this writing, the checking is pretty weak; see below. */

static Boolean SelectMatch(DB_Document *doc, LINK matchedObjA[MAX_PATLEN],
			LINK matchedSubobjA[MAX_PATLEN], INT16 count, Boolean matchTiedDur);
static Boolean SelectMatch(
			Document *doc,
			LINK matchedObjA[MAX_PATLEN],
			LINK matchedSubobjA[MAX_PATLEN],
			INT16 count,
			Boolean /* matchTiedDur	*/			/* Ignored for now */
			)
{
	INT16 n, staffn;
	LINK pL, aNoteL;
	
	/* Try to protect ourselves against "improper situations", but NB: this won't catch
	   every problem case by a long shot! We're not checking that the subobjs exist, or
	   that they match the objects they're supposed to belong to. */
	   
	for (n = 0; n<count; n++) {
		if (!InObjectList(doc, matchedObjA[n], MAIN_DSTR))
			return False;
	}
	
	DeselAll(doc);
	
	for (n = 0; n<count; n++) {
		pL = matchedObjA[n];
		if (!SyncTYPE(pL)) continue;
		
		aNoteL = matchedSubobjA[n];
		LinkSEL(pL) = True;
		NoteSEL(aNoteL) = True;
		//if (matchTiedDur) SelectTiedNotes(pL, aNoteL);
		staffn = NoteSTAFF(aNoteL);
	}

	doc->selStartL = matchedObjA[0];
	for (pL = LeftLINK(doc->tailL); pL!=LeftLINK(doc->selStartL); pL = LeftLINK(pL))
		if (LinkSEL(pL)) break;
	doc->selEndL = RightLINK(pL);
	doc->selStaff = staffn;

	InvalWindow(doc);
	return True;
}

/* Show the given match by bringing the score to the front, selecting its notes/rests,
and scrolling to get them in view. Return True if we succeed, False if not. ??FIXME:
PROBABLY NEED MORE INFO ON THE SCORE FOR THIS...FILE refNum, ETC. IS PROBABLY THE ONLY
ROBUST WAY, AND BEST TO ADD TO <MATCHINFO>! */

Boolean ShowMatch(INT16 matchNum);
Boolean ShowMatch(INT16 matchNum)
{
	extern Document *documentTable;			/* Ptr to head of score table */
	Document		*doc;
	long			offset;

	doc = documentTable+docNumA[matchNum];
	if (!doc->inUse) {
		CParamText("Can't show that match: the score is no longer open.", "",
						"", "");								// ??I18N BUG AND NOT USER-FRIENDLY
		StopInform(GENERIC_ALRT);
		return False;
	}
	
	offset = matchNum*MAX_PATLEN;
	//DebugShowObjAndSubobj("ShowMatch", (DB_LINK (*)[MAX_PATLEN])(matchedObjFA+offset),
	//						(DB_LINK (*)[MAX_PATLEN])(matchedSubobjFA+offset), sPatLen, 1);
	InstallDoc(doc);
#if 111
	if (!SelectMatch(doc, (matchedObjFA+offset), (matchedSubobjFA+offset), sPatLen, False)) {
		CParamText("Can't show that match: the score is no longer open, or the matching section has been edited.", "",
						"", "");								// ??I18N BUG AND NOT USER-FRIENDLY
		StopInform(GENERIC_ALRT);
		return False;
	}
#else
	DeselAll(doc);
	doc->selStartL = doc->selEndL = foundLA[matchNum];	/* Set insertion point */
	doc->selStaff = VoiceNum2StaffNum(doc, foundVoiceA[matchNum]);
	MEAdjustCaret(doc, True);
#endif

	SelectWindow((WindowPtr)doc);
	GoToSel(doc);
	InvalWindow(doc);
	EraseAndInvalMessageBox(doc);
	return True;
}


/* -------------------------------------------------------------------------------------- */
/* DoDialogEvent - After checking that EventFilter's result is True, this calls
DialogSelect to check if any controls were hit, and if so this acts accordingly. */

#if 0
void DoDialogEvent(EventRecord *theEvt)
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

	if (EventFilter(theEvt, frontWindow)) {
		TextFont(0);									/* Set to the system font */
		TextSize(12);
		DialogPtr theDialog = GetDialogFromWindow(frontWindow);
		if (DialogSelect(theEvt, &theDialog, &theItem)) {
			if (theDialog!=NULL) {
				switch (theItem) {
					case LIST2:
						where = theEvt->where;
						GlobalToLocal(&where);
						modifiers = theEvt->modifiers;
						if (LClick(where, modifiers, theList.hndl)) {
							theList.cell.h = theList.cell.v = 0;
							if (LGetSelect(True, &theList.cell, theList.hndl)) {
								short i;
				
								/* A cell was double-clicked: its number is in
								   <theList.cell.v>. Show the corresponding match. */
								   
								for (i = 0; i<theList.nCells; i++) {
									if (i==theList.cell.v) {
										(void)ShowMatch(i);
										goto Finish;
									}
								}
								AlwaysErrMsg("DoDialogEvent: Something is wrong.");
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

/* -------------------------------------------------------------------------------------- */

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
	LActivate(True, l->hndl);
}


static void PrepareResultList(UserList *l) 
{
	INT16 count = GetListLength();
	INT16 delta = count - l->nCells;
	
LogPrintf(LOG_DEBUG, "DoResultListDoc: count=%d delta=%d\n", count, delta); 
	if (delta > 0)	LAddRow(delta, l->nCells, l->hndl);
	else			LDelRow(count, delta, l->hndl);
	
	SetListLength(l);
	
	for (INT16 i = 0; i<l->nCells; i++) {
		l->cell.h = 0;  l->cell.v = i;
		LClrCell(l->cell, l->hndl);
	}
}

/* Build a new list in given user item box of given dialog, with cell height
determined by the given font size.  If success, delivers True; if couldn't allocate
ListMgr list (no more memory or whatever), delivers False. */

short BuildList(DialogPtr dlog, short item, short fontSize, UserList *l)
{
	short type, csize;
	Rect box;  Handle hndl;

	csize = fontSize+4;
	
	/* Content area (plus scroll bar) of list corresponds to user item box */
	
	GetDialogItem(dlog,item, &type, &hndl, &box);
	l->bounds = box; InsetRect(&l->bounds, -1, -1);
	SetDialogItem(dlog, item, userItem, NULL, &l->bounds);

	l->scroll = box;  l->scroll.left = l->scroll.right - 15;		/* Scrollbar width */
	l->content = box; 
	l->content.right = l->scroll.left;
	l->content.bottom = box.bottom - 15;

	SetRect(&l->dataBounds, 0, 0, 1, 0);
	l->cSize.v = csize > 0 ? csize : 1;
	l->cSize.h = l->content.right - l->content.left;

	/* Set font for list */
	TextFont(1);			// ??OR SYSFONTID_SANSSERIF
	TextSize(fontSize);
	TextFace(0);											/* Plain */

	l->hndl = LNew(&l->content,&l->dataBounds,l->cSize,0,GetDialogWindow(dlog),False,
					False,False,True);
	if (!l->hndl) return False;

	(*l->hndl)->selFlags = lOnlyOne;		/* Or whatever */
	
	GetResultListData(l);

	l->cell.v = 0;
	LSetSelect(True, l->cell, l->hndl);
	EraseRect(&l->content);
	InvalWindowRect(GetDialogWindow(dlog), &l->bounds);
	//InvalRect(&l->bounds);
	LSetDrawingMode(True, l->hndl);

	return True;
}



#define RESULTLIST_DLOG 966

/* -------------------------------------------------------------------------------------- */
/* DoResultList - Creates the modeless dialog (from a DLOG resource) and sets
up the initial state of the controls and user items.  Note that it also allocates
two globals that will always point to the user item proc routines.  This is needed
only once and can be called by all sebsequent calls to DoResultList, because
the routines won't change.  In the 68K world, these routines are redefined to just 
be plain old proc pointers; on PowerPC builds they use real UPPs.
*/

/* Display the Result List modeless dialog.  Return True if all OK, False on error. */

Boolean DoResultList(char label[])
{
	short 				tempType;
	Handle				tempHandle;
	Rect				tempRect;
	DialogPtr			mlDlog;
	static UserItemUPP	procForBorderUserItem = NULL;
	static UserItemUPP	procForListUserItem = NULL;

	mlDlog = GetNewDialog(RESULTLIST_DLOG, NULL, BRING_TO_FRONT);
	if (mlDlog==NULL) { MissingDialog(RESULTLIST_DLOG);  return False; }
	
	//gResultListDlog = mlDlog;
	
	if (procForBorderUserItem==NULL)
		procForBorderUserItem = NewUserItemUPP(DrawButtonBorder);	

	if (procForListUserItem==NULL)
		procForListUserItem = NewUserItemUPP(DrawList);	
		
	PutDlgString(mlDlog, LABEL_MATCHES, CToPString(label), False);

	if (!BuildList(mlDlog, LIST2, 10, &theList)) {
		CParamText("DoResultList: couldn't build list; probably out of memory.", "",
						"", "");								// ??I18N BUG AND NOT USER-FRIENDLY
		StopInform(GENERIC_ALRT);
		return False;
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

	return True;
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
	Handle	theControl;
	short	aType;
	Rect	aRect;

	GetDialogItem((DialogPtr)theDialog, BUT1_OK, &aType, &theControl, &aRect);
	
	PenSize(3, 3);
	InsetRect(&aRect, -4, -4);
	FrameRoundRect(&aRect, 16, 16);
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
		palettesVisible[TOOL_PALETTE] = False;
		DoCloseWindow(pg->paletteWindow);				
	}			
}


/* EventFilter - This is the first thing done if a dialog event is received, giving 
you a chance to perform any special stuff before passing control on to 
DialogSelect.

If this routine returns True, the event processing will continue, and DialogSelect
will be called to perform hit detection on the controls.  If False is returned,
it means the event is already handled and the main event loop will continue. */
	
Boolean EventFilter(EventRecord *theEvt, WindowPtr theFrontWindow)
{
	char 	theKey;
	short 	theHiliteCode;
	
	switch (theEvt->what) {	
		case keyDown:
		case autoKey:	
				theKey = theEvt->message & charCodeMask;

				if ((theEvt->modifiers & cmdKey)!=0) {	
#if 0
					long menuResult;
				
					AdjustMenus();
					menuResult = MenuKey(theKey);
			
					if ((menuResult >> 16) != 0) {
						Boolean editOpPerformed = MenuCommand(menuResult);
						
						if (editOpPerformed)
							/* Why are we exiting when an edit operation is performed?
							Because DialogSelect performs some automatic handling for
							any editText items that may be in the Dialog, and since
							we've already handled them in MenuCommand() we don't want
							DialogSelect to do anything. */
							
							return False;
					}
#endif
				}
				
				break;
				
		case mouseDown:
			WindowPtr w = (WindowPtr)theEvt->message;
			WindowPartCode part = FindWindow(theEvt->where, &w);
			//w==GetDialogWindow(dlog);
			Rect bounds = GetQDScreenBitsBounds();
			switch (part) {
				case inContent:
					if (HandleListEvent(theEvt)) {
						return True;
					}
					break;
				case inDrag:
					DragWindow(w, theEvt->where, &bounds);
					return True;
					break;
					
				default:
					break;					
			}
			return  False;
			break;

		case activateEvt:				
				/* Take care of hiliting the controls according to whether or not the
				dialog is frontmost. */
				
				theFrontWindow = (WindowPtr)theEvt->message;
				if (theEvt->modifiers & activeFlag)
					theHiliteCode = CTL_ACTIVE;
				else
					theHiliteCode = CTL_INACTIVE;

				HiliteAllControls(GetDialogFromWindow(theFrontWindow), theHiliteCode);
				
				//if (theHiliteCode == CTL_ACTIVE)
				//	CloseToolPalette();
				
				return False;

	}

	/* If we haven't returned False by now, just return True so DialogSelect can do its
	   thing: update the window, deal with an item hit, etc. */

	return True;
}

static Boolean HandleListEvent(EventRecord *theEvt) 
{
	Point where = theEvt->where;
	GlobalToLocal(&where);
	int modifiers = theEvt->modifiers;
	if (LClick(where, modifiers, theList.hndl)) {
		theList.cell.h = theList.cell.v = 0;
		if (LGetSelect(True, &theList.cell, theList.hndl)) {
			short i;

			/* A cell was double-clicked: its number is in <theList.cell.v>. Show the
			   corresponding match. */
			   
			for (i = 0; i<theList.nCells; i++) {
				if (i==theList.cell.v) {
					(void)ShowMatch(i);
					return True;
				}
			}
			AlwaysErrMsg("HandleListEvent: Something is wrong.");
		}
		
		return True;
	}
	
	return False;
}

static pascal Boolean ResultListFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
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
				return True;
			}
			break;
		case activateEvt:
				/* Take care of hiliting the controls according to whether or not the
				   dialog is frontmost. */
				   
				theFrontWindow = (WindowPtr)evt->message;
				if (evt->modifiers & activeFlag)	theHiliteCode = CTL_ACTIVE;
				else								theHiliteCode = CTL_INACTIVE;

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
						return True;
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

	return False;
}

static void ProcessModalEvents() 
{
	ModalFilterUPP	filterUPP;
	Boolean done = False;
	short ditem;
	
	WindowPtr frontWindow = FrontWindow();
	GrafPtr	origPort;
	
	GetPort(&origPort);
	SetPort(GetWindowPort(frontWindow));	
	
	filterUPP = NewModalFilterUPP(ResultListFilter);
	if (filterUPP == NULL) { return; }
	
	while (!done) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
				done = True;
				break;
		}
	}
	
	if (filterUPP != NULL)
		DisposeModalFilterUPP(filterUPP);
	SetPort(origPort);
}

/* -------------------------------------------------------------------------------------- */

Boolean BuildDocList(Document *doc, short fontSize)
{
	short csize;  Rect box;
	WindowPtr w = doc->theWindow;
	UserList *l = &theList;
		
LogPrintf(LOG_DEBUG, "BuildDocList: l=%x doc->lookVoice=%d\n", l, doc->lookVoice);
	csize = fontSize+4;
	
	/* Content area (plus scroll bar) of list corresponds to user item box */
	
	GetWindowPortBounds(w, &box);
	//InsetRect(&box, 4, 4);
	l->bounds = box; InsetRect(&l->bounds, -1, -1);

	l->scroll = box;  l->scroll.left = l->scroll.right - 15;		/* Scrollbar width */
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

	l->hndl = LNew(&l->content,&l->dataBounds,l->cSize,0,w,False,False,False,True);
	if (!l->hndl) return False;

	(*l->hndl)->selFlags = lOnlyOne;		/* Or whatever */
	
	l->cell.v = 0;
	LSetSelect(True, l->cell, l->hndl);
	EraseRect(&l->content);
	InvalWindowRect(w, &l->bounds);
	LSetDrawingMode(True, l->hndl);
	
LogPrintf(LOG_DEBUG, "BuildDocList: l=%x\n", l);
	gResUserList = l;

	return True;
}

Boolean HandleResultListUpdate()
{
	GrafPtr	oldPort;
	Rect bounds = theList.bounds;
	
	InsetRect(&bounds, -1, -1);
	FrameRect(&bounds);
	
	WindowPtr w = gResultListDoc->theWindow;
	GetPort(&oldPort);
	SetPort(GetWindowPort(w));
	
	BeginUpdate(w);
	
	if (w != NULL && theList.hndl != NULL) LUpdateWVisRgn(w, theList.hndl);
	
	EndUpdate(w);
	SetPort(oldPort);
	return True;
}

Boolean HandleResultListActivate(Boolean activ)
{
	WindowPtr w = gResultListDoc->theWindow;
	
	SetPort(GetWindowPort(w));
	if (w != NULL && theList.hndl != NULL) {
		LActivate(activ, theList.hndl);
		ControlRef ctl = GetListVerticalScrollBar(theList.hndl);
		if (activ)	ActivateControl(ctl);
		else		DeactivateControl(ctl);
	}
	return True;
}

static void PenBlack()
	{
		PenPat(NGetQDGlobalsBlack());
	}

static void FillUpdateRgn(WindowPtr w)
	{
		GrafPtr oldPort;  long soon;
		RgnHandle updateRgn=NewRgn();
		PenState pen;  Point pt;
		Boolean eraseUpdates = True;
		
		GetPort(&oldPort);  SetPort(GetWindowPort(w));
		pt.h = pt.v = 0;
		LocalToGlobal(&pt);		
		GetWindowRegion(w, kWindowUpdateRgn, updateRgn);
		OffsetRgn(updateRgn, -pt.h, -pt.v);
		GetPenState(&pen);  PenBlack();
		PaintRgn(updateRgn);
		
		/* Wait a half second or until Shift key is no longer down */
		
		soon = TickCount() + 30;
		while (TickCount()<soon || ShiftKeyDown()) ;
		if (eraseUpdates) EraseRgn(updateRgn);
		OffsetRgn(updateRgn, pt.h, pt.v);
		SetPenState(&pen);
	}
	
static void InvalWindowPort(WindowPtr w) 
{
	Rect box;
	GetWindowPortBounds(w, &box);
	InvalWindowRect(w, &box);
}

Boolean HandleResultListMouseDown(Point where, int modifiers) 
{
#if 0
	WindowPtr w = gResultListDoc->theWindow;
	InvalWindowPort(w);
	FillUpdateRgn(w);
#endif
	
	//GlobalToLocal(&where);
	if (LClick(where, modifiers, theList.hndl)) {
		theList.cell.h = theList.cell.v = 0;
		if (LGetSelect(True, &theList.cell, theList.hndl)) {
			short i;

			/* A cell was double-clicked: its number is in <theList.cell.v>. Show the
			   corresponding match. */
			   
			for (i = 0; i<theList.nCells; i++) {
				if (i==theList.cell.v) {
					(void)ShowMatch(i);
					return True;
				}
			}
			
			AlwaysErrMsg("HandleResultListMouseDown: Something is wrong.");
		}
		
		return True;
	}
	
	return False;
}

Boolean HandleResultListDragWindow() 
{
	return True;
}

/* -------------------------------------------------------------------------------------- */
/* DoResultListDoc - Document replacement for DoResultList */

Boolean DoResultListDoc(char /* label */ []) 
{
	Document *resListDoc = NULL;
	Boolean ok;
	UserList *l;
	
	ok = DoOpenResListDocument(&resListDoc);
	if (!gResUserList) {
		AlwaysErrMsg("DoResultListDoc: there's no gResUserList.");
		return False;
	}
	
	l = gResUserList;	
	//LDelRow(0,0,l->hndl);
	PrepareResultList(l);
	GetResultListData(l);
#if 0
	l->cell.v = 0;
	LSetSelect(True,l->cell,l->hndl);
	EraseRect(&l->content);
	WindowPtr w = gResultListDoc->theWindow;
	InvalWindowRect(w,&l->bounds);
	LSetDrawingMode(True,l->hndl);
#endif
	return ok;
}


#endif /* !SEARCH_DBFORMAT_MEF */
