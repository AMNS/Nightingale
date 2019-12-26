/* FileUtils.c - Font utility routines for Nightingale */

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
#include "FileUtils.h"

/*  Created by Michel Alexandre Salim on 2/4/08. */

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
		/* fopen puts error in errno, but this is of type int. */
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
	short ditem, anInt, j;
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
		for (j = 0; j<doc->nfontsUsed; j++)
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
	short	j, nMissing;
	
	if (doc->nfontsUsed>MAX_SCOREFONTS || doc->nfontsUsed<0)
		MayErrMsg("FillFontTable: %ld fonts is illegal.", (long)doc->nfontsUsed);

	EnumerateFonts(doc);	

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
resource fork, if there is one, is left closed.

File versions before N104 use doc->hPrint stored in HPRT 128 resource. In N104 and after
this resource is ignored, and a new resource PFMT 128 is created. Since N103 files are
converted into new N104 files when opened by N104 version application, there is no need
to check for and remove the old resource. */

void GetPrintHandle(Document *doc, unsigned long version, short /*vRefNum*/, FSSpec *pfsSpec)
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
			// Nothing to write. We should never get here anyway.
			doc->flatFormatHandle = NULL;
			return True;
		}
		else {
			short refnum, dummy;
			
			// store the PMPageFormat in a handle
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
