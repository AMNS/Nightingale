/***************************************************************************
*	FILE:	Event.c
*	PROJ:	Nightingale
*	DESC:	Event-related routines
/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "CarbonPrinting.h"

#define mouseMovedEvt			0xFA
#define suspResumeEvt			0x01
#define isResume(e)				( ((e)->message & 1) != 0 )
#define isSuspend(e)			( ((e)->message & 1) == 0 )
#define clipboardChanged(e)		( ((e)->message & 2) != 0 )

/* Prototypes for private routines */

static void		DoNullEvent(EventRecord *evt);
static Boolean	DoMouseDown(EventRecord *evt);
static void		DoContent(WindowPtr w, Point pt, short modifiers, long when);
static void		DoDocContent(WindowPtr w, Point p, short modifiers, long when);
static void		DoGrow(WindowPtr w, Point pt, Boolean command);
static Boolean	DoKeyDown(EventRecord *evt);
static void		DoKeyUp(EventRecord *event);
static void		CheckMemory(void);

static void		DoHighLevelEvent(EventRecord *evt);
static OSErr	MyGotRequiredParams(const AppleEvent *appleEvent);

static Boolean printOnly;
static ControlActionUPP	actionUPP = NULL;


/* Handle each event as it arrives from the event queue.  Check available memory every
so often on NULL events.  Also maintain the cursor and try to purge all segments. */

#define MEMCHECK_INTVL 1800L		/* Ticks between checks of memory availability */
#define DSCHECK_INTVL 150L			/* Ticks between event and checking main object list */

Boolean DoEvent()
	{
		Boolean haveEvent,keepGoing = TRUE,activ;
		long soon; short result;
		static short fixCount = 1;
		Point corner;
		static long checkMemTime=BIGNUM, checkDSTime=BIGNUM;

		if (hasWaitNextEvent) {
			soon = (config.mShakeThresh ? 0 : 8);
			haveEvent = WaitNextEvent(everyEvent, &theEvent, soon, NULL);
		}
		else {
			haveEvent = GetNextEvent(everyEvent, &theEvent);
			}


		AnalyzeWindows();			/* After event has been gotten */
		
		
		FixCursor();				/* After windows have been analyzed */
		if (fixCount>0 && !quitting) {
			FixMenus();
			fixCount--;
			}
		
		if (haveEvent && !quitting) {
			switch (theEvent.what) {
				case mouseDown:
					keepGoing = DoMouseDown(&theEvent);
					checkMemTime = 0L;										/* Check memory now */
					checkDSTime = TickCount();								/* Check obj list after delay */
					break;
				case autoKey:
				case keyDown:
					keepGoing = DoKeyDown(&theEvent);
					checkMemTime = 0L;										/* Check memory now */
					checkDSTime = TickCount();								/* Check obj list after delay */
					break;
				case keyUp:
					DoKeyUp(&theEvent);
					break;
        	    case activateEvt:
        	    	activ = (theEvent.modifiers &activeFlag) != 0;
        	    	DoActivate(&theEvent,activ,FALSE);
        	    	break;
        	    case updateEvt:
        	    	DoUpdate((WindowPtr)(theEvent.message));
        	    	break;
        	    case diskEvt:
					result = HiWord(theEvent.message);
					if (result) {
						/*
						 *	It'd be better to center the alert, but anyway, (112,80) is
						 *	the location used by Finder 6.1.
						 */
						SetPt(&corner, 112, 80);
						}
        	    	break;
        	    case app4Evt:
        	    	DoSuspendResume(&theEvent);
        	    	break;
        	    case kHighLevelEvent:
        	    	if (appleEventsOK)
        	    		DoHighLevelEvent(&theEvent);
        	    	break;
        	    	
				default:
					break;
				}
			fixCount = 1;				/* Force 1 null event calls to FixMenus() */
			}
			
		 else
			if (closingAll || quitting)
				/*
				 *	If the application is quitting or user asked to have all windows
				 *	closed, we close the top visible window during each pass through the
				 * event loop until all windows are closed.
				 */
				if (TopWindow)
					DoCloseWindow(TopWindow);
				else {
					closingAll = FALSE;
					if (quitting) keepGoing = FALSE;
					}
			 else {
				/* Just a plain old vanilla null event */
				if ((TickCount()-checkMemTime) > MEMCHECK_INTVL) {
					/* Check avail. memory if enough time has passed since last check. */
					CheckMemory();
					checkMemTime = TickCount();
					}
				 DoNullEvent(&theEvent);
				}
		
		return(keepGoing);
	}	


/* Check for low memory.  This puts up a warning each time the available free
memory goes from above to below config.lowMemory, and on every call if memory
goes below config.minMemory. Intended to be called from the main event loop. */

static void CheckMemory()
	{
		long nBytes;
		long grow=0L;
		static long thresh=0L;
		char numStr[32];

		nBytes = grow + FreeMem();				/* Total free memory */
		nBytes /= 1024;							/* Convert to Kbytes */
		if (nBytes <= 0) nBytes = 1;			/* Since alert says "less than " */
		
		if (nBytes < thresh) {
			sprintf(numStr, "%ld", nBytes);
			CParamText(numStr,"","","");
			/* PlaceAlert(lowMemoryID,NULL,0,0); */
			CautionAlert(lowMemoryID,NULL);
			thresh = config.minMemory;
			}
		 else
			if (nBytes >= config.lowMemory || thresh <= 0)
				thresh = config.lowMemory;
	}


/* Do what has to be done on null events.  This is usually nothing, but at the
very least we have to make carets blink in any open non-modal dialogs with
editable text in them.  If we're using the windowKind field to distinguish
among them, then we have to  restore it to dialogKind temporarily for the
benefit of some too-smart-for-our-own-good Toolbox routines. */

static void DoNullEvent(EventRecord *evt)
	{
		WindowPtr w; short itemHit; Boolean other = FALSE;
		Document *doc;
		
		if (w = TopWindow) 
			switch (GetWindowKind(w)) {
				case dialogKind:
					DialogSelect(evt,(DialogPtr *)&w,&itemHit);	/* Force carets to blink */
					other = TRUE;
					break;
				case PALETTEKIND:
					break;
				case DOCUMENTKIND:
					break;
				default:
					other = TRUE;
					break;
				}
		
		if (!other)
			if (doc = GetDocumentFromWindow(TopDocument)) {	/* If TopDocument exists, use it to idle the caret */
				if (doc != clipboard) {
					if (!doc->overview && !doc->masterView) MEIdleCaret(doc);
					}
				}
	}

/* Handle a generic update event, doing different things depending on the
type of window the event is for. */

void DoUpdate(WindowPtr w)
	{
		GrafPtr oldPort; 
		Rect bBox; Boolean doView;
		
		GetPort(&oldPort); SetPort(GetWindowPort(w));
		Document *doc, *topDoc;
		
		BeginUpdate(w);
	
		switch (GetWindowKind(w)) {
			case DOCUMENTKIND:
				doc=GetDocumentFromWindow(w);
				if (doc!=NULL) {
					doView = TRUE;
					/* Get bounding box of region to redraw in local coords */
					RgnHandle visRgn = NewRgn();
					GetPortVisibleRegion(GetWindowPort(w), visRgn);
					GetRegionBounds(visRgn,&bBox);
					DisposeRgn(visRgn);
					InstallDoc(doc);
					DrawDocumentControls(doc);
					DrawMessageBox(doc, TRUE);
					if (doView) DrawDocumentView(doc, &bBox);
					topDoc = GetDocumentFromWindow(TopDocument);
					if (topDoc!=NULL) InstallDoc(topDoc);
				}
				break;
			case PALETTEKIND:
				UpdatePalette(w);
			default:
				break;
			}
		
		EndUpdate(w);
		SetPort(oldPort);
	}


/* For all visible windows in window list, update them.  This is used after
various alerts/dialogs that are followed by lengthy operations to keep those
unsightly holes from staying in the windows during the lengthy operation. */

void UpdateAllWindows()
	{
		WindowPtr wp;
		
		for (wp=FrontWindow(); wp; wp = GetNextWindow(wp))
			if (IsWindowVisible(wp)) DoUpdate(wp);
	}


/* Handle an activate/deactivate event.  If isJuggle is TRUE, then this is
the final part of a suspend/resume event. */

void DoActivate(EventRecord *event, Boolean activ, Boolean isJuggle)
	{
		WindowPtr w; short itemHit;
		ScrapRef scrap;
		static Boolean wasOurWindow;
		WindowPtr curAct;
		Document *doc;
		DialogPtr d;
		
		w = (WindowPtr)event->message;
		HiliteUserWindows();		/* Make sure all windows are properly hilited */
		
		if (IsPaletteKind(w))
			w = TopDocument;			/* Pass the activate event from a palette to the TopDocument. */
		
		if (w == NULL) return;
		
		/*
		 *	If the previously active window was a Desk Accessory, then
		 *	check for a changed scrap, and pull any scrap in.  If a DA
		 *	window just got deactivated, it'll be in CurDeactive, but
		 *	if it just got closed, CurDeactivate will be NULL and no use.
		 *	We have to remember in wasOurWindow when one of our windows
		 *	is in front in order to properly convert the scrap.
		 *	If a DA window is about to get activated, then we have to
		 *	make the scrap public.
		 */
		if (activ) {
			if (!isJuggle) {
				GetCurrentScrap(&scrap);
				if (gNightScrap!=NULL) {
					if (scrap != gNightScrap)		/* Scrap changed */
						if (!wasOurWindow)
							gNightScrap = scrap;
				}
				else
					gNightScrap = scrap;
				}
			wasOurWindow = TRUE;
			}
		 else {
		 	/* 
		 	 * DMcK, 4/11/91: Apple DTS says the top bit of CurActivate is used as
		 	 * a flag for something or other and should be masked off!
		 	 */
//			curAct = (WindowPeek)((long)LMGetCurActivate() & 0x7FFFFFFF);
			curAct = FrontWindow();
			if (curAct!=NULL && GetWindowKind(curAct)<0) {
				GetCurrentScrap(&gNightScrap);
				TEToScrap();
				wasOurWindow = FALSE;
				}
			}
		
		/* Now we go on and do more window-specific activation chores */
		
		switch (GetWindowKind(w)) {
			case DOCUMENTKIND:
				doc = GetDocumentFromWindow(w);
				if (doc) {
					ActivateDocument(doc,activ);
				}
				break;
			case PALETTEKIND:
				break;
			case dialogKind:
				d = GetDialogFromWindow(w);
				if (d) {
					if (activ) {
						DialogSelect(event,&d,&itemHit);
						}
					FrameDefault(d,1,activ);
					}
				break;
			default:
				break;
			}
		
		if (activ) {
			SetPort(GetWindowPort(w));
			ArrowCursor();
			}
	}


/* Handle MultiFinder Suspend and Resume (app4) events */

void DoSuspendResume(EventRecord *event)
	{
		Byte evtType; Boolean activ;
		static Boolean clipShow;
		
		activ = isResume(event);
		
		ShowHidePalettes(activ);
		AnalyzeWindows();
		HiliteUserWindows();
		
		evtType = event->message >> 24;			/* Get high byte */
		switch(evtType) {
			case mouseMovedEvt:
				break;
			case suspResumeEvt:
				if (activ) {
					ScrapRef scrap;
					GetCurrentScrap(&scrap);
					if (ClipboardChanged(scrap)) {
						gNightScrap = scrap;
						TEFromScrap();
						ImportDeskScrap();
						}
					/* Convert to activate event */
					event->modifiers |= activeFlag;
					inBackground = FALSE;
					if (clipShow) {
						/* DoSelectWindow(clipboard->theWindow); */
						ShowWindow(clipboard->theWindow);
						}
#ifdef WantKeyUps
					SetEventMask(oldEventMask|keyUpMask);
#endif
					ArrowCursor();
					}
				 else {				/* Else it's a suspend event */
					inBackground = TRUE;
					/* Convert to deactivate event */
					event->modifiers &= ~activeFlag;
					if (clipShow = IsWindowVisible(clipboard->theWindow))
						HideWindow(clipboard->theWindow);
					SetEventMask(oldEventMask);
					}
				AnalyzeWindows();
				event->message = (long)TopDocument;
				event->what = activateEvt;
				if (event->message) {
					DoActivate(event,activ,TRUE);
					}
				break;
			}
	}


/* Handle a generic mouse down event */

static Boolean DoMouseDown(EventRecord *event)
	{
		short part;
		Boolean keepGoing = TRUE,option,shift,command,frontClick;
		WindowPtr w; Point pt;
		
		option  = (event->modifiers & optionKey) != 0;
		shift   = (event->modifiers & shiftKey) != 0;
		command = (event->modifiers & cmdKey) != 0;
		
		pt = event->where;
		part = FindWindow(pt,&w);
		
		//if (gResList)
		//	WindowPtr front = FrontWindow();		
		
		//indowPtr rlw = GetWindow
		
		if (w == NULL)
			switch(part) {
				case inDesk:
					break;
				case inMenuBar:
					ArrowCursor();
					keepGoing = DoMenu(MenuSelect(pt));
					break;
				}
		 else
			if (part == inSysWindow)
				;
			 else {
			 	frontClick = TRUE;
				if (!ActiveWindow(w)) {
					/*
					 *	If the window clicked in is a palette, bring it to front like
					 *	any other, but continue processing click as if it had been active.
					 */
					frontClick = (IsPaletteKind(w) && w!=TopPalette);
					DoSelectWindow(w);
					if (frontClick) {
						DoUpdate(w);
						AnalyzeWindows();
						}
					}
				if (frontClick)
					switch(part) {
						case inContent:
							DoContent(w,pt,event->modifiers,event->when);
							break;
						case inGoAway:
							if (TrackGoAway(w,pt)) {
								DoCloseWindow(w);
								closingAll = option!=0;
								}
							break;
						case inZoomIn:
						case inZoomOut:
							if (TrackBox(w,pt,part)) {
								/*
								 *	Force caret into off state, otherwise any invalid
								 *	background bits covered by palette may about to become
								 *	into view.
								 */
								if (IsPaletteKind(w))
									if (TopDocument)
										MEHideCaret(GetDocumentFromWindow(TopDocument));
								DoZoom(w,part);
								}
							break;
						case inDrag:
							/*
							 *	Force caret into off state, otherwise any invalid
							 *	background bits covered by palette may about to become
							 *	into view.
							 */
							if (IsPaletteKind(w))
								if (TopDocument)
									MEHideCaret(GetDocumentFromWindow(TopDocument));
							DoDragWindow(w);
							SetZoomState(w);
							break;
						case inGrow:
							DoGrow(w,pt,command);
							break;
						}
				}
		return(keepGoing);
	}	


/* Handle a general content mouse down event.  pt is expected in global coords. */

static void DoContent(WindowPtr w, Point pt, short modifiers, long when)
	{
		short code,val,oldVal,change; short index; GrafPtr oldPort;
		ControlHandle control;
		Rect portRect;
		short contrlHilite;
		Document *doc;

		if (actionUPP == NULL)			/* first time, so allocate it */
			actionUPP = NewControlActionUPP(ScrollDocument);

		GetPort(&oldPort); SetPort(GetWindowPort(w));
		GlobalToLocal(&pt);
		
		switch (GetWindowKind(w)) {
			case DOCUMENTKIND:
				doc = GetDocumentFromWindow(w);
				if (doc) {
					//switch( code = FindControl(pt,w,&control) ) {
					control = FindControlUnderMouse(pt, w, &code);
					switch( code ) {
						case kControlUpButtonPart:
						case kControlPageUpPart:
						case kControlDownButtonPart:
						case kControlPageDownPart:
							contrlHilite = GetControlHilite(control);
							if (contrlHilite != 255) {
								MEHideCaret(doc);
								GetWindowPortBounds(w,&portRect);
								ClipRect(&portRect);
								TrackControl(control,pt,actionUPP);
								ClipRect(&portRect);
								DrawControls(w);
								ClipRect(&doc->viewRect);
								}
							break;
						case kControlIndicatorPart:
							contrlHilite = GetControlHilite(control);
							if (contrlHilite != 255) {
								oldVal = GetControlValue(control);
								GetWindowPortBounds(w,&portRect);
								ClipRect(&portRect);
								TrackControl(control,pt,NULL);
								val = GetControlValue(control);
								ClipRect(&doc->viewRect);
								if (change = val-oldVal) {
									if (change & 7)
										/* Keep change a multiple of 8 */
										if (change > 0) change += (8-(change&7));
										 else			change -= (change & 7);
									/*
									 *	Reset it, because QuickScroll expects old value,
									 *	but don't let user see you set it back.
									 */
									SetControlValue(control,oldVal);
									MEHideCaret(doc);
									/* OK, now go ahead */
									if (control == doc->vScroll) QuickScroll(doc,0,change,TRUE,TRUE);
									 else						 		  QuickScroll(doc,change,0,TRUE,TRUE);
									}
								}
							break;
						default:
							if (doc!=clipboard)
								DoDocContent(w,pt,modifiers,when);
							break;
						}
					}
				break;
			case PALETTEKIND:
				notMenu = TRUE;
				index = GetWRefCon(w);
				if (index == TOOL_PALETTE) DoToolContent(pt,modifiers);
				if (index == CLAVIER_PALETTE) SysBeep(60);
				notMenu = FALSE;
				break;
					
			}
		
		SetPort(oldPort);
	}


/* This routine is called from GetSelection() and other click and drag loops
so that the view of the Document can be scrolled automatically if the
user pulls the mouse outside the viewRect of the window. */

void AutoScroll()
	{
		Point pt; short dx,dy; Document *theDoc = GetDocumentFromWindow(TopDocument);
		PenState pen;
		
		if (theDoc) {
			GetMouse(&pt);
			if (!PtInRect(pt,&theDoc->viewRect)) {
				dx = dy = 0;
				if (pt.h < theDoc->viewRect.left) dx = -24;
				 else if (pt.h >= theDoc->viewRect.right) dx = 24;
				if (pt.v < theDoc->viewRect.top) dy = -24;
				 else if (pt.v >= theDoc->viewRect.bottom) dy = 24;
				
				GetPenState(&pen);
				QuickScroll(theDoc,dx,dy,TRUE,TRUE);		/* Change coordinate system */
				SetPenState(&pen);
				}
			}
	}


/* Handle a mouse down event in Document content area, i.e., in a Document window
but not in a control.  pt is expected in local coords. In documents, entertain
selection rectangle with automatic scrolling if user drags mouse out of view. */

static void DoDocContent(WindowPtr w, Point pt, short modifiers, long when)
	{
		Rect paper; Document *doc = GetDocumentFromWindow(w);
		short ans,doubleClick;
		Boolean didSomething;
		
		if (doc==NULL) return;
		
		if (PtInRect(pt,&doc->viewRect))
			if (FindSheet(doc,pt,&ans)) {
				GetSheetRect(doc,ans,&paper);
				doc->currentSheet = ans;
				doc->currentPaper = paper;
				if (doc->overview) ;			/* See NightArchive:MiscCode:OverviewGoTo for former code */
				 else {
					doubleClick = IsDoubleClick(pt, 1, when);

					doc->scaleCenter = pt;		/* Save window coord for SheetMagnify */
					
					/* Transform to paper relative coords: in pixels relative to
						upper left corner of page (e.g. by subtracting (-24000,-24000)) */
					pt.h -= paper.left;
					pt.v -= paper.top;
					
					if (doc->masterView) {
						didSomething = DoEditMaster(doc,pt,modifiers,doubleClick);
						if (didSomething) doc->masterChanged = TRUE;
					}
					else if (doc->showFormat) {
						didSomething = DoEditFormat(doc,pt,modifiers,doubleClick);

						if (didSomething)
							/* doc->formatChanged TRUE; */
							;
					}
					else
						didSomething = DoEditScore(doc,pt,modifiers,doubleClick);

#ifdef SHEETSSELECTION
					if (didSomething)
						doc->changed = TRUE;
					 else {
						theSelectionType = MARCHING_ANTS;
						GetSelection(w,&result,/*&paper*/NULL,AutoScroll);
						}
#endif
						
					}
				}
			 else {
#ifdef SHEETSSELECTION
				theSelectionType = MARCHING_ANTS;
				GetSelection(w,&result,NULL,AutoScroll);
#endif
				}
		 else {
			/* Click in message box */
			}
		SetRect(&theSelection,0,0,0,0);
	}


/* Deal with a generic key down event.  In general, keys typed are intended
to be choices among the various symbols and tools in the Tools Palette,
which may or may not be visible, and which can be visible even when there's
no current document open.  Other keys, such as DELETE or RETURN, are dealt
with separately.  Routine delivers whether to continue event loop or not. */


static Boolean DoKeyDown(EventRecord *event)
	{
		short ch,itemHit,key; short nparts; WindowPtr wp;
		Boolean scoreView;
		Document *doc = GetDocumentFromWindow(TopDocument);
		unsigned long cmdCode;
		
		ch = (unsigned)(event->message & charCodeMask);
		key = (event->message>>8) & charCodeMask;
		
		if (event->modifiers & cmdKey) {
#if TARGET_API_MAC_CARBON
			//cmdCode = MenuKey((char)(event->message & charCodeMask));
			cmdCode = MenuEvent(event);
#else
			cmdCode = MDEF_MenuKey(event->message, event->modifiers, hMenu);
#endif
			return(DoMenu(cmdCode));
		}
		
		wp = FrontWindow();
		if (wp == NULL) return(TRUE);
		
		scoreView = doc ? (!doc->masterView && !doc->showFormat) : FALSE;

		if (GetWindowKind(wp)== dialogKind) {
			/* Deal with key down in modeless dialog, such as get info dialogs,etc. */
			if (DialogSelect(event,(DialogPtr *)&wp,&itemHit)) { }
			}
		 else
			switch(ch) {
				case CH_BS:							/* Handle backspace to delete */
					if (doc) {
						if (scoreView && ContinSelection(doc, config.strictContin!=0)) {
							if (BFSelClearable(doc, BeforeFirstMeas(doc->selStartL))) {
								DoClear(doc);
								doc->changed = TRUE;
								}
							}
						else if (doc && doc->masterView) {
							nparts = LinkNENTRIES(doc->masterHeadL);	/* FIXME: nparts isn't used! */
							if (PartIsSel(doc)) MPDeletePart(doc);
							}
						}
					break;
				case '\r':
					if (doc) {
						/* For whatever, like system breaks or adding new systems ... */
						}
					break;
				default:
					if (doc || IsWindowVisible(palettes[TOOL_PALETTE]))
						if (scoreView)
							DoToolKeyDown(ch,key,event->modifiers);
				}
			
		return(TRUE);
	}


/* Entertain key up events (now ignored, but we might use them someday). */

static void DoKeyUp(EventRecord */*event*/)
	{
	}


/* Handle a generic mouse down event in any window's grow area. */

static void DoGrow(WindowPtr w, Point pt, Boolean /*command*/)
	{
		Rect limitRect,oldRect,oldMessageRect; GrafPtr oldPort;
		long newSize; short x,y; long kind;
		
		GetPort(&oldPort);
		
		switch (GetWindowKind(w)) {
			case PALETTEKIND:
				/*
				 *	Force caret into off state, otherwise invalid background bits
				 *	covered by palette may come into view.
				 */
				if (TopDocument)
					MEHideCaret(GetDocumentFromWindow(TopDocument));
				kind = GetWRefCon(w);
				if (kind == (long)TOOL_PALETTE) {
					DoToolGrow(pt);
				}
				break;
			case DOCUMENTKIND:
				GetWindowPortBounds(w, &oldRect);
				Document *doc = GetDocumentFromWindow(w);
				if (doc)  {
					PrepareMessageDraw(doc,&oldMessageRect,TRUE);
					oldMessageRect.top--;		/* Include DrawGrowIcon line */
					SetRect(&limitRect,MESSAGEBOX_WIDTH+70,80,20000,20000);
					newSize = GrowWindow(w,pt,&limitRect);
					if (newSize) {
						EraseAndInval(&oldMessageRect);
						x = LoWord(newSize); y = HiWord(newSize);
						SizeWindow(w,x,y,TRUE);
						RecomputeWindow(w);
						SetZoomState(w);
						}					
				}
				break;
			}
		SetPort(oldPort);
	}

/* ============================= APPLE EVENT ROUTINES ============================= */
/* For AppleEvent-handling routines, THINK C 7 (with Univ Interfaces 2.1) wants the
third param to be just <long> but doesn't insist; CW Pro 2 (w/Univ Interfaces 3.0.1)
insists the third param be <unsigned long>. */

/* Open-an-application event. */

pascal OSErr HandleOAPP(const AppleEvent *appleEvent, AppleEvent */*reply*/, /*unsigned*/ long /*refcon*/)
	{
		OSErr err = noErr;
		
		err = MyGotRequiredParams(appleEvent);
		if (!err)
			if (inBackground) {
				/* Use notification manager */
				}
			 else
				DoOpenApplication(TRUE);				/* Ask for input file */
		
		return(err);
	}


/* Finder (or someone) has asked us to open a document, usually when user double-
clicks on or drag-drops a score in Finder. */

pascal OSErr HandleODOC(const AppleEvent *appleEvent, AppleEvent */*reply*/, /*unsigned*/ long /*refcon*/)
	{
		OSErr err = noErr;
		long iFile,nFiles;
		Size actualSize;
		AEKeyword keywd;
		DescType returnedType;
		AEDescList docList;
		FSSpec theFile; Document *doc;
		Boolean isFirst;
		
		err = AEGetParamDesc(appleEvent, keyDirectObject, typeAEList, &docList);
		if (err) AppleEventError("HandleODOC-PDOC/AEGetParamDesc",err);
		
		err = MyGotRequiredParams(appleEvent);
		if (!err) {
			AECountItems(&docList,&nFiles);
			for (iFile=1; iFile<=nFiles; iFile++) {
				err = AEGetNthPtr(&docList,iFile,typeFSS,&keywd,&returnedType,
												(Ptr)&theFile,sizeof(FSSpec),&actualSize);
				if (err) {
					AppleEventError("HandleODOC-PDOC/AEGetNthPtr",err);
					break;
					}
				
				isFirst = (TopDocument==NULL);
				doc = FSpecOpenDocument(&theFile);
				if (doc) {
					if (printOnly) {
						UpdateAllWindows();
						/* FIXME: Is TopDocument the right way to get the document just opened? */
						NDoPrintScore(doc);
						DoCloseWindow(doc->theWindow);
						}
					else if (isFirst)
						DoViewMenu(VM_SymbolPalette);
					}
				 else
					break;					/* Couldn't open; error already reported */
				}
			}

		AEDisposeDesc(&docList);
		return(err);
	}


/* Open the given file as specified in an FSSpec record, presumably from an Apple event.
The file may be a regular score, a Notelist file, or a MIDI file. If we succeed, return
the score, else give an error message and return NULL. */

#ifdef TARGET_API_MAC_CARBON_FILEIO

Document *FSpecOpenDocument(FSSpec *theFile)
{
	Document *doc;
	FInfo fndrInfo;
	char aStr[256], fmtStr[256];
	
	OSErr result = FSpGetFInfo(theFile, &fndrInfo);
	if (result!=noErr) return NULL;

	switch (fndrInfo.fdType) {
		case DOCUMENT_TYPE_NORMAL:
			if (DoOpenDocument(theFile->name, theFile->vRefNum, FALSE, theFile)) {
				LogPrintf(LOG_DEBUG, "Opened file '%s'.\n", PToCString(theFile->name));
				break;
			}
			return NULL;
		case 'TEXT':
			if (OpenNotelistFile(theFile->name, theFile)) break;
			return NULL;
		case 'Midi':
			if (ImportMIDIFile(theFile)) break;			
			return NULL;
		default:
			/* FIXME: We should give a specific, more helpful error message if it's a Help file. */		
			GetIndCString(fmtStr, FILEIO_STRS, 7);			/* "file version is illegal" */
			sprintf(aStr, fmtStr, ACHAR(fndrInfo.fdType,3), ACHAR(fndrInfo.fdType,2),
						 ACHAR(fndrInfo.fdType,1), ACHAR(fndrInfo.fdType,0));
			LogPrintf(LOG_NOTICE, aStr); LogPrintf(LOG_NOTICE, "\n");
			CParamText(aStr, "", "", "");
			StopInform(READ_ALRT);
			return NULL;
	}

	AnalyzeWindows();
	doc = GetDocumentFromWindow(TopDocument);
	
	return doc;
}

#else

Document *FSpecOpenDocument(FSSpec *theFile)
{
	Document *doc;
	WDPBRec wdRec;
	OSErr result; FInfo fndrInfo;
	char aStr[256], fmtStr[256];
	
	/* Convert FSSpec to a new working directory */
	
	wdRec.ioCompletion = NULL;
	wdRec.ioNamePtr = NULL;
	wdRec.ioWDDirID = theFile->parID;
	wdRec.ioVRefNum = theFile->vRefNum;
	wdRec.ioWDProcID = creatorType;
	PBOpenWD(&wdRec,FALSE);
	if (wdRec.ioResult!=noErr) return NULL;
		
	result = FSpGetFInfo(theFile, &fndrInfo);
	if (result!=noErr) return NULL;

	switch (fndrInfo.fdType) {
		case DOCUMENT_TYPE_NORMAL:
			if (DoOpenDocument(theFile->name, wdRec.ioVRefNum, FALSE)) break;
			return NULL;
		case 'TEXT':
			if (OpenNotelistFile(theFile->name, wdRec.ioVRefNum)) break;
			return NULL;
		case 'Midi':
			if (ImportMIDIFile(theFile->name, wdRec.ioVRefNum)) break;			
			return NULL;
		default:
			/* FIXME: We should give a specific, more helpful error message if it's a Help file. */		
			GetIndCString(fmtStr, FILEIO_STRS, 7);			/* "file version is illegal" */
			sprintf(aStr, fmtStr, ACHAR(fndrInfo.fdType,3), ACHAR(fndrInfo.fdType,2),
						 ACHAR(fndrInfo.fdType,1), ACHAR(fndrInfo.fdType,0));
			LogPrintf(LOG_NOTICE, aStr); LogPrintf(LOG_NOTICE, "\n");
			CParamText(aStr, "", "", "");
			StopInform(READ_ALRT);
			return NULL;
	}

	AnalyzeWindows();
	doc = GetDocumentFromWindow(TopDocument);
	
	return doc;
}

#endif	//	TARGET_API_MAC_CARBON_FILEIO


pascal OSErr HandlePDOC(const AppleEvent *appleEvent, AppleEvent *reply, /*unsigned*/ long refcon)
	{
		short err;
		
		printOnly = TRUE;
		err = HandleODOC(appleEvent,reply,refcon);
		printOnly = FALSE;
		
		return(err);
	}


pascal OSErr HandleQUIT(const AppleEvent */*appleEvent*/, AppleEvent */*reply*/, /*unsigned*/ long /*refcon*/)
	{
		short keepGoing = DoFileMenu(FM_Quit);
		
		return(keepGoing);	/* FIXME: Return value is nonsense! But unused as of v. 3.1 */
	}


/* Check to see that all required parameters have been retrieved (Inside Mac 6-47). */

static OSErr MyGotRequiredParams(const AppleEvent *appleEvent)
	{
		DescType returnedType;
		Size actualSize;
		OSErr err;
		
		err = AEGetAttributePtr(appleEvent, keyMissedKeywordAttr, typeWildCard,
										&returnedType, NULL, 0, &actualSize);
		if (err == errAEDescNotFound) return(noErr);
		if (!err) err = errAEEventNotHandled;
		
		return(err);
	}


/* Call this when event manager says a high level event has happened.  This just
calls back to the Apple Event Manager to call our handlers according to the
event class: our own events or the core events.  Right now, we support only the
core events. */

static void DoHighLevelEvent(EventRecord *evt)
	{
		OSErr err = noErr;

#ifdef FUTUREEVENTS
		if (evt->message==myAppType || evt->message==(long)kCoreEventClass) {
#else
		if (evt->message == (long)kCoreEventClass) {
#endif
			err = AEProcessAppleEvent(evt);
			}
	}
