/* CheckMerge.c for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

typedef struct {
	ListHandle hndl;
	Rect bounds,scroll,content,dataBounds;
	Point cSize,cell;
	short nCells;
} UserList;

/* Symbolic Dialog Item Numbers */

static enum {
	BUT1_OK = 1,
	LIST2,
	STXT3_Cannot,
	LASTITEM
} E_CheckMergeItems;

#define OK_ITEM 		BUT1_OK
#define CANCEL_ITEM 	OK_ITEM

/* Local Prototypes */

static DialogPtr	OpenThisDialog(Document *doc);
static void			CloseThisDialog(DialogPtr dlog);
static void			DoDialogUpdate(DialogPtr dlog);
static void			DoDialogActivate(DialogPtr dlog, short activ);
static short		DoDialogItem(DialogPtr dlog, short itemHit);

static pascal		Boolean MyFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static Boolean		CheckUserItems(Point where, short *itemHit);
static short		AnyBadValues(DialogPtr dlog);

static void			GetChkMergeData(Document *doc, UserList *l);

static short		BuildList(Document *doc, DialogPtr dlog, short item, short csize, UserList *l);

static short		DoOpenCell(UserList *l, StringPtr buf, short *len);

static Point where;
static short modifiers;

/* Lists and/or popups */

static UserList list2;

static ModalFilterUPP dlogFilterUPP;


/* Display the CheckMerge dialog.  Return True if OK, False if CANCEL or error. */

short DoChkMergeDialog(Document *doc)
	{
		short itemHit,okay,keepGoing=True;
		register DialogPtr dlog;
		GrafPtr oldPort;

		/* Build dialog window and install its item values */
		
		GetPort(&oldPort);
		dlog = OpenThisDialog(doc);
		if (dlog == NULL) return(False);

		/* Entertain filtered user events until dialog is dismissed */
		
		while (keepGoing) {
			ModalDialog(dlogFilterUPP,&itemHit);
			keepGoing = DoDialogItem(dlog,itemHit);
			}
		
		/*
		 *	Do final processing of item values, such as exporting them to caller.
		 *	DoDialogItem() has already called AnyBadValues().
		 */
		
		okay = (itemHit==OK_ITEM);
		if (okay) {		/* Or whatever is equivalent */
			}

		/* That's all, folks! */
		
		CloseThisDialog(dlog);
		SetPort(oldPort);
		
		return(okay);
	}


static pascal Boolean MyFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
	{
		Boolean ans=False,doHilite=False; WindowPtr w;
		short type,ch; Handle hndl; Rect box;
		
		w = (WindowPtr)(evt->message);
		switch(evt->what) {
			case updateEvt:
				if (w == GetDialogWindow(dlog)) {
					/* Update our dialog contents */
					DoDialogUpdate(dlog);
					ans = True; *itemHit = 0;
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
				ans = CheckUserItems(where,itemHit);
				break;
			case keyDown:
				if ((ch=(unsigned char)evt->message)=='\r' || ch==CH_ENTER) {
					*itemHit = OK_ITEM;
					doHilite = ans = True;
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
                            {
								short keyFocus = GetDialogKeyboardFocusItem(dlog);
								if (keyFocus > 0) {
									/* Dialog has text edit item: select all */
									SelectDialogItemText(dlog,keyFocus,0,ENDTEXT);
									}
								 else {
									}
								*itemHit = 0;
								}
							break;
						case '.':
							*itemHit = CANCEL_ITEM;
							doHilite = True;
							break;
						}
					ans = True;		/* Other cmd-chars ignored */
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


/* Mouse down event:
Check if it's in some user item, and convert to itemHit if appropriate. */

static Boolean CheckUserItems(Point /*where*/, short */*itemHit*/)
	{
		return(False);
	}


/* Redraw the contents of this dialog due to update event. */

static void DoDialogUpdate(DialogPtr dlog)
	{
		GrafPtr oldPort;

		GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
		BeginUpdate(GetDialogWindow(dlog));

		UpdateDialogVisRgn(dlog);
		FrameDefault(dlog,BUT1_OK,True);
		FrameRect(&list2.bounds);
		LUpdateDVisRgn(dlog,list2.hndl);

		EndUpdate(GetDialogWindow(dlog));
		SetPort(oldPort);
	}


/* Activate event: Activate or deactivate this dialog and any items in it */

static void DoDialogActivate(DialogPtr dlog, short activ)
	{
		SetPort(GetDialogWindowPort(dlog));
		LActivate(activ,list2.hndl);
	}


/* Build this dialog's window on desktop, and install initial item values.
Return the dlog opened, or NULL if error (no resource, no memory). */

static DialogPtr OpenThisDialog(Document *doc)
	{
		GrafPtr oldPort;
		DialogPtr dlog;

		dlogFilterUPP = NewModalFilterUPP(MyFilter);
		if (dlogFilterUPP == NULL) {
			MissingDialog(CHKMERGE_DLOG);
			return NULL;
		}
		GetPort(&oldPort);
		dlog = GetNewDialog(CHKMERGE_DLOG,NULL,BRING_TO_FRONT);
		if (dlog==NULL) {
			DisposeModalFilterUPP(dlogFilterUPP);
			MissingDialog(CHKMERGE_DLOG);
			return NULL;
		}

		CenterWindow(GetDialogWindow(dlog), 60);
		SetPort(GetDialogWindowPort(dlog));

		/* Fill in dialog's values here */

		if (!BuildList(doc,dlog,LIST2,16,&list2)) goto broken;

		ShowWindow(GetDialogWindow(dlog));
		return(dlog);

		/* Error return */
broken:
		CloseThisDialog(dlog);
		SetPort(oldPort);
		return(NULL);
	}


/* Clean up any allocated stuff, and return dialog to primordial mists. */

static void CloseThisDialog(DialogPtr dlog)
	{
		if (list2.hndl) { LDispose(list2.hndl); list2.hndl = NULL; }
		DisposeModalFilterUPP(dlogFilterUPP);
		DisposeDialog(dlog);
	}


/* Handle OK button, and possibly handle a click in the list.
The local point is in where; modifiers in modifiers.
Returns whether or not the dialog should be closed (keepGoing). */

static short DoDialogItem(DialogPtr dlog, short itemHit)
	{
		short okay=False,keepGoing=True,val;
		Str255 str;

		if (itemHit<1 || itemHit>=LASTITEM)
			return(keepGoing);				/* Only legal items, please */

		switch(itemHit) {
			case BUT1_OK:
				keepGoing = False; okay = True;
				break;
			case LIST2:
				/*
				 * In case we wish to do anything particular with any given
				 *	voice.
				 */
				if (LClick(where,modifiers,list2.hndl)) {
					val = 256;
					DoOpenCell(&list2,str,&val);
					}
				break;
			}

		if (okay) keepGoing = AnyBadValues(dlog);
		return(keepGoing);
	}


/* Pull values out of dialog items and deliver True if any of them are
illegal or inconsistent; otherwise deliver False. */

static short AnyBadValues(DialogPtr /*dlog*/)
	{
		return(False);
	}

/* -------------------------------------------------------------------------------- */

/* ClipVInfo is defined in <NTypes.h>. */

static short GetLengthList(UserList *l, ClipVInfo *clipVInfo);

/* Set the length of the list to the number of voices with problems, and return this
value. */

static short GetLengthList(UserList *l, ClipVInfo *clipVInfo)
	{
		short v,nBad = 0;

		for (v=1; v<=MAXVOICES; v++)
			if (clipVInfo[v].vBad) nBad++;

		return(l->nCells = nBad);
	}


static void GetChkMergeData(Document *doc, UserList *l)
	{
		static char cellstr[64],partName[24];
		ClipVInfo clipVInfo[MAXVOICES+1];
		char fmtStr[256];
		short i,v,userV,stfDiff;
		LINK partL; PPARTINFO pPart;

		for (v=1; v<=MAXVOICES; v++) {
			clipVInfo[v].startTime = -1L;
			clipVInfo[v].clipEndTime = -1L;
			clipVInfo[v].firstStf = -1;
			clipVInfo[v].lastStf = -1;
			clipVInfo[v].singleStf = True;
			clipVInfo[v].hasV = False;
			clipVInfo[v].vBad = False;
		}

		stfDiff = CheckMerge1(doc,clipVInfo);

		GetLengthList(l,clipVInfo);
		LAddRow(l->nCells,0,l->hndl);
			
		for (i=0,v=1; v<=MAXVOICES; v++) {
			if (clipVInfo[v].vBad) {
				l->cell.h = 0; l->cell.v = i++;

				Int2UserVoice(doc, v+stfDiff, &userV, &partL);
				pPart = GetPPARTINFO(partL);
				strcpy(partName, (strlen(pPart->name)>8 ? pPart->shortName : pPart->name));
				GetIndCString(fmtStr, CLIPERRS_STRS, 9);					/* "Voice %d of %s" */
				sprintf(cellstr,fmtStr,userV,partName);

				CtoPstr((StringPtr)cellstr);
					
				LSetCell(cellstr+1,*cellstr,l->cell,l->hndl);
				}
			}
	}


/* Build a new list in given user item box of dialog, dlog, with cell height,
csize.  If success, delivers True; if couldn't allocate ListMgr list (no more memory
or whatever), delivers False. */

static short BuildList(Document *doc, DialogPtr dlog, short item, short csize, UserList *l)
	{
		short type;
		Rect box;
		Handle hndl;

		/* Content area (plus scroll bar) of list corresponds to user item box */
		
		GetDialogItem(dlog,item,&type,&hndl,&box);
		l->bounds = box; InsetRect(&l->bounds,-1,-1);
		SetDialogItem(dlog,item,userItem,NULL,&l->bounds);

		l->scroll = box; l->scroll.left = l->scroll.right - 15;		/* Scrollbar width */
		l->content = box; l->content.right = l->scroll.left;

		SetRect(&l->dataBounds,0,0,1,0);
		l->cSize.v = csize > 0 ? csize : 1;
		l->cSize.h = l->content.right - l->content.left;

		l->hndl = LNew(&l->content,&l->dataBounds,l->cSize,0,GetDialogWindow(dlog),False,
						False,False,True);
		if (!l->hndl) return False;

		(*l->hndl)->selFlags = lOnlyOne;		/* Or whatever */
		
		GetChkMergeData(doc,l);

		l->cell.v = 0;
		LSetSelect(True,l->cell,l->hndl);
		EraseRect(&l->content);
		InvalWindowRect(doc->theWindow,&l->bounds);
		LSetDrawingMode(True,l->hndl);

		return(True);
	}


/* Do whatever when user double clicks (opens) on a list cell.  Delivers True or
False according to whether any cell was selected or not. */

static short DoOpenCell(UserList *l, StringPtr buf, short *len)
	{
		short ans;

		l->cell.h = l->cell.v = 0;
		ans = LGetSelect(True,&l->cell,l->hndl);
		if (ans) {
			LGetCell((Ptr)buf,len,l->cell,l->hndl);
			/* Got data for first selected cell: do whatever with it */
			/* ... */
			}

		return(ans);
	}
