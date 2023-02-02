/* ResultListDocument.c for Nightingale Search, by Donald Byrd. Incorporates modeless-dialog
handling code from Apple sample source code by Nitin Ganatra. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "SearchScorePrivate.h"

#include <string.h>

static void RecomputeResListView(Document * /* doc */)
{
}
	

/* Set up the result-list document <gResultListDoc>. It's nothing like, and far simpler,
than a normal Nightingale document -- it contains nothing but lines of text -- so
building it requires very little work. */

Boolean BuildResListDocument(Document *doc)
{
	int fontSize = 10;
	
	doc->origin.h = 0;
	doc->origin.v = 0;

	return BuildDocList(doc, fontSize);	 
}
	
void ActivateResListDocument(Document *doc, INT16 activ)
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
		
		HandleResultListActivate(activ);
			
		pt = TOP_LEFT(doc->growRect);
		DrawGrowIcon(w);
		ClipRect(&doc->viewRect);
		
		if (!activ) SetPort(oldPort);
	}
}
	
Boolean DoCloseResListDocument(Document *doc)
{
	Boolean keepGoing = True;
	
	if (doc)
		if (doc == gResultListDoc) HideWindow(doc->theWindow);
	
	return(keepGoing);
}
	
/* This routine should be called after preparing another document to be shown. */

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
		SendBehind(gResultListDoc->theWindow, BottomPalette);
	 else
		BringToFront(gResultListDoc->theWindow);
	ShowDocument(gResultListDoc);		
}

Boolean DoOpenResListDocument(Document **pDoc)
{
	WindowPtr w;  Document *doc;

LogPrintf(LOG_DEBUG, "DoOpenResListDocument1: gResultListDoc=%x\n", gResultListDoc);
	*pDoc = NULL;
	
	/* If an existing document and already open, just bring it to front */
	
	if (gResultListDoc) {
		ShowResultListDocument();
		*pDoc = gResultListDoc;
		return(True);
	}
	
	/* Otherwise, open document */
	
	doc = FirstFreeDocument();
	if (doc == NULL) { TooManyDocs();  *pDoc = NULL;  return(True); }
	
	gResultListDoc = doc;
	gResultListDoc->inUse = True;
	gResultListDoc->readOnly = True;
	
	w = GetNewWindow(docWindowID, NULL, (WindowPtr)-1);
	if (w == NULL) return(True);
	gResultListDoc->theWindow = w;		
	SetWindowKind(w, DOCUMENTKIND);
	
	SetWCTitle(w, "Result List");
	Pstrcpy(gResultListDoc->name, "\pResult List");
	
	BuildEmptyDoc(gResultListDoc);

	SetPort(GetWindowPort(w));
LogPrintf(LOG_DEBUG, "DoOpenResListDocument2: gResultListDoc=%x\n", gResultListDoc);
	BuildResListDocument(gResultListDoc);
	ShowResultListDocument();
	*pDoc = gResultListDoc;
	return(w != NULL);
}
