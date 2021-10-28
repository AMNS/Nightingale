/* FileUtils.c - File utility routines for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "CarbonPrinting.h"

/*  Created by Michel Alexandre Salim on 2/4/08.FIXME: This gets a compiler warning
"'#' flag used with '%s' printf format", and for years it's been called only in
MidiMap.c. Is that why Score Info always says "No midiMap"? */

FILE *FSpOpenInputFile(Str255 macfName, FSSpec *fsSpec)
{
	OSErr	result;
	char	ansifName[256];
	FILE	*f;

	result = HSetVol(NULL,fsSpec->vRefNum,fsSpec->parID);
	if (result!=noErr) goto err;
	
	sprintf(ansifName, "%#s", (char *)macfName);
	f = fopen(ansifName, "r");
	if (f==NULL) {
		/* fopen puts error code in errno, but this is of type int. */
		
		result = errno;
		goto err;
	}
	
	return f;
err:
	ReportIOError(result, OPENTEXTFILE_ALRT);
	return NULL;
}


/* ---------------------------------------------------------- MissingFontsDialog et al -- */

static short MFDrawLine(unsigned char *);
static void MissingFontsDialog(Document *, short);

#define LEADING 15			/* Vertical distance between lines displayed (pixels) */

static Rect textRect;
static short linenum;

/* Draw the specified Pascal string in quotes on the next line */

static short MFDrawLine(unsigned char *s)
{
	MoveTo(textRect.left, textRect.top+linenum*LEADING);
	DrawChar('"');
	DrawString(s);
	DrawChar('"');
	return ++linenum;
}

#define TEXT_DI 4

static void MissingFontsDialog(Document *doc, short nMissing)
{
	DialogPtr dialogp;
	GrafPtr oldPort;
	short ditem, anInt;
	Handle aHdl;
	 
	GetPort(&oldPort);
	dialogp = GetNewDialog(MISSINGFONTS_DLOG, NULL, BRING_TO_FRONT);
	if (dialogp) {
		SetPort(GetDialogWindowPort(dialogp));
		sprintf(strBuf, "%d", nMissing);
		CParamText(strBuf, "", "", "");
		GetDialogItem(dialogp, TEXT_DI, &anInt, &aHdl, &textRect);
	
		CenterWindow(GetDialogWindow(dialogp), 60);
		ShowWindow(GetDialogWindow(dialogp));
						
		ArrowCursor();
		OutlineOKButton(dialogp, True);
	
		/* List the missing fonts. */

		linenum = 1;
		for (short j = 0; j<doc->nfontsUsed; j++)
			if (doc->fontTable[j].fontID==applFont) {
				MFDrawLine(doc->fontTable[j].fontName);
			}
	
		ModalDialog(NULL, &ditem);								/* Wait for OK */
		HideWindow(GetDialogWindow(dialogp));
		
		DisposeDialog(dialogp);									/* Free heap space */
		SetPort(oldPort);
	}
	else
		MissingDialog(MISSINGFONTS_DLOG);
}


/* --------------------------------------------------------------------- FillFontTable -- */
/* Fill score header's fontTable, which maps internal font numbers to system font
numbers for the computer we're running on so we can call TextFont. */

void FillFontTable(Document *doc)
{
	short j, nMissing;
	
	if (doc->nfontsUsed>MAX_SCOREFONTS || doc->nfontsUsed<0)
		MayErrMsg("FillFontTable: %ld fonts is an illegal number.", (long)doc->nfontsUsed);

	EnumerateDocumentFonts(doc);	

	for (nMissing = j = 0; j<doc->nfontsUsed; j++) {
		if (doc->fontTable[j].fontID<0) {
			nMissing++;
			doc->fontTable[j].fontID = applFont;				/* Use the default */
		}
	}

	if (nMissing!=0)
		MissingFontsDialog(doc, nMissing);
}

// Really format handle

void PrintHandleError(short err)
{
	char fmtStr[256];
	
	GetIndCString(fmtStr, FILEIO_STRS, 2);			/* "Can't get Page Setup (error ID=%d)" */
	sprintf(strBuf, fmtStr, err);
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);	
}

/* For the given document, try to open its resource fork, and set the document's
docPageFormat field to either the previously saved print record if one is there, or NULL
if there is no resource fork or we can't get the print record for some other reason.  It
is the responsibility of other routines to check doc->docPageFormat for NULLness or not,
since we don't want to open the Print Driver every time we open a file (which is when
this routine should be called) in order to create a default print handle. The document's
resource fork, if there is one, is left closed. */

void GetPrintHandle(Document *doc, unsigned long /*version*/, short /*vRefNum*/, FSSpec *pfsSpec)
	{
		short refnum;
		OSStatus err = noErr;
		
		FSpOpenResFile(pfsSpec, fsRdWrPerm);	/* Open the resource file */	
		
		err = ResError();
		if (err==noErr) {
			refnum = CurResFile();
		}
		else {
			doc->flatFormatHandle = NULL;
			if (err!=eofErr && err!=fnfErr) {
				PrintHandleError(err);
			}				
			return;
		}
	
		doc->flatFormatHandle = (Handle)Get1Resource('PFMT', 128);
		
		/* Ordinarily, we'd now call ReportBadResource, which checks if the new Handle
		   is NULL as well as checking ResError. But in this case, we don't want to report
		   an error if it's NULL--or do we? */
		   
		err = ResError();
		if (err==noErr) {
			if (doc->flatFormatHandle) {
				LoadResource(doc->flatFormatHandle);			/* Guarantee that it's in memory */
				err = ResError();
				if (err==noErr) {	
					DetachResource(doc->flatFormatHandle);
					HNoPurge(doc->flatFormatHandle);
				}
			}
		}
		
		/* Close the resource file we just opened */
		
		CloseResFile(refnum);
		
		if (err == noErr) {				
			LoadAndUnflattenPageFormat(doc);
		}
		else {
			if (doc->flatFormatHandle != NULL) {
				DisposeHandle(doc->flatFormatHandle);
				doc->flatFormatHandle = NULL;
			}
			if (err!=resNotFound) {
				PrintHandleError(err);
			}
		}
	}


/* ------------------------------------------------------------------ WritePrintHandle -- */
/* Save a copy of the document's print record (if any) in the resource fork of the
score file.  The resource fork may already exist or it may not. Should be called when
saving a document.

Leaves the resource fork closed and returns True if all went well, False if not. */

Boolean WritePrintHandle(Document *doc)
{
	short err = noErr;
	
	if (THIS_FILE_VERSION<'N104') {
		/* There's nothing to write, but we should never get here anyway. */
		
		doc->flatFormatHandle = NULL;
		return True;
	}
	else {
		short refnum, dummy;
		
		/* Store the PMPageFormat in a handle. */
		
		FlattenAndSavePageFormat(doc);
		
		if (doc->flatFormatHandle) {
			FSSpec fsSpec = doc->fsSpec;
			HSetVol(NULL, fsSpec.vRefNum, fsSpec.parID);
			FSpCreateResFile(&fsSpec, creatorType, documentType, smRoman);
			refnum = FSpOpenResFile(&fsSpec, fsRdWrPerm);
			err = ResError();

			if (err == noErr) {

				/* We want to add the data in the print record to the resource file
				   but leave the record in memory independent of it, so we add it
				   to the resource file, write out the file, detach the handle from
				   the resource, and close the file. Before doing any of this, we
				   RemoveResource the print record; this should be unnecessary, since
				   we DetachResource print records after reading them in, but we're
				   playing it safe. */
				 
				dummy = GetResAttrs(doc->flatFormatHandle);
				if (!ResError()) RemoveResource(doc->flatFormatHandle);

				AddResource(doc->flatFormatHandle, 'PFMT', 128, "\pPMPageFormat");
				err = ResError();
				UpdateResFile(refnum);
				DetachResource(doc->flatFormatHandle);
				CloseResFile(refnum);
				UseResFile(appRFRefNum);						/* a precaution */
			}
			else {
				PrintHandleError(err);
			}		
		}
	}
	return (err == noErr);
}


/* ======================================= Utilities to display and check file headers == */

/* ---------------------------------------------------- Utilities for Document headers -- */

#define in2d(x)	pt2d(in2pt(x))		/* Convert inches to DDIST */
#define ERR(fn) { nerr++; LogPrintf(LOG_WARNING, "err #%d, ", fn); if (firstErr==0) firstErr = fn; }

/* Display almost all Document header fields; there are about 18 in all. */

void DisplayDocumentHdr(short id, Document *doc)
{
	LogPrintf(LOG_INFO, "Displaying Document header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  (1)origin.h=%d", doc->origin.h);
	LogPrintf(LOG_INFO, "  (2).v=%d", doc->origin.v);
	
	LogPrintf(LOG_INFO, "  (3)paperRect.top=%d", doc->paperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->paperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->paperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->paperRect.right);
	LogPrintf(LOG_INFO, "  (4)origPaperRect.top=%d", doc->origPaperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->origPaperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->origPaperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->origPaperRect.right);

	LogPrintf(LOG_INFO, "  (5)marginRect.top=%d", doc->marginRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->marginRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->marginRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->marginRect.right);

	LogPrintf(LOG_INFO, "  (6)currentSheet=%d", doc->currentSheet);
	LogPrintf(LOG_INFO, "  (7)numSheets=%d", doc->numSheets);
	LogPrintf(LOG_INFO, "  (8)firstSheet=%d", doc->firstSheet);
	LogPrintf(LOG_INFO, "  (9)firstPageNumber=%d", doc->firstPageNumber);
	LogPrintf(LOG_INFO, "  (10)startPageNumber=%d\n", doc->startPageNumber);
	
	LogPrintf(LOG_INFO, "  (11)numRows=%d", doc->numRows);
	LogPrintf(LOG_INFO, "  (12)numCols=%d", doc->numCols);
	LogPrintf(LOG_INFO, "  (13)pageType=%d", doc->pageType);
	LogPrintf(LOG_INFO, "  (14)measSystem=%d\n", doc->measSystem);
	
	LogPrintf(LOG_INFO, "  (15)headerFooterMargins.top=%d", doc->headerFooterMargins.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->headerFooterMargins.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->headerFooterMargins.bottom);
	LogPrintf(LOG_INFO, "  .right=%d", doc->headerFooterMargins.right);
	LogPrintf(LOG_INFO, "  (16)littleEndian=%d\n", doc->littleEndian);
}


/* Do a reality check for Document header values that might be bad. If any problems are
found, say so. Return the number of errors found. We assume we're called as soon as the
Document header is read. NB: We're not checking anywhere near everything we could! */

short CheckDocumentHdr(Document *doc, short *pFirstErr)
{
	short nerr = 0, firstErr = 0;
	
#ifdef NOTYET
	/* paperRect considers magnification. This is difficult to handle before <doc> is
	   fully installed and ready to draw, so skip checking for now. --DAB, April 2020 */
	   
	if (!RectIsValid(doc->paperRect, pt2p(0), pt2p(in2pt(12)))) ERR(3);
#endif
	if (!RectIsValid(doc->origPaperRect, 0, in2pt(12)))  ERR(4);
	if (!RectIsValid(doc->marginRect, 4, in2pt(12)))  ERR(5);
	if (doc->numSheets<1 || doc->numSheets>400)  ERR(7);
	if (doc->firstPageNumber<0 || doc->firstPageNumber>400)  ERR(9);
	if (doc->startPageNumber<0 || (doc->startPageNumber>400 && doc->startPageNumber!=SHRT_MAX))
		ERR(10);
	if (doc->numRows < 1 || doc->numRows > 250)  ERR(11);
	if (doc->numCols < 1 || doc->numCols > 250)  ERR(12);
	if (doc->pageType < 0 || doc->pageType > 20)  ERR(13);
	*pFirstErr = firstErr;
	return nerr;
}


/* ------------------------------------------------------- Utilities for Score headers -- */

/* Display some Score header fields, but nowhere near all. There are around 200 fields;
about half give information about fonts. */

void DisplayScoreHdr(short id, Document *doc)
{
	unsigned char tempFontName[32];
	
	LogPrintf(LOG_INFO, "Displaying Score header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  (1)nstaves=%d", doc->nstaves);
	LogPrintf(LOG_INFO, "  (2)nsystems=%d", doc->nsystems);
	LogPrintf(LOG_INFO, "  (3)noteInsFeedback=%d", doc->noteInsFeedback);
	LogPrintf(LOG_INFO, "  (4)used=%d", doc->used);
	LogPrintf(LOG_INFO, "  (5)transposed=%d\n", doc->transposed);
	
	LogPrintf(LOG_INFO, "  (6)polyTimbral=%d", doc->polyTimbral);
	LogPrintf(LOG_INFO, "  (7)spacePercent=%d", doc->spacePercent);
	LogPrintf(LOG_INFO, "  (8)srastral=%d", doc->srastral);	
	LogPrintf(LOG_INFO, "  (9)altsrastral=%d\n", doc->altsrastral);
		
	LogPrintf(LOG_INFO, "  (10)tempo=%d", doc->tempo);
	LogPrintf(LOG_INFO, "  (11)channel=%d", doc->channel);
	LogPrintf(LOG_INFO, "  (12)velocity=%d", doc->velocity);
	LogPrintf(LOG_INFO, "  (13)headerStrOffset=%d", doc->headerStrOffset);
	LogPrintf(LOG_INFO, "  (14)footerStrOffset=%d\n", doc->footerStrOffset);
	
	LogPrintf(LOG_INFO, "  (15)dIndentOther=%d", doc->dIndentOther);
	LogPrintf(LOG_INFO, "  (16)firstNames=%d", doc->firstNames);
	LogPrintf(LOG_INFO, "  (17)otherNames=%d", doc->otherNames);
	LogPrintf(LOG_INFO, "  (18)lastGlobalFont=%d\n", doc->lastGlobalFont);

	LogPrintf(LOG_INFO, "  (19)aboveMN=%c", (doc->aboveMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  (20)sysFirstMN=%c", (doc->sysFirstMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  (21)startMNPrint1=%c", (doc->startMNPrint1? '1' : '0'));
	LogPrintf(LOG_INFO, "  (22)firstMNNumber=%d\n", doc->firstMNNumber);

	LogPrintf(LOG_INFO, "  (23)nfontsUsed=%d", doc->nfontsUsed);
	LogPrintf(LOG_INFO, "  (24)fontTable[0].fontID=%d", doc->fontTable[0].fontID);	
	Pstrcpy(tempFontName, doc->musFontName);
	LogPrintf(LOG_INFO, "  (25)musFontName='%s'\n", PtoCstr(tempFontName));
	
	LogPrintf(LOG_INFO, "  (26)magnify=%d", doc->magnify);
	LogPrintf(LOG_INFO, "  (27)selStaff=%d", doc->selStaff);
	LogPrintf(LOG_INFO, "  (28)currentSystem=%d", doc->currentSystem);
	LogPrintf(LOG_INFO, "  (29)spaceTable=%d", doc->spaceTable);
	LogPrintf(LOG_INFO, "  (30)htight=%d\n", doc->htight);
	
	LogPrintf(LOG_INFO, "  (31)lookVoice=%d", doc->lookVoice);
	LogPrintf(LOG_INFO, "  (32)ledgerYSp=%d", doc->ledgerYSp);
	LogPrintf(LOG_INFO, "  (33)deflamTime=%d", doc->deflamTime);
	LogPrintf(LOG_INFO, "  (34)colorVoices=%d", doc->colorVoices);
	LogPrintf(LOG_INFO, "  (35)dIndentFirst=%d\n", doc->dIndentFirst);
}


/* Do a reality check for Score header values that might be bad. If any problems are
found, say so. Return the number of errors found. NB: We're not checking anywhere near
everything we could! But we assume we're called as soon as the score header is read,
and before the stringPool size is, so we can't fully check numbers that depend on that. */

short CheckScoreHdr(Document *doc, short *pFirstErr)
{
	short nerr = 0, firstErr = 0;
	
	if (doc->nstaves<1 || doc->nstaves>MAXSTAVES) ERR(1);
	if (doc->nsystems<1 || doc->nsystems>2000) ERR(2);
	if (doc->spacePercent<MINSPACE || doc->spacePercent>MAXSPACE) ERR(7);
	if (doc->srastral<0 || doc->srastral>MAXRASTRAL) ERR(8);
	if (doc->altsrastral<1 || doc->altsrastral>MAXRASTRAL) ERR(9);
	if (doc->tempo<MIN_BPM || doc->tempo>MAX_BPM) ERR(10);
	if (doc->channel<1 || doc->channel>MAXCHANNEL) ERR(11);
	if (doc->velocity<-127 || doc->velocity>127) ERR(12);

	/* We can't check if headerStrOffset and footerStrOffset are too large: we don't
	   know the size of the stringPool yet. */
	   
	if (doc->headerStrOffset<0) ERR(13);
	if (doc->footerStrOffset<0) ERR(14);
	
	if (doc->dIndentOther<0 || doc->dIndentOther>in2d(5)) ERR(15);
	if (doc->firstNames<0 || doc->firstNames>MAX_NAMES_TYPE) ERR(16);
	if (doc->otherNames<0 || doc->otherNames>MAX_NAMES_TYPE) ERR(17);
	if (doc->lastGlobalFont<FONT_THISITEMONLY || doc->lastGlobalFont>MAX_FONTSTYLENUM) ERR(18);
	if (doc->firstMNNumber<0 || doc->firstMNNumber>MAX_FIRSTMEASNUM) ERR(22);
	if (doc->nfontsUsed<1 || doc->nfontsUsed>MAX_SCOREFONTS) ERR(23);
	if (doc->magnify<MIN_MAGNIFY || doc->magnify>MAX_MAGNIFY) ERR(26);
	if (doc->selStaff<-1 || doc->selStaff>doc->nstaves) ERR(27);
	if (doc->currentSystem<1 || doc->currentSystem>doc->nsystems) ERR(28);
	if (doc->spaceTable<0) ERR(29);
	if (doc->htight<MINSPACE || doc->htight>MAXSPACE) ERR(30);
	if (doc->lookVoice<-1 || doc->lookVoice>MAXVOICES) ERR(31);
	if (doc->ledgerYSp<0 || doc->ledgerYSp>40) ERR(32);
	if (doc->deflamTime<1 || doc->deflamTime>1000) ERR(33);
	if (doc->dIndentFirst<0 || doc->dIndentFirst>in2d(5)) ERR(35);
	
	*pFirstErr = firstErr;
	return nerr;
}


/* ---------------------------------------------------------------------- NOpenBMPFile -- */
/* Open the given BMP file. If we succeed, return a pointer to it, with parameters set to
the offset from the beginning of the file to the pixel array and the width and height of
the image; if we fail, return NULL. */

FILE *NOpenBMPFile(char *filename, long *pixOffset, short *pWidth, short *pByteWidth,
		short *pByteWidthWithPad, short *pHeight)
{
	FILE *bmpf;
	BMPFileHeader fileHdr;
	BMPInfoHeader infoHdr;
	short nRead, byteWidth, n4BChunks;
	char signature[2];
	char detailStr[256];
	
	LogPrintf(LOG_INFO, "Opening BMP file '%s'...  (NOpenBMPFile)\n", filename);

	/* Open the file and read the BMP header.  */
	
#if 13
char cwd[PATH_MAX];
if (getcwd(cwd, sizeof(cwd)) != NULL)
   LogPrintf(LOG_DEBUG, "Current working dir: %s\n", cwd);
else
   LogPrintf(LOG_DEBUG, "getcwd() error\n");
#endif

	errno = 0;
	bmpf = fopen((const char *)filename, "r");
	if (!bmpf) {
		strcpy(detailStr, " ");
		if (errno==ENOENT) strcpy(detailStr, "(No such file) ");
		LogPrintf(LOG_ERR, "Can't open bitmap image file '%s'. errno=%d %s (NOpenBMPFile)\n",
							filename, errno, detailStr);
		return NULL;
	}
	nRead = fread(&signature[0], 2, 1, bmpf);
	if (signature[0]!='B' || signature[1]!='M') {
		LogPrintf(LOG_ERR, "BMP file doesn't start with the 'BM' signature.  (NOpenBMPFile)\n");
		return NULL;
	}

	nRead = fread(&fileHdr, sizeof(BMPFileHeader), 1, bmpf);
	if (nRead!=1) {
		LogPrintf(LOG_ERR, "Couldn't read the BMP file header.  (NOpenBMPFile)\n");
		return NULL;
	}
	EndianFixBMPFileHdr(&fileHdr);
//LogPrintf(LOG_DEBUG, "fileSize=%u offsetToPixelArray=%u  (NOpenBMPFile)\n",
//				fileHdr.fileSize, fileHdr.offsetToPixelArray);

	nRead = fread(&infoHdr, sizeof(BMPInfoHeader), 1, bmpf);
	if (nRead!=1) {
		LogPrintf(LOG_ERR, "Couldn't read the BMP info header.  (NOpenBMPFile)\n");
		return NULL;
	}
	
	EndianFixBMPInfoHdr(&infoHdr);
//LogPrintf(LOG_DEBUG, "infoHdrSize=%u width=%u height=%u bits=%u  (NOpenBMPFile)\n",
//	infoHdr.infoHdrSize, infoHdr.width, infoHdr.height, infoHdr.bits);
//LogPrintf(LOG_DEBUG, "imageSize=%u xResolution=%u yResolution=%u colors=%u  (NOpenBMPFile)\n",
//	infoHdr.imageSize, infoHdr.xResolution, infoHdr.yResolution, infoHdr.colors);

	if (infoHdr.colors!=BITMAP_NCOLORS) {
		LogPrintf(LOG_ERR, "BMP file has %d colors instead of the required %d.  (NOpenBMPFile)\n",
			infoHdr.colors, BITMAP_NCOLORS);
		return NULL;
	}
	
	*pixOffset = fileHdr.offsetToPixelArray;
	*pWidth = infoHdr.width;
	*pHeight = infoHdr.height;
	byteWidth = infoHdr.width/8;
	n4BChunks = (byteWidth%4==0? byteWidth/4 : (byteWidth/4)+1);	/* Round up unless divisible by 4 */
	*pByteWidth = byteWidth;
	*pByteWidthWithPad = 4*n4BChunks;

	return bmpf;
}
