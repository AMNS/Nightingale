/* ResultListDocument.c for Nightingale Search, by Donald Byrd. Incorporates modeless-dialog
handling code from Apple sample source code by Nitin Ganatra. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "SearchScorePrivate.h"

#include <string.h>

static void RecomputeResListView(Document * /* doc */)
	{
	}
	
Boolean BuildResListDocument(register Document *doc)
	{
		int fontSize = 10;
		
		doc->origin.h = 0;
		doc->origin.v = 0;
		return BuildDocList(doc, fontSize);	 
	}
	
void ActivateResListDocument(register Document *doc, INT16 activ)
	{
		Point pt;  GrafPtr oldPort;
		Rect portRect;
		
		if (doc) {
			WindowPtr w = doc->theWindow; 
			GetPort(&oldPort);
			
			HiliteWindow(w, activ);
			
			GetWindowPortBounds(w, &portRect);
			SetPort(GetWindowPort(w));
			SetOrigin(doc->origin.h, doc->origin.v);
			ClipRect(&portRect);
			
			//if (doc->active = (activ!=0)) {
			//	HiliteControl(doc->hScroll,CTL_ACTIVE);
			//	HiliteControl(doc->vScroll,CTL_ACTIVE);
			//	}
			// else {
			//	HiliteControl(doc->hScroll,CTL_INACTIVE);
			//	HiliteControl(doc->vScroll,CTL_INACTIVE);
			//	}
			
			HandleResListActivate(activ);
				
			pt = TOP_LEFT(doc->growRect);
			DrawGrowIcon(w);
			ClipRect(&doc->viewRect);
			
			if (activ) {
				}
			 else
				SetPort(oldPort);
			}
	}
	
Boolean DoCloseResListDocument(register Document *doc)
	{
		Boolean keepGoing = True;
		
		if (doc)
			if (doc == gResListDocument) HideWindow(doc->theWindow);
		
		return(keepGoing);
	}
	
/*
 *	This routine is called after preparing another document to be shown.
 */

void ShowResListDocument(Document *doc)
	{
		RecomputeResListView(doc);
		if (TopPalette) {
			if (TopPalette != TopWindow)
				/* Bring the TopPalette to the front and generate activate event. */
				SelectWindow(TopPalette);
			else {
				/* There won't be an activate event for doc, so do it here */
				
				ShowWindow(doc->theWindow);
				HiliteWindow(doc->theWindow, True);
				ActivateResListDocument(doc, True);
				}
			if (TopDocument)
				/* Ensure the TopDocument and its controls are unhilited properly. */
				
				ActivateResListDocument(GetDocumentFromWindow(TopDocument), False);
			}
		ShowWindow(doc->theWindow);
		SelectWindow(doc->theWindow);
	}
	
static void ShowResultListDocument()
	{
	 	if (BottomPalette != BRING_TO_FRONT)
	 		SendBehind(gResListDocument->theWindow, BottomPalette);
	 	 else
	 		BringToFront(gResListDocument->theWindow);
		ShowDocument(gResListDocument);		
	}

Boolean DoOpenResListDocument(Document **pDoc)
	{
		WindowPtr w;  Document *doc;

LogPrintf(LOG_DEBUG, "DoOpenResListDocument1: gResListDocument=%x\n", gResListDocument);
		*pDoc = NULL;
		
		/* If an existing document and already open, then just bring it to front */
		
		if (gResListDocument) {
			if (gResListDocument) {
				ShowResultListDocument();
				*pDoc = gResListDocument;
				return(True);
				}
			}
		
		/* Otherwise, open document */
		
		doc = FirstFreeDocument();
		if (doc == NULL) { TooManyDocs();  *pDoc = NULL;  return(True); }
		
		gResListDocument = doc;
		gResListDocument->inUse = True;
		gResListDocument->readOnly = True;
		
		w = GetNewWindow(docWindowID, NULL, (WindowPtr)-1);
		if (w == NULL) return(True);
		gResListDocument->theWindow = w;		
		SetWindowKind(w, DOCUMENTKIND);
		
		SetWCTitle(w,"Result List");
		Pstrcpy(gResListDocument->name,"\pResult List");
		
		BuildEmptyDoc(gResListDocument);

		SetPort(GetWindowPort(w));
LogPrintf(LOG_DEBUG, "DoOpenResListDocument2: gResListDocument=%x\n", gResListDocument);
		BuildResListDocument(gResListDocument);
		ShowResultListDocument();
		*pDoc = gResListDocument;
		return(w != NULL);
	}
