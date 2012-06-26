/*
	File:		MyCarbonPrinting.c
	
	Contains:	Routines needed to perform printing. This example uses sheets and provides for
                        save as PDF.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Copyright © 1999-2001 Apple Computer, Inc., All Rights Reserved
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "UIFUtils.h"
#include "CarbonPrinting.h"

enum {
  kPMNInvalidPageRange = -30901,
  kPMNSysJustifUserCancel = -30902  
};

/*------------------------------------------------------------------------------
	Globals
------------------------------------------------------------------------------*/
static	Handle	gflatPageFormat = NULL;		// used in FlattenAndSavePageFormat

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
static OSStatus SetupPagesToPrint(Document *doc);
static void PrintDemoBanner(Document *doc, Boolean toPostScript);
static Boolean IncludePostScriptInSpoolFile(PMPrintSession printSession);
static short GetPrintDestination(Document *doc);
static void NDoPrintLoop(Document *doc);
static OSStatus PrintImageWriter(Document *doc, UInt32 firstSheet, UInt32 lastSheet);
static OSStatus PrintLaserWriter(Document *doc, UInt32 firstSheet, UInt32 lastSheet);
static OSStatus DocFormatGetLandscape(Document *doc, Boolean *landscape);
static OSStatus DocFormatGetAdjustedPaperRect(Document *doc, Rect *paperRect, Boolean xlateScale);
static UInt32 NPagesInDoc(Document *doc);
static OSStatus NDocDrawPage(Document *doc, UInt32 pageNum);
static Boolean DocSessionOK(Document *doc, OSStatus status);
static void DocReleasePrintSession(Document *doc);
static void DocReleasePageFormat(Document *doc);
static void DocReleasePrintSettings(Document *doc);
static void DocSPFReleasePrintSession(Document *doc,PMPrintSession printSession,PMPageFormat pageFormat);
static void DocPostPrintingError(Document *doc,OSStatus status, CFStringRef errorFormatStringKey);

static void FillFontUsedTbl(Document *doc);
static short PSTypeDialog(short, short);


// --------------------------------------------------------------------------------------------------------------
static OSStatus DocCreatePrintSessionProcs(PMSheetDoneUPP *pageSetupDoneUPP,
															PMSheetDoneUPP *printDialogDoneUPP)
{
	OSStatus err = noErr;

	if (gMyPageSetupDoneProc == NULL) {
		gMyPageSetupDoneProc = NewPMSheetDoneUPP(NPageSetupDoneProc);
		
		if (gMyPageSetupDoneProc == NULL)
			err = memFullErr;
	}
	
	if (err == noErr) {
		*pageSetupDoneUPP = gMyPageSetupDoneProc;
		
		gMyPrintDialogDoneProc = NewPMSheetDoneUPP(NPrintDialogDoneProc);
		if (gMyPrintDialogDoneProc == NULL)
			err = memFullErr;
			
		*printDialogDoneUPP = gMyPrintDialogDoneProc;
	}
	
	if (err != noErr) {
		*pageSetupDoneUPP = NULL;
		*printDialogDoneUPP = NULL;
	}

	return err;
}

// --------------------------------------------------------------------------------------------------------------
// Create DocPrintInfo record for the document.

Boolean IsDocPrintInfoInstalled(Document *doc)
{
	return (doc->docPrintInfo.docPageSetupDoneUPP != NULL);
}

// --------------------------------------------------------------------------------------------------------------
// Create DocPrintInfo record for the document.

OSStatus InstallDocPrintInfo(Document *doc)
{
	PMSheetDoneUPP		pageSetupDoneUPP;
	PMSheetDoneUPP		printDialogDoneUPP;
	OSStatus				err = noErr;
	
	doc->docPrintInfo.docPrintSession = NULL;
	doc->docPrintInfo.docPageFormat = kPMNoPageFormat;
	doc->docPrintInfo.docPrintSettings = kPMNoPrintSettings;
	
	err = DocCreatePrintSessionProcs(&pageSetupDoneUPP, &printDialogDoneUPP);
	
	if (err == noErr)
	{
		doc->docPrintInfo.docPageSetupDoneUPP = pageSetupDoneUPP;
		doc->docPrintInfo.docPrintDialogDoneUPP = printDialogDoneUPP;
	}
	else {
		doc->docPrintInfo.docPageSetupDoneUPP = NULL;
		doc->docPrintInfo.docPrintDialogDoneUPP = NULL;
	}
	
	return err;
}

// --------------------------------------------------------------------------------------------------------------

static OSStatus DocSetupPageFormat(Document *doc)
{
	OSStatus status = noErr;

	PMPrintSession printSession = NULL;
	PMPageFormat pageFormat = kPMNoPageFormat;
	
	if (!IsDocPrintInfoInstalled(doc))
		InstallDocPrintInfo(doc);
	
	status = PMCreateSession(&printSession);
	
	if (status != noErr)
		return status;
		
	if (doc->docPrintInfo.docPageFormat == kPMNoPageFormat) {
	
		status = PMCreatePageFormat(&pageFormat);
		
		if (status == noErr && pageFormat != kPMNoPageFormat) {
		
			status = PMSessionDefaultPageFormat(printSession, pageFormat);
			
			if (status == noErr)
				doc->docPrintInfo.docPageFormat = pageFormat;
			
		}
		else if (status == noErr)
			status = kPMGeneralError;
	}
	else {
		status = PMSessionValidatePageFormat(printSession,
															doc->docPrintInfo.docPageFormat,
															kPMDontWantBoolean);
	}
	
	if (status == noErr) {
		doc->docPrintInfo.docPrintSession = printSession;
	}
	else {
		DocSPFReleasePrintSession(doc,printSession,pageFormat);
	}
	
	return status;
	
}

// --------------------------------------------------------------------------------------------------------------

static pascal void NPageSetupDoneProc(PMPrintSession printSession,
					                        WindowRef docWindow,
					                        Boolean accepted)
{
	Document *doc = GetDocumentFromWindow(docWindow);
	OSStatus status = noErr;
	
	if (doc!=NULL && doc->docPrintInfo.docPageFormat != kPMNoPageFormat && accepted)
	{
		/*
		 *	Now if doc->paperRect is different from rPaper (user has chosen different
		 *	size paper), we change to new size and inval everything.
		 */
		PMRect pmRPaper;
		Rect rPaper;
		PMOrientation pmOrientation;
		double scale;
		
		status = PMGetAdjustedPaperRect(doc->docPrintInfo.docPageFormat,&pmRPaper);
		
		if (status == noErr)
			status = PMGetScale(doc->docPrintInfo.docPageFormat,&scale);

		if (status == noErr)
		{
			scale /= 100.0;
			
			pmRPaper.top *= scale;
			pmRPaper.left *= scale;
			pmRPaper.bottom *= scale;
			pmRPaper.right *= scale;
			
			rPaper.top = pmRPaper.top;
			rPaper.left = pmRPaper.left;
			rPaper.bottom = pmRPaper.bottom;
			rPaper.right = pmRPaper.right;
			
			OffsetRect(&rPaper,-rPaper.left,-rPaper.top);
			if (!EqualRect(&doc->origPaperRect,&rPaper)) {
				short marginRight = doc->origPaperRect.right - doc->marginRect.right;
				short marginBottom = doc->origPaperRect.bottom - doc->marginRect.bottom;
				doc->marginRect.right = rPaper.right - marginRight;
				doc->marginRect.bottom = rPaper.bottom - marginBottom;
				doc->origPaperRect = rPaper;
				MagnifyPaper(&doc->origPaperRect,&doc->paperRect,doc->magnify);
				doc->changed = TRUE;
				InvalSheetView(doc);
			}
		}		

		status = PMGetOrientation(doc->docPrintInfo.docPageFormat,&pmOrientation);
		if (status == noErr)
		{
			SignedByte oldLandscape = doc->landscape;
	
			doc->landscape = (pmOrientation == kPMLandscape || pmOrientation == kPMReverseLandscape);
			
			if (doc->landscape != oldLandscape)
				doc->changed = TRUE;
		}
	}
	
	if (doc != NULL && printSession == doc->docPrintInfo.docPrintSession) {
		DocReleasePrintSession(doc);
	}
	else {
    	status = PMRelease(printSession);	
	}

	if (status != noErr) {
		CFStringRef cfFormatStr = CFSTR("Page Setup Dialog Done error. Error number: %d.");
		
		DocPostPrintingError(doc,status,cfFormatStr);
	}
}

// --------------------------------------------------------------------------------------------------------------

void NDoPageSetup(Document *doc)
{
	WindowRef w = doc->theWindow;
	Boolean accepted = FALSE;
	
	OSStatus status = DocSetupPageFormat(doc);
	
	if (status == noErr) {
		PMSessionUseSheets(doc->docPrintInfo.docPrintSession,
									w,
									doc->docPrintInfo.docPageSetupDoneUPP);
		
		status = PMSessionPageSetupDialog(doc->docPrintInfo.docPrintSession,
														doc->docPrintInfo.docPageFormat,
														&accepted);
	}
	
	if (status != noErr) {
		DocReleasePrintSession(doc);
		
		CFStringRef cfFormatStr = CFSTR("Page Setup Dialog error. Error number: %d.");
		
		DocPostPrintingError(doc,status,cfFormatStr);
	}	
}

// --------------------------------------------------------------------------------------------------------------

static pascal void NPrintDialogDoneProc(PMPrintSession /*printSession*/,
						                        WindowRef docWindow,
						                        Boolean accepted)
{
	Document *doc = GetDocumentFromWindow(docWindow);
	
	if (doc != NULL)
		if (accepted)
			NDoPrintLoop(doc);
}

// --------------------------------------------------------------------------------------------------------------

void NDoPrintScore(Document *doc)
{
	OSStatus status = DocSetupPageFormat(doc);
	PMPrintSettings printSettings;
	Boolean accepted;
	CFStringRef wTitleRef;
	
	if (status == noErr) {
		
		status = PMCreatePrintSettings(&printSettings);
	}
	
	if (status == noErr && printSettings != kPMNoPrintSettings)
	{	
		status = CopyWindowTitleAsCFString(doc->theWindow,&wTitleRef);		
	}
	
	if (status == noErr)
	{	
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
	}
	
	if (status != noErr) {
		if (printSettings != kPMNoPrintSettings)
			PMRelease(printSettings);
			
		DocReleasePrintSettings(doc);
		DocReleasePrintSession(doc);
		
		CFStringRef cfFormatStr = CFSTR("Print Dialog error. Error number: %d.");
		
		DocPostPrintingError(doc,status,cfFormatStr);
	}
}

// --------------------------------------------------------------------------------------------------------------

static OSStatus SetupPrintDlogPages(Document *doc)
{
	OSStatus status = noErr;
	
	short firstPage = doc->firstPageNumber;
	short lastPage = doc->firstPageNumber + doc->numSheets - 1;
	
	status = PMSetPageRange(doc->docPrintInfo.docPrintSettings, firstPage, lastPage);
	
	return status;
}

// --------------------------------------------------------------------------------------------------------------

static OSStatus GetPrintPageRange(Document *doc, UInt32 *firstPage, UInt32 *lastPage)
{
	OSStatus status = PMGetFirstPage(doc->docPrintInfo.docPrintSettings, firstPage);
	
	if (status == noErr)
		status = PMGetLastPage(doc->docPrintInfo.docPrintSettings, lastPage);
	
	return status;
}

// --------------------------------------------------------------------------------------------------------------

static OSStatus SetPrintPageRange(Document *doc, UInt32 firstPage, UInt32 lastPage)
{
	OSStatus status = PMSetFirstPage(doc->docPrintInfo.docPrintSettings, firstPage, FALSE);

	if (status == noErr)
		status = PMSetLastPage(doc->docPrintInfo.docPrintSettings, lastPage, FALSE);
				
	return status;
}

// --------------------------------------------------------------------------------------------------------------

/*
 * Check that systems in a range of pages are all justified. If SysJustFact() is
 * significantly different from 1.0, tell user and give them a chance to
 * cancel. Return FALSE if user says stop, TRUE otherwise (all justified or user
 * says continue anyway). (It'd be nice some day to give them an opportunity to
 * justify right now.)
 */

#define JUSTSLOP .001

static Boolean IsRightJustOK(Document *doc, 
										short firstSheet, short lastSheet)	/* Inclusive range of sheet numbers */
{
	LINK startPageL, endPageL, pL, firstMeasL, termSysL, lastMeasL;
	FASTFLOAT justFact;
	DDIST staffWidth, lastMeasWidth;
	
	startPageL = LSSearch(doc->headL, PAGEtype, firstSheet, GO_RIGHT, FALSE);
	endPageL = LSSearch(doc->headL, PAGEtype, lastSheet+1, GO_RIGHT, FALSE);
	if (!endPageL) endPageL = doc->tailL;
	if (!startPageL) {
		MayErrMsg("IsRightJustOK: can't find startPageL.");
		return TRUE;
	}
	for (pL = startPageL; pL!=endPageL; pL = RightLINK(pL))
		if (SystemTYPE(pL)) {
			firstMeasL = LSSearch(pL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
			termSysL = EndSystemSearch(doc, pL);
			lastMeasL = LSSearch(termSysL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
			justFact = SysJustFact(doc, firstMeasL, lastMeasL, &staffWidth, &lastMeasWidth);
			if (justFact<1.0-JUSTSLOP || justFact>1.0+JUSTSLOP) {
				return (CautionAdvise(NOTJUST_ALRT)!=Cancel);
			}
		}
		
	return TRUE;
}


// --------------------------------------------------------------------------------------------------------------

static OSStatus SetupPagesToPrint(Document *doc, UInt32 *firstPg, UInt32 *lastPg)
{
	OSStatus status = noErr;
	UInt32	firstPage,lastPage;
	UInt32	firstSheet,lastSheet;
	
	status = GetPrintPageRange(doc, &firstPage, &lastPage);
	
	if (status == noErr) {
		firstSheet = firstPage - doc->firstPageNumber;
		lastSheet  = lastPage - doc->firstPageNumber;
		if (firstSheet > lastSheet)
			{ short sh = firstSheet; firstSheet = lastSheet; lastSheet = sh; }
		if (firstSheet < 0) firstSheet = 0;
		if (lastSheet >= doc->numSheets) lastSheet = doc->numSheets-1;

		/* If the page numbers aren't in forward order now, they don't exist. */

		if (firstSheet > lastSheet) {
			status = kPMNInvalidPageRange;
		}
		
		/*
		 * Check that systems to be printed are all justified; if not, tell user and
		 * give them a chance to cancel.
		 */
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

// --------------------------------------------------------------------------------------------------------------

static void PrintDemoBanner(Document *doc, Boolean toPostScript)
{
#ifdef DOING_INITIAL_ENCODE
	const unsigned char *banLine1 = "\pPrinted by Nightingale(R)";
	const unsigned char *banLine2 = "\pAdvanced Music Notation Systems, Inc.";
	const unsigned char *banLine3 = "\pfax: 413-268-7317  e-mail: info@ngale.com";
#else
	const unsigned char *banLine1 = "\p\33T+(/VAkD;f\25ZB#R+(<RI.Ž";  /* Ends in Option-e e */
	const unsigned char *banLine2 = "\p\37C/64AD'\6\3%.ZQ2\6\02244WP(R1";
	const unsigned char *banLine3 = "\ps\26rko\1\23f\24tqh\23\5k\6btk\5\b\177\20pkj\3\25|";
#endif
 
#ifdef DOING_INITIAL_ENCODE
	short i,k,len,klen;
	/*
	 * Include this paragraph to encrypt <banLine>s the first time so that you
	 * can break below with the debugger in order to write down the encrypted
	 * version for plugging in above.  The banKey string should be unreadable
	 * garbage, and should be stored far away (in the global strings) from the
	 * encrypted banner string.
	 *
	 * This should never be included in any working version of the program.
	 * The purpose of all this is to avoid having the banner text easily findable
	 * while looking at the program either statically (with a resource editor) or
	 * dynamically (with a debugger), so it won't be too easy for a hacker to
	 * disable--e.g., by replacing the text with blanks.
	 */
	len = *banLine1;
	k = 1; klen = *banKey;
	for (i=1; i<=len; i++) {						/* For each byte in banner text... */
		banLine1[i] ^= banKey[k++];					/* XOR in the next key byte */
		if (k > klen) k = 1;						/* Cycle through all key bytes */
	}

	len = *banLine2;
	k = 1; klen = *banKey;
	for (i=1; i<=len; i++) {						/* For each byte in banner text... */
		banLine2[i] ^= banKey[k++];					/* XOR in the next key byte */
		if (k > klen) k = 1;						/* Cycle through all key bytes */
	}

	len = *banLine3;
	k = 1; klen = *banKey;
	for (i=1; i<=len; i++) {						/* For each byte in banner text... */
		banLine3[i] ^= banKey[k++];					/* XOR in the next key byte */
		if (k > klen) k = 1;						/* Cycle through all key bytes */
	}
#endif

}

/* ------------------------------------------------------------------------------
 *   Function:	IncludePostScriptInSpoolFile
 *	
 *   Parameters:
 *      printSession	- current printing session
 *
 *  Description:
 *  	Check if current printer driver supports embedding of PostScript in the spool file, and
 *      if it does, instruct the Carbon Printing Manager to generate a PICT w/ PS spool file.
 */              		

static Boolean IncludePostScriptInSpoolFile(PMPrintSession printSession)
{
	Boolean	includePostScript = false;
	OSStatus	status;
	CFArrayRef	supportedFormats = NULL;
	SInt32	i, numSupportedFormats;

	// Get the list of spool file formats supported by the current driver.
	// PMSessionGetDocumentFormatGeneration returns the list of formats which can be generated
	// by the spooler (client-side) AND converted by the despooler (server-side).
	// PMSessionGetDocumentFormatSupported only returns the list of formats which can be converted
	// by the despooler.

	status = PMSessionGetDocumentFormatGeneration(printSession, &supportedFormats);
	if (status == noErr)
	{
		// Check if PICT w/ PS is in the list of supported formats.
		numSupportedFormats = CFArrayGetCount(supportedFormats);

		for (i=0; i < numSupportedFormats; i++)
		{
			if ( CFStringCompare((CFStringRef)CFArrayGetValueAtIndex(supportedFormats, i),
										kPMDocumentFormatPICTPS, kCFCompareCaseInsensitive) == kCFCompareEqualTo )
			{
				// PICT w/ PS is supported, so tell the Printing Mgr to generate a PICT w/ PS spool file

				// Build an array of graphics contexts containing just one type, Quickdraw,
				// meaning that we will be using a QD port to image our pages in the print loop.
				CFStringRef	strings[1];
				CFArrayRef		arrayOfGraphicsContexts;

				strings[0] = kPMGraphicsContextQuickdraw;
				arrayOfGraphicsContexts = CFArrayCreate(kCFAllocatorDefault,
																		(const void **)strings, 1, &kCFTypeArrayCallBacks);

				if (arrayOfGraphicsContexts != NULL)
				{
					// Request a PICT w/ PS spool file
					status = PMSessionSetDocumentFormatGeneration(printSession, kPMDocumentFormatPICTPS, 
																					arrayOfGraphicsContexts, NULL);

					if (status == noErr)
						includePostScript = true;	// Enable use of PS PicComments in DrawPage.

					// Deallocate the array used for the list of graphics contexts.
					CFRelease(arrayOfGraphicsContexts);
				}
				break;
			}
		}

		// Deallocate the array used for the list of supported spool file formats.
		CFRelease(supportedFormats);
	}

	return includePostScript;
}	//	IncludePostScriptInSpoolFile


// --------------------------------------------------------------------------------------------------------------

static short GetPrintDestination(Document *doc)
{
	Boolean includePostScript = IncludePostScriptInSpoolFile(doc->docPrintInfo.docPrintSession);
	
	if (includePostScript)
		return toPostScript;
		
	return toImageWriter;
}


// --------------------------------------------------------------------------------------------------------------

#define USE_PREC103 1

static Handle printDictHdl;

static Handle PS_PreparePrintDictHdl(Document *doc, Rect *imageRect)
	{
		//Handle text; long size; 
		Rect box;
		short oldFile = CurResFile();
		FSSpec fsSpec; OSErr thisError;
		
//		UseResFile(appRFRefNum);
//		prec103 = Get1Resource('PREC',103);
//		UseResFile(oldFile);
		
//		if (GoodResource(prec103)) {
			//HNoPurge(prec103);
			thisError = PS_Open(doc, NULL, 0, USING_HANDLE, 0L, &fsSpec);
			//if (thisError) { ReleaseResource(prec103); return; }
			if (thisError) { return NULL; }
			
			/* Now stuff prec103 with stuff from our resources */
			
			SetRect(&box,0,0,0,0);
			PS_HeaderHdl(doc,"\p??",1,1.0,FALSE,TRUE,&box,imageRect);
			//SetHandleSize(prec103,size=GetHandleSize(text = PS_GetTextHandle()));
			//BlockMove(*text,*prec103,size);
			return PS_GetTextHandle();
#ifdef FOR_DEBUGGING_ONLY
			ChangedResource(prec103);
			WriteResource(prec103);
			UpdateResFile(appRFRefNum);
			ExitToShell();
#endif
//			}
	}

void PS_FinishPrintDictHdl()
	{
	}



static void NDoPrintLoop(Document *doc)
{
	short		saveOutputTo;
	OSStatus status;
	UInt32	firstPage,lastPage;
	UInt32	firstSheet,lastSheet;
	UInt32	copies;
	Rect		imageRect;
	
	saveOutputTo = outputTo;
	
	status = SetupPagesToPrint(doc, &firstPage, &lastPage);
	require(status==noErr,displayError);
	
	firstSheet = firstPage - doc->firstPageNumber;
	lastSheet = lastPage - doc->firstPageNumber;

	if (status == noErr)
		status = PMGetCopies(doc->docPrintInfo.docPrintSettings, &copies);	
	require(status==noErr,displayError);
	
	UpdateAllWindows();
	
	status = DocFormatGetAdjustedPaperRect(doc, &imageRect, TRUE);
	require(status==noErr,displayError);
	
#if USE_PREC103
	if (status == noErr)
		PS_PreparePrintDict(doc,&imageRect);			/* Create 'PREC' 103 */
#else
	if (status == noErr)
		printDictHdl = PS_PreparePrintDictHdl(doc,&imageRect);			/* Create 'PREC' 103 */
#endif
		
	if (status == noErr)
		outputTo = GetPrintDestination(doc);
	require(status==noErr,displayError);
	
	if (status == noErr) {
//		for ( ; copies>0; copies--) {
			switch (outputTo) {
				case toImageWriter:
					status = PrintImageWriter(doc,firstSheet,lastSheet);
					if (status != noErr) copies = -1;
					break;
				case toPostScript:
					status = PrintLaserWriter(doc,firstSheet,lastSheet);
					if (status != noErr) copies = -1;
					break;
			}
//		}
	}
	
	outputTo = saveOutputTo;
	
displayError:

#if USE_PREC103
	PS_FinishPrintDict();
#else
	PS_FinishPrintDictHdl();	
#endif
	
	if (status != noErr &&
		 status != kPMNSysJustifUserCancel) 
	{
		CFStringRef cfFormatStr = CFSTR("Print loop error. Error number: %d.");
		
		DocPostPrintingError(doc,status,cfFormatStr);
	}
                                                            
	//	Release the PrintSession and PrintSettings objects.  PMRelease decrements the
	//	ref count of the allocated objects.  We let the Printing Manager decide when
	//	to release the allocated memory.
	// PageFormat persists between sessions.
	
	//if (doc->docPrintInfo.docPageFormat != kPMNoPageFormat) {
	//	PMRelease(doc->docPrintInfo.docPageFormat);
	//	doc->docPrintInfo.docPageFormat = kPMNoPageFormat;
	//}
	if (doc->docPrintInfo.docPrintSettings != kPMNoPrintSettings) {
		PMRelease(doc->docPrintInfo.docPrintSettings);
		doc->docPrintInfo.docPrintSettings = kPMNoPrintSettings;
	}

	//	Terminate the current printing session. 
	PMRelease(doc->docPrintInfo.docPrintSession);
	doc->docPrintInfo.docPrintSession = NULL;
}

// --------------------------------------------------------------------------------------------------------------

static OSStatus PrintImageWriter(Document *doc, UInt32 firstSheet, UInt32 lastSheet)
{
	OSStatus err = noErr,status = noErr;
	UInt32 sheetNum;
	GrafPtr	currPort,printPort;
	
	status = PMSessionBeginDocument(doc->docPrintInfo.docPrintSession,
												doc->docPrintInfo.docPrintSettings,
												doc->docPrintInfo.docPageFormat);
												
	if (status == noErr) {
		sheetNum = firstSheet;
		
		while (sheetNum <= lastSheet && DocSessionOK(doc,status))
		{
			status = PMSessionBeginPage(doc->docPrintInfo.docPrintSession,
													doc->docPrintInfo.docPageFormat,
													NULL);
													
			if (status == noErr)
			{
				GetPort(&currPort);
				
				status = PMSessionGetGraphicsContext(doc->docPrintInfo.docPrintSession,
																	kPMGraphicsContextQuickdraw,
																	(void**)&printPort);					
				if (status == noErr)
				{
					SetPort(printPort);
					
						PrintDemoBanner(doc, FALSE);
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

static OSStatus TestDrawPage(PMPrintSession printSession, UInt32 pageNumber, Boolean addPostScript);

// --------------------------------------------------------------------------------------------------------------

#define TEST_DRAWPAGE 0

static OSStatus PrintLaserWriter(Document *doc, UInt32 firstSheet, UInt32 lastSheet)
{
	OSStatus	err = noErr,status = noErr;
	UInt32	sheet;
//	Boolean	landscape;
//	Rect 		printRect,frameRect;
	GrafPtr	currPort,printPort;
	Rect		imageRect,paper;
	FSSpec	fsSpec;
	
	status = PMSessionBeginDocument(doc->docPrintInfo.docPrintSession,
												doc->docPrintInfo.docPrintSettings,
												doc->docPrintInfo.docPageFormat);
												
	if (status == noErr) {
	
		/*
		 * Get physical size of paper (topleft should be (0,0)) and imageable area
		 * (topleft should be something like (-31,-30))
		 */
		 	
		//printRect = (**hPrint).prInfo.rPage;
		//imageRect = (*hPrint)->rPaper;		
		status = DocFormatGetAdjustedPaperRect(doc, &imageRect, TRUE);

		paper = doc->paperRect;

#ifdef UNUSED_CODE_FROM_PRINT_C
		frameRect = doc->marginRect;

		if (status == noErr) {
			status = DocFormatGetLandscape(doc, &landscape);
			if (landscape)
				scaleFactor = landScaleFactor;
		}
#endif
		
		if (status == noErr && PS_Open(doc,NULL,0,USING_PRINTER,'????',&fsSpec) == noErr) {

			for (sheet=firstSheet; sheet<=lastSheet && DocSessionOK(doc,status); sheet++) {
			
				status = PMSessionBeginPage(doc->docPrintInfo.docPrintSession,
														doc->docPrintInfo.docPageFormat,
														NULL);
														
				if (status == noErr)
				{
					GetPort(&currPort);
					
					status = PMSessionGetGraphicsContext(doc->docPrintInfo.docPrintSession,
																		kPMGraphicsContextQuickdraw,
																		(void**)&printPort);					
					if (status == noErr)
					{
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
#if TEST_DRAWPAGE
						TestDrawPage(doc->docPrintInfo.docPrintSession, 1, TRUE);
#else
						PS_NewPage(doc,NULL,sheet+doc->firstPageNumber);
						
						PS_OutOfQD(doc,TRUE,&imageRect);
						//PrintDemoBanner(doc, TRUE);
						DrawPageContent(doc, sheet, &paper, &doc->currentPaper);
//						PS_Print("showpage\r");
						PS_IntoQD(doc,TRUE);
						
						PS_EndPage();
#endif
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
/*------------------------------------------------------------------------------
    Function:	DrawPage
	
    Parameters:
        printSession	- current printing session
        pageNumber	- the logical page number in the document
        addPostScript	- flag to enable PostScript code
	
    Description:
        Draws the contents of a single page.  If addPostScript is true, DrawPage
        adds PostScript code into the spool file.  See the Printing chapter in
        Inside Macintosh, Imaging with QuickDraw, for details about PostScript
        PicComments.
		
------------------------------------------------------------------------------*/
static OSStatus TestDrawPage(PMPrintSession printSession, UInt32 pageNumber, Boolean addPostScript)
{
	OSStatus status = noErr;
	Str255 pageNumberString;
	//	In this sample code we do some very simple text drawing.    
	MoveTo(72,72);
	TextFont(kFontIDHelvetica);
	TextSize(24);
	DrawString("\pDrawing Page Number ");
	NumToString(pageNumber, pageNumberString);
	DrawString(pageNumberString);

	//	Conditionally insert PostScript into the spool file.
	if (addPostScript)
	{

//	 	Str255		p2[4] = {
//			"\puserdict /NightingaleDict 80 dict def\r",
//			"\pNightingaleDict begin\r",
//			"\p/BD {bind def}bind def\r",
//			"\pend\r" };
			
        // The following PostScript code handles the transformation to QuickDraw's coordinate system
        // assuming a letter size page.
        Str255		postScriptStr1 = "\p0 792 translate 1 -1 scale \r";
	    
        // Set the current PostScript font to Times and draw some more text.
        Str255		postScriptStr2 = "\p/Times-Roman findfont 12 scalefont setfont \r";
        Str255		postScriptStr3 = "\p( and some PostScript text) show\r";
       	    
        status = PMSessionPostScriptBegin(printSession);
        if (status == noErr)
        {
				short font = GetPortTxFont();
				short size = GetPortTxSize();
				
            status = PMSessionPostScriptData(printSession, (char *)&postScriptStr1[1], postScriptStr1[0]);
            if (status == noErr)
                status = PMSessionPostScriptData(printSession, (char *)&postScriptStr2[1], postScriptStr2[0]);
            if (status == noErr)
                status = PMSessionPostScriptData(printSession, (char *)&postScriptStr3[1], postScriptStr3[0]);
            if (status == noErr)
                status = PMSessionPostScriptEnd(printSession);
                
        		TextFont(font);
        		TextSize(size);
        }
    }   
		
    return status;
}	//	DrawPage




// --------------------------------------------------------------------------------------------------------------

static OSStatus NDocDrawPage(Document *doc, UInt32 sheetNum)
{
	Rect paper;
	
	paper = doc->paperRect;
	DrawPageContent(doc, sheetNum, &paper, &doc->viewRect);
	
	return noErr;
}

// --------------------------------------------------------------------------------------------------------------

static OSStatus DocFormatGetLandscape(Document *doc, Boolean *landscape)
{
	PMOrientation pmOrientation;
	
	OSStatus status = PMGetOrientation(doc->docPrintInfo.docPageFormat,&pmOrientation);
	if (status == noErr)
	{
		*landscape = (pmOrientation == kPMLandscape || pmOrientation == kPMReverseLandscape);
	}
	
	return status;
}

// --------------------------------------------------------------------------------------------------------------

static OSStatus DocFormatGetAdjustedPaperRect(Document *doc, Rect *rPaper, Boolean xlateScale)
{
	PMRect pmRPaper;
	double scale;
	
	OSStatus status = PMGetAdjustedPaperRect(doc->docPrintInfo.docPageFormat,&pmRPaper);
	
	if (xlateScale) {
		if (status == noErr)
			status = PMGetScale(doc->docPrintInfo.docPageFormat,&scale);
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

// --------------------------------------------------------------------------------------------------------------

static UInt32 NPagesInDoc(Document *doc)
{
	return doc->numSheets;
}

// --------------------------------------------------------------------------------------------------------------

static Boolean DocSessionOK(Document *doc, OSStatus status)
{
	if (status != noErr) return FALSE;
	if (doc->docPrintInfo.docPrintSession == NULL) return FALSE;
	
	return (PMSessionError(doc->docPrintInfo.docPrintSession) == noErr);
}

// --------------------------------------------------------------------------------------------------------------

static void DocReleasePrintSession(Document *doc)
{
	if (doc->docPrintInfo.docPrintSession != NULL) {
		PMRelease(doc->docPrintInfo.docPrintSession);
		doc->docPrintInfo.docPrintSession = NULL;
	}
}


// --------------------------------------------------------------------------------------------------------------

static void DocReleasePageFormat(Document *doc)
{
	if (doc->docPrintInfo.docPageFormat != kPMNoPageFormat) {
		PMRelease(doc->docPrintInfo.docPageFormat);
		doc->docPrintInfo.docPageFormat = kPMNoPageFormat;
	}
}

// --------------------------------------------------------------------------------------------------------------

static void DocReleasePrintSettings(Document *doc)
{
	if (doc->docPrintInfo.docPrintSettings != kPMNoPrintSettings) {
		PMRelease(doc->docPrintInfo.docPrintSettings);
		doc->docPrintInfo.docPrintSettings = kPMNoPrintSettings;
	}
}

// --------------------------------------------------------------------------------------------------------------
static void DocSPFReleasePrintSession(Document *doc,PMPrintSession printSession,PMPageFormat pageFormat)
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
		
------------------------------------------------------------------------------*/
OSStatus FlattenAndSavePageFormat(Document *doc)
{
	OSStatus	status;
	Handle	flatFormatHandle = NULL;

	//	Flatten the PageFormat object to memory.
	status = PMFlattenPageFormat(doc->docPrintInfo.docPageFormat, &flatFormatHandle);

	if (status == noErr) {
		//	Write the PageFormat data to file.
		//	In this sample code we simply copy it to a global.	
		doc->flatFormatHandle = flatFormatHandle;
	}

	return status;
}	//	FlattenAndSavePageFormat



/*------------------------------------------------------------------------------
    Function:	LoadAndUnflattenPageFormat
	
    Parameters:
        pageFormat	- PageFormat object read from document file
	
    Description:
        Gets flattened PageFormat data from the document and returns a PageFormat
        object.
        The function is not called in this sample code but your application
        will need to retrieve PageFormat data saved with documents.
		
------------------------------------------------------------------------------*/
OSStatus	LoadAndUnflattenPageFormat(Document *doc)
{
	OSStatus	status = noErr;
	Handle	flatFormatHandle = NULL;
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
}	//	LoadAndUnflattenPageFormat




// --------------------------------------------------------------------------------------------------------------

static void DocPostPrintingError (Document */*doc*/, OSStatus status, CFStringRef errorFormatStringKey)
{   
	CFStringRef formatStr = NULL, 
					printErrorMsg = NULL;
	SInt16      alertItemHit = 0;
	Str255      stringBuf;

	if ((status != noErr) && (status != kPMCancel)) 
	{
		formatStr =  CFCopyLocalizedString (errorFormatStringKey, NULL);
		if (formatStr != NULL)
		{
			printErrorMsg = CFStringCreateWithFormat(NULL, NULL, formatStr, status);
			if (printErrorMsg != NULL)
			{
				if (CFStringGetPascalString (printErrorMsg,    
														stringBuf, sizeof (stringBuf), 
														GetApplicationTextEncoding()))
				{
					StandardAlert(kAlertStopAlert, stringBuf, NULL, NULL, &alertItemHit);
				}
				
				CFRelease (printErrorMsg);
			}
			
			CFRelease (formatStr);
		}
	}
}


// --------------------------------------------------------------------------------------------------------------
// Save as EPSF routines.

/*
 *	Go thru the entire data structure and make a list of what font/style comb-
 *	inations are actually used.
 */

static void FillFontUsedTbl(Document *doc)
	{
		short	j, k, styleBits;
		LINK pL;
		PGRAPHIC p;
	
		for (j = 0; j<doc->nfontsUsed; j++) {
			PStrCopy((StringPtr)doc->fontTable[j].fontName,
						(StringPtr)fontUsedTbl[j].fontName);
			for (k = 0; k<4; k++)
				fontUsedTbl[j].style[k] = FALSE;
			}
		for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
			if (GraphicTYPE(pL)) {
				p = GetPGRAPHIC(pL);
				styleBits = p->fontStyle % 4;
				fontUsedTbl[p->fontInd].style[styleBits] = TRUE;
				}
	}


Boolean NDoPostScript(Document *doc)
	{
		short			saveOutputTo, saveMagnify, sheet, sufIndex;
		short			len, vref, rfNum, suffixLen, ch, firstSheet, topSheet;
		short			newType, anInt, pageNum, sheetNum;
		Str255		outname;
		PicHandle	picHdl;
		RgnHandle	rgnHdl;
		Rect			paperRect;
		short			EPSFile;
		static short	type=1;
		char				fmtStr[256];

		NSClientData	nscd;
		Boolean			keepGoing;
		FSSpec 			rfSpec;
		ScriptCode		scriptCode = smRoman;
		OSErr				theErr;
		
		
		Sel2MeasPage(doc, &anInt, &pageNum);
		newType = PSTypeDialog(type, pageNum);
		sheetNum = pageNum-doc->firstPageNumber;
		if (!newType) return FALSE;
		type = newType;
		
		EPSFile = (type == 1);
		UpdateAllWindows();
		
		/*
		 *	Create a default EPS filename by looking up the suffix string and appending
		 *	it to the current name.  If the current name is so long that there would not
		 *	be room to append the suffix, we truncate the file name before appending the
		 *	suffix so that we don't run the risk of overwriting the original score file.
		 */
		
		sufIndex = EPSFile ? 4 : 8;
		GetIndString(outname,MiscStringsID,sufIndex);		/* Get suffix (".EPSF" or ".ps") length */
		suffixLen = *(unsigned char *)outname;
	
		/* Get current name and its length, and truncate name to make room for suffix */
		
		if (doc->named)	PStrCopy((StringPtr)doc->name,(StringPtr)outname);
		 else			GetIndString(outname,MiscStringsID,1);		/* "Untitled" */
		len = *(unsigned char *)outname;
		if (len >= (64-suffixLen)) len = (64-suffixLen);	/* 64 is max file name size */
		
		/* Finally append suffix */
		
		ch = outname[len];							/* Hold last character of name */
		GetIndString(outname+len,MiscStringsID,sufIndex);	/* Append suffix, obliterating last char */
		outname[len] = ch;							/* Overwrite length byte with saved char */
		*outname = (len + suffixLen);				/* And ensure new string knows new length */
		
		/* Ask user where to put this PostScript file */

		//Pstrcpy(outname,doc->name);
		//PStrCat(outname,EPSFile? (StringPtr)"\p.eps":(StringPtr)"\p.txt");
		keepGoing = GetOutputName(MiscStringsID,EPSFile? 7:9,outname,&vref,&nscd);
		if (!keepGoing) return FALSE;
		
		rfSpec = nscd.nsFSSpec;
		
		saveOutputTo = outputTo;					/* Save state */
		outputTo = toPostScript;
		
		WaitCursor();
		theErr = PS_Open(doc, outname, vref, USING_FILE, EPSFile? 'EPSF':'TEXT',&rfSpec);
		if (theErr == noErr) {
			
			paperRect = doc->origPaperRect;
			
			PS_Header(doc,outname,EPSFile?1:doc->numSheets,1.0,FALSE,TRUE,&paperRect,&paperRect);
			
			if (EPSFile) {
				firstSheet = sheetNum;				/* Do current sheet only */
				topSheet = firstSheet + 1;
				}
			 else {
				firstSheet = 0;						/* Do all sheets */
				topSheet = doc->numSheets;
				}
			
			for (sheet=firstSheet; sheet<topSheet; sheet++) {
				PS_NewPage(doc,NULL,sheet+doc->firstPageNumber);
				DrawPageContent(doc, sheet, &paperRect, &doc->viewRect);
				PS_EndPage();
				}
			
			if (EPSFile) {
				/*
				 *	Write additional EPSF information: in the PostScript program itself, the
				 *	fonts used and the image's bounding box; in the resource fork of the same
				 *	file, a PICT of the page image.
				 */
				
				rgnHdl = NewRgn();					/* Save current clipping region */
				GetClip(rgnHdl);
				
				saveMagnify = doc->magnify;			/* Save current magnification, and reset to none */
				doc->magnify = 0;
		 		
		 		outputTo = toPICT;					/* Draw page contents into a PICT */
		 		ClipRect(&paperRect);
				picHdl = OpenPicture(&paperRect);
				DrawPageContent(doc, sheetNum, &paperRect, &doc->viewRect);
				FrameRect(&paperRect);
				ClosePicture();
				
				doc->magnify = saveMagnify;			/* Restore environment */
				InstallMagnify(doc);
				
				SetClip(rgnHdl);
				outputTo = toPostScript;
				}

			FillFontUsedTbl(doc);					/* Make list of font/style combinations used */
	
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
					sprintf(strBuf, fmtStr, PtoCstr(outname),ResError()); 
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					}
				 else {
					AddResource((Handle)picHdl, 'PICT', 256, "\p");
					CloseResFile(rfNum);
					UseResFile(appRFRefNum);				/* a precaution */
					}
				}
			}

		outputTo = saveOutputTo;
		ArrowCursor();

		return TRUE;
	}
	
static enum {
	BUT1_OK = 1,
	BUT2_Cancel,
	ICON3,
	RAD4_EPSF,
	RAD5_PostScript,
	STXT6_Specify
	} E_PSTypeItems;

static short group1;

static short PSTypeDialog(short oldType, short pageNum)
	{
		short itemHit,okay,type,keepGoing=TRUE;
		DialogPtr dlog; GrafPtr oldPort;
		ModalFilterUPP	filterUPP;
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
		dlog = GetNewDialog(PSTYPE_DLOG,NULL,BRING_TO_FRONT);
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

		PlaceWindow(GetDialogWindow(dlog),NULL,0,80);
		ShowWindow(GetDialogWindow(dlog));

		/* Entertain filtered user events until dialog is dismissed */
		
		while (keepGoing) {
			ModalDialog(filterUPP,&itemHit);
			switch(itemHit) {
				case BUT1_OK:
					keepGoing = FALSE;
					okay = (group1==RAD4_EPSF) ? 1 : 2;
					break;
				case BUT2_Cancel:
					keepGoing = FALSE;
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
