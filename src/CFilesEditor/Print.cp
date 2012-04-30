/***************************************************************************
FILE:	Print.c
PROJ:	Nightingale, rev. for v.3.5
DESC:	Routines for printing and for handling printing-related dialogs:
		PRChanged			DoPageSetup		AbortPrint
		SetupPrintHandle
		NumCopies			DoPrintScore	PrintImageWriter
		PrintLaserWriter	FillFontUsedTbl	DoPostScript
		PSTypeDialog
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

enum { bDevCItoh = 1, bDevDaisy, bDevLaser };	/* From ancient Apple header files */

static THPrint hPrint;
static Boolean aborted;					/* unused */

static INT16 origScale = 1.0,origLandScale = 1.0;
static FASTFLOAT scaleFactor = 1.0, landScaleFactor = 1.0;

static Point SFPwhere = { 106, 104 };	/* Where SFPutFile dialog should be */

FONTUSEDITEM	fontUsedTbl[MAX_SCOREFONTS];

/* Prototypes for private functions */

// MAS Boolean	PRChanged(THPrint pr1, THPrint pr2);
Boolean	AbortPrint(void);
Boolean	SetupPrintHandle(Document *doc);
INT16	NumCopies(void);
void	QD_StrEncrypted(unsigned char *, unsigned char *);
void	QD_Banner(Document *,unsigned char [],unsigned char [],unsigned char [],unsigned char []);
void	PrintDemoBanner(Document *, Boolean);
INT16	PrintImageWriter(Document *doc, INT16 firstSheet, INT16 lastSheet);
INT16	PrintLaserWriter(Document *doc, INT16 firstSheet, INT16 lastSheet);
void	FillFontUsedTbl(Document *doc);
INT16	PSTypeDialog(INT16, INT16);

/*
 *	Compare two print record handles and deliver TRUE if they are different.
 *	Assumes the two handles are unlocked.
 */

static Boolean PRChanged(THPrint hPrint, THPrint oldHPrint)
	{
		INT16 ans;
		
		/* Since we're saving the print record, see if user made any changes */
		
		HLock((Handle)hPrint);
		HLock((Handle)oldHPrint);
		ans = BlockCompare(*hPrint,*oldHPrint,sizeof(TPrint));
		HUnlock((Handle)hPrint);
		HUnlock((Handle)oldHPrint);
		
		return(ans != 0);
	}

/*
 *	Call the system Page Setup dialog, leaving hPrint containing proper values.
 *	The printer driver is left closed after this call, since there's no guarantee
 *	that the user is going to print after calling us here, and it is not good
 *	citizenship to leave it open.
 *
 *	N.B. PrOpen opens the printer resource file, which PrStlDialog apparently
 *	depends on, and PrClose might as well; hence we don't set the current resource
 *	file back to our default of the application resource file until after PrClose.
 *	Caveat.
 */

void DoPageSetup(Document *doc)
	{
		FASTFLOAT denom;
		Rect rPaper; TGetRotnBlk pData;
		THPrint oldHPrint;
		Boolean keephPrint;
		
		ArrowCursor();
		
		PrOpen();
				
		/*
		 * If we don't have a print record (indicated by doc->hPrint==NULL), we'll
		 * need to create one for Page Setup to use; but when we're through with Page
		 * Setup, we only want to keep it if it contains useful information, i.e.,
		 * we started out with one or a newly-created one has been changed from the
		 * default. Set a flag to tell us what to do.
		 */
		keephPrint = (doc->hPrint!=NULL);
		
		if (PrError()==noErr && SetupPrintHandle(doc)) {
		
			/* Duplicate print record so we can compare for changes below */
			oldHPrint = hPrint;
			HandToHand((Handle *)&oldHPrint);
			
			if (PrStlDialog(hPrint)) {
			
				/* Check denominator, since rPaper may not be defined if user cancels above */
				denom = (FASTFLOAT)((*hPrint)->rPaper.bottom-(*hPrint)->rPaper.top);
				scaleFactor = denom==0.0 ? 1.0 : ((FASTFLOAT)origScale / denom);
				denom = (FASTFLOAT)((*hPrint)->rPaper.bottom-(*hPrint)->rPaper.top);
				landScaleFactor = denom==0.0 ? 1.0 : ((FASTFLOAT)origLandScale / denom);
				
				/*
				 *	Now if doc->paperRect is different from rPaper (user has chosen different
				 *	size paper), we change to new size and inval everything.
				 */
				rPaper = (*hPrint)->rPaper;
				OffsetRect(&rPaper,-rPaper.left,-rPaper.top);
				if (!EqualRect(&doc->origPaperRect,&rPaper)) {
					INT16 marginRight = doc->origPaperRect.right - doc->marginRect.right;
					INT16 marginBottom = doc->origPaperRect.bottom - doc->marginRect.bottom;
					doc->marginRect.right = rPaper.right - marginRight;
					doc->marginRect.bottom = rPaper.bottom - marginBottom;
					doc->origPaperRect = rPaper;
					MagnifyPaper(&doc->origPaperRect,&doc->paperRect,doc->magnify);
					doc->changed = TRUE;
					doc->hPrint = hPrint;
					PrGeneral((Ptr)&pData);						/* Check landscape orientation */
					doc->landscape = (pData.fLandscape!=0);
					InvalSheetView(doc);
					}
				
				if (PRChanged(hPrint,oldHPrint))
					doc->changed = keephPrint = TRUE;
				}
				
			DisposeHandle((Handle)oldHPrint);
			}
			
		PrClose();
		
		if (!keephPrint) doc->hPrint = NULL;

		UseResFile(appRFRefNum);							/* a precaution */
	}

#ifdef NOMORE
/*
 *	Check to see if any errors or if user has asked to quit by typing Command-period.
 *	This routine is intended to be called repeatedly within the printing loop;
 *	currently (v.3.5), it's not called anywhere!
 */

static Boolean AbortPrint()
	{
		if (!aborted)
			if (PrError() || CheckAbort()) aborted = TRUE;
		return(aborted);
	}
#endif

/*
 *	Allocate a print record if we haven't already, and install default values.
 *	Deliver TRUE if okay, FALSE if problem.  After a call to this routine,
 *	doc->hPrint will only be NULL if there were problems.
 */

static Boolean SetupPrintHandle(Document *doc)
	{
		if (doc == NULL) return(FALSE);
		
		hPrint = doc->hPrint;
		if (hPrint == NULL) {
			if (hPrint = (TPrint **) NewHandle( sizeof( TPrint ))) {
				doc->hPrint = hPrint;
				PrintDefault(hPrint);
				if (PrError())
					{ DisposeHandle((Handle)hPrint); hPrint = NULL; }
				 else {
					origScale = (*hPrint)->rPaper.bottom-(*hPrint)->rPaper.top;
					origLandScale = (*hPrint)->rPaper.right-(*hPrint)->rPaper.left;
					(*hPrint)->prJob.iFstPage = doc->firstPageNumber;
					(*hPrint)->prJob.iLstPage = doc->firstPageNumber + doc->numSheets - 1;
					}
				}
			else
				NoMoreMemory();
			}
		
		return(hPrint != NULL);
	}

/*
 *	Deliver the number of copies that we should take care of, according to the
 *	current print record.
 */

static INT16 NumCopies()
	{
		return( ((*hPrint)->prJob.bJDocLoop==bDraftLoop) ? 
										(*hPrint)->prJob.iCopies : 1 );
	}

/*
 * Check that systems in a range of pages are all justified. If SysJustFact() is
 * significantly different from 1.0, tell user and give them a chance to
 * cancel. Return FALSE if user says stop, TRUE otherwise (all justified or user
 * says continue anyway). (It'd be nice some day to give them an opportunity to
 * justify right now.)
 */

#define JUSTSLOP .001

Boolean IsRightJustOK(Document *doc, INT16 firstSheet, INT16 lastSheet);
static Boolean IsRightJustOK(
					Document *doc,
					INT16 firstSheet, INT16 lastSheet)	/* Inclusive range of sheet numbers */
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


/* ---------------------------------------------------------------- DoPrintScore -- */

/*
 *	Print the given score on whatever the current printer is, as determined from
 *	the standard print dialog and print record information.
 */

void DoPrintScore(Document *doc)
	{
		GrafPtr		savePort;
		TPrStatus	prStatus;
		INT16		copies, saveOutputTo, saveMagnify, sheet, firstSheet, lastSheet;
		INT16		hiByte, spooling,rRefNum, err;
		Rect		frameRect,paperRect,paper,printRect,imageRect;
		THPrint		oldHPrint;
		Boolean		keephPrint;
		char		fmtStr[256];
		
		GetPort(&savePort);
		
		/* Open the printer driver, and make sure we've got a good print record.
		 * If we don't have a print record (indicated by doc->hPrint==NULL), we'll need
		 * to create one for print routines to use; but when we're through with Page
		 * Setup, we only want to keep it if it contains useful information, i.e.,
		 * we started out with one or a newly-created one has been changed from the
		 * default. Set a flag to tell us what to do.
		 */
		keephPrint = (doc->hPrint!=NULL);		
		PrOpen();
		if (PrError()!=noErr || !SetupPrintHandle(doc)) { PrClose(); goto Cleanup; }
		PrValidate(hPrint);
		if (PrError()!=noErr) { PrClose(); goto Cleanup; }
		/* Entertain standard Print dialog: close driver if user cancels */
		
		ArrowCursor();
		
		/* Duplicate print record so we can compare for changes below */
		oldHPrint = hPrint;
		HandToHand((Handle *)&oldHPrint);
		
		if (!PrJobDialog(hPrint)) { PrClose(); DisposeHandle((Handle)oldHPrint); goto Cleanup; }

#ifdef ONHOLD
		if (PRChanged(hPrint,oldHPrint))
			doc->changed = keephPrint = TRUE;
#endif
		DisposeHandle((Handle)oldHPrint);
		
		/* Get the sheet numbers to print, converting from page numbers, and make
			sure they're in forward order and properly defined. */
		
		firstSheet = (*hPrint)->prJob.iFstPage - doc->firstPageNumber;
		lastSheet  = (*hPrint)->prJob.iLstPage - doc->firstPageNumber;
		if (firstSheet > lastSheet)
			{ INT16 sh = firstSheet; firstSheet = lastSheet; lastSheet = sh; }
		if (firstSheet < 0) firstSheet = 0;
		if (lastSheet >= doc->numSheets) lastSheet = doc->numSheets-1;

		/* If the page numbers aren't in forward order now, they don't exist. */

		if (firstSheet > lastSheet) {
			GetIndCString(fmtStr, PRINTERRS_STRS, 3);    /* "No pages with numbers...exist..." */
			sprintf(strBuf, fmtStr, (*hPrint)->prJob.iFstPage, (*hPrint)->prJob.iLstPage); 
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			PrClose();
			goto Cleanup;
		}
		
		/*
		 * Check that systems to be printed are all justified; if not, tell user and
		 * give them a chance to cancel.
		 */
		if (config.printJustWarn)
			if (!IsRightJustOK(doc, firstSheet, lastSheet)) {
				PrClose();
				goto Cleanup;
			}

		(*hPrint)->prJob.iFstPage = 1;
		(*hPrint)->prJob.iLstPage = lastSheet-firstSheet+1;
		PrClose();
		UpdateAllWindows();
		
		imageRect = (*hPrint)->rPaper;
		PS_PreparePrintDict(doc,&imageRect);			/* Create 'PREC' 103 */
		
		PrOpen();
		
		/* Determine type of printer user has chosen */
		
		saveOutputTo = outputTo;
			
		/* Are we spooling or going directly to printer? */

		spooling = ((*hPrint)->prJob.bJDocLoop == bSpoolLoop);

		hiByte = (((*hPrint)->prStl.wDev) >> 8) & 0xFF;
		switch(hiByte) {
			case bDevLaser:
				outputTo = toPostScript;
				break;
			case bDevCItoh:
			default:									/* Perhaps Preview */
				outputTo = toImageWriter;
				/*
				 * Flag whether user wants Best Quality Print, so Draw routines
				 * can compensate for the screen font size used. Apple won't tell
				 * us how you can tell if it's Best Quality, but I think this
				 * is right...but what if they change it?? Not very satisfying.
				 */
				bestQualityPrint = (((*hPrint)->prStl.wDev) & 0x01)==1 && spooling;
				break;
			}

		WaitCursor();
		for (copies=NumCopies(); copies>0; copies--) {
			switch(outputTo) {
				case toImageWriter:
					if (!PrintImageWriter(doc,firstSheet,lastSheet)) copies = -1;
					break;
				case toPostScript:
					if (!PrintLaserWriter(doc,firstSheet,lastSheet)) copies = -1;
					break;
				}
			if (spooling) {
				/* Should unload segments here to make as much room as possible */
				PrPicFile(hPrint, NULL, NULL, NULL, &prStatus);
				err = PrError();
				if (err!=noErr && err!=iPrAbort)
					MayErrMsg("DoPrintScore: after PrPicFile, PrError()=%ld.",
							(long)err);
				/* Drop out to PrClose even if PrError() */
				}
			}
		
		/* All done */

		PrClose();
		
		outputTo = saveOutputTo;
		SetPort(savePort);

Cleanup:
		if (!keephPrint) doc->hPrint = NULL;
		PS_FinishPrintDict();
	}


/* --------------------------------------------------------------- DrawPatString -- */
/* Draw the given string in the given Pattern. */

#define MAX_WIDTH 640			/* Of a string (pixels) */
#define MAX_HEIGHT 100			/* Of a string (pixels) */

#define TOP_OFFSET 10
#define BOTTOM_OFFSET 10
#define WIDTH_OFFSET 20

void DrawPatString(unsigned char *, Pattern);
void DrawPatString(unsigned char *str,
					Pattern pattern)		/* Normally <gray>, less often <black> */
{
	INT16 charHt, strWidth;
	Point pt; Rect srcRect, dstRect, r;
	GrafPtr oldPort, strPort; FontInfo fInfo;
	RgnHandle rgn;
	
	if (pattern.pat==qd.black.pat) {
		DrawString(str);
		return;
	}

	else {
		/*
		 * Create an off-screen drawing environment; then switch ports to
		 * draw into it, but using the same font as the screen port.
		 */
		
		strPort = NewGrafPort(MAX_WIDTH, MAX_HEIGHT);
		if (!strPort) return;

		GetPen(&pt);

		GetFontInfo(&fInfo);

		GetPort(&oldPort);
		SetPort(strPort);
		TextFont(oldPort->txFont);
		TextSize(oldPort->txSize);
		TextFace(oldPort->txFace);

		/*
		 * Erase our off-screen bitmap, then draw the string into it.
		 */
		SetRect(&r, 0, 0, MAX_WIDTH-1, MAX_HEIGHT-1);
		EraseRect(&r);
		
		MoveTo(0, fInfo.ascent+TOP_OFFSET);
		DrawString(str);
		
		/*
		 * Overwrite black pixels with a gray pattern, then copy the result
		 * back into the original port. We set the clipping region for copying
		 * it back on the assumption the original port has standard horiz. and
		 * vertical scrollbars.
		 */
		
		strWidth = StringWidth(str)+WIDTH_OFFSET;
		charHt = fInfo.ascent+fInfo.descent+fInfo.leading;
		SetRect(&srcRect, 0, 0, strWidth, charHt);
		
		srcRect.top -= TOP_OFFSET;
		srcRect.bottom += BOTTOM_OFFSET;

		/*
		 * The destination rect is the same size as the source, but positioned so
		 * its lower (not upper--hence the offset involves <txSize>) left corner is
		 * at the current pen position.
		 */
		dstRect = srcRect;
		OffsetRect(&dstRect, pt.h, pt.v-oldPort->txSize);

		OffsetRect(&srcRect, 0, TOP_OFFSET);

		PenMode(notPatBic);
		PenPat(&pattern);
		PaintRect(&srcRect);

		rgn = oldPort->visRgn;
		(*rgn)->rgnBBox.bottom -= SCROLLBAR_WIDTH;
		(*rgn)->rgnBBox.right -= SCROLLBAR_WIDTH;

		/*
		 * If we're drawing on the screen, it doesn't seem to matter, but for printing,
		 * we MUST make the printer port the current port before CopyBits!
		 */
		
		SetPort(oldPort);
		CopyBits(&strPort->portBits, &oldPort->portBits, &srcRect, &dstRect,
							srcOr, rgn);

		(*rgn)->rgnBBox.bottom += SCROLLBAR_WIDTH;
		(*rgn)->rgnBBox.right += SCROLLBAR_WIDTH;

		DisposGrafPort(strPort);

		pt.h += StringWidth(str);
		MoveTo(pt.h, pt.v);
	}
}


static void QD_StrEncrypted(unsigned char *string, unsigned char *key)
{
   	short i,k,len,klen;
   	
	/*
	 *  Decrypt text string, draw it, and encrypt again. Both decrypting and
	 *	encrypting are done with a simple XOR, two applications of which leave
	 *  the string in the same state it was in before.
	 */
	klen = *key; len = *string;
	for (k=1, i=1; i<=len; i++) {
		string[i] ^= key[k++];
		if (k > klen) k = 1;
	}
	
	DrawPatString(string, qd.gray);
	
	for (k=1, i=1; i<=len; i++) {
		string[i] ^= key[k++];
		if (k > klen) k = 1;
	}
}


void QD_Banner(
		Document *doc,				/* Used for testing only */
		unsigned char *banKey,
		unsigned char *banLine1,
		unsigned char *banLine2,
		unsigned char *banLine3)
{
#ifdef DEMO_VERSION

	INT16 dx=0, dy=0;

	/*
	 * It'd be nice to draw this text diagonally, as we do in PostScript, but
	 * QuickDraw doesn't make it easy. And there are more important things to do.
	 */

	TextFont(textFontNum);
	TextFace(0);									/* Plain */

#ifdef SCREEN_TEST
	dx = doc->sheetOrigin.h;
	dy = doc->sheetOrigin.v;
#endif
	TextSize(48);
	MoveTo(30+dx, 300+dy);
	QD_StrEncrypted(banLine1, banKey);
 	
	TextSize(44);
	MoveTo(30+dx, 370+dy);
	QD_StrEncrypted(banLine2, banKey);
	
	TextSize(36);
	MoveTo(10+dx, 430+dy);
	QD_StrEncrypted(banLine3, banKey);
#endif
}


static unsigned char *banKey = "\pK&BF[3%";  /* Encryption key for banner text */
   /* NB: Don't change this without re-encrypting banLine1 below in PrintDemoBanner! */

/*
 *	If this is a demo version, print the "demo version" banner across the page; else
 *	do nothing. Should be called in the page loop just before DrawPageContent (for
 *	PostScript, after dropping out of QuickDraw, i.e., calling PS_OutOfQD).
 */

#define NotDOING_INITIAL_ENCODE

static void PrintDemoBanner(Document *doc, Boolean toPostScript)
{
#ifdef DOING_INITIAL_ENCODE
	unsigned char *banLine1 = "\pPrinted by Nightingale(R)";
	unsigned char *banLine2 = "\pAdvanced Music Notation Systems, Inc.";
	unsigned char *banLine3 = "\pfax: 413-268-7317  e-mail: info@ngale.com";
#else
	unsigned char *banLine1 = "\p\33T+(/VAkD;f\25ZB#R+(<RI.Ž";  /* Ends in Option-e e */
	unsigned char *banLine2 = "\p\37C/64AD'\6\3%.ZQ2\6\02244WP(R1";
	unsigned char *banLine3 = "\ps\26rko\1\23f\24tqh\23\5k\6btk\5\b\177\20pkj\3\25|";
#endif
	short i,k,len,klen;
 
#ifdef DOING_INITIAL_ENCODE
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

#ifdef DEMO_VERSION
	if (toPostScript)
		PS_Banner(banKey, banLine1, banLine2, banLine3);
	else
		QD_Banner(doc, banKey, banLine1, banLine2, banLine3);
#endif
}


/*
 *	Print the given sheets from the given document to the ImageWriter.  Deliver
 *	TRUE if all OK, FALSE if PrError() says an error occurred.
 */

static INT16 PrintImageWriter(Document *doc, INT16 firstSheet, INT16 lastSheet)
	{
		Rect 		printRect,paper;
		TPPrPort	printPort;
		INT16		sheet;
	
		printPort = PrOpenDoc(hPrint, NULL, NULL);
		if (PrError()) { PrCloseDoc(printPort); return(FALSE); }
		
		printRect = (**hPrint).prInfo.rPage;
		paper = doc->paperRect;
		
		for (sheet=firstSheet; sheet<=lastSheet; sheet++) {
		
			PrOpenPage(printPort, NULL);
			if (PrError()) { PrClosePage(printPort); break; }
			
			PrintDemoBanner(doc, FALSE);
			DrawPageContent(doc, sheet, &paper, &doc->viewRect);
			
			PrClosePage(printPort);
			if (PrError()) break;
			}
		PrCloseDoc(printPort);
		
		return(sheet > lastSheet);						/* TRUE if got through whole loop */
	}

/*
 *	Print the given sheets from the given document on the LaserWriter or other
 *	PostScript printer.  Return TRUE if all OK, FALSE if an error occurred, either
 *	according to PrError() or otherwise.
 *	This code goes through the standard PrintMgr via PicComments.
 */

static INT16 PrintLaserWriter(Document *doc, INT16 firstSheet, INT16 lastSheet)
	{
		Rect 		printRect,frameRect,imageRect,paper;
		TPPrPort	printPort;
		TGetRotnBlk	pData;
		INT16		sheet;

		printPort = PrOpenDoc(hPrint, NULL, NULL);					/* Sets port */
		if (PrError()) { PrCloseDoc(printPort); return(FALSE); }

		/*
		 * Get physical size of paper (topleft should be (0,0)) and imageable area
		 * (topleft should be something like (-31,-30))
		 */	
		printRect = (**hPrint).prInfo.rPage;
		imageRect = (*hPrint)->rPaper;

		paper = doc->paperRect;
		frameRect = doc->marginRect;
		
		pData.hPrint = hPrint;
		pData.iOpCode = getRotnOp;
		PrGeneral((Ptr)&pData);									/* Check landscape orientation */
		if (pData.fLandscape) scaleFactor = landScaleFactor;
		
		if (PS_Open(doc,NULL,0,USING_PRINTER,'????') == noErr) {

			for (sheet=firstSheet; sheet<=lastSheet; sheet++) {

				PrOpenPage(printPort, NULL);
				if (PrError()) { PrClosePage(printPort); break; }
				
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
				
				PS_NewPage(doc,NULL,sheet+doc->firstPageNumber);
				
				PS_OutOfQD(doc,TRUE,&imageRect);
				PrintDemoBanner(doc, TRUE);
				DrawPageContent(doc, sheet, &paper, &doc->currentPaper);
				PS_IntoQD(TRUE);
				
				PS_EndPage();
				
				PrClosePage(printPort);
				if (PrError()) break;
				}
			PS_Close();
			}
		else {
			NoMoreMemory();
			sheet = lastSheet;							/* Force return value of FALSE */
			}

		PrCloseDoc(printPort);
		return(sheet > lastSheet);				/* TRUE if got through loop successfully */
	}


/*
 *	Go thru the entire data structure and make a list of what font/style comb-
 *	inations are actually used.
 */

static void FillFontUsedTbl(Document *doc)
	{
		INT16	j, k, styleBits;
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

/*
 *	Save the current page of the given document as a PostScript file specified by the user.
 *	If EPSFile is TRUE, the file will be an Encapsulated PostScript File, where the data
 *	fork will contain the Encapsulated PostScript code to draw the current page, and the
 *	resource fork will have a PICT resource containing a picture of the page as drawn on
 *	the screen.
 *	If EPSFile is FALSE, the file will contain our own self-contained PostScript for
 *	drawing the entire document.  This is not Encapsulated PostScript, since more than
 *	one page may be output, and since there is no associated PICT.  It is a pure text
 *	file suitable for downloading to any PostScript printer.
 */
 
#ifdef DEMO_VERSION

Boolean DoPostScript(Document *doc)
{
	GetIndCString(strBuf, PRINTERRS_STRS, 1);    /* "Sorry, this demo version of Nightingale can't Save PostScript." */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
}

#else

Boolean DoPostScript(Document *doc)
	{
		INT16			saveOutputTo, saveMagnify, sheet, sufIndex;
		INT16			len, vref, rfNum, suffixLen, ch, firstSheet, topSheet;
		INT16			newType, anInt, pageNum, sheetNum;
		Str255			outname,prompt;
		PicHandle		picHdl;
		RgnHandle		rgnHdl;
		Rect			paperRect;
		SFReply			reply;
		INT16			EPSFile;
		static INT16	type=1;
		char			fmtStr[256];
		
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
		
		GetIndString(prompt, MiscStringsID, EPSFile? 7:9);
		SFPutFile(SFPwhere, prompt, outname, NULL, &reply);
		if (!reply.good) return FALSE;
		PStrCopy(reply.fName, (StringPtr)outname);
		vref = reply.vRefNum;
		
		saveOutputTo = outputTo;					/* Save state */
		outputTo = toPostScript;
		
		WaitCursor();
		if (PS_Open(doc, outname, vref, USING_FILE, EPSFile? 'EPSF':'TEXT') == noErr) {
		
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
				
				CreateResFile(outname);
				rfNum = OpenResFile((unsigned char *)StripAddress(outname));
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

#endif

/*
 *	Get type of PostScript file user wants.  We use a dialog with an ICON 1 in it to
 *	simulate an alert.  Return 0 for cancel, 1 for EPS file, 2 for full PostScript.
 */

/* Symbolic Dialog Item Numbers */

static enum {
	BUT1_OK = 1,
	BUT2_Cancel,
	ICON3,
	RAD4_EPSF,
	RAD5_PostScript,
	STXT6_Specify
	};

static INT16 group1;

static INT16 PSTypeDialog(INT16 oldType, INT16 pageNum)
	{
		INT16 itemHit,okay,type,keepGoing=TRUE;
		DialogPtr dlog; GrafPtr oldPort;
		ModalFilterUPP	filterUPP;
		Handle hndl;
		Rect box;
		char fmtStr[256], str[256];

		/* Build dialog window and install its item values */
		
		filterUPP = NewModalFilterProc(OKButFilter);
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
		SetPort(dlog);

		/* Change title of "EPSF" checkbox to reflect page number. */
		GetIndCString(fmtStr, MiscStringsID, 15);	/* "EPS file for page %d only" */
		sprintf(str, fmtStr, pageNum);
		CtoPString(str);
		GetDialogItem(dlog, RAD4_EPSF, &type, &hndl, &box);
		SetControlTitle((ControlHandle)hndl, (StringPtr)str);

		group1 = (oldType==1? RAD4_EPSF : RAD5_PostScript);
		PutDlgChkRadio(dlog, RAD4_EPSF, (group1==RAD4_EPSF));
		PutDlgChkRadio(dlog, RAD5_PostScript, (group1==RAD5_PostScript));

		PlaceWindow(dlog,NULL,0,80);
		ShowWindow(dlog);

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
