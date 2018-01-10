/* Document.c for Nightingale - routines that deal with generic Document functions */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Prototypes for private routines */

static Boolean AllStavesSameSize(Document *);

/* --------------------------------------------------------- InstallDoc and associates -- */

void InstallDoc(Document *doc)
{
	InstallDocHeaps(doc);
	InstallStrPool(doc);
	InstallMagnify(doc);
	
	currentDoc = doc;
}
	
/* For a given document, set globals for use by the macros that convert to/from
screen pixels (d2p, etc.). NB: as of this writing (v.3.1), this is called by
NPtStringWidth as well as InstallDoc, so it ends up getting called a lot, and
it's desirable that it be reasonably fast.

NB: <magRound> is to minimize roundoff error. It's used in some code that behaves
straightforwardly for even magnifications, but which for odd magnifications,
does something tricky for the sake of speed. As a result, it's hard to see what
the optimal setting is for odd magnifications, though it seems it should be
somewhere from 2/3 to 3/4 the setting for the equivalent even magnification. For now:

	Magnification	25%		37.5%	50%		75%		100%	150%	200%	300%	400%	600%
	magnify			-4		-3		-2		-1		0		1		2		3		4		5
	magShift		6		6		5		5		4		4		3		3		2		2
	magRound		32		32?		16		16?		8		8?		4		4?		2		2?
*/

void InstallMagnify(Document *doc)
{
	magMagnify = doc->magnify;
	if (doc->magnify<0) {
		magShift = 4-((doc->magnify-1)/2);			/* Rounds <magShift> up */
		magRound = 8<<(-(doc->magnify-1)/2);
	}
	else {
		magShift = 4-(doc->magnify/2);				/* Rounds <magShift> up */
		magRound = 8>>(doc->magnify/2);
	}
	
#ifdef TEST_MAGNIFY
{
	DDIST d;  short pxl, dummy;
	pxl = d2p(160); d = p2d(pxl);
	d = p2d(1); pxl = d2p(d);
	d = p2d(2); pxl = d2p(d);
	d = p2d(3); pxl = d2p(d);
	d = p2d(4); pxl = d2p(d);
	dummy = 0;		/* Need a breakpoint to look at vars. in debugger */
}
#endif

}

/*
 *	For a given document, install its heaps into the Nightingale globals, making them
 *	the current heaps with respect to the myriad macros that reference these globals.
 * NB: This should rarely if ever be called directly: instead call InstallDoc.
 */

void InstallDocHeaps(Document *doc)
{
	register HEAP *hp;
	
	Heap = hp = doc->Heap;
	
	/* DO NOT CHANGE THE ORDER OF THESE ASSIGNMENTS! */
	
	PARTINFOheap = hp++;
	TAILheap = hp++;
	NOTEheap = hp++;
	RPTENDheap = hp++;
	PAGEheap = hp++;
	SYSTEMheap = hp++;
	STAFFheap = hp++;
	MEASUREheap = hp++;
	CLEFheap = hp++;
	KEYSIGheap = hp++;
	TIMESIGheap = hp++;
	NOTEBEAMheap = hp++;
	CONNECTheap = hp++;
	DYNAMheap = hp++;
	MODNRheap = hp++;
	GRAPHICheap = hp++;
	NOTEOTTAVAheap = hp++;
	SLURheap = hp++;
	NOTETUPLEheap = hp++;
	GRNOTEheap = hp++;
	TEMPOheap = hp++;
	SPACEheap = hp++;
	ENDINGheap = hp++;
	PSMEASheap = hp++;
	OBJheap = hp++;
}

void InstallStrPool(Document *doc)
{
	SetStringPool(doc->stringPool);
}


/*
 *	Deliver pointer to first free (current unused) slot in document table, or NULL
 *	if none.  All data in the record is set to 0 first.
 */

Document *FirstFreeDocument()
{
	register Document *doc;
	
	for (doc=documentTable; doc<topTable; doc++)
		if (!doc->inUse) {
			ZeroMem(doc, sizeof(Document));
			return(doc);
		}
	
	return(NULL);
}

/*
 * Deliver pointer to document table whose window is w. Analagous to Carbon functions
 * that cast up, e.g. GetDialogFromWindow(WindowPtr).
 */

Document *GetDocumentFromWindow(WindowPtr w)
{
	register Document *doc;
	
	if (w==(WindowPtr)NULL) return(NULL);
		
	for (doc=documentTable; doc<topTable; doc++)
		if (doc->theWindow == w) {
			return(doc);
		}
	
	return(NULL);
}


Boolean EqualFSSpec(FSSpec *fs1, FSSpec *fs2)
{
	if (fs1->parID != fs2->parID) return False;
	if (fs1->vRefNum != fs2->vRefNum) return False;
	
	return (Pstreql(fs1->name, fs2->name));
}


/*
 *	Search for this file in this directory among all already open Documents.
 *	If one is found, return it; otherwise NULL.
 */

Document *AlreadyInUse(unsigned char *name, short /*vrefnum*/, FSSpec *pfsSpec)
{
	register Document *doc;
	
	for (doc=documentTable; doc<topTable; doc++)
		if (doc->inUse && doc!=clipboard)
			if (EqualFSSpec(&doc->fsSpec, pfsSpec))				// if(doc->vrefnum == vrefnum)
				if (EqualString(doc->name, name, False, False)) return(doc);
	return(NULL);
}

#ifdef NOMORE	These functions might be useful in the future, so keep the code.
static short NumFreeDocuments(void);
static short NumOpenDocuments(void);

static short NumFreeDocuments()
{
	Document *doc;  short n = 0;
	
	for (doc=documentTable; doc<topTable; doc++)
		if (!doc->inUse) n++;
	return(n);
}
	
static short NumOpenDocuments()
{
	Document *doc;  short n = 0;
	
	for (doc=documentTable; doc<topTable; doc++)
		if (doc->inUse) n++;
	if (clipboard->inUse) n--;
	
	return(n);
}
#endif

/*
 *	Either show or bring to front the clipboard document. If this is the first time
 *	it's being shown, then position it on the right side of the main screen.
 */

void ShowClipDocument()
{
	static Boolean once = True;
	Rect scrn, bounds, box;
	
	if (clipboard)
		if (IsWindowVisible(clipboard->theWindow))
			SelectWindow(clipboard->theWindow);
		 else {
			if (once) {
				WindowPtr w = clipboard->theWindow;
				scrn = GetQDScreenBitsBounds();
				AdjustWinPosition(w);				/* Avoid conflicts with other windows */
				GetGlobalPort(w, &box);				/* set bottom of window near screen bottom */
				bounds = GetQDScreenBitsBounds();
				box.bottom = bounds.bottom - 24;
				if (box.right > bounds.right-24) box.right = bounds.right - 24;
				SizeWindow(w, box.right-box.left, box.bottom-box.top, False);
				once = False;
			}
			if (BottomPalette != BRING_TO_FRONT)
				SendBehind(clipboard->theWindow, BottomPalette);
			 else
				BringToFront(clipboard->theWindow);
			ShowDocument(clipboard);
		}
}


/*
 * Find a size and position for the given window that, as much as possible, avoids
 * conflict with either the tool palette or other Document windows.
 */

void PositionWindow(WindowPtr w, Document *doc)
{
	Rect box, bounds;
	short palWidth, palHeight;
	
	if (EmptyRect(&revertWinPosition)) {
		/* Get width of tool palette */
		GetWindowPortBounds((*paletteGlobals[TOOL_PALETTE])->paletteWindow,&box);
//			box = (*paletteGlobals[TOOL_PALETTE])->paletteWindow->portRect;
		palWidth = box.right - box.left;
		palHeight = box.bottom - box.top;
		/* Place new document window in non-conflicting position */
		GetGlobalPort(w,&box);								/* Set bottom of window near screen bottom */
		bounds = GetQDScreenBitsBounds();
		if (box.left < bounds.left+4)
			box.left = bounds.left+4;
		if (palWidth < palHeight)
			box.left += palWidth;
		MoveWindow(doc->theWindow, box.left, box.top, False);
		AdjustWinPosition(w);
		GetGlobalPort(w, &box);
		bounds = GetQDScreenBitsBounds();
		box.bottom = bounds.bottom - 4;
		if (box.right > bounds.right-4)
			box.right = bounds.right - 4;
		SizeWindow(doc->theWindow, box.right-box.left, box.bottom-box.top, False);
	}
	 else {
		MoveWindow(doc->theWindow, revertWinPosition.left, revertWinPosition.top, False);
		SizeWindow(doc->theWindow, revertWinPosition.right-revertWinPosition.left,
					   revertWinPosition.bottom-revertWinPosition.top, False);
		SetRect(&revertWinPosition, 0, 0, 0, 0);			/* Make it empty again */
	}
}

/*
 * If fileName is non-NULL, open document file of that name in the given directory; if
 * fileName is NULL, then open an Untitled new document instead.  In either case, open a
 * window for the document, and return True if successful, False if not. If the named
 * document is already open, just bring its window to the front and return True.
 */

Boolean DoOpenDocument(unsigned char *fileName, short vRefNum, Boolean readOnly, FSSpec *pfsSpec)
	{
		register WindowPtr w;  register Document *doc, *d;
		short numNew;  long fileVersion;
		static char ID = '0';
		
		/* If an existing file and already open, then just bring it to front */
		
		if (fileName) {
			doc = AlreadyInUse(fileName, vRefNum, pfsSpec);
			if (doc) {
				DoSelectWindow(doc->theWindow);
				return(True);
				}
			}
		
		/* Otherwise, open file */
		
		doc = FirstFreeDocument();
		if (doc == NULL) { TooManyDocs(); return(False); }

		w = GetNewWindow(docWindowID, NULL, BottomPalette);
		if (w) {
			doc->theWindow = w;
			SetWindowKind(w, DOCUMENTKIND);
			// ((WindowPeek)w)->spareFlag = True;
			ChangeWindowAttributes(w, kWindowFullZoomAttribute, kWindowNoAttributes);
			doc->inUse = True;
			doc->readOnly = readOnly;
			if (fileName) {
				doc->docNew = False;
				Pstrcpy(doc->name, fileName);
				doc->vrefnum = vRefNum;
				doc->fsSpec = *pfsSpec;
				}
			 else {
				doc->docNew = True;
				/* Count number of new (untitled) documents on desktop */
				for (numNew=0, d=documentTable; d<topTable; d++)
					if (d->inUse && d->docNew && d!=clipboard) numNew++;
				GetIndString(tmpStr, MiscStringsID, 1);	/* Get "Untitled" */
				Pstrcpy(doc->name, tmpStr);
				*(doc->name + 1 + *(doc->name)) = '-';
				*(doc->name + 2 + *(doc->name)) = ID + numNew;
				(*doc->name) += 2;
				doc->vrefnum = 0;
				}
				
			if (!BuildDocument(doc, fileName, vRefNum, pfsSpec, &fileVersion, fileName==NULL)) {
				DoCloseDocument(doc);
				w = NULL;
				}
			 else {
				PositionWindow(w, doc);
				SetOrigin(doc->origin.h, doc->origin.v);
				RecomputeView(doc);
				SetControlValue(doc->hScroll, doc->origin.h);
				SetControlValue(doc->vScroll, doc->origin.v);
				if (fileVersion != THIS_VERSION) {
					/* Append " (converted)" to its name and mark the document changed */
					unsigned char str[64];
					GetIndString(str, MiscStringsID, 5);
					PStrCat(doc->name, str);
					doc->changed = doc->docNew = !readOnly;
					doc->converted = True;
					}
				else
					doc->converted = False;
				SetWTitle(w, doc->name);
				ShowDocument(doc);
				
				/* ERROR: new documents are not updated in Panther 10.3. This is a workaround */
				if (doc->docNew) {
					Rect portRect;
					GetWindowPortBounds(doc->theWindow, &portRect);
					DoUpdate(doc->theWindow);
					}
					
				ArrowCursor();
				}
			}
			
		return(w != NULL);
	}

/*
 * If fileName is non-NULL, open document file of that name in the given directory; if
 * fileName is NULL, then open an Untitled new document instead.  In either case, open a
 * window for the document, and return True if successful, False if not. If the named
 * document is already open, just bring its window to the front and return True.
 * FIXME: This function is simply DoOpenDocument() except that it has an additional
 * parameter in which it returns the Document! Either DoOpenDocument() should simply
 * call this function, or calls to DoOpenDocument() should directly call this function.
 */

Boolean DoOpenDocumentX(unsigned char *fileName, short vRefNum, Boolean readOnly, FSSpec *pfsSpec,
Document **pDoc)
	{
		register WindowPtr w;  register Document *doc, *d;
		short numNew;  long fileVersion;
		static char ID = '0';
		
		*pDoc = NULL;
		
		/* If an existing file and already open, then just bring it to front */
		
		if (fileName) {
			doc = AlreadyInUse(fileName, vRefNum, pfsSpec);
			if (doc) {
				DoSelectWindow(doc->theWindow);
				*pDoc = doc;
				return(True);
				}
			}
		
		/* Otherwise, open file */
		
		doc = FirstFreeDocument();
		if (doc == NULL) { TooManyDocs(); *pDoc = NULL; return(False); }

		w = GetNewWindow(docWindowID, NULL, BottomPalette);
		if (w) {
			doc->theWindow = w;
			SetWindowKind(w, DOCUMENTKIND);
			// ((WindowPeek)w)->spareFlag = True;
			ChangeWindowAttributes(w, kWindowFullZoomAttribute, kWindowNoAttributes);
			doc->inUse = True;
			doc->readOnly = readOnly;
			if (fileName) {
				doc->docNew = False;
				Pstrcpy(doc->name, fileName);
				doc->vrefnum = vRefNum;
				doc->fsSpec = *pfsSpec;
				}
			 else {
				doc->docNew = True;
				/* Count number of new (untitled) documents on desktop */
				for (numNew=0, d=documentTable; d<topTable; d++)
					if (d->inUse && d->docNew && d!=clipboard) numNew++;
				GetIndString(tmpStr, MiscStringsID, 1);	/* Get "Untitled" */
				Pstrcpy(doc->name, tmpStr);
				*(doc->name + 1 + *(doc->name)) = '-';
				*(doc->name + 2 + *(doc->name)) = ID + numNew;
				(*doc->name) += 2;
				doc->vrefnum = 0;
				}
				
			if (!BuildDocument(doc, fileName, vRefNum, pfsSpec, &fileVersion, fileName==NULL)) {
				DoCloseDocument(doc);
				w = NULL;
				}
			 else {
				PositionWindow(w, doc);
				SetOrigin(doc->origin.h, doc->origin.v);
				RecomputeView(doc);
				SetControlValue(doc->hScroll, doc->origin.h);
				SetControlValue(doc->vScroll, doc->origin.v);
				if (fileVersion != THIS_VERSION) {
					/* Append " (converted)" to its name and mark the document changed */
					unsigned char str[64];
					GetIndString(str, MiscStringsID, 5);
					PStrCat(doc->name, str);
					doc->changed = doc->docNew = !readOnly;
					doc->converted = True;
					}
				else
					doc->converted = False;
				SetWTitle(w, doc->name);
				ShowDocument(doc);
				
				/* ERROR: new documents are not updated in Panther 10.3. This is a workaround */
				if (doc->docNew) {
					Rect portRect;
					GetWindowPortBounds(doc->theWindow, &portRect);
					DoUpdate(doc->theWindow);
					}
				}
				
				if (w != NULL) *pDoc = doc;
			}
			
		return(w != NULL);
	}

/*
 *	This routine should be called after preparing another document to be shown.
 */

void ShowDocument(Document *doc)
{
	RecomputeView(doc);
	if (TopPalette) {
		if (TopPalette != TopWindow)
			/* Bring the TopPalette to the front and generate activate event. */
			SelectWindow(TopPalette);
		else {
			/* There won't be an activate event for doc, so do it here */
			ShowWindow(doc->theWindow);
			HiliteWindow(doc->theWindow, True);
			ActivateDocument(doc, True);
		}
		if (TopDocument)
			/* Ensure the TopDocument and its controls are unhilited properly. */
			ActivateDocument(GetDocumentFromWindow(TopDocument), False);
	}
	ShowWindow(doc->theWindow);		
}

/*
 *	Close a given document, saving it if necessary, and giving it a name if necessary.
 *	Return True if all okay, False if user cancels or error.
 */

Boolean DoCloseDocument(register Document *doc)
{
	Boolean keepGoing = True;
	
	if (doc)
		if (IsDocumentKind(doc->theWindow))
			if (doc == clipboard)
				HideWindow(doc->theWindow);
			 else if (doc == searchPatDoc)
				HideWindow(doc->theWindow);
			 else
				if ( (keepGoing = DocumentSaved(doc)) ) {
					HideWindow(doc->theWindow);
					if (doc->vScroll) DisposeControl(doc->vScroll);
					doc->vScroll = NULL;
					if (doc->hScroll) DisposeControl(doc->hScroll);
					doc->hScroll = NULL;
				
					if (doc->undo.undoRecord) DisposeHandle(doc->undo.undoRecord);
					doc->undo.undoRecord = NULL;
					
					DestroyAllHeaps(doc);

					doc->undo.hasUndo = False;
					
					if (doc->stringPool) DisposeStringPool(doc->stringPool);
					doc->stringPool = NULL;
				
					DisposeWindow(doc->theWindow);
					doc->inUse = False;
				}
	
	return(keepGoing);
}

void ActivateDocument(register Document *doc, short activ)
{
	Point pt;  GrafPtr oldPort;
	Rect portRect;
	
	if (doc) {
		WindowPtr w = doc->theWindow; 
		GetPort(&oldPort);
		
		HiliteWindow(w, activ);
		
		GetWindowPortBounds(w, &portRect);
		SetPort(GetWindowPort(w));
		ClipRect(&portRect);
		
		doc->active = (activ!=0);
		if (doc->active) {
			HiliteControl(doc->hScroll, CTL_ACTIVE);
			HiliteControl(doc->vScroll, CTL_ACTIVE);
		}
		 else {
			HiliteControl(doc->hScroll, CTL_INACTIVE);
			HiliteControl(doc->vScroll, CTL_INACTIVE);
		}
			
		pt = TOP_LEFT(doc->growRect);
		DrawGrowIcon(w);
		ClipRect(&doc->viewRect);
		
		if (!doc->masterView && !doc->showFormat)
			MEActivateCaret(doc, activ);
		
		if (activ) {
			InstallDoc(doc);
			InstallDocMenus(doc);
		}
		 else
			SetPort(oldPort);
	}
}

/* Ensure that a given document is ready to be closed.  Ask user if any changes
should be saved, and save them if he does.  If user says to Cancel, then return
False, otherwise return True.  We use a simple alert, with a filter that lets us
update the Document underneath it so the user can see what it looks like before
making the decision. */

Boolean DocumentSaved(register Document *doc)
{
	short itemHit; Boolean keepGoing = True;
	
	InstallDoc(doc);
	if (doc->masterView)
		if (!ExitMasterView(doc))			/* Dispose all memory allocated by masterPage */
			return False;

	if (doc->changed) {
		DoUpdate(doc->theWindow);
		ArrowCursor();
		ParamText(doc->name, "\p", "\p", "\p");
		PlaceAlert(saveChangesID, doc->theWindow, 0, 30);
		itemHit = CautionAlert(saveChangesID, NULL);
		if (itemHit == Cancel) keepGoing = False;
		if (itemHit == OK) {
			WaitCursor();
			UpdateAllWindows();
			keepGoing = DoSaveDocument(doc);
		}
	}
	return(keepGoing);
}

/*
 *	Given a document, either new or old, save it.  If it's new, then call
 *	DoSaveAs on it.  Return True if all went well, False if not.
 *
 * ??This is not correct. As previously written, returned True unless DoSaveAs
 * tried to save a file and got an error or fdType != 'SCOR'. All errors except
 *	those resulting from action of GetOutputName were ignored.
 *
 * Currently, will return False if returned False before, and will return
 * False if SaveFile returns CANCELop, and True otherwise. If keepGoing is
 * True, this means that the save (or close, or quit) operation can continue,
 * otherwise the op is aborted, and closing or quitting is discontinued.
 *
 * If we set keepGoing False upon a file system error, we could get into a 
 * situation where it is impossible to quit Nightingale because every time we
 * try to close a file, we get a file system error, and keepGoing is set False,
 * the close op is discontinued, and the doc is kept onScreen, and quitting
 * is aborted. Thus, only add explicit user cancellation to the list of 
 * conditions which can abort a close, for both DoSaveDocument and DoSaveAs.
 */

Boolean DoSaveDocument(register Document *doc)
{
	Boolean keepGoing = False;  short err;
	
	if (doc->docNew || doc->readOnly) return(DoSaveAs(doc));
	
	err = SaveFile(doc, False);

	if (err!=CANCEL_INT) {
		HSetVol(NULL,doc->vrefnum,0);
	
		keepGoing = True;
	}

	return(keepGoing);
}

/*
 *	Save a given document under a new name, and give new name to document, which remains
 *	open.
 */

Boolean DoSaveAs(register Document *doc)
{
	Boolean keepGoing;
	Str255 name;  short vrefnum, err;
	OSErr result;
	FInfo theInfo;
	FSSpec fsSpec;
	NSClientData nscd;
	
	Pstrcpy(name,doc->name);
	keepGoing = GetOutputName(MiscStringsID,3,name,&vrefnum,&nscd);
	if (keepGoing) {
		//result = FSMakeFSSpec(vrefnum, 0, name, &fsSpec);
		fsSpec = nscd.nsFSSpec;
		Pstrcpy(name,fsSpec.name);
		
		//result = GetFInfo(name, vrefnum, &theInfo);
		//if (result == noErr)
			result = FSpGetFInfo (&fsSpec, &theInfo);
		
		if (result == noErr && theInfo.fdType != documentType) {
			GetIndCString(strBuf, FILEIO_STRS, 11);	/* "You can replace only files created by Nightingale" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return False;
		}

		result = HSetVol(NULL,fsSpec.vRefNum,fsSpec.parID);
		
		if (result == noErr) {
			/* Save the file under this name */
			Pstrcpy(doc->name,name);
			doc->vrefnum = fsSpec.vRefNum;
			doc->fsSpec = fsSpec;
			SetWTitle(doc->theWindow,name);
			err = SaveFile(doc, True);
		}
		else {
			err = CANCEL_INT;
		}

		keepGoing = (err!=CANCEL_INT);
		if (keepGoing) {
			doc->changed = False;
			doc->named = True;
			doc->readOnly = False;
		}
	}
	
	return(keepGoing);
}

/*
 *	Discard all changes to given document, but only with user's permission.
 */

void DoRevertDocument(register Document *doc)
{
	short itemHit, vrefnum, docReadOnly;
	Str255 name;  unsigned char *p;
	
	if (doc->changed) {
		ParamText(doc->name,"\p","\p","\p");
		PlaceAlert(discardChangesID,doc->theWindow,0,30);
		itemHit = CautionAlert(discardChangesID,NULL);
		if (itemHit == OK) {
			doc->changed = False;		/* So DoCloseWindow doesn't call alert */
			Pstrcpy(name,doc->name);
			vrefnum = doc->vrefnum;
			FSSpec fsSpec = doc->fsSpec;
			docReadOnly = doc->readOnly;
			GetGlobalPort(doc->theWindow,&revertWinPosition);
			p = doc->docNew ? NULL : Pstrcpy(name,doc->name);
			DoCloseDocument(doc);
			DoOpenDocument(p,vrefnum,docReadOnly,&fsSpec);
		}
	}
}

/* If all staves (of the first Staff object only!) are the same size and that size is
not the score's <srastral>, offer user a change to set <srastral> accordingly. In all
cases, return True if all staves are the same size, else False. */

static Boolean AllStavesSameSize(Document *doc)
{
	LINK staffL, aStaffL;
	Boolean firstTime=True, allSameSize=True;
	static DDIST lnSpace;  DDIST thisLnSpace;
	short i;
	
	staffL = SSearch(doc->headL, STAFFtype, GO_RIGHT);				/* Should never fail */
	for (aStaffL = FirstSubLINK(staffL); aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		if (firstTime) {
			lnSpace = StaffHEIGHT(aStaffL)/(StaffSTAFFLINES(aStaffL)-1);
			thisLnSpace = lnSpace;
			firstTime = False;
		}
		else
			thisLnSpace = StaffHEIGHT(aStaffL)/(StaffSTAFFLINES(aStaffL)-1);
		if (thisLnSpace!=lnSpace) {
			allSameSize = False;
			break;
		}
	}

	if (allSameSize) {
		DDIST rastralLnSpace = drSize[doc->srastral]/(STFLINES-1);
		if (lnSpace!=rastralLnSpace)
			if (CautionAdvise(STFSIZE_ALRT)==OK) {
				DDIST stfHeight = lnSpace*(STFLINES-1);
				for (i = 0; i<=MAXRASTRAL; i++)
					if (stfHeight==drSize[i]) doc->srastral = i;
				doc->nonstdStfSizes = FillRelStaffSizes(doc); 		/* Should return False! */
			}
			return True;
	}
	return False;
}


/* --------------------------------------------------------------------- InitDocFields -- */
/* Initialize miscellaneous fields in the given Document. If there's a problem (out
of memory), give an error message and return False, else True. */

Boolean InitDocFields(Document *doc)
{
	short i;

	SetRect(&doc->prevMargin, 0, 0, 0, 0);
	doc->headerFooterMargins = config.pageNumMarg;
	
	doc->sheetOrigin.h = config.origin.h - config.hPageSep;
	doc->sheetOrigin.v = config.origin.v - config.vPageSep;
	
	/* Have to reset the document's origin (scroll position) after reading from file */
	doc->origin = doc->sheetOrigin;
	
	doc->numRows = config.numRows;
	doc->numCols = config.numCols;
	
	doc->firstPageNumber = 1;
	doc->startPageNumber = 2;
	doc->topPGN = False;
	doc->hPosPGN = CENTER;
	
	doc->firstSheet = 0;
	doc->currentSheet = 0;
	doc->numSheets = 1;
	
	/* Compute bounding box of all sheets, and allocate and compute background region. */
	doc->background = NewRgn();
	if (doc->background==NULL) {
		NoMoreMemory();
		return False;
	}
	GetAllSheets(doc);
	
	/*
	 * Set initial zoom origin to a point that's not on any page so SheetMagnify
	 * will just use the current page.
	 */
	doc->scaleCenter.h = doc->scaleCenter.v = SHRT_MAX;

	doc->vScroll = doc->hScroll = NULL;
	doc->changed = False;
	doc->canCutCopy = False;
	doc->active = doc->caretActive = doc->caretOn = False;
	doc->masterView = doc->overview = False;
	doc->masterChanged = False;
	doc->showFormat = doc->enterFormat = False;
	doc->locFmtChanged = False;
	doc->hasCaret = False;							/* Tell DoUpdate to initialize caret */
	doc->autoRespace = True;
	doc->pianoroll = False;
	doc->showSyncs = False;
	doc->frameSystems = False;
	doc->colorVoices = 2;							/* Show all voices but no. 1 in color */
	doc->showInvis = False;
	doc->showDurProb = True;
	doc->showWaitCurs = True;
	
	doc->magnify = 0;
	InstallMagnify(doc);
	
	doc->spacePercent = doc->htight = 100;
	doc->lookVoice = -1;
	doc->selStaff = 1;
	doc->deflamTime = 50;
	
	doc->srastral = config.defaultRastral;			/* Default staff rastral size */
	
	doc->altsrastral = 2;							/* Default alternate staff rastral size */
	
	doc->numberMeas = -1;							/* Show measure nos. on every system */
	doc->otherMNStaff = 0;
	doc->firstMNNumber = 1;
	doc->aboveMN = True;
	doc->yMNOffset = 5;
	doc->xMNOffset = 0;
	doc->sysFirstMN = True;
	doc->xSysMNOffset = 0;		
	
	doc->insertMode = False;
	doc->beamRests = False;
	doc->recordFlats = False;

	for (i=0; i<MAXSTAVES; i++)
		doc->omsPartDeviceList[i] = 0;
	doc->omsInputDevice = 0;

	for (i=0; i<MAXSTAVES; i++)
		doc->cmPartDeviceList[i] = 0;
	doc->cmInputDevice = 0;

	doc->fmsInputDevice = noUniqueID;
	/* ??We're probably not supposed to play with these fields... */
	doc->fmsInputDestination.basic.destinationType = 0,
	doc->fmsInputDestination.basic.name[0] = 0;

	doc->nonstdStfSizes = False;

	/* Avoid certain Debugger complaints in MEHideCaret */
	SetRect(&doc->viewRect, 0, 0, 0, 0);
	SetRect(&doc->growRect, 0, 0, 0, 0);

	if (!(doc->stringPool = NewStringPool())) {
		DisposeRgn(doc->background);
		MayErrMsg("InitDocFields: couldn't allocate string pool for document");	/* ??CHG TO USER ERR MSG? */
		return False;
		}

	doc->mutedPartNum = 0;

	if (config.musicFontID==0)
		Pstrcpy(doc->musFontName, "\pSonata");
	else {
		Str255 fontName;
		GetFontName(config.musicFontID, fontName);
		if (Pstrlen(fontName)==0) {					/* no such font in system; use Sonata */
			Pstrcpy(doc->musFontName, "\pSonata");
		}
		else
			Pstrcpy(doc->musFontName, fontName);
	}
	InitDocMusicFont(doc);

	/* initialize Carbon Printing data structures */
	doc->docPrintInfo.docPrintSession = NULL;
	doc->docPrintInfo.docPageFormat = kPMNoPageFormat;
	doc->docPrintInfo.docPrintSettings = kPMNoPrintSettings;
	doc->docPrintInfo.docPageSetupDoneUPP = NULL;
	doc->docPrintInfo.docPrintDialogDoneUPP = NULL;
	
	doc->midiMapFSSpecHdl = NULL;
	
	return True;
}

/* ----------------------------------------------------------------------- InitDocUndo -- */
/* Initialize the given Document's Undo fields, Undo record, and (empty) Undo
object list. If there's a problem (out of memory), return False, else True. */

Boolean InitDocUndo(Document *doc)
{
	doc->undo.lastCommand = U_NoOp;
	doc->undo.subCode = 0;
	doc->undo.headL = NILINK;
	doc->undo.tailL = NILINK;
	doc->undo.selStartL = doc->undo.selEndL = NILINK;
	doc->undo.param1 = doc->undo.param2 = 0;
	doc->undo.redo = False;
	doc->undo.hasUndo = False;
	doc->undo.canUndo = False;
	strcpy(doc->undo.menuItem, "");
	doc->undo.undoRecord = NewHandle(0L);
	if (!doc->undo.undoRecord) { NoMoreMemory(); return False; }

	BuildEmptyList(doc,&doc->undo.headL, &doc->undo.tailL);	/* ??Can fail: should check! */
	
	return True;
}


/* --------------------------------------------------------------------- BuildDocument -- */
/*
 *	Initialise a Document.  If <isNew>, make a new Document, untitled and empty;
 *	otherwise, read the given file, decode it, and attach it to this Document.
 *	In any case, make the Document the current grafPort and install it, including
 *	magnification. Return True normally, False in case of error.
 */

Boolean BuildDocument(
		register Document *doc,
		unsigned char *fileName,
		short vRefNum,
		FSSpec *pfsSpec,
		long *fileVersion,
		Boolean isNew		/* True=new file, False=open existing file */
		)
{
	WindowPtr w = doc->theWindow;
	Rect r;
	DDIST sysTop;
	
	SetPort(GetWindowPort(w));
	
	/*
	 *	About coordinate systems:
	 *	The point (0,0) is placed at the upper left corner of the initial paperRect.
	 *  The viewRect is the same as the window portRect, but without the scroll bar
	 *	areas. The display of the document's contents should always be in this
	 *	coordinate system, so that printing will be in the same coordinate system.
	 */
	
	/* Set the initial paper size, margins, etc. from the config */
	
	doc->pageType = 2;
	doc->measSystem = 0;
	doc->paperRect = config.paperRect;
	doc->origPaperRect = config.paperRect;

	doc->marginRect.top = doc->paperRect.top+config.pageMarg.top;
	doc->marginRect.left = doc->paperRect.left+config.pageMarg.left;
	doc->marginRect.bottom = doc->paperRect.bottom-config.pageMarg.bottom;
	doc->marginRect.right = doc->paperRect.right-config.pageMarg.right;

	if (!InitDocFields(doc)) return False;

	FillSpaceMap(doc, 0);

	/*
	 *	Add the standard scroll bar controls to Document's window. The scroll bars
	 *	are created with a maximum value of 0 here, but that will have no effect if
	 *	we call RecomputeView, since it resets the max.
	 */
	GetWindowPortBounds(w,&r);
	r.left = r.right - (SCROLLBAR_WIDTH+1);
	r.bottom -= SCROLLBAR_WIDTH;
	r.top--;
	
	doc->vScroll = NewControl(w,&r,"\p",True,doc->origin.h,doc->origin.h,
														0,scrollBarProc,0L);
	GetWindowPortBounds(w,&r);
	r.top = r.bottom - (SCROLLBAR_WIDTH+1);
	r.right -= SCROLLBAR_WIDTH;
	r.left += MESSAGEBOX_WIDTH;
	
	doc->hScroll = NewControl(w,&r,"\p",True,doc->origin.v,doc->origin.v,
														0,scrollBarProc,0L);
	if (!InitAllHeaps(doc)) { NoMoreMemory(); return False; }
	InstallDoc(doc);
	BuildEmptyList(doc,&doc->headL,&doc->tailL);
	
	doc->selStartL = doc->selEndL = doc->tailL;				/* Empty selection  */
	
	/*
	 * Set part name showing and corresponding system indents. We'd like to use
	 * PartNameMargin() to get the appropriate indents, but not enough of the data
	 * structure is set up, so do something cruder.
	 */
	doc->firstNames = FULLNAMES;							/* 1st system: full part names */
	doc->firstIndent = qd2d(config.indent1st, drSize[doc->srastral], STFLINES);
	doc->otherNames = NONAMES;								/* Other systems: no part names */
	doc->otherIndent = 0;
	
	if (isNew) {
		*fileVersion = THIS_VERSION;
		NewDocScore(doc);									/* Set up initial staves, clefs, etc. */
		doc->firstSheet = 0;
		doc->currentSheet = 0;
		doc->origin = doc->sheetOrigin;						/* Ignore position recorded in file */
	}
	else {													/* Finally READ THE FILE! */
		if (OpenFile(doc,(unsigned char *)fileName,vRefNum,pfsSpec,fileVersion)!=noErr)
			return False;
		doc->firstSheet = 0;								/* Or whatever; may be document specific! */
		doc->currentSheet = 0;
		doc->lastGlobalFont = 4;							/* Default is Regular1 */
		InstallMagnify(doc);								/* Set for file-specified magnification */
		if (doc->masterHeadL == NILINK) {
			/* This is an ancient file without Master Page object list: make default one */
			sysTop = SYS_TOP(doc);
			NewMasterPage(doc,sysTop,True);
			}
		doc->nonstdStfSizes = FillRelStaffSizes(doc);
		if (doc->nonstdStfSizes)
			if (!AllStavesSameSize(doc)) {
				GetIndCString(strBuf, MISCERRS_STRS, 6);	/* "Not all staves are the std. size" */
				CParamText(strBuf, "", "", "");
				CautionInform(GENERIC_ALRT);
			}
		}

	if (!InitDocUndo(doc)) return False;

	doc->yBetweenSysMP = doc->yBetweenSys = 0;				/* No longer used */
	SetOrigin(doc->origin.h, doc->origin.v);
	GetAllSheets(doc);

	/* Finally, set empty selection just after the first Measure and put caret there.
		NB: This will not necessarily be on the screen! We should eventually make
		the initial selection agree with the doc's scrolled position. */
	
	SetDefaultSelection(doc);
	doc->selStaff = 1;
	
	MEAdjustCaret(doc, False);
	
	return True;
}
