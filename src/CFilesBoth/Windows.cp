/* Window.c for Nightingale - special window management routines for palettes, About
box, Message box, etc. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

void DoCloseWindow(WindowPtr w)
{
	short kind; PaletteGlobals *pg;
	
	kind = GetWindowKind(w);
	if (kind < 0) {
		//CloseDeskAcc(kind);
		return;
	}
							
	if (kind == DOCUMENTKIND) {
		if (!DoCloseDocument(GetDocumentFromWindow(w))) {
			/* User canceled, or error */
			
			quitting = False;
			closingAll = False;
			return;
		}
		
		if (TopPalette == TopWindow) {
			/* Find the new TopDocument, and insure it and its controls are hilited properly. */
			
			AnalyzeWindows();
			if (TopDocument)
				ActivateDocument(GetDocumentFromWindow(TopDocument), True);
		}
	}
	else {
		/* w is still defined here */
		
		if (TopDocument) {
		
			/* Force caret into off state, otherwise any invalid background bits
			   covered by palette may about to become into view. */
			   
			MEHideCaret(GetDocumentFromWindow(TopDocument));
			/* Don't generate any events that might cause the TopDocument to be unhilited. */
			ShowHide(w, False);
		}
		
		else
		 
			/* Ensure an activate event is generated in case the next visible window
			   does not belong to the application. */
			   
			HideWindow(w);
	
		/* Reset tool palette size back to default, after it's been hidden (closed).
		   For all palettes, unload the palette-specific code segment. */
		   
		if (kind == PALETTEKIND)
			switch (GetWRefCon(w)) {
				case TOOL_PALETTE:
					pg = *paletteGlobals[TOOL_PALETTE];
					palettesVisible[TOOL_PALETTE] = False;
					ChangeToolSize(pg->firstAcross, pg->firstDown,True);
					break;
				default:
					;
			}
	}
}


/* Close all of our "normal" documents that have visible windows; leave special documents
(e.g., clipboard) alone. Return the number of windows closed. */

short DoCloseAllDocWindows()
{
	WindowPtr w, next;
	short kind, nClosed;

	/* Go thru the system window list and close our documents. NB: closing in this order
	   can cause a lot of unnecessary screen drawing for pieces of windows that get
	   exposed, then immediately closed; it'd be better to use a different order.
	   Probably the best strategy is to close all dirty windows from back to front, then
	   clean windows from front to back. Cf. AnalyzeWindows and DoCloseWindow. */
	   
	for (nClosed = 0, w = FrontWindow(); w; w = next) {
		next = GetNextWindow(w);
		kind = GetWindowKind(w);
		
		if (kind == DOCUMENTKIND) {
#ifdef TEST_SEARCH_NG
				if (w==clipboard->theWindow || w==searchPatDoc->theWindow) continue;
#else
				if (w==clipboard->theWindow) continue;
#endif
			if (DoCloseDocument(GetDocumentFromWindow(w)))
				nClosed++;
			else {
				/* User canceled, or error */
				
				return nClosed;
			}
			
			if (TopPalette==TopWindow) {
				/* Find the new TopDocument, and insure it and its controls are hilited
				   properly. */
				   
				AnalyzeWindows();
				if (TopDocument) ActivateDocument(GetDocumentFromWindow(w), True);
			}
		}
	}
	
	return nClosed;
}


void RecomputeWindow(WindowPtr w)
{
	switch (GetWindowKind(w)) {
		case DOCUMENTKIND:
			Document *doc = GetDocumentFromWindow(w);
			if (doc) RecomputeView(doc);
			break;
	}
}


/* After moving or resizing a window, call this to update its zoom state. This is
because we're using a more sophisticated zoom strategy that takes into account
multi-screen systems. */

void SetZoomState(WindowPtr /*w*/)
{
}


/*	Handle a user zoom box event */

void DoZoom(WindowPtr w, short part)
{
	PaletteGlobals *pg;
	GrafPtr oldPort;
	short across, down, kind;
	
	GetPort(&oldPort); SetPort(GetWindowPort(w));
	Document *doc;
	
	switch (GetWindowKind(w)) {
		case DOCUMENTKIND:
			doc = GetDocumentFromWindow(w); 
			Rect portRect;
			
			if (doc) {
				GetWindowPortBounds(w, &portRect);
				ClipRect(&portRect);
				EraseRect(&portRect);
				ZoomWindow(w, part, False);
				RecomputeWindow(w);
				InvalWindowRect(w, &portRect);
				ClipRect(&doc->viewRect);
			}
			break;
		case PALETTEKIND:
			kind = GetWRefCon(w);
			switch(kind) {
				case TOOL_PALETTE:
					/* Zoom the tools palette to the smallest size that shows all tool
					   cells, unless the Option key is down; in that case, zoom it to
					   its largest size. */
					   
					pg = *paletteGlobals[kind];
					if (OptionKeyDown())
						{ across = pg->maxAcross; down = pg->maxDown; }
					 else
						{ across = pg->zoomAcross; down = pg->zoomDown; }
					if (pg->across==across && pg->down==down)
						/* Zoom in or back to old values */
						
						ChangeToolSize(pg->oldAcross, pg->oldDown, True);
					 else
						/* Zoom it to the max */
						
						ChangeToolSize(across, down, True);
					break;
				}
			break;
		}
	
	SetPort(oldPort);
}


/* -------------------------------------------------------- DrawMessageBox and friends -- */

static void sprintfMagnify(Document *doc, char str[]);
static void sprintfMagnify(Document *doc, char str[])
{
	if (doc->magnify!=0)
		sprintf(str, "   %d%%", UseMagnifiedSize(100, doc->magnify));
	else
		sprintf(str, "   ");
}


/* Deliver the message box's bounds as defined here. If boundsOnly is True, that's all;
otherwise, also erase the message box; position the pen so that subsequent strings
are drawn at an appropriate point; change the clipping to the message box; and change
the font to the message box font (saving the old font in some local globals for
FinishMessageDraw to restore).

This routine lets us isolate the definition of the bounds of the messageRect in one
place.

If you've precomputed the string to display, and it's only one string, then you should
use DrawMessageString to do everything in one call. */

static short msgOldFont, msgOldFace, msgOldSize;

void PrepareMessageDraw(Document *doc, Rect *messageRect, Boolean boundsOnly)
{
	WindowPtr w = doc->theWindow;

	/* First get the message rectangle bounds. The top of the message rectangle is just
	   below the DrawGrowIcon line at the bottom of the window. */
	   
	Rect portRect;
	
	GetWindowPortBounds(w,&portRect);
	SetRect(messageRect, portRect.left, 
					portRect.bottom-(SCROLLBAR_WIDTH-1),
					portRect.left+MESSAGEBOX_WIDTH-1,
					portRect.bottom);

	if (boundsOnly) return;
	
	/* We're going to draw a message in box: prepare for temporary font/clip change */
	
	msgOldSize = GetWindowTxSize(w);	/* This is not a stack, so these routines don't nest! */
	msgOldFont = GetWindowTxFont(w);
	msgOldFace = GetWindowTxFace(w);
	
	ClipRect(messageRect);
	EraseRect(messageRect);

	MoveTo(messageRect->left+6, messageRect->bottom-4);		/* These depend on font */

	TextFont(textFontNum); TextSize(textFontSmallSize); TextFace(0);
	
	/* Ready for caller to draw some text. */
}


/* After calling PrepareMessageDraw, and drawing any number of strings/chars, call this
to restore the state of things in the document. */

void FinishMessageDraw(Document *doc)
{
	TextFont(msgOldFont); TextSize(msgOldSize); TextFace(msgOldFace);
	ClipRect(&doc->viewRect);	
}


/* For single strings, simply display them in the message box in one call that never
needs to know where messageRect is. */

void DrawMessageString(Document *doc, char *msgStr)
{
	Rect messageRect;
	
	PrepareMessageDraw(doc,&messageRect,False);
	DrawCString(msgStr);
	FinishMessageDraw(doc);
}


/* Draw the normal contents of the message box (information like selection page and
measure numbers, viewing magnification, and voice number and part name being Looked At)
in the area to the left of the horizontal scroll bar in <doc>, if reallyDraw is True;
otherwise, just Inval the message box. */

void DrawMessageBox(Document *doc, Boolean reallyDraw)
{
	Rect		messageRect;
	LINK		partL;
	short		measNum, pageNum, userVoice;
	GrafPtr		oldPort;
	WindowPtr	w=doc->theWindow;
	char		partName[256], strBuf2[256];
	PPARTINFO 	pPart;
	char		fmtStr[256];
	
	if (!reallyDraw) {
		GetPort(&oldPort);  SetPort(GetWindowPort(w));
		PrepareMessageDraw(doc, &messageRect,True);				/* Get messageRect only */
		InvalWindowRect(w, &messageRect);
		SetPort(oldPort);
		return;
	}

	strBuf2[0] = '\0';							/* strBuf is always used but this buffer isn't */

	if (doc->masterView) {
		GetIndCString(fmtStr, MESSAGEBOX_STRS, 1);   			/* "Master Page" */
		sprintf(strBuf, fmtStr);
		sprintfMagnify(doc, &strBuf[strlen(strBuf)]);
		GetIndCString(fmtStr, MESSAGEBOX_STRS, 2);   			/* "    CMD-; TO EXIT" */
		sprintf(&strBuf[strlen(strBuf)], fmtStr);
	}
	else {
		Sel2MeasPage(doc, &measNum, &pageNum);				
		if (doc->showFormat) {
			GetIndCString(fmtStr, MESSAGEBOX_STRS, 3);			/* "Work on Format  page %d" */
			sprintf(strBuf, fmtStr, pageNum);
			sprintfMagnify(doc, &strBuf[strlen(strBuf)]);
		}
		else {
			GetIndCString(fmtStr, MESSAGEBOX_STRS, 4);			/* "page %d m.%d" */
			sprintf(strBuf, fmtStr, pageNum, measNum);
			partL = GetSelPart(doc);
			pPart = GetPPARTINFO(partL);
			sprintf(&strBuf[strlen(strBuf)], " %s",
				(strlen(pPart->name)>6? pPart->shortName : pPart->name));
			sprintfMagnify(doc, &strBuf[strlen(strBuf)]);
			if (doc->lookVoice>=0) {
				if (!Int2UserVoice(doc, doc->lookVoice, &userVoice, &partL))
					MayErrMsg("DrawMessageBox: Int2UserVoice(%ld) failed.",
								(long)doc->lookVoice);
				pPart = GetPPARTINFO(partL);
				strcpy(partName, (strlen(pPart->name)>6? pPart->shortName : pPart->name));
				GetIndCString(fmtStr, MESSAGEBOX_STRS, 5);   		/* "   VOICE %d OF %s" */
				sprintf(strBuf2, fmtStr, userVoice, partName);
			}
		}
	}

	PrepareMessageDraw(doc, &messageRect, False);
	DrawCString(strBuf);
	TextFace(bold);  DrawCString(strBuf2);
	FinishMessageDraw(doc);
}

	
/* ========================================================= FLOATING PALETTE ROUTINES == */

/* Analyze the current window list in order to set the four globals that tell us about 
it. The globals are TopWindow, TopPalette, BottomPallete, and TopDocument, which all
point to visible windows, or NULL. If the TopDocument is above any visible palette, we
adjust the order of the windows. If a visible non-application window is between any
palettes and the TopDocument, adjust the order also. This function can replace most uses
of FrontWindow() in the rest of the program. */
	
void AnalyzeWindows()
{
	short palettesFound;  Boolean inOrder;
	WindowPtr next;
			
	/* Make sure all SendBehind() and NewWindow() calls bring a window to the front if
	   there are no visible palettes. */
	   
	TopWindow = TopPalette = TopDocument = NULL;
	BottomPalette = BRING_TO_FRONT;
	
	inOrder = True;
	palettesFound = 0;
	
	for (next=FrontWindow(); next; next=GetNextWindow(next)) {
		if (IsWindowVisible(next)) {
			if (TopWindow == NULL) TopWindow = next;				/* Record 1st visible window */
			if (GetWindowKind(next) == DOCUMENTKIND) {
				if (TopDocument == NULL) {
					TopDocument = next;								/* Record 1st visible Document */
					if (palettesFound == TOTAL_PALETTES) break;
				}
			}
			 else if (GetWindowKind(next) == PALETTEKIND) {
				if (TopPalette == NULL) TopPalette = next;			/* Record 1st visible palette */
				BottomPalette = next;								/* Valid when we stop looping */
			}
			 else
				if (TopDocument!=NULL || TopPalette!=NULL)
					/* Some other (?) window is visible between the palettes and TopDocument. */
					
					inOrder = False;
		}
		
		/* All windows before the last palette get to this point. */
		
		if (GetWindowKind(next) == PALETTEKIND) {
			palettesFound++;
		}
	}
	
}

/* Make sure hilites are all proper for given window list state. */

void HiliteUserWindows() 
{
	Boolean hilite;
	short index;
	WindowPtr *wp, next;
	
	if (TopPalette) {
		hilite = (TopWindow == TopPalette);
		
		wp = palettes;
		for(index=0; index<TOTAL_PALETTES; index++, wp++) {
			if (IsWindowVisible(*wp))
				HiliteWindow(*wp, hilite);
		}
			
		if (TopDocument) {
			HiliteWindow(TopDocument, hilite);
			
			/* Unhilite the remaining document windows */
			
			for (next=GetNextWindow(TopDocument); next; next=GetNextWindow(next))
				if (GetWindowKind(next) == DOCUMENTKIND) {
					ActivateDocument(GetDocumentFromWindow(next), False);
				}
		}
	}
}


/* Return True if w is a palette in front, or it's TopDocument and the application is
active. */

Boolean ActiveWindow(WindowPtr w)
{
	short kind;
	
	if (w) {
		kind = GetWindowKind(TopWindow);
		if (kind<OURKIND || kind>=TOPKIND) return(False);				/* Not us */
		if (w==TopDocument || (GetWindowKind(w)==PALETTEKIND && w==TopPalette))
			return(True);
	}
		
	return(False);
}


/* DragWindow() brings the window to the front, so we have to put things back in their
place in order to keep any palettes floating. */

void DoDragWindow(WindowPtr w)
{
	if (theEvent.modifiers & cmdKey)
		OurDragWindow(w);
	 else
		if (!ActiveWindow(w))
			DoSelectWindow(w);
		else {
			OurDragWindow(w);
		}
}


/* Application's own version of dragging. We use this instead of DragWindow because
we want the DragGrayRgn feedback to show that the window is behind our floating
palettes. A comment here clearly dating from the 1990's: "This version can handle,
e.g., Radius FPD-equipped SE's." */

void OurDragWindow(WindowPtr whichWindow)
{
	Rect portBounds = GetQDPortBounds();
	DragWindow(whichWindow, theEvent.where, &portBounds);
}


/* Send a window to the back of the list, modulo the palette positions. */

void SendToBack(WindowPtr w)
{
	WindowPtr next, below;
	
	if (IsDocumentKind(w)) {
		below = w;
		
		/* Find the document below TopDocument. Documents are always visible */
		
		for (next=GetNextWindow(w); next; next=GetNextWindow(next))
			if (IsDocumentKind(next)) {
				below = next;
				break;
			}
		if (w != below) {
			SendBehind(w, NULL);		/* Send the document all the way to the back */
			if (TopPalette == TopWindow) {
				/* Ensure w and its controls are unhilited properly. Find the new
				   TopDocument and ensure the TopDocument and its controls are hilited
				   properly. */
				   
				ActivateDocument(GetDocumentFromWindow(w), False);
				AnalyzeWindows();
				ActivateDocument(GetDocumentFromWindow(TopDocument), True);
			}
		}
	} 
	else
		if (w!=BottomPalette && TopPalette!=BottomPalette)
			SendBehind(w, BottomPalette);	/* Move the palette behind the BottomPalette. */
}


/* Since SelectWindow() brings any window to the front, we use our own version to keep
palettes floating above documents. FIXME: if a palette is visible, this version erases
and redraws all of the new front window (PaintOne does this, I think), even if NO window
(including the palette) is overlapping any other! */

void DoSelectWindow(WindowPtr w)
{
	//RgnHandle updateRgn; short dx, dy;
	
	/* No Palettes, bring the document to the front and generate an activate event. */
		
	if (IsDocumentKind(w))
		SelectWindow(w);
	else
		if (IsDocumentKind(TopWindow))
			/* Bring the palette to the front but don't generate an activate event. */
			
			BringToFront(w);
		else {
			/* Bring the palette to the front and generate an activate event. */
			
			if (IsPaletteKind(TopWindow) && IsPaletteKind(w))
				BringToFront(w);
			 else
				SelectWindow(w);
		}
}


/* Turn all palettes on or off together. */

void ShowHidePalettes(Boolean show)
{
	short *pv, index;
	WindowPtr *wp;
	
	wp = palettes;
	pv = palettesVisible;				/* ??This seems to be uninitialized, yet it works! */

	for (index=0; index<TOTAL_PALETTES; index++, wp++, pv++) {
		if (*wp) {
			if (show) {
				//ShowHide(*wp, *pv);
				ShowHide(*wp, True);
				//*pv = False;
			}
			else {
//					*pv = (IsWindowVisible(*wp) != 0);
				if (*pv)
					ShowHide(*wp, False);
			}
		}

		if (index == TOOL_PALETTE) break;
	}
}


/* Update the given palette. */

void UpdatePalette(WindowPtr whichPalette)
{
	/* Use the palette's refCon to determine which palette needs updating */
	
	notInMenu = True;
	switch (GetWRefCon(whichPalette)) {
		case TOOL_PALETTE:
			Rect portRect;
			
			GetWindowPortBounds(whichPalette, &portRect);
			DrawToolPalette(&portRect);
			HiliteToolItem(&portRect, (*paletteGlobals[TOOL_PALETTE])->currentItem);
			break;
		case HELP_PALETTE:
		case CLAVIER_PALETTE:
			SysBeep(60);
			break;
	}
	notInMenu = False;
}


/* Invalidate the window. */
 
void InvalWindow(Document *doc)
{
	GrafPtr	oldPort, ourPort;
	
	ourPort = GetWindowPort(doc->theWindow);
	GetPort(&oldPort);
	SetPort(ourPort);
	MEHideCaret(doc);
	
	/* DrawDocument fills in the background and erases each sheet being updated first,
	   so there's no need to erase first. This reduces flash significantly. */
	
	InvalWindowRect(doc->theWindow, &doc->viewRect);
	SetPort(oldPort);
}


/* Erase and invalidate the message box. */

void EraseAndInvalMessageBox(Document *doc)
{
	WindowPtr w;
	Rect messageRect;
	Rect portRect;
	
	w = doc->theWindow;
	GetWindowPortBounds(w, &portRect);
	
	SetRect(&messageRect, portRect.left, portRect.bottom-SCROLLBAR_WIDTH,
					portRect.left+MESSAGEBOX_WIDTH-1, portRect.bottom);
	EraseAndInval(&messageRect);
}
