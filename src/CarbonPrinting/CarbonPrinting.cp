/*
	File:		CarbonPrinting.c for Nightingale
	Contains:	Routines needed for printing. This example uses sheets and provides for
				save as PDF. Based on Apple's MyCarbonPrinting.c.
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "UIFUtils.h"
#include "CarbonPrinting.h"

enum {
  kPMNInvalidPageRange = -30901,
  kPMNSysJustifUserCancel = -30902  
};

/* --------------------------------------------------------------------------- Globals -- */
	
static PMSheetDoneUPP gMyPageSetupDoneProc = NULL;
static PMSheetDoneUPP gMyPrintDialogDoneProc = NULL;

FONTUSEDITEM	fontUsedTbl[MAX_SCOREFONTS];

static OSStatus DocCreatePrintSessionProcs(PMSheetDoneUPP *pageSetupDoneUPP, PMSheetDoneUPP *printDialogDoneUPP);
static OSStatus DocSetupPageFormat(Document *doc);
static pascal void NPageSetupDoneProc(PMPrintSession printSession, WindowRef docWindow, Boolean accepted);
static pascal void NPrintDialogDoneProc(PMPrintSession printSession, WindowRef docWindow, Boolean accepted);
static OSStatus SetupPrintDlogPages(Document *doc);
static OSStatus GetPrintPageRange(Document *doc, UInt32 *firstPage, UInt32 *lastPage);
static OSStatus SetPrintPageRange(Document *doc, UInt32 firstPage, UInt32 lastPage);
static Boolean IsRightJustOK(Document *doc, short firstSheet, short lastSheet);
static Boolean IncludePostScriptInSpoolFile(PMPrintSession printSession);
static short GetPrintDestination(Document *doc);
static void NDoPrintLoop(Document *doc);
static OSStatus PrintBitmap(Document *doc, UInt32 firstSheet, UInt32 lastSheet);
static OSStatus PrintPostScript(Document *doc, UInt32 firstSheet, UInt32 lastSheet);
static OSStatus DocFormatGetAdjustedPaperRect(Document *doc, Rect *paperRect, Boolean xlateScale);
static OSStatus NDocDrawPage(Document *doc, UInt32 pageNum);
static Boolean DocSessionOK(Document *doc, OSStatus status);
static void DocReleasePrintSession(Document *doc);
static void DocReleasePageFormat(Document *doc);
static void DocReleasePrintSettings(Document *doc);
static void DocSPFReleasePrintSession(Document *doc,PMPrintSession printSession,PMPageFormat pageFormat);
static void DocPostPrintingError(Document *doc,OSStatus status, CFStringRef errorFormatStringKey);

static void FillFontUsedTbl(Document *doc);
static short PSTypeDialog(short, short);


/* -------------------------------------------------------------------------------------- */

static OSStatus DocCreatePrintSessionProcs(PMSheetDoneUPP *pageSetupDoneUPP,
											PMSheetDoneUPP *printDialogDoneUPP)
{
	OSStatus err = noErr;

	if (gMyPageSetupDoneProc == NULL) {
		gMyPageSetupDoneProc = NewPMSheetDoneUPP(NPageSetupDoneProc);
		
		if (gMyPageSetupDoneProc == NULL) err = memFullErr;
	}
	
	if (err == noErr) {
		*pageSetupDoneUPP = gMyPageSetupDoneProc;
		
		gMyPrintDialogDoneProc = NewPMSheetDoneUPP(NPrintDialogDoneProc);
		if (gMyPrintDialogDoneProc == NULL) err = memFullErr;
		*printDialogDoneUPP = gMyPrintDialogDoneProc;
	}
	
	if (err != noErr) {
		*pageSetupDoneUPP = NULL;
		*printDialogDoneUPP = NULL;
	}

	return err;
}


/* -------------------------------------------------------------------------------------- */
/* Create DocPrintInfo record for the document. */

Boolean IsDocPrintInfoInstalled(Document *doc)
{
	return (doc->docPrintInfo.docPageSetupDoneUPP != NULL);
}

/* -------------------------------------------------------------------------------------- */
/* Create DocPrintInfo record for the document. */

OSStatus InstallDocPrintInfo(Document *doc)
{
	PMSheetDoneUPP	pageSetupDoneUPP;
	PMSheetDoneUPP	printDialogDoneUPP;
	OSStatus		err = noErr;
	
	doc->docPrintInfo.docPrintSession = NULL;
	doc->docPrintInfo.docPageFormat = kPMNoPageFormat;
	doc->docPrintInfo.docPrintSettings = kPMNoPrintSettings;
	
	err = DocCreatePrintSessionProcs(&pageSetupDoneUPP, &printDialogDoneUPP);
	
	if (err == noErr) {
		doc->docPrintInfo.docPageSetupDoneUPP = pageSetupDoneUPP;
		doc->docPrintInfo.docPrintDialogDoneUPP = printDialogDoneUPP;
	}
	else {
		doc->docPrintInfo.docPageSetupDoneUPP = NULL;
		doc->docPrintInfo.docPrintDialogDoneUPP = NULL;
	}
	
	return err;
}

/* -------------------------------------------------------------------------------------- */

static OSStatus DocSetupPageFormat(Document *doc)
{
	OSStatus status = noErr;

	PMPrintSession printSession = NULL;
	PMPageFormat pageFormat = kPMNoPageFormat;
	
	if (!IsDocPrintInfoInstalled(doc)) InstallDocPrintInfo(doc);
	
	status = PMCreateSession(&printSession);
	if (status != noErr) return status;
		
	if (doc->docPrintInfo.docPageFormat == kPMNoPageFormat) {
		status = PMCreatePageFormat(&pageFormat);
		if (status == noErr && pageFormat != kPMNoPageFormat) {
			status = PMSessionDefaultPageFormat(printSession, pageFormat);
			if (status == noErr) doc->docPrintInfo.docPageFormat = pageFormat;
			
		}
		else if (status == noErr) status = kPMGeneralError;
	}
	else {
		status = PMSessionValidatePageFormat(printSession,
					doc->docPrintInfo.docPageFormat, kPMDontWantBoolean);
	}
	
	if (status == noErr) {
		doc->docPrintInfo.docPrintSession = printSession;
	}
	else {
		DocSPFReleasePrintSession(doc,printSession,pageFormat);
	}
	
	return status;
}


/* -------------------------------------------------------------------------------------- */

static pascal void NPageSetupDoneProc(PMPrintSession printSession,
										WindowRef docWindow,
										Boolean accepted)
{
	Document *doc = GetDocumentFromWindow(docWindow);
	OSStatus status = noErr;
	
	if (doc!=NULL && doc->docPrintInfo.docPageFormat!=kPMNoPageFormat && accepted) {
	
		/* If doc->paperRect is different from rPaper (user has chosen a different size
		   paper), we change to new size and inval everything. */
		
		PMRect pmRPaper;
		Rect rPaper;
		PMOrientation pmOrientation;
		double scale;
		
		status = PMGetAdjustedPaperRect(doc->docPrintInfo.docPageFormat, &pmRPaper);
		
		if (status==noErr)
			status = PMGetScale(doc->docPrintInfo.docPageFormat, &scale);

		if (status==noErr) {
			scale /= 100.0;
			pmRPaper.top *= scale;
			pmRPaper.left *= scale;
			pmRPaper.bottom *= scale;
			pmRPaper.right *= scale;
			
			rPaper.top = pmRPaper.top;
			rPaper.left = pmRPaper.left;
			rPaper.bottom = pmRPaper.bottom;
			rPaper.right = pmRPaper.right;
			
			OffsetRect(&rPaper, -rPaper.left, -rPaper.top);
			LogPrintf(LOG_INFO, "NPageSetupDoneProc: origPaperRect= %d,%d,%d,%d\n", doc->origPaperRect.left,
						doc->origPaperRect.top, doc->origPaperRect.right, doc->origPaperRect.bottom);
			LogPrintf(LOG_INFO, "                    rPaper=        %d,%d,%d,%d\n", rPaper.left,
						rPaper.top, rPaper.right, rPaper.bottom);
			if (!EqualRect(&doc->origPaperRect, &rPaper)) {
				short marginRight = doc->origPaperRect.right - doc->marginRect.right;
				short marginBottom = doc->origPaperRect.bottom - doc->marginRect.bottom;
				doc->marginRect.right = rPaper.right - marginRight;
				doc->marginRect.bottom = rPaper.bottom - marginBottom;
				doc->origPaperRect = rPaper;
				MagnifyPaper(&doc->origPaperRect, &doc->paperRect, doc->magnify);
				doc->changed = True;
				InvalSheetView(doc);
			}
		}		

		status = PMGetOrientation(doc->docPrintInfo.docPageFormat, &pmOrientation);
		if (status==noErr) {
			SignedByte oldLandscape = doc->landscape;
	
			doc->landscape = (pmOrientation==kPMLandscape || pmOrientation==kPMReverseLandscape);
			if (doc->landscape != oldLandscape) doc->changed = True;
		}
	}
	
	if (doc != NULL && printSession==doc->docPrintInfo.docPrintSession) {
		DocReleasePrintSession(doc);
	}
	else {
    	status = PMRelease(printSession);	
	}

	if (status != noErr) {
		CFStringRef cfFormatStr = CFSTR("Page Setup Dialog Done error. Error number: %d.");
		DocPostPrintingError(doc, status, cfFormatStr);
	}
}


/* -------------------------------------------------------------------------------------- */

void DoPageSetup(Document *doc)
{
	WindowRef w = doc->theWindow;
	Boolean accepted = False;
	
	OSStatus status = DocSetupPageFormat(doc);
	if (status==noErr) {
		PMSessionUseSheets(doc->docPrintInfo.docPrintSession,  w,
							doc->docPrintInfo.docPageSetupDoneUPP);
		status = PMSessionPageSetupDialog(doc->docPrintInfo.docPrintSession,
											doc->docPrintInfo.docPageFormat,
											&accepted);
	}
	if (status!=noErr) {
		DocReleasePrintSession(doc);
		CFStringRef cfFormatStr = CFSTR("Page Setup Dialog error. Error number: %d.");
		DocPostPrintingError(doc, status, cfFormatStr);
	}	
}


/* -------------------------------------------------------------------------------------- */
/* Actually do the printing. */

static pascal void NPrintDialogDoneProc(PMPrintSession /*printSession*/,
											WindowRef docWindow,
											Boolean accepted)
{
	Document *doc = GetDocumentFromWindow(docWindow);
	
	if (doc != NULL)
		if (accepted) NDoPrintLoop(doc);
}


/* -------------------------------------------------------------------------------------- */

void ShowOutputDevice()
{
	if (outputTo==toBitmapPrint) LogPrintf(LOG_INFO, "device %d (BitMap (QuickDraw) printer)\n",
			outputTo);
	else if (outputTo==toPostScript) LogPrintf(LOG_INFO, "device %d (PostScript)\n", outputTo);
	else LogPrintf(LOG_INFO, "device %d (unknown device)\n", outputTo);
}


/* -------------------------------------------------------------------------------------- */
/* Despite its name, DoPrintScore doesn't actually do any printing; it handles interaction
with the user and sets things up for printing. */

void DoPrintScore(Document *doc)
{
	OSStatus status = DocSetupPageFormat(doc);
	PMPrintSettings printSettings;
	Boolean accepted;
	CFStringRef wTitleRef;
	
	if (status == noErr) {
		status = PMCreatePrintSettings(&printSettings);
	}
	
	if (status == noErr && printSettings != kPMNoPrintSettings) {	
		status = CopyWindowTitleAsCFString(doc->theWindow, &wTitleRef);		
	}
	
	if (status == noErr) {	
		status = PMSetJobNameCFString(printSettings, wTitleRef);
		CFRelease(wTitleRef);
	}
	
	if (status == noErr) {
		status = PMSessionDefaultPrintSettings(doc->docPrintInfo.docPrintSession, printSettings);
	}
	
	if (status == noErr) {
		doc->docPrintInfo.docPrintSettings = printSettings;
		printSettings = kPMNoPrintSettings;
		
		status = SetupPrintDlogPages(doc);
	}
	
	if (status == noErr) {
		PMSessionUseSheets(doc->docPrintInfo.docPrintSession,
							doc->theWindow,
							doc->docPrintInfo.docPrintDialogDoneUPP);
		
		status = PMSessionPrintDialog(doc->docPrintInfo.docPrintSession,
										doc->docPrintInfo.docPrintSettings,
										doc->docPrintInfo.docPageFormat,
										&accepted);
		LogPrintf(LOG_INFO, "Preparing to print via ");
		ShowOutputDevice();
	}
	
	if (status != noErr) {
		if (printSettings != kPMNoPrintSettings) PMRelease(printSettings);
			
		DocReleasePrintSettings(doc);
		DocReleasePrintSession(doc);
		
		CFStringRef cfFormatStr = CFSTR("Print Dialog error. Error number: %d.");
		
		DocPostPrintingError(doc, status, cfFormatStr);
	}
}


/* -------------------------------------------------------------------------------------- */

static OSStatus SetupPrintDlogPages(Document *doc)
{
	OSStatus status = noErr;
	
	short firstPage = doc->firstPageNumber;
	short lastPage = doc->firstPageNumber + doc->numSheets - 1;
	
	status = PMSetPageRange(doc->docPrintInfo.docPrintSettings, firstPage, lastPage);
	
	return status;
}


/* -------------------------------------------------------------------------------------- */

static OSStatus GetPrintPageRange(Document *doc, UInt32 *firstPage, UInt32 *lastPage)
{
	OSStatus status = PMGetFirstPage(doc->docPrintInfo.docPrintSettings, firstPage);
	
	if (status == noErr)
		status = PMGetLastPage(doc->docPrintInfo.docPrintSettings, lastPage);
	
	return status;
}


/* -------------------------------------------------------------------------------------- */

static OSStatus SetPrintPageRange(Document *doc, UInt32 firstPage, UInt32 lastPage)
{
	OSStatus status = PMSetFirstPage(doc->docPrintInfo.docPrintSettings, firstPage, False);

	if (status == noErr)
		status = PMSetLastPage(doc->docPrintInfo.docPrintSettings, lastPage, False);
				
	return status;
}


/* -------------------------------------------------------------------------------------- */
/* Check that systems in a range of pages are all justified. If SysJustFact() is
significantly different from 1.0, tell user and give them a chance to cancel. Return
False if user says stop or there's an error, True otherwise (all justified, or user
sqys continue anyway). (It'd be nice some day to give them an opportunity to justify
right now.) */

static Boolean IsRightJustOK(Document *doc, 
								short firstSheet, short lastSheet)	/* Inclusive range of sheet numbers */
{
	LINK startPageL, endPageL;
	short firstUnjustPg;
	
	startPageL = LSSearch(doc->headL, PAGEtype, firstSheet, GO_RIGHT, False);
	endPageL = LSSearch(doc->headL, PAGEtype, lastSheet+1, GO_RIGHT, False);
	if (!endPageL) endPageL = doc->tailL;
	if (!startPageL) {
		MayErrMsg("IsRightJustOK: can't find startPageL.");
		return False;
	}
	
	if (CountUnjustifiedSystems(doc, startPageL, endPageL, &firstUnjustPg)>0)
		return (CautionAdvise(NOTJUST_ALRT)!=Cancel);
	return True;
}


/* -------------------------------------------------------------------------------------- */

static OSStatus SetupPagesToPrint(Document *doc, UInt32 *firstPg, UInt32 *lastPg)
{
	OSStatus status = noErr;
	UInt32	firstPage, lastPage;
	UInt32	firstSheet, lastSheet;
	
	status = GetPrintPageRange(doc, &firstPage, &lastPage);
	
	if (status == noErr) {
		firstSheet = firstPage - doc->firstPageNumber;
		lastSheet  = lastPage - doc->firstPageNumber;
		if (firstSheet > lastSheet)
			{ short sh = firstSheet; firstSheet = lastSheet; lastSheet = sh; }
		if (firstSheet < 0) firstSheet = 0;
		if (lastSheet >= (unsigned)doc->numSheets) lastSheet = doc->numSheets-1;

		/* If the page numbers aren't in forward order now, they don't exist. */

		if (firstSheet > lastSheet) status = kPMNInvalidPageRange;
		
		/* Check that systems to be printed are all justified; if not, tell user and
		   give them a chance to cancel. */
		   
		if (config.printJustWarn)
			if (!IsRightJustOK(doc, firstSheet, lastSheet)) {
				status = kPMNSysJustifUserCancel;
			}

		if (status == noErr) {
			firstPage = firstSheet + doc->firstPageNumber;
			lastPage = lastSheet + doc->firstPageNumber;
			status = SetPrintPageRange(doc, firstPage, lastPage);

			if (status == noErr) {
				*firstPg = firstPage;
				*lastPg = lastPage;
			}
		}
			
	}
	
	return status;
}


/* ------------------------------------------------------------------------------
 * Function:	IncludePostScriptInSpoolFile
 * Parameters:
 *		printSession - current printing session
 * Description:
		Check if current printer driver supports embedding of PostScript in the spool
		file, and if it does, instruct the Carbon Printing Manager to generate a PICT
		with PS spool file. */              		

static Boolean IncludePostScriptInSpoolFile(PMPrintSession printSession)
{
	Boolean		includePostScript = False;
	OSStatus	status;
	CFArrayRef	supportedFormats = NULL;
	SInt32		i, numSupportedFormats;

	/* Get the list of spool file formats supported by the current driver.
	   PMSessionGetDocumentFormatGeneration returns the list of formats which can be
	   generated by the spooler (client-side) AND converted by the despooler (server-side).
	   PMSessionGetDocumentFormatSupported only returns the list of formats which can be
	   converted by the despooler. */

	status = PMSessionGetDocumentFormatGeneration(printSession, &supportedFormats);
	if (status == noErr) {
		/* Check if PICT with PS is in the list of supported formats. */
		
		numSupportedFormats = CFArrayGetCount(supportedFormats);

		for (i=0; i < numSupportedFormats; i++) {
			if ( CFStringCompare((CFStringRef)CFArrayGetValueAtIndex(supportedFormats, i),
									kPMDocumentFormatPICTPS, kCFCompareCaseInsensitive) == kCFCompareEqualTo ) {

				/* PICT with PS is supported, so tell the Printing Mgr to generate a PICT
				   with PS spool file.

				   Build an array of graphics contexts containing just one type, QuickDraw,
				   meaning that we will be using a QD port to image our pages in the print
				   loop. */
				   
				CFStringRef	strings[1];
				CFArrayRef arrayOfGraphicsContexts;

				strings[0] = kPMGraphicsContextQuickdraw;
				arrayOfGraphicsContexts = CFArrayCreate(kCFAllocatorDefault,
														(const void **)strings, 1, &kCFTypeArrayCallBacks);

				if (arrayOfGraphicsContexts != NULL) {
					/* Request a PICT with PS spool file */
					
					status = PMSessionSetDocumentFormatGeneration(printSession, kPMDocumentFormatPICTPS, 
																	arrayOfGraphicsContexts, NULL);

					if (status == noErr)
						includePostScript = True;	/* Allow use of PS PicComments in DrawPage */

					/* Deallocate array used for the list of graphics contexts. */
					
					CFRelease(arrayOfGraphicsContexts);
				}
				break;
			}
		}

		/* Deallocate array used for the list of supported spool file formats. */
		
		CFRelease(supportedFormats);
	}

	return includePostScript;
}


/* -------------------------------------------------------------------------------------- */

/* Decide whether to do PostScript or bitmap printing. We do this based on "whether the
current printer driver supports embedding of PostScript in the spool file" (cf.
IncludePostScriptInSpoolFile). FIXME: This code dates from ca. 2001. As of this writing
(2016), surely PostScript is almost always what the user wants, since it'll be supported
somehow; but in my hasty test, just insisting on PostScript here produced a blank page,
so this needs more work. --DAB */

static short GetPrintDestination(Document *doc)
{
	Boolean includePostScript = IncludePostScriptInSpoolFile(doc->docPrintInfo.docPrintSession);
	if (includePostScript) return toPostScript;
		
	return toBitmapPrint;
}


/* -------------------------------------------------------------------------------------- */

static void NDoPrintLoop(Document *doc)
{
	short saveOutputTo;
	OSStatus status;
	UInt32	firstPage, lastPage;
	UInt32	firstSheet, lastSheet;
	UInt32	copies;
	Rect imageRect;
	
	saveOutputTo = outputTo;
	
	status = SetupPagesToPrint(doc, &firstPage, &lastPage);
	require(status==noErr,displayError);
	
	firstSheet = firstPage - doc->firstPageNumber;
	lastSheet = lastPage - doc->firstPageNumber;

	if (status == noErr) status = PMGetCopies(doc->docPrintInfo.docPrintSettings, &copies);	
	require(status==noErr, displayError);
	
	UpdateAllWindows();
	
	status = DocFormatGetAdjustedPaperRect(doc, &imageRect, True);
	require(status==noErr, displayError);
	
	if (status == noErr) PS_PreparePrintDict(doc, &imageRect);		/* Create 'PREC' 103 */
		
	if (status == noErr) outputTo = GetPrintDestination(doc);
	require(status==noErr, displayError);
	
	if (status == noErr) {
		switch (outputTo) {
			case toBitmapPrint:
				status = PrintBitmap(doc, firstSheet, lastSheet);
				if (status != noErr) copies = -1;
				break;
			case toPostScript:
				status = PrintPostScript(doc, firstSheet, lastSheet);
				if (status != noErr) copies = -1;
				break;
		}
	}
	
	outputTo = saveOutputTo;
	
displayError:

	PS_FinishPrintDict();
	
	if (status != noErr && status != kPMNSysJustifUserCancel) {
		CFStringRef cfFormatStr = CFSTR("Print loop error. Error number: %d.");
		
		DocPostPrintingError(doc, status, cfFormatStr);
	}
                                                            
	/* Release the PrintSession and PrintSettings objects.  PMRelease decrements the
	   ref count of the allocated objects.  We let the Printing Manager decide when
	   to release the allocated memory. PageFormat persists between sessions. */
	
	//if (doc->docPrintInfo.docPageFormat != kPMNoPageFormat) {
	//	PMRelease(doc->docPrintInfo.docPageFormat);
	//	doc->docPrintInfo.docPageFormat = kPMNoPageFormat;
	//}
	if (doc->docPrintInfo.docPrintSettings != kPMNoPrintSettings) {
		PMRelease(doc->docPrintInfo.docPrintSettings);
		doc->docPrintInfo.docPrintSettings = kPMNoPrintSettings;
	}

	/* Terminate the current printing session. */
	
	PMRelease(doc->docPrintInfo.docPrintSession);
	doc->docPrintInfo.docPrintSession = NULL;
}


/* -------------------------------------------------------------------------------------- */

static OSStatus PrintBitmap(Document *doc, UInt32 firstSheet, UInt32 lastSheet)
{
	OSStatus err = noErr, status = noErr;
	UInt32 sheetNum;
	GrafPtr	currPort, printPort;
	
	status = PMSessionBeginDocument(doc->docPrintInfo.docPrintSession,
										doc->docPrintInfo.docPrintSettings,
										doc->docPrintInfo.docPageFormat);
												
	if (status == noErr) {
		sheetNum = firstSheet;
		
		while (sheetNum <= lastSheet && DocSessionOK(doc, status)) {
			status = PMSessionBeginPage(doc->docPrintInfo.docPrintSession,
											doc->docPrintInfo.docPageFormat,
											NULL);
													
			if (status == noErr) {
				GetPort(&currPort);
				status = PMSessionGetGraphicsContext(doc->docPrintInfo.docPrintSession,
														kPMGraphicsContextQuickdraw,
														(void**)&printPort);					
				if (status == noErr) {
					SetPort(printPort);
					status = NDocDrawPage(doc,sheetNum);
					SetPort(currPort);
				}
				
				err = PMSessionEndPage(doc->docPrintInfo.docPrintSession);
				if (status == noErr)
					status = err;
					
				sheetNum++;
			}
		}
	
		err = PMSessionEndDocument(doc->docPrintInfo.docPrintSession);
		if (status == noErr)
			status = err;
	}
	
	return status;
}


/* -------------------------------------------------------------------------------------- */

static OSStatus PrintPostScript(Document *doc, UInt32 firstSheet, UInt32 lastSheet)
{
	OSStatus	err = noErr, status = noErr;
	UInt32		sheet;
//	Rect		printRect, frameRect;
	GrafPtr		currPort, printPort;
	Rect		imageRect, paper;
	FSSpec		fsSpec;
	
	status = PMSessionBeginDocument(doc->docPrintInfo.docPrintSession,
									doc->docPrintInfo.docPrintSettings,
									doc->docPrintInfo.docPageFormat);
												
	if (status == noErr) {
	
		/* Get physical size of paper (topleft should be (0,0)) and imageable area
		   (topleft should be something like (-31,-30)). */
		   
		//printRect = (**hPrint).prInfo.rPage;
		//imageRect = (*hPrint)->rPaper;		
		status = DocFormatGetAdjustedPaperRect(doc, &imageRect, True);

		paper = doc->paperRect;
		
		if (status == noErr && PS_Open(doc,NULL,0,USING_PRINTER,'????',&fsSpec) == noErr) {

			for (sheet=firstSheet; sheet<=lastSheet && DocSessionOK(doc,status); sheet++) {
			
				status = PMSessionBeginPage(doc->docPrintInfo.docPrintSession,
												doc->docPrintInfo.docPageFormat,
												NULL);
														
				if (status == noErr) {
					GetPort(&currPort);
					
					status = PMSessionGetGraphicsContext(doc->docPrintInfo.docPrintSession,
															kPMGraphicsContextQuickdraw,
															(void**)&printPort);					
					if (status == noErr) {
						SetPort(printPort);
						
						/*
						 *	The general paradigm for printing our own PostScript is to send
						 *	all of it through PicComments until it's time to change fonts.
						 *	When that happens, we get out of PicComment mode, and use
						 *	QuickDraw calls to change the font.  The calls are converted to
						 *	the proper LaserPrep PostScript incantations, including possibly
						 *	downloading the font if we draw a single space character.
						 *	Then we get back into PicComment mode and continue drawing.
						 *
						 *	At this level, however, we know none of this.  As far as we're
						 *	concerned here, we immediately go into PicComment mode, stay in
						 *	in it throughout the call to DrawPageContent(), and then go back
						 *	to LaserPrep land for the end of the page.  Other routines in
						 *	PS_Stdio will swap us in and out at the necessary times, but
						 *	always do it in matching pairs.  (See PS_FontRunAround.)
						 *
						 *	We pass imageRect to PS_OutOfQD because unfortunately changing
						 *	fonts causes all of our private PostScript header definitions to
						 *	be thrown away, so we must recreate them every time, and imageRect
						 *	is needed for defining the transformation matrix we're using.
						 */
						PS_NewPage(doc, NULL, sheet+doc->firstPageNumber);
						
						PS_OutOfQD(doc, True, &imageRect);
						//PrintDemoBanner(doc, True);
						DrawPageContent(doc, sheet, &paper, &doc->currentPaper);
//						PS_Print("showpage\r");
						PS_IntoQD(doc, True);
						
						PS_EndPage();
						SetPort(currPort);
					}
					
					err = PMSessionEndPage(doc->docPrintInfo.docPrintSession);
					if (status == noErr)
						status = err;
				}
			}
			
			PS_Close();
		}
		
		status = PMSessionEndDocument(doc->docPrintInfo.docPrintSession);
	}
	
	return status;
}

/* -------------------------------------------------------------------------------------- */

static OSStatus NDocDrawPage(Document *doc, UInt32 sheetNum)
{
	Rect paper;
	
	paper = doc->paperRect;
	DrawPageContent(doc, sheetNum, &paper, &doc->viewRect);
	
	return noErr;
}


/* -------------------------------------------------------------------------------------- */

static OSStatus DocFormatGetAdjustedPaperRect(Document *doc, Rect *rPaper, Boolean xlateScale)
{
	PMRect pmRPaper;
	double scale;
	
	OSStatus status = PMGetAdjustedPaperRect(doc->docPrintInfo.docPageFormat, &pmRPaper);
	
	if (xlateScale) {
		if (status == noErr)
			status = PMGetScale(doc->docPrintInfo.docPageFormat, &scale);
	}
	else {
		scale = 100.0;
	}
	
	if (status == noErr)
	{
		scale /= 100.0;
		pmRPaper.top *= scale;
		pmRPaper.left *= scale;
		pmRPaper.bottom *= scale;
		pmRPaper.right *= scale;
		
		rPaper->top = pmRPaper.top;
		rPaper->left = pmRPaper.left;
		rPaper->bottom = pmRPaper.bottom;
		rPaper->right = pmRPaper.right;
	}
	
	return status;
}


/* -------------------------------------------------------------------------------------- */

static Boolean DocSessionOK(Document *doc, OSStatus status)
{
	if (status != noErr) return False;
	if (doc->docPrintInfo.docPrintSession == NULL) return False;
	
	return (PMSessionError(doc->docPrintInfo.docPrintSession) == noErr);
}


/* -------------------------------------------------------------------------------------- */

static void DocReleasePrintSession(Document *doc)
{
	if (doc->docPrintInfo.docPrintSession != NULL) {
		PMRelease(doc->docPrintInfo.docPrintSession);
		doc->docPrintInfo.docPrintSession = NULL;
	}
}


static void DocReleasePageFormat(Document *doc)
{
	if (doc->docPrintInfo.docPageFormat != kPMNoPageFormat) {
		PMRelease(doc->docPrintInfo.docPageFormat);
		doc->docPrintInfo.docPageFormat = kPMNoPageFormat;
	}
}


static void DocReleasePrintSettings(Document *doc)
{
	if (doc->docPrintInfo.docPrintSettings != kPMNoPrintSettings) {
		PMRelease(doc->docPrintInfo.docPrintSettings);
		doc->docPrintInfo.docPrintSettings = kPMNoPrintSettings;
	}
}


/* -------------------------------------------------------------------------------------- */

static void DocSPFReleasePrintSession(Document *doc, PMPrintSession printSession,
										PMPageFormat pageFormat)
{
	if (doc->docPrintInfo.docPageFormat != pageFormat) {
		if (pageFormat != kPMNoPageFormat)
			PMRelease(pageFormat);
	}

	DocReleasePageFormat(doc);

	if (doc->docPrintInfo.docPrintSession != printSession) {
		if (printSession != NULL)
			PMRelease(printSession);
	}

	DocReleasePrintSession(doc);
}


/*------------------------------------------------------------------------------
Function:
	FlattenAndSavePageFormat
Parameters:
	pageFormat	-	a PageFormat object
Description:
	Flattens a PageFormat object so it can be saved with the document.
	Assumes caller passes a validated PageFormat object.
		
----------------------------------------------------------------------------------------- */

OSStatus FlattenAndSavePageFormat(Document *doc)
{
	OSStatus status;
	Handle flatFormatHandle = NULL;

	//	Flatten the PageFormat object to memory.
	status = PMFlattenPageFormat(doc->docPrintInfo.docPageFormat, &flatFormatHandle);

	if (status == noErr) {
		//	Write the PageFormat data to file.
		//	In this sample code we simply copy it to a global.	
		doc->flatFormatHandle = flatFormatHandle;
	}

	return status;
}


/*--------------------------------------------------------------------------------------
Function:	LoadAndUnflattenPageFormat
Parameters:
	pageFormat	- PageFormat object read from document file
Description:
	Gets flattened PageFormat data from the document and returns a PageFormat
	object.
	The function is not called in this sample code but your application
	will need to retrieve PageFormat data saved with documents.
		
----------------------------------------------------------------------------------------- */

OSStatus LoadAndUnflattenPageFormat(Document *doc)
{
	OSStatus status = noErr;
	Handle flatFormatHandle = NULL;
	PMPageFormat pageFormat = kPMNoPageFormat;
	
	if (doc->flatFormatHandle != NULL) 
	{	
		Size hdlSize = GetHandleSize(doc->flatFormatHandle);
		if (hdlSize == 0) 
		{
			DisposeHandle(doc->flatFormatHandle);
			doc->flatFormatHandle = NULL;
		}
	}
	
	//	Read the PageFormat flattened data from file.
	//	In this sample code we simply copy it from a global.
	
	flatFormatHandle = doc->flatFormatHandle;
	if (flatFormatHandle) {
		//	Convert the PageFormat flattened data into a PageFormat object.
		
		status = PMUnflattenPageFormat(flatFormatHandle, &pageFormat);
	}

	if (status == noErr)
		doc->docPrintInfo.docPageFormat = pageFormat;
	else
		doc->docPrintInfo.docPageFormat = kPMNoPageFormat;			

	return status;
}


/* -------------------------------------------------------------------------------------- */

static void DocPostPrintingError(Document * /*doc*/, OSStatus status,
			CFStringRef errorFormatStringKey)
{   
	CFStringRef formatStr = NULL, 
				printErrorMsg = NULL;
	SInt16      alertItemHit = 0;
	Str255      stringBuf;

	if ((status != noErr) && (status != kPMCancel)) {
		formatStr =  CFCopyLocalizedString (errorFormatStringKey, NULL);
		if (formatStr != NULL) {
			printErrorMsg = CFStringCreateWithFormat(NULL, NULL, formatStr, status);
			if (printErrorMsg != NULL) {
				if (CFStringGetPascalString (printErrorMsg,    
												stringBuf, sizeof (stringBuf), 
												GetApplicationTextEncoding())) {
					StandardAlert(kAlertStopAlert, stringBuf, NULL, NULL, &alertItemHit);
				}
				
				CFRelease(printErrorMsg);
			}
			
			CFRelease(formatStr);
		}
	}
}


/* --------------------------------------------------------- Roputines to save as EPSF -- */

/* Go thru the entire object list and make a list of the font/style combinations that are
actually used. */

static void FillFontUsedTbl(Document *doc)
{
	short j, k, styleBits;
	LINK pL;
	PGRAPHIC p;

	for (j = 0; j<doc->nfontsUsed; j++) {
		Pstrcpy((StringPtr)fontUsedTbl[j].fontName, (StringPtr)doc->fontTable[j].fontName);
		for (k = 0; k<4; k++)
			fontUsedTbl[j].style[k] = False;
		}
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (GraphicTYPE(pL)) {
			p = GetPGRAPHIC(pL);
			styleBits = p->fontStyle % 4;
			fontUsedTbl[p->fontInd].style[styleBits] = True;
			}
}


Boolean DoPostScript(Document *doc)
	{
		short			saveOutputTo, saveMagnify, sheet, sufIndex;
		short			len, vref, rfNum, suffixLen, ch, firstSheet, topSheet;
		short			newType, anInt, pageNum, sheetNum;
		Str255			outname;
		PicHandle		picHdl;
		RgnHandle		rgnHdl;
		Rect			paperRect;
		short			EPSFile;
		static short	type=2;						/* 1=EPSF, 2=PostScript */
		char			fmtStr[256];

		NSClientData	nscd;
		Boolean			keepGoing;
		FSSpec 			rfSpec;
		ScriptCode		scriptCode = smRoman;
		OSErr			theErr;
		
		if (!Sel2MeasPage(doc, &anInt, &pageNum)) {
			AlwaysErrMsg("DoPostScript: can't get measure & page from selection.");
			return False;
		}
		newType = PSTypeDialog(type, pageNum);
		sheetNum = pageNum-doc->firstPageNumber;
		if (!newType) return False;
		type = newType;
		LogPrintf(LOG_INFO, "Preparing to create PostScript file.  (DoPostScript)\n");
		
		EPSFile = (type==1);
		UpdateAllWindows();

		/* Create a default EPS filename by looking up the suffix string and appending
		   it to the current name.  If the current name is so long that there would not
		   be room to append the suffix, we truncate the file name before appending the
		   suffix so that we don't run the risk of overwriting the original score file. */
		   
		sufIndex = EPSFile ? 4 : 8;
		GetIndString(outname, MiscStringsID, sufIndex);		/* Get suffix (".EPSF" or ".ps") length */
		suffixLen = *(unsigned char *)outname;
	
		/* Get current name (and its length), and truncate it if necessary */
				
		if (doc->named)	Pstrcpy((StringPtr)outname, (StringPtr)doc->name);
		 else			GetIndString(outname, MiscStringsID, 1);		/* "Untitled" */
		len = *(unsigned char *)outname;
		if (len >= (FILENAME_MAXLEN-suffixLen)) len = (FILENAME_MAXLEN-suffixLen);
		
		/* Finally append suffix */
		
		ch = outname[len];									/* Hold last character of name */
		GetIndString(outname+len, MiscStringsID, sufIndex);	/* Append suffix, obliterating last char */
		outname[len] = ch;									/* Overwrite length byte with saved char */
		*outname = (len + suffixLen);						/* And ensure new string knows new length */
		
		/* Ask user where to put this PostScript file */

		keepGoing = GetOutputName(MiscStringsID, EPSFile? 7:9, outname, &vref, &nscd);
		if (!keepGoing) return False;
		
		rfSpec = nscd.nsFSSpec;
		
		saveOutputTo = outputTo;						/* Save state */
		outputTo = toPostScript;
		
		WaitCursor();
		theErr = PS_Open(doc, outname, vref, USING_FILE, EPSFile? 'EPSF':'TEXT', &rfSpec);
		if (theErr == noErr) {
			paperRect = doc->origPaperRect;
			PS_Header(doc, outname, (EPSFile? 1:doc->numSheets), 1.0, False, True,
						&paperRect, &paperRect);
			if (EPSFile) {
				firstSheet = sheetNum;					/* Do current sheet only */
				topSheet = firstSheet + 1;
			}
			else {
				firstSheet = 0;							/* Do all sheets */
				topSheet = doc->numSheets;
			}
			
			for (sheet=firstSheet; sheet<topSheet; sheet++) {
				PS_NewPage(doc,NULL,sheet+doc->firstPageNumber);
				DrawPageContent(doc, sheet, &paperRect, &doc->viewRect);
				PS_EndPage();
			}
			
			if (EPSFile) {
				/* Write additional EPSF information: in the PostScript program itself, the
				   fonts used and the image's bounding box; in the resource fork of the same
				   file, a PICT of the page image. */
				   
				rgnHdl = NewRgn();						/* Save current clipping region */
				GetClip(rgnHdl);
				
				saveMagnify = doc->magnify;				/* Save current magnification, and reset to none */
				doc->magnify = 0;
		 		
		 		outputTo = toPICT;						/* Draw page contents into a PICT */
		 		ClipRect(&paperRect);
				picHdl = OpenPicture(&paperRect);
				DrawPageContent(doc, sheetNum, &paperRect, &doc->viewRect);
				FrameRect(&paperRect);
				ClosePicture();
				
				doc->magnify = saveMagnify;				/* Restore environment */
				InstallMagnify(doc);
				
				SetClip(rgnHdl);
				outputTo = toPostScript;
			}

			FillFontUsedTbl(doc);							/* Make list of font/style combinations used */
	
			PS_Trailer(doc, doc->nfontsUsed, fontUsedTbl,
						(doc->numberMeas!=0? doc->fontNameMN : NULL),
						&paperRect);
			PS_Close();
			if (thinLines) {
				GetIndCString(strBuf, PRINTERRS_STRS, 2);    /* "Some line(s) are so thin that they may not reproduce well." */
				CParamText(strBuf, "", "", "");
				CautionInform(GENERIC_ALRT);
			}
				
			if (EPSFile) {
				/* Postscript text file is closed: add PICT to resource fork of new EPSF file */
				
				FSpCreateResFile(&rfSpec, creatorType, 'EPSF', scriptCode);
				theErr = ResError();
				rfNum = FSpOpenResFile(&rfSpec, fsRdWrPerm);
				//CreateResFile(outname);
				//rfNum = OpenResFile((unsigned char *)StripAddress(outname));
				if (rfNum == -1) {
					DisposeHandle((Handle)picHdl);
					GetIndCString(fmtStr, PRINTERRS_STRS, 4);    /* "DoPrintScore: can't create or open resource file %s (Error %d)" */
					sprintf(strBuf, fmtStr, PtoCstr(outname), ResError()); 
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					}
				 else {
					AddResource((Handle)picHdl, 'PICT', 256, "\p");
					CloseResFile(rfNum);
					UseResFile(appRFRefNum);					/* a precaution */
				}
			}
		}

		outputTo = saveOutputTo;
		ArrowCursor();

		return True;
	}


enum {
	BUT1_OK = 1,
	BUT2_Cancel,
	ICON3,
	RAD4_EPSF,
	RAD5_PostScript,
	STXT6_Specify
};

static short group1;

static short PSTypeDialog(short oldType, short pageNum)
	{
		short itemHit, okay, type, keepGoing=True;
		DialogPtr dlog; GrafPtr oldPort;
		ModalFilterUPP filterUPP;
		Handle hndl;
		Rect box;
		char fmtStr[256], str[256];

		/* Build dialog window and install its item values */
		
		filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP == NULL) {
			MissingDialog(PSTYPE_DLOG);
			return 0;
		}
		GetPort(&oldPort);
		dlog = GetNewDialog(PSTYPE_DLOG, NULL, BRING_TO_FRONT);
		if (dlog==NULL) {
			DisposeRoutineDescriptor(filterUPP);
			MissingDialog(PSTYPE_DLOG);
			return(0);
		}
		SetPort(GetDialogPort(dlog));

		/* Change title of "EPSF" checkbox to reflect page number. */
		
		GetIndCString(fmtStr, MiscStringsID, 15);	/* "EPS file for page %d only" */
		sprintf(str, fmtStr, pageNum);
		CToPString(str);
		GetDialogItem(dlog, RAD4_EPSF, &type, &hndl, &box);
		SetControlTitle((ControlHandle)hndl, (StringPtr)str);

		group1 = (oldType==1? RAD4_EPSF : RAD5_PostScript);
		PutDlgChkRadio(dlog, RAD4_EPSF, (group1==RAD4_EPSF));
		PutDlgChkRadio(dlog, RAD5_PostScript, (group1==RAD5_PostScript));

		PlaceWindow(GetDialogWindow(dlog),NULL, 0, 80);
		ShowWindow(GetDialogWindow(dlog));

		/* Entertain filtered user events until dialog is dismissed */
		
		while (keepGoing) {
			ModalDialog(filterUPP, &itemHit);
			switch(itemHit) {
				case BUT1_OK:
					keepGoing = False;
					okay = (group1==RAD4_EPSF) ? 1 : 2;
					break;
				case BUT2_Cancel:
					keepGoing = False;
					okay = 0;
					break;
				case RAD4_EPSF:
				case RAD5_PostScript:
					if (itemHit!=group1) SwitchRadio(dlog, &group1, itemHit);
					break;
			}
		}
		
		DisposeRoutineDescriptor(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
		
		return(okay);
	}
