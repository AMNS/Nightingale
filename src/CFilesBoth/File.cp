/***************************************************************************
*	FILE:	File.c
*	PROJ:	Nightingale, rev. for v.99
*	DESC:	File-related routines
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "FileConversion.h"		/* Must follow Nightingale.precomp.h! */
#include "CarbonPrinting.h"
#include "MidiMap.h"

/* Version code is 'N' followed by three digits, e.g., 'N09n': N-Zero-9-n */
static long	version;							/* File version code read/written */

/* Prototypes for internal functions: */
static void FillFontTable(Document *);
static void GetPrintHandle(Document *, short vRefNum, FSSpec *pfsSpec);
static Boolean WritePrintHandle(Document *);
static Boolean ConvertScore(Document *, long);
static Boolean ModifyScore(Document *, long);
static void SetTimeStamps(Document *);

/* Codes for object types being read/written or for non-read/write call when an
I/O error occurs; note that all are negative. See HeapFileIO.c for additional,
positive, codes. */

enum {
	HEADERobj=-999,
	VERSIONobj,
	SIZEobj,
	CREATEcall,
	OPENcall,				/* -995 */
	CLOSEcall,
	DELETEcall,
	RENAMEcall,
	WRITEcall,
	STRINGobj,
	INFOcall,
	SETVOLcall,
	BACKUPcall,
	MAKEFSSPECcall,
	NENTRIESerr = -899
};


/* ----------------------------------------------------- MissingFontsDialog et al -- */

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
	DialogPtr dialogp; GrafPtr oldPort;
	short ditem, anInt, j;
	Handle aHdl;
	 
	GetPort(&oldPort);
	dialogp = GetNewDialog(MISSINGFONTS_DLOG, NULL, BRING_TO_FRONT);
	if (dialogp) {
		SetPort(GetDialogWindowPort(dialogp));
		sprintf(strBuf, "%d", nMissing);
		CParamText(strBuf, "", "", "");
		GetDialogItem(dialogp, TEXT_DI, &anInt, &aHdl, &textRect);
	
		CenterWindow(GetDialogWindow(dialogp),60);
		ShowWindow(GetDialogWindow(dialogp));
						
		ArrowCursor();
		OutlineOKButton(dialogp, TRUE);
	
		/* List the missing fonts. */

		linenum = 1;
		for (j = 0; j<doc->nfontsUsed; j++)
			if (doc->fontTable[j].fontID==applFont) {
				MFDrawLine(doc->fontTable[j].fontName);
			}
	
		ModalDialog(NULL, &ditem);									/* Wait for OK */
		HideWindow(GetDialogWindow(dialogp));
		
		DisposeDialog(dialogp);										/* Free heap space */
		SetPort(oldPort);
	}
	else
		MissingDialog(MISSINGFONTS_DLOG);
}


/* ---------------------------------------------------------------- FillFontTable -- */
/* Fill score header's fontTable, which maps internal font numbers to system
font numbers for the Macintosh system we're running on so we can call TextFont. */

extern void EnumerateFonts(Document *doc);

#define MAX_SHORT	32767

static void FillFontTable(Document *doc)
{
	short		j;
	//short		nFonts, i;
	short		nMissing;
	//short		fontID;
	//Handle	fontHdl;
	//ResType	fontType;
	//Str255	fontName;
	
	fix_end(doc->nfontsUsed);
	if (doc->nfontsUsed>MAX_SCOREFONTS || doc->nfontsUsed<0)
		MayErrMsg("FillFontTable: %ld fonts is illegal.", (long)doc->nfontsUsed);

#if TARGET_API_MAC_CARBON

	EnumerateFonts(doc);	
	
#else
	/*
	 * Except for one thing, this could be done very simply with GetFNum, like this:
  	 *		for (j = 0; j<doc->nfontsUsed; j++) {
	 *			GetFNum(doc->fontTable[j].fontName, &fNum);
	 *			doc->fontTable[j].fontID = (fNum? fNum : 1);
	 *		}
	 * The problem is, GetFNum doesn't distinguish between "no such font" and "font
	 * is the System font". So do it ourselves: for each font name, search the available
	 *	FOND resources until we find a match. If we don't find one, substitute the
	 * application font.
	 */
	for (j = 0; j<doc->nfontsUsed; j++)
		doc->fontTable[j].fontID = -1;
		
	nFonts = CountResources('FOND');

	for (i = 1; i<=nFonts; i++) {
		SetResLoad(FALSE);
		fontHdl = GetIndResource('FOND', i);
		SetResLoad(TRUE);
		GetResInfo(fontHdl, &fontID, &fontType, fontName);
		for (j = 0; j<doc->nfontsUsed; j++)
			if (PStrnCmp((StringPtr)doc->fontTable[j].fontName,
							 (StringPtr)fontName, 32)) {
				doc->fontTable[j].fontID = fontID;
		}
	}
	
#endif

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

static void PrintHandleError(short err)
{
	char fmtStr[256];
	
	GetIndCString(fmtStr, FILEIO_STRS, 2);			/* "Can't get Page Setup (error ID=%d)" */
	sprintf(strBuf, fmtStr, err);
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);	
}

/* 
 * For given document, try to open its resource fork, and set the document's docPageFormat
 * field to either the previously saved print record if one is there, or NULL if there
 * is no resource fork, or we can't get the print record for some other reason.  It
 * is the responsibility of other routines to check doc->docPageFormat for NULLness or not,
 * since we don't want to open the Print Driver every time we open a file (which is
 * when this routine should be called) in order to create a default print handle. The
 * document's resource fork, if there is one, is left closed. 
 *
 * Versions prior to N104 use doc->hPrint stared in HPRT 128 resource. N104 and after this
 * resource is simply ignored, and a new resource PFMT 128 is created. Since N103 files are
 * converted into new N104 files when opened by N104 version application, there is no need
 * to check for and remove the old resource.
 */

static void GetPrintHandle(Document *doc, short /*vRefNum*/, FSSpec *pfsSpec)
	{
		short refnum;
		OSStatus err = noErr;
		
		if (version < 'N104' || THIS_VERSION<'N104') {
			doc->flatFormatHandle = NULL;
		}
		else {
			FSpOpenResFile(pfsSpec, fsRdWrPerm);	/* Open the file */	
			
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
		
			doc->flatFormatHandle = (Handle)Get1Resource('PFMT',128);
			/*
			 *	Ordinarily, we'd now call ReportBadResource, which checks if the new Handle
			 * is NULL as well as checking ResError. But in this case, we don't want to report
			 * an error if it's NULL--or do we?
			 */
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
			
			// Close the res file we just opened
			CloseResFile(refnum);
			
			if (err == noErr) {				
				// code to read the resource..
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
	}


/* ------------------------------------------------------------- WritePrintHandle -- */
/* Save a copy of the document's print record (if any) in the resource fork of the
score file.  The resource fork may already exist or it may not. Should be called when
saving a document.

Leaves the resource fork closed and returns TRUE if all went well, FALSE if not. */

static Boolean WritePrintHandle(Document *doc)
	{
		short err = noErr;
		
		if (THIS_VERSION<'N104') {
			// Nothing to write.. we should never get here anyway.
			doc->flatFormatHandle = NULL;
			return TRUE;
		}
		else {
			short refnum, dummy;
			
			// store the PMPageFormat in a handle
			FlattenAndSavePageFormat(doc);
			
			if (doc->flatFormatHandle) {
				
				FSSpec fsSpec = doc->fsSpec;
				HSetVol(NULL,fsSpec.vRefNum,fsSpec.parID);
				FSpCreateResFile(&fsSpec, creatorType, documentType, smRoman);
				refnum = FSpOpenResFile(&fsSpec,fsRdWrPerm);
				err = ResError();

				if (err == noErr) {
				
					/*
					 *	We want to add the data in the print record to the resource file
					 *	but leave the record in memory independent of it, so we add it
					 * to the resource file, write out the file, detach the handle from
					 * the resource, and close the file. Before doing any of this, we
					 * RemoveResource the print record; this should be unnecessary, since
					 * we DetachResource print records after reading them in, but we're
					 *	playing it safe.
					 */
					dummy = GetResAttrs(doc->flatFormatHandle);
					if (!ResError()) RemoveResource(doc->flatFormatHandle);

					AddResource(doc->flatFormatHandle,'PFMT',128,"\pPMPageFormat");
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


/* --------------------------------------------------------- ConvertScore helpers -- */

static void OldGetSlurContext(Document *, LINK, Point [], Point []);
static void ConvertChordSlurs(Document *);
static void ConvertModNRVPositions(Document *, LINK);
static void ConvertStaffLines(LINK startL);

/* Given a Slur object, return arrays of the paper-relative starting and ending
positions (expressed in points) of the notes delimiting its subobjects. This is
an ancient version of GetSlurContext, from Nightingale .996. */

static void OldGetSlurContext(Document *doc, LINK pL, Point startPt[], Point endPt[])
	{
		CONTEXT 	localContext;
		DDIST		dFirstLeft, dFirstTop, dLastLeft, dLastTop,
					xdFirst, xdLast, ydFirst, ydLast;
		PANOTE 	aNote, firstNote, lastNote;
		PSLUR p; PASLUR	aSlur;
		LINK		aNoteL, firstNoteL, lastNoteL, aSlurL, firstSyncL, 
					lastSyncL, pSystemL;
		PSYSTEM  pSystem;
		short 	k, xpFirst, xpLast, ypFirst, ypLast, firstStaff, lastStaff;
		SignedByte	firstInd, lastInd;
		Boolean  firstMEAS, lastSYS;

		firstMEAS = lastSYS = FALSE;
			
		/* Handle special cases for crossSystem & crossStaff slurs. If p->firstSyncL
		not a sync, it must be a measure; find the first sync to get the context from.
		If p->crossStaff, and slur drawn bottom to top, firstStaff is one greater, else
		lastStaff. */
		p = GetPSLUR(pL);
		firstStaff = lastStaff = p->staffn;
		if (p->crossStaff) {
			if (p->crossStfBack)
					firstStaff += 1;
			else	lastStaff += 1;
		}

		if (SyncTYPE(p->firstSyncL))
			GetContext(doc, p->firstSyncL, firstStaff, &localContext);	/* Get left end context */
		else {
			if (MeasureTYPE(p->firstSyncL)) firstMEAS = TRUE;
			else MayErrMsg("OldGetSlurContextontext: for pL=%ld firstSyncL=%ld is bad",
								(long)pL, (long)p->firstSyncL);
			firstSyncL = LSSearch(p->firstSyncL, SYNCtype, firstStaff, GO_RIGHT, FALSE);
			GetContext(doc, firstSyncL, firstStaff, &localContext);
		}
		dFirstLeft = localContext.measureLeft;									/* abs. origin of left end coords. */
		dFirstTop = localContext.measureTop;
		
		/* Handle special cases for crossSystem slurs. If p->lastSyncL not a sync,
		it must be a system, and must have an RSYS; find the first sync to the
		left of RSYS to get the context from. */
		p = GetPSLUR(pL);
		if (SyncTYPE(p->lastSyncL))
			GetContext(doc, p->lastSyncL, lastStaff, &localContext);		/* Get right end context */
		else {
			if (SystemTYPE(p->lastSyncL) && LinkRSYS(p->lastSyncL)) {
				lastSYS = TRUE;
				pSystemL = p->lastSyncL;
				lastSyncL = LSSearch(LinkRSYS(pSystemL),SYNCtype,lastStaff,GO_LEFT,FALSE);
				GetContext(doc, lastSyncL, lastStaff, &localContext);
			}
			else MayErrMsg("OldGetSlurContextontext: for pL=%ld lastSyncL=%ld is bad",
								(long)pL, (long)p->lastSyncL);
		}
		if (!lastSYS)
			dLastLeft = localContext.measureLeft;							/* abs. origin of right end coords. */
		else {
			pSystem = GetPSYSTEM(pSystemL);
			dLastLeft = pSystem->systemRect.right;
		}
		dLastTop = localContext.measureTop;

		/* Find the links to the first and last notes to which each slur/tie is attached */
		
		p = GetPSLUR(pL);
		aSlurL = FirstSubLINK(pL);
		for (k = 0; aSlurL; k++, aSlurL = NextSLURL(aSlurL)) {
			firstInd = lastInd = -1;
			if (!firstMEAS) {
				aNoteL = FirstSubLINK(p->firstSyncL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->voice==p->voice) {
						aSlur = GetPASLUR(aSlurL);
						if (++firstInd == aSlur->firstInd) firstNoteL = aNoteL;
					}
				}
			}
		
			if (!lastSYS) {
				aNoteL = FirstSubLINK(p->lastSyncL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->voice==p->voice) {
						aSlur = GetPASLUR(aSlurL);
						if (++lastInd  == aSlur->lastInd) lastNoteL = aNoteL;
					}
				}
			}
			
			if (firstMEAS) {
				lastNote = GetPANOTE(lastNoteL);
				xdFirst = dFirstLeft;
				ydFirst = dFirstTop + lastNote->yd;
			}
			else {
				firstNote = GetPANOTE(firstNoteL);
				xdFirst = dFirstLeft + LinkXD(p->firstSyncL) + firstNote->xd;	/* abs. position of 1st note */
				ydFirst = dFirstTop + firstNote->yd;
			}
			
			if (lastSYS) {
				firstNote = GetPANOTE(firstNoteL);
				xdLast = dLastLeft;
				ydLast = dLastTop + firstNote->yd;
			}
			else {
				lastNote = GetPANOTE(lastNoteL);
				xdLast = dLastLeft + LinkXD(p->lastSyncL) + lastNote->xd;		/* abs. position of last note */
				ydLast = dLastTop + lastNote->yd;
			}

			xpFirst = d2p(xdFirst);
			ypFirst = d2p(ydFirst);
			xpLast = d2p(xdLast);
			ypLast = d2p(ydLast);
			SetPt(&startPt[k], xpFirst, ypFirst);
			SetPt(&endPt[k], xpLast, ypLast);
		}
	}


static void ConvertChordSlurs(Document *doc)
{
	LINK pL, aNoteL, aSlurL; PASLUR aSlur; Boolean foundChordSlur;
	Point startPt[2],endPt[2], oldStartPt[2],oldEndPt[2];
	short v, changeStart, changeEnd;
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		if (SlurTYPE(pL) && !SlurTIE(pL)) {
			foundChordSlur = FALSE;
			v = SlurVOICE(pL);
			if (SyncTYPE(SlurFIRSTSYNC(pL))) {
				aNoteL = FindMainNote(SlurFIRSTSYNC(pL), v); 
				if (NoteINCHORD(aNoteL)) foundChordSlur = TRUE;
			}
			if (SyncTYPE(SlurLASTSYNC(pL))) {
				aNoteL = FindMainNote(SlurLASTSYNC(pL), v); 
				if (NoteINCHORD(aNoteL)) foundChordSlur = TRUE;
			}

			if (foundChordSlur) {
				GetSlurContext(doc, pL, startPt, endPt);
				OldGetSlurContext(doc, pL, oldStartPt, oldEndPt);
				changeStart = startPt[0].v-oldStartPt[0].v;
				changeEnd = endPt[0].v-oldEndPt[0].v;
	
				aSlurL = FirstSubLINK(pL);
				aSlur = GetPASLUR(aSlurL);
				aSlur->seg.knot.v -= p2d(changeStart);
				aSlur->endpoint.v -= p2d(changeEnd);
			}
		}
	}
}

static void ConvertModNRVPositions(Document */*doc*/, LINK syncL)
{
	LINK aNoteL, aModNRL; PANOTE aNote; PAMODNR aModNR; short yOff; Boolean above;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->firstMod) {
			aModNRL = aNote->firstMod;
			for ( ; aModNRL; aModNRL=NextMODNRL(aModNRL)) {
				aModNR = GetPAMODNR(aModNRL);
				switch (aModNR->modCode) {
					case MOD_FERMATA:
						above = (aModNR->ystdpit<=0);		/* Not guaranteed but usually right */
						yOff = (above? 4 : -4);
						break;
					case MOD_TRILL:
						yOff = 4;
						break;
					case MOD_ACCENT:
						yOff = 3;
						break;
					case MOD_HEAVYACCENT:
						yOff = 5;
						break;
					case MOD_WEDGE:
						yOff = 2;
						break;
					case MOD_MORDENT:
						yOff = 4;
						break;
					case MOD_INV_MORDENT:
						yOff = 4;
						break;
					case MOD_UPBOW:
						yOff = 6;
						break;
					case MOD_DOWNBOW:
						yOff = 5;
						break;
					case MOD_HEAVYACC_STACC:
						yOff = 5;
						break;
					case MOD_LONG_INVMORDENT:
						yOff = 4;
						break;
					default:
						yOff = 0;
				}
				
				aModNR->ystdpit -= yOff;	/* Assumes ystdpit units (STDIST) = 1/8 spaces */
			}
		}
	}
}

/* Convert old staff <oneLine> field to new <showLines> field. Also initialize
new <showLedgers> field.
		Old ASTAFF had this:
			char			filler:7,
							oneLine:1;
		New ASTAFF has this:
			char			filler:3,
							showLedgers:1,
							showLines:4;
<filler> was initialized to zero in InitStaff, so we can just see if showLines is
non-zero. If it is, then the oneLine flag was set. 	-JGG, 7/22/01 */

static void ConvertStaffLines(LINK startL)
{
	LINK pL, aStaffL;

	for (pL = startL; pL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==STAFFtype) {
			for (aStaffL = FirstSubLINK(pL); aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				Boolean oneLine = (StaffSHOWLINES(aStaffL)!=0);		/* NB: this really is "oneLine"! */
				if (oneLine)
					StaffSHOWLINES(aStaffL) = 1;
				else
					StaffSHOWLINES(aStaffL) = SHOW_ALL_LINES;
				StaffSHOWLEDGERS(aStaffL) = TRUE;
#ifdef STAFFRASTRAL
				// NB: Bad if NightStaffSizer used! See StaffSize2Rastral in ResizeStaff.c
				// from NightStaffSizer code as a way to a possible solution.
				StaffRASTRAL(aStaffL) = doc->srastral;
#endif
			}
		}
	}
}


/* ----------------------------------------------------------------- ConvertScore -- */
/* Any file-format-conversion code that doesn't affect the length of the header or
lengths of objects should go here. This function should only be called after the
header and entire object list have been read. (Tweaks that affect lengths must be
done earlier: to the header, in OpenFile; to objects, in ReadObjHeap.) Return TRUE
if all goes well, FALSE if not. */

static Boolean ConvertScore(Document *doc, long fileTime)
{
	LINK pL; DateTimeRec date;
	
	SecondsToDate(fileTime, &date);

	/* Put all Dynamic horizontal position info into object xd */
	
	if (version<='N100') {
		LINK aDynamicL; PADYNAMIC aDynamic;
	
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==DYNAMtype) {
				aDynamicL = FirstSubLINK(pL);
				aDynamic = GetPADYNAMIC(aDynamicL);
				LinkXD(pL) += aDynamic->xd;
				aDynamic->xd = 0;
			}
	}

	/* Convert Octava position info to new form: if nxd or nyd is nonzero, it's in the
		old form, so move values into xdFirst and ydFirst, and copy them into xdLast and
		ydLast also. */
		
	if (version<='N100') {
		POCTAVA octavap;
	
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==OCTAVAtype) {
				octavap = GetPOCTAVA(pL);
				if (octavap->nxd!=0 || octavap->nyd!=0) {
					octavap->xdFirst = octavap->xdLast = octavap->nxd;
					octavap->ydFirst = octavap->ydLast = octavap->nyd;
					octavap->nxd = octavap->nyd = 0;
				}
			}
	}

	/* Move Measure "fake" flag from subobject to object. If file version code is
		N100, it's tricky to decide whether to do this, but we'll try. */
	
	if (version<='N099'
	|| (version=='N100' && date.year==1991
		&& (date.month<=8 || (date.month==9 && date.day<=6))) ) {
		LINK aMeasL; PAMEASURE aMeas;
	
		if (version>'N099') {
			/* This is all but obsolete--it's not worth moving this string to a resource */
			CParamText("File has a suspicious date, so converting measFake flags.", "", "", "");
			CautionInform(GENERIC_ALRT);
		}
		
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==MEASUREtype) {
				aMeasL = FirstSubLINK(pL);
				aMeas = GetPAMEASURE(aMeasL);
				MeasISFAKE(pL) = aMeas->oldFakeMeas;
			}
	}

	/* Convert Tuplet position info to new form: if acnxd or acnyd is nonzero, it's in the
		old form, so move values into xdFirst and ydFirst, and copy them into xdLast and
		ydLast also. */
	if (version<='N100') {
		PTUPLET pTuplet;
	
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==TUPLETtype) {
				pTuplet = GetPTUPLET(pL);
				if (pTuplet->acnxd!=0 || pTuplet->acnyd!=0) {
					pTuplet->xdFirst = pTuplet->xdLast = pTuplet->acnxd;
					pTuplet->ydFirst = pTuplet->ydLast = pTuplet->acnyd;
					pTuplet->acnxd = pTuplet->acnyd = 0;
				}
			}
	}

	/* Move all slurs to correct position in d.s., immediately before their
		firstSyncL. If the slur is a SlurLastSYSTEM slur, move it immediately
		after the first invis meas, e.g. before the RightLINK of its firstSyncL. */
		
	if (version<='N100') {
		LINK nextL;
		
		for (pL = doc->headL; pL; pL = nextL) {
			nextL = RightLINK(pL);
			if (SlurTYPE(pL))
				if (!SlurLastSYSTEM(pL))
					MoveNode(pL,SlurFIRSTSYNC(pL));
				else
					MoveNode(pL,RightLINK(SlurFIRSTSYNC(pL)));
		}
	}

	/* Look for slurs with <dashed> flag set, which is probably spurious, and if
		any are found, offer to fix them. */
		
	if (version<='N100') {
		LINK aSlurL; PASLUR aSlur; Boolean foundDashed=FALSE;
		
		for (pL = doc->headL; pL && !foundDashed; pL = RightLINK(pL)) 
			if (SlurTYPE(pL)) {
				aSlurL = FirstSubLINK(pL);
				for ( ; aSlurL; aSlurL = NextSLURL(aSlurL)) {
					aSlur = GetPASLUR(aSlurL);
					if (aSlur->dashed) { foundDashed = TRUE; break; }
				}
			}

		if (foundDashed) {
			GetIndCString(strBuf, FILEIO_STRS, 4);		/* "Found dashed slurs that may be spurious" */
			CParamText(strBuf, "", "", "");
			if (CautionAdvise(GENERIC3_ALRT)==OK) {
				for (pL = doc->headL; pL; pL = RightLINK(pL)) 
					if (SlurTYPE(pL)) {
						aSlurL = FirstSubLINK(pL);
						for ( ; aSlurL; aSlurL = NextSLURL(aSlurL)) {
							aSlur = GetPASLUR(aSlurL);
							aSlur->dashed = 0;
						}
					}
			}
		}
	}

	/* Set Clef <small> flag according to whether it's <inMeas>: this is to make
		clef size explicit, so it can be overridden. */

	if (version<='N100') {
		Boolean inMeas; LINK aClefL; PACLEF aClef;
		
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==CLEFtype) {
				inMeas = ClefINMEAS(pL);
				for (aClefL = FirstSubLINK(pL); aClefL; aClefL = NextCLEFL(aClefL)) {
					aClef = GetPACLEF(aClefL);
					aClef->small = inMeas;
				}
			}
	}

	/* Convert octave sign y-position to new representation */
	
	if (version<='N100') {
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==OCTAVAtype) {
				SetOctavaYPos(doc, pL);
			}
	}

	/* Convert Tempo metronome mark from int to string */
	
	if (version<='N100') {
		PTEMPO pTempo; long beatsPM; Str255 string;
		STRINGOFFSET offset;
		
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==TEMPOtype) {
				pTempo = GetPTEMPO(pL);
				beatsPM = pTempo->tempo;
				NumToString(beatsPM, string);
				offset = PStore(string);
				if (offset<0L)
					{ NoMoreMemory(); return FALSE; }
				else
					TempoMETROSTR(pL) = offset;
			}
	}

	/* Move Beamset fields to adjust for removal of the <lGrip> and <rGrip> fields */
	
	if (version<='N100') {
		PBEAMSET pBeamset;
			
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==BEAMSETtype) {
				pBeamset = GetPBEAMSET(pL);
				/* Move fields from <voice> to end--many fields but only 2 bytes! */
				BlockMove((Ptr)(&pBeamset->voice)+8, (Ptr)(&pBeamset->voice), 2);
			}
	}


	/* Move endpoints of slurs on chords. If file version code is N101 and file was written
	by .997a12 or later (flagged by top bit of <fileTime> off!), this should never be done;
	if file version code is N101 and it was written by an earlier version, it probably
	should be, but ask. */

	if (version<='N100'
	|| (version=='N101' && (fileTime & 0x80000000)!=0)
	|| ShiftKeyDown()) {
		short v; LINK aNoteL; Boolean foundChordSlur;
		
		for (foundChordSlur = FALSE, pL = doc->headL; pL; pL = RightLINK(pL)) {
			if (SlurTYPE(pL) && !SlurTIE(pL)) {
				v = SlurVOICE(pL);
				if (SyncTYPE(SlurFIRSTSYNC(pL))) {
					aNoteL = FindMainNote(SlurFIRSTSYNC(pL), v); 
					if (NoteINCHORD(aNoteL)) { foundChordSlur = TRUE; break; }
				}
				if (SyncTYPE(SlurLASTSYNC(pL))) {
					aNoteL = FindMainNote(SlurLASTSYNC(pL), v); 
					if (NoteINCHORD(aNoteL)) { foundChordSlur = TRUE; break; }
				}
			}
		}
		
		if (foundChordSlur) {
			if (version<='N100')
				ConvertChordSlurs(doc);
			else {
				GetIndCString(strBuf, FILEIO_STRS, 5);		/* "Found slur(s) starting/ending with chords" */
				CParamText(strBuf, "", "", "");
				if (CautionAdvise(GENERIC3_ALRT)==OK)
					ConvertChordSlurs(doc);
			}
		}
	}

	/* Fill in page no. position pseudo-Rect. */

	if (version<='N101')
		doc->headerFooterMargins = config.pageNumMarg;

	/* Update ModNR vertical positions to compensate for new centering. */
	
	if (version<='N101')
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (SyncTYPE(pL))
				ConvertModNRVPositions(doc, pL);
		
	/* Update Graphic horizontal positions to compensate for new subobj-relativity. */
	
	if (version<='N101') {
		LINK firstObjL; DDIST newxd, oldxd; CONTEXT context;
		
		for (pL = doc->headL; pL; pL = RightLINK(pL))
			if (GraphicTYPE(pL)) {
				firstObjL = GraphicFIRSTOBJ(pL);
				if (!firstObjL || PageTYPE(firstObjL)) continue;
				
				GetContext(doc, firstObjL, GraphicSTAFF(pL), &context);
				newxd = GraphicPageRelxd(doc, pL,firstObjL, &context);
				oldxd = PageRelxd(firstObjL, &context);
				LinkXD(pL) += oldxd-newxd;		
			}
	}
	
	/* Initialize dynamic subobject endyd field. */

	if (version<='N102') {
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==DYNAMtype) {
				LINK aDynamicL = FirstSubLINK(pL);
				if (IsHairpin(pL))
					DynamicENDYD(aDynamicL) = DynamicYD(aDynamicL);
				else
					DynamicENDYD(aDynamicL) = 0;
			}
	}

	/* Initialize header subobject (part) bank select and FreeMIDI fields for
		both main data structure and master page. */

	if (version<='N102') {
		PPARTINFO pPart; LINK partL;

		partL = NextPARTINFOL(FirstSubLINK(doc->headL));
		for ( ; partL; partL = NextPARTINFOL(partL)) {
			pPart = GetPPARTINFO(partL);
			pPart->fmsOutputDevice = noUniqueID;
			/* ??We're probably not supposed to play with these fields... */
			pPart->fmsOutputDestination.basic.destinationType = 0,
			pPart->fmsOutputDestination.basic.name[0] = 0;
		}

		partL = NextPARTINFOL(FirstSubLINK(doc->masterHeadL));
		for ( ; partL; partL = NextPARTINFOL(partL)) {
			pPart = GetPPARTINFO(partL);
			pPart->fmsOutputDevice = noUniqueID;
			/* ??We're probably not supposed to play with these fields... */
			pPart->fmsOutputDestination.basic.destinationType = 0,
			pPart->fmsOutputDestination.basic.name[0] = 0;
		}
	}

	/* For GRChordSym graphics:
			1)	Initialize info field. This is now used for chord symbol options -- currently
				only whether to show parentheses around extensions -- so we set it to the
				current config setting that used to be the only way to toggle showing 
				parentheses. This seems like the best way to get what the user expects.
			2) Append the chord symbol delimiter character to the graphic string, to represent
				an empty substring for the new "/bass" field.
																			-JGG, 6/16/01 */
	#define CS_DELIMITER FWDDEL_KEY		/* MUST match DELIMITER in ChordSym.c! */

	if (version<='N102') {
		Str255 string, delim; LINK aGraphicL; STRINGOFFSET offset;

		delim[0] = 1; delim[1] = CS_DELIMITER;
		for (pL = doc->headL; pL; pL = RightLINK(pL)) {
			if (ObjLType(pL)==GRAPHICtype && GraphicSubType(pL)==GRChordSym) {
				GraphicINFO(pL) = config.chordSymDrawPar? 1 : 0;

				aGraphicL = FirstSubLINK(pL);
				PStrCopy((StringPtr)PCopy(GraphicSTRING(aGraphicL)), (StringPtr)string);
				PStrCat(string, delim);
				offset = PReplace(GraphicSTRING(aGraphicL), string);
				if (offset<0L) {
					NoMoreMemory();
					return FALSE;
				}
				else
					GraphicSTRING(aGraphicL) = offset;
			}
		}
	}

	/* Convert old staff <oneLine> field to new <showLines> field, and initialize
		new <showLedgers> field, for staves in the score and in master page. */

	if (version<='N102') {
		ConvertStaffLines(doc->headL);
		ConvertStaffLines(doc->masterHeadL);
	}

	/* Make sure all staves are visible in Master Page. They should never be invisible,
	but (as of v.997), they sometimes were, probably because not exporting changes to
	Master Page was implemented by reconstructing it from the 1st system of the score.
	That was fixed in about .998a10, so it should be safe to remove this call
	before too long, but making them visible here shouldn't cause any problems. */

	VisifyMasterStaves(doc);
	
	return TRUE;
}


/* ----------------------------------------------------------------- ModifyScore -- */
/* Any temporary file-content-updating code (a.k.a. hacking) that doesn't affect the
length of the header or lengths of objects should go here. This function should only be
called after the header and entire object list have been read. Return TRUE if all goes
well, FALSE if not.

NB: If code here considers changing something, and especially if it ends up actually
doing so, it should call LogPrintf to display at least one very prominent message
in the console window, and SysBeep to draw attention to it. It should perhaps also
set doc->changed, though this will make it easier for people to accidentally overwrite
the original version.

NB2: Be sure that all of this code is removed or commented out in ordinary versions!
To facilitate that, when done with hacking, add an "#error" line; cf. examples
below. */

static void ShowTops(Document *doc, LINK pL, short staffN1, short staffN2);
static void ShowTops(Document *doc, LINK pL, short staffN1, short staffN2)
{
	CONTEXT context; short staffTop1, staffTop2;
	pL = SSearch(doc->headL, STAFFtype, FALSE);
	GetContext(doc, pL, staffN1, &context);
	staffTop1 =  context.staffTop;
	GetContext(doc, pL, staffN2, &context);
	staffTop2 =  context.staffTop;
	LogPrintf(LOG_NOTICE, "ShowTops(%d): staffTop1=%d, staffTop2=%d\n", pL, staffTop1, staffTop2);
}

static void SwapStaves(Document *doc, LINK pL, short staffN1, short staffN2);
static void SwapStaves(Document *doc, LINK pL, short staffN1, short staffN2)
{
	LINK aStaffL, aMeasureL, aPSMeasL, aClefL, aKeySigL, aTimeSigL, aNoteL,
			aTupletL, aRptEndL, aDynamicL, aGRNoteL;
	CONTEXT context;
	short staffTop1, staffTop2;

	switch (ObjLType(pL)) {
		case HEADERtype:
			break;

		case PAGEtype:
			break;

		case SYSTEMtype:
			break;

		case STAFFtype:
LogPrintf(LOG_NOTICE, "  Staff L%d\n", pL);
			GetContext(doc, pL, staffN1, &context);
			staffTop1 =  context.staffTop;
			GetContext(doc, pL, staffN2, &context);
			staffTop2 =  context.staffTop;
LogPrintf(LOG_NOTICE, "    staffTop1, 2=%d, %d\n", staffTop1, staffTop2);
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				if (StaffSTAFF(aStaffL)==staffN1) {
					StaffSTAFF(aStaffL) = staffN2;
					StaffTOP(aStaffL) = staffTop2;
				}
				else if (StaffSTAFF(aStaffL)==staffN2) {
					StaffSTAFF(aStaffL) = staffN1;
					StaffTOP(aStaffL) = staffTop1;
				}
//GetContext(doc, pL, staffN1, &context);
//LogPrintf(LOG_NOTICE, "(1)    pL=%d staffTop1=%d\n", pL, staffTop1);
			}
//GetContext(doc, pL, staffN1, &context);
//LogPrintf(LOG_NOTICE, "(2)    pL=%d staffTop1=%d\n", pL, staffTop1);
//ShowTops(doc, pL, staffN1, staffN2);
			break;

		case CONNECTtype:
			break;

		case MEASUREtype:
LogPrintf(LOG_NOTICE, "  Measure L%d\n", pL);
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
				if (MeasureSTAFF(aMeasureL)==staffN1) MeasureSTAFF(aMeasureL) = staffN2;
				else if (MeasureSTAFF(aMeasureL)==staffN2) MeasureSTAFF(aMeasureL) = staffN1;
			}
			break;

		case PSMEAStype:
LogPrintf(LOG_NOTICE, "  PSMeas L%d\n", pL);
			aPSMeasL = FirstSubLINK(pL);
			for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL)) {
				if (PSMeasSTAFF(aPSMeasL)==staffN1) PSMeasSTAFF(aPSMeasL) = staffN2;
				else if (PSMeasSTAFF(aPSMeasL)==staffN2) PSMeasSTAFF(aPSMeasL) = staffN1;
			}
			break;

		case CLEFtype:
LogPrintf(LOG_NOTICE, "  Clef L%d\n", pL);
			aClefL = FirstSubLINK(pL);
			for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
				if (ClefSTAFF(aClefL)==staffN1) ClefSTAFF(aClefL) = staffN2;
				else if (ClefSTAFF(aClefL)==staffN2) ClefSTAFF(aClefL) = staffN1;
			}
			break;

		case KEYSIGtype:
LogPrintf(LOG_NOTICE, "  Keysig L%d\n", pL);
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
				if (KeySigSTAFF(aKeySigL)==staffN1) KeySigSTAFF(aKeySigL) = staffN2;
				else if (KeySigSTAFF(aKeySigL)==staffN2) KeySigSTAFF(aKeySigL) = staffN1;
			}
			break;

		case TIMESIGtype:
LogPrintf(LOG_NOTICE, "  Timesig L%d\n", pL);
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
				if (TimeSigSTAFF(aTimeSigL)==staffN1) TimeSigSTAFF(aTimeSigL) = staffN2;
				else if (TimeSigSTAFF(aTimeSigL)==staffN2) TimeSigSTAFF(aTimeSigL) = staffN1;
			}
			break;

		case SYNCtype:
LogPrintf(LOG_NOTICE, "  Sync L%d\n", pL);
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if (NoteSTAFF(aNoteL)==staffN1) {
					NoteSTAFF(aNoteL) = staffN2;
					NoteVOICE(aNoteL) = staffN2;		/* Assumes only 1 voice on the staff */
				}
				else if (NoteSTAFF(aNoteL)==staffN2) {
					NoteSTAFF(aNoteL) = staffN1;
					NoteVOICE(aNoteL) = staffN1;		/* Assumes only 1 voice on the staff */
				}
			}
			break;

		case BEAMSETtype:
LogPrintf(LOG_NOTICE, "  Beamset L%d\n", pL);
			if (BeamSTAFF(pL)==staffN1) BeamSTAFF((pL)) = staffN2;
			else if (BeamSTAFF((pL))==staffN2) BeamSTAFF((pL)) = staffN1;
			break;

		case TUPLETtype:
LogPrintf(LOG_NOTICE, "  Tuplet L%d\n", pL);
			if (TupletSTAFF(pL)==staffN1) TupletSTAFF((pL)) = staffN2;
			else if (TupletSTAFF((pL))==staffN2) TupletSTAFF((pL)) = staffN1;
			break;

/*
		case RPTENDtype:
			?? = FirstSubLINK(pL);
			for (??) {
			}
			break;

		case ENDINGtype:
			?!
			break;
*/
		case DYNAMtype:
LogPrintf(LOG_NOTICE, "  Dynamic L%d\n", pL);
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL=NextDYNAMICL(aDynamicL)) {
				if (DynamicSTAFF(aDynamicL)==staffN1) DynamicSTAFF(aDynamicL) = staffN2;
				else if (DynamicSTAFF(aDynamicL)==staffN2) DynamicSTAFF(aDynamicL) = staffN1;
			}
			break;

		case GRAPHICtype:
LogPrintf(LOG_NOTICE, "  Graphic L%d\n", pL);
			if (GraphicSTAFF(pL)==staffN1) GraphicSTAFF((pL)) = staffN2;
			else if (GraphicSTAFF((pL))==staffN2) GraphicSTAFF((pL)) = staffN1;
			break;

/*
		case OCTAVAtype:
			?!
			break;
*/
		case SLURtype:
LogPrintf(LOG_NOTICE, "  Slur L%d\n", pL);
			if (SlurSTAFF(pL)==staffN1) SlurSTAFF((pL)) = staffN2;
			else if (SlurSTAFF((pL))==staffN2) SlurSTAFF((pL)) = staffN1;
			break;

/*
		case GRSYNCtype:
			?? = FirstSubLINK(pL);
			for (??) {
			}
			break;

		case TEMPOtype:
			?!
			break;

		case SPACEtype:
			?!
			break;
*/

		default:
			break;	
	}
}


static Boolean ModifyScore(Document *doc, long /*fileTime*/)
{
#ifdef SWAP_STAVES
#error ModifyScore: ATTEMPTED TO COMPILE OLD HACKING CODE!
	/* DAB carelessly put a lot of time into orchestrating his violin concerto with
		a template having the trumpet staff above the horn; this is to correct that.
		NB: To swap two staves, in addition to running this code, use Master Page to:
			1. Fix the staves' vertical positions
			2. If the staves are Grouped, un-Group and re-Group
			3. Swap the Instrument info
		NB2: If there's more than one voice on either of the staves, this is not
		likely to work at all well :-( .
																--DAB, Jan. 2016 */
	
	short staffN1 = 5, staffN2 = 6;
	LINK pL;
	
	SysBeep(1);
	LogPrintf(LOG_NOTICE, "ModifyScore: SWAPPING STAVES %d AND %d (of %d) IN MASTER PAGE....\n",
				staffN1, staffN2, doc->nstaves);
	for (pL = doc->masterHeadL; pL!=doc->masterTailL; pL = RightLINK(pL)) {
		SwapStaves(doc, pL, staffN1, staffN2);
	}
	LogPrintf(LOG_NOTICE, "ModifyScore: SWAPPING STAVES %d AND %d (of %d) IN SCORE OBJECT LIST....\n",
				staffN1, staffN2, doc->nstaves);
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		SwapStaves(doc, pL, staffN1, staffN2);
	}
  	doc->changed = TRUE;

#endif

#ifdef FIX_MASTERPAGE_SYSRECT
#error ModifyScore: ATTEMPTED TO COMPILE OLD HACKING CODE!
	/* This is to fix a score David Gottlieb is working on, in which Ngale draws
	 * _completely_ blank pages. Nov. 1999.
	 */
	 
	LINK sysL, staffL, aStaffL; DDIST topStaffTop; PASTAFF aStaff;

	LogPrintf(LOG_NOTICE, "ModifyScore: fixing Master Page sysRects and staffTops...\n");
	// Browser(doc,doc->masterHeadL, doc->masterTailL);
	sysL = SSearch(doc->masterHeadL, SYSTEMtype, FALSE);
	SystemRECT(sysL).bottom = SystemRECT(sysL).top+pt2d(72);

	staffL = SSearch(doc->masterHeadL, STAFFtype, FALSE);
	aStaffL = FirstSubLINK(staffL);
	topStaffTop = pt2d(24);
	/* The following assumes staff subobjects are in order from top to bottom! */
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->staffTop = topStaffTop;
		topStaffTop += pt2d(36);
	}
  	doc->changed = TRUE;
#endif

#ifdef FIX_UNBEAMED_FLAGS_AUGDOT_PROB
#error ModifyScore: ATTEMPTED TO COMPILE OLD HACKING CODE!
	short alteredCount; LINK pL;

  /* From SetupNote in Objects.c:
   * "On upstemmed notes with flags, xmovedots should really 
   * be 2 or 3 larger [than normal], but then it should change whenever 
   * such notes are beamed or unbeamed or their stem direction or 
   * duration changes." */

  /* TC Oct 7 1999 (modified by DB, 8 Oct.): quick and dirty hack to fix this problem
   * for Bill Hunt; on opening file, look at all unbeamed dotted notes with subtype >
   * quarter note and if they have stem up, increase xmovedots by 2; if we find any
   * such notes, set doc-changed to TRUE then simply report to user. It'd probably
   * be better to consider stem length and not do this for notes with very long
   * stems, but we don't consider that. */

  LogPrintf(LOG_NOTICE, "ModifyScore: fixing augdot positions for unbeamed upstemmed notes with flags...\n");
  alteredCount = 0;
  for (pL = doc->headL; pL; pL = RightLINK(pL)) 
    if (ObjLType(pL)==SYNCtype) {
      LINK aNoteL = FirstSubLINK(pL);
      for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
        PANOTE aNote = GetPANOTE(aNoteL);
        if((aNote->subType > QTR_L_DUR) && (aNote->ndots))
          if(!aNote->beamed)
            if(MainNote(aNoteL) && (aNote->yd > aNote->ystem)) {
              aNote->xmovedots = 3 + WIDEHEAD(aNote->subType) + 2;
              alteredCount++;
            }
      }
    }
  
  if (alteredCount) {
  	doc->changed = TRUE;

    NumToString((long)alteredCount,(unsigned char *)strBuf);
    ParamText((unsigned char *)strBuf, 
      (unsigned char *)"\p unbeamed notes with stems up found; aug-dots corrected. NB Save the file with a new name!", 
        (unsigned char *)"", (unsigned char *)"");
    CautionInform(GENERIC2_ALRT);
  }
#endif

#ifdef FIX_BLACK_SCORE
#error ModifyScore: ATTEMPTED TO COMPILE OLD HACKING CODE!
	/* Sample trashed-file-fixing hack: in this case, to fix an Arnie Black score. */
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) 
		if (pL>=492 && ObjLType(pL)==SYNCtype) {
			LINK aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				NoteBEAMED(aNoteL) = FALSE;
		}
#endif

	return TRUE;
}


/* ---------------------------------------------------------------- SetTimeStamps -- */
/* Recompute timestamps up to the first Measure that has any unknown durs. in it. */

void SetTimeStamps(Document *doc)
{
	LINK firstUnknown, stopMeas;
	
	firstUnknown = FindUnknownDur(doc->headL, GO_RIGHT);
	stopMeas = (firstUnknown?
						SSearch(firstUnknown, MEASUREtype, GO_LEFT)
					:  doc->tailL);
	FixTimeStamps(doc, doc->headL, LeftLINK(stopMeas));
}


/* --------------------------------------------------------------------- OpenFile -- */
/*	Open and read in the specified file. If there's an error, normally (see comments
in OpenError) gives an error message, and returns <errCode>; else returns noErr (0). 
Also sets *fileVersion to the Nightingale version that created the file. NB: even
though vRefNum is a parameter, (routines called by) OpenFile assume the volume is
already set! This should be changed. */

enum {
	LOW_VERSION_ERR=-999,
	HI_VERSION_ERR,
	HEADER_ERR,
	LASTTYPE_ERR,				/* Value for LASTtype in file is not we expect it to be */
	TOOMANYSTAVES_ERR,
	LOWMEM_ERR,
	BAD_VERSION_ERR
};

extern void StringPoolEndianFix(StringPoolRef pool);
extern short StringPoolProblem(StringPoolRef pool);

#include <ctype.h>


short OpenFile(Document *doc, unsigned char *filename, short vRefNum,
					FSSpec *pfsSpec, long *fileVersion)
{
	short		errCode, refNum, strPoolErrCode;
	short 		errInfo,				/* Type of object being read or other info on error */
				lastType;
	long		count, stringPoolSize,
				fileTime;
	Boolean		fileOpened;
	OMSSignature omsDevHdr;
	long		fmsDevHdr;
	long		omsBufCount, omsDevSize;
	short		i;
	FInfo		fInfo;
	FSSpec 		fsSpec;
	long		cmHdr;
	long		cmBufCount, cmDevSize;
	FSSpec		*pfsSpecMidiMap;

	WaitCursor();

	fileOpened = FALSE;
	
	//errCode = FSMakeFSSpec(vRefNum, 0, filename, &fsSpec);
	//if (errCode) { errInfo = MAKEFSSPECcall; goto Error; }
	
	//errCode = FSOpen(filename, vRefNum, &refNum );		/* Open the file */
	
	fsSpec = *pfsSpec;
	errCode = FSpOpenDF (&fsSpec, fsRdWrPerm, &refNum );	/* Open the file */
	if (errCode == fLckdErr || errCode == permErr) {
		doc->readOnly = TRUE;
		errCode = FSpOpenDF (&fsSpec, fsRdPerm, &refNum );	/* Try again - open the file read-only */
//		errCode = noErr;
	}
	if (errCode) { errInfo = OPENcall; goto Error; }
	fileOpened = TRUE;

	count = sizeof(version);										/* Read & check file version code */
	errCode = FSRead(refNum, &count, &version);
	fix_end(version);
	if (errCode) { errInfo = VERSIONobj; goto Error;}
	
	/* If user has the secret keys down, pretend file is in current version. */
	
	if (CapsLockKeyDown() && ShiftKeyDown() && OptionKeyDown() && CmdKeyDown()) {
#ifndef PUBLIC_VERSION
		LogPrintf(LOG_NOTICE, "IGNORING FILE'S VERSION CODE '%T'.\n", version);
#endif

		GetIndCString(strBuf, FILEIO_STRS, 6);		/* "IGNORING FILE'S VERSION CODE!" */
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
		version = THIS_VERSION;
	}

	*fileVersion = version;
	if (ACHAR(version,3)!='N') { errCode = BAD_VERSION_ERR; goto Error; }
	if ( !isdigit(ACHAR(version,2))
			|| !isdigit(ACHAR(version,1))
			|| !isdigit(ACHAR(version,0)) )
		 { errCode = BAD_VERSION_ERR; goto Error; }
	if (version<FIRST_VERSION) { errCode = LOW_VERSION_ERR; goto Error; }
	if (version>THIS_VERSION) { errCode = HI_VERSION_ERR; goto Error; }
#ifndef PUBLIC_VERSION
	if (version!=THIS_VERSION) LogPrintf(LOG_NOTICE, "CONVERTING VERSION '%T' FILE.\n", version);
#endif

	count = sizeof(fileTime);										/* Time file was written */
	errCode = FSRead(refNum, &count, &fileTime);
	fix_end(fileTime);
	if (errCode) { errInfo = VERSIONobj; goto Error; }
		
	/* Read and, if necessary, convert document (i.e., Sheets) and score headers. */
	
	switch(version) {
		case 'N100':
		case 'N101':
		case 'N102':
		{
			char *src, *dst; SCOREHEADER_N102 oldScoreHdr;

			count = sizeof(DOCUMENTHDR);
			errCode = FSRead(refNum, &count, &doc->origin);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			
			count = sizeof(SCOREHEADER_N102);
			errCode = FSRead(refNum, &count, &doc->headL);
			if (errCode) { errInfo = HEADERobj; goto Error; }

			/* Make room for extra comment chars. */
			src = (char *)&doc->comment[MAX_COMMENT_LEN_N102+1];
			dst = src + (MAX_COMMENT_LEN-MAX_COMMENT_LEN_N102);
			/* NB: Compiler won't let us compute <count> the same way as for fontTable (below), 
				probably because <feedback> is a bitfield (?). Instead, we use the next field,
				<currentPage>, and adjust the total by one byte. */
			count = sizeof(SCOREHEADER_N102) - 
								((char *)&oldScoreHdr.currentPage - (char *)&oldScoreHdr.headL);
			count++;
			BlockMove(src, dst, count);

			/* Make room for 4 new text styles, and init them. */
			doc->nFontRecords = 15;
			src = (char *)&doc->fontName6[0];
			dst = (char *)&doc->fontName6[0] + (4 * 36);			/* 4 new text styles, 36 bytes each */
			count = sizeof(SCOREHEADER_N102) - 
								((char *)&oldScoreHdr.nfontsUsed - (char *)&oldScoreHdr.headL);
			BlockMove(src, dst, count);
			InitN103FontRecs(doc);

			/* Make room for more slots in the fontTable array. NOTE: We don't init these slots. */
			src = (char *)&doc->fontTable[MAX_SCOREFONTS_N102];
			dst = src + (sizeof(FONTITEM) * (MAX_SCOREFONTS-MAX_SCOREFONTS_N102));
			count = sizeof(SCOREHEADER_N102) - 
								((char *)&oldScoreHdr.magnify - (char *)&oldScoreHdr.headL);
			BlockMove(src, dst, count);

			/* Make room for music font name, initialized to Sonata. */
			src = (char *)&doc->musFontName[0];
			dst = (char *)&doc->magnify;
			count = sizeof(SCOREHEADER_N102) - 
								((char *)&oldScoreHdr.magnify - (char *)&oldScoreHdr.headL);
			BlockMove(src, dst, count);
			Pstrcpy(doc->musFontName, "\pSonata");

			break;
		}
		case 'N103':
		case 'N104':
		case 'N105':
			count = sizeof(DOCUMENTHDR);
			errCode = FSRead(refNum, &count, &doc->origin);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			
			count = sizeof(SCOREHEADER);
			errCode = FSRead(refNum, &count, &doc->headL);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			break;
		
		default:
			;
	}

	fix_end(doc->nstaves);
	if (doc->nstaves>MAXSTAVES) {
		errCode = TOOMANYSTAVES_ERR;
		errInfo = doc->nstaves;
		goto Error;
	}

	count = sizeof(lastType);
	errCode = FSRead(refNum, &count, &lastType);
	if (errCode) { errInfo = HEADERobj; goto Error; }
	fix_end(lastType);

	if (lastType!=LASTtype) {
		errCode = LASTTYPE_ERR;
		errInfo = HEADERobj;
		goto Error;
	}

	count = sizeof(stringPoolSize);
	errCode = FSRead(refNum, &count, &stringPoolSize);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	fix_end(stringPoolSize);
	if (doc->stringPool)
		DisposeStringPool(doc->stringPool);
	/*
	 * Allocate from the StringManager, not NewHandle, in case StringManager is tracking
	 * its allocations. Then, since we're going to overwrite everything with stuff from file
	 * below, we can resize it to what it was when saved.
	 */
	doc->stringPool = NewStringPool();
	if (doc->stringPool == NULL) { errInfo = STRINGobj; goto Error; }
	SetHandleSize((Handle)doc->stringPool,stringPoolSize);
	
	HLock((Handle)doc->stringPool);
	errCode = FSRead(refNum, &stringPoolSize, *doc->stringPool);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	HUnlock((Handle)doc->stringPool);
	SetStringPool(doc->stringPool);
	// MAS FIXME:
	// code inherited from CW -- always shows error message when loading file
	StringPoolEndianFix(doc->stringPool);
	strPoolErrCode = StringPoolProblem(doc->stringPool);
	if (strPoolErrCode!=0)
		AlwaysErrMsg("OpenFile: the string pool is probably bad (code=%ld).", (long)strPoolErrCode);
	
	//errCode = GetFInfo(filename, vRefNum, &fInfo);
	errCode = FSpGetFInfo (&fsSpec, &fInfo);
	if (errCode!=noErr) { errInfo = INFOcall; goto Error; }
	
	errCode = ReadHeaps(doc, refNum, version, fInfo.fdType);	/* Read the rest of the file! */
	if (errCode) return errCode;

	/* Be sure we have enough memory left for a maximum-size segment and a bit more. */
	
	if (!PreflightMem(40)) { NoMoreMemory(); return LOWMEM_ERR; }
	
	ConvertScore(doc, fileTime);							/* Do any conversion of old files needed */

	PStrCopy(filename, doc->name);							/* Remember filename and vol refnum after scoreHead is overwritten */
	doc->vrefnum = vRefNum;
	doc->changed = FALSE;
	doc->saved = TRUE;										/* Has to be, since we just opened it! */
	doc->named = TRUE;										/* Has to have a name, since we just opened it! */

	ModifyScore(doc, fileTime);								/* Do any hacking needed--ordinarily none */

	/*
	 * Read the document's OMS device numbers for each part. if "devc" signature block not
	 * found at end of file, pack the document's omsPartDeviceList with default values.
	 */
	omsBufCount = sizeof(OMSSignature);
	errCode = FSRead(refNum, &omsBufCount, &omsDevHdr);
	if (!errCode && omsDevHdr == 'devc') {
		omsBufCount = sizeof(long);
		errCode = FSRead(refNum, &omsBufCount, &omsDevSize);
		if (errCode) return errCode;
		if (omsDevSize!=(MAXSTAVES+1)*sizeof(OMSUniqueID)) return errCode;
		errCode = FSRead(refNum, &omsDevSize, &(doc->omsPartDeviceList[0]));
		if (errCode) return errCode;
	}
	else {
		for (i = 1; i<=LinkNENTRIES(doc->headL)-1; i++)
			doc->omsPartDeviceList[i] = config.defaultOutputDevice;
		doc->omsInputDevice = config.defaultInputDevice;
	}

	/* Read the FreeMIDI input device data. */
	doc->fmsInputDevice = noUniqueID;
	/* ??We're probably not supposed to play with these fields... */
	doc->fmsInputDestination.basic.destinationType = 0,
	doc->fmsInputDestination.basic.name[0] = 0;
	count = sizeof(long);
	errCode = FSRead(refNum, &count, &fmsDevHdr);
	if (!errCode) {
		if (fmsDevHdr==0x00000000)
			;					/* end marker, so file version is < N103, and we're done */
		else if (fmsDevHdr==FreeMIDISelector) {
			count = sizeof(fmsUniqueID);
			errCode = FSRead(refNum, &count, &doc->fmsInputDevice);
			if (errCode) return errCode;
			count = sizeof(destinationMatch);
			errCode = FSRead(refNum, &count, &doc->fmsInputDestination);
			if (errCode) return errCode;
		}
	}
	
	if (version >= 'N105') {
		cmBufCount = sizeof(long);
		errCode = FSRead(refNum, &cmBufCount, &cmHdr);
		if (!errCode && cmHdr == 'cmdi') {
			cmBufCount = sizeof(long);
			errCode = FSRead(refNum, &cmBufCount, &cmDevSize);
			if (errCode) return errCode;
			if (cmDevSize!=(MAXSTAVES+1)*sizeof(MIDIUniqueID)) return errCode;
			errCode = FSRead(refNum, &cmDevSize, &(doc->cmPartDeviceList[0]));
			if (errCode) return errCode;
		}
	}
	else {
		for (i = 1; i<=LinkNENTRIES(doc->headL)-1; i++)
			doc->cmPartDeviceList[i] = config.cmDefaultOutputDevice;
	}
	doc->cmInputDevice = config.cmDefaultInputDevice;

	errCode = FSClose(refNum);
	if (errCode) { errInfo = CLOSEcall; goto Error; }
	
	if (!IsDocPrintInfoInstalled(doc))
		InstallDocPrintInfo(doc);
	
	GetPrintHandle(doc, vRefNum, pfsSpec);				/* doc->name must be set before this */
	
	GetMidiMap(doc, pfsSpec);
	
	pfsSpecMidiMap = GetMidiMapFSSpec(doc);
	if (pfsSpecMidiMap != NULL) {
		OpenMidiMapFile(doc, pfsSpecMidiMap);
		ReleaseMidiMapFSSpec(doc);		
	}

	FillFontTable(doc);

	InitDocMusicFont(doc);

	SetTimeStamps(doc);									/* Up to first meas. w/unknown durs. */


	/*
	 * Assume that no information in the score having to do with window-relative
	 * positions is valid. Besides clearing the object <valid> flags to indicate
	 * this, protect ourselves against functions that might not check the flags
	 * properly by setting all such positions to safe values now.
	 */
	InvalRange(doc->headL, doc->tailL);					/* Force computing all objRects */

	doc->hasCaret = FALSE;									/* Caret must still be set up */
	doc->selStartL = doc->headL;							/* Set selection range */								
	doc->selEndL = doc->tailL;								/*   to entire score, */
	DeselAllNoHilite(doc);									/*   deselect it */
	SetTempFlags(doc, doc, doc->headL, doc->tailL, FALSE);	/* Just in case */

	if (ScreenPagesExceedView(doc))
		CautionInform(MANYPAGES_ALRT);

	ArrowCursor();	
	return 0;

Error:
	OpenError(fileOpened, refNum, errCode, errInfo);
	return errCode;
}


/* -------------------------------------------------------------------- OpenError -- */
/* Handle errors occurring while reading a file. fileOpened indicates  whether or
not the file was open when the error occurred; if so, OpenError closes it. <errCode>
is either an error code return by the file system manager or one of our own codes
(see enum above). <errInfo> indicates at what step of the process the error happened
(CREATEcall, OPENcall, etc.: see enum above), the type of object being read, or
some other additional information on the error. NB: If errCode==0, this will close
the file but not give an error message; I'm not sure that's a good idea.

Note that after a call to OpenError with fileOpened, you should not try to keep
reading, since the file will no longer be open! */

void OpenError(Boolean fileOpened,
					short refNum,
					short errCode,
					short errInfo)	/* More info: at what step error happened, type of obj being read, etc. */
{
	char aStr[256], fmtStr[256]; StringHandle strHdl;
	short strNum;
	
	if (fileOpened) FSClose(refNum);
	if (errCode!=0) {
		switch (errCode) {
			/*
			 * First handle our own codes that need special treatment.
			 */
			case BAD_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 7);			/* "file version is illegal" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case LOW_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 8);			/* "too old for this version of Nightingale" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case HI_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 9);			/* "newer than this version of Nightingale" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case TOOMANYSTAVES_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 10);		/* "this version can handle only %d staves" */
				sprintf(aStr, fmtStr, errInfo, MAXSTAVES);
				break;
			default:
				/*
				 * We expect descriptions of the common errors stored by code (negative
				 * values, for system errors; positive ones for our own I/O errors) in
				 * individual 'STR ' resources. If we find one for this error, print it,
				 * else just print the raw code.
				 */
				strHdl = GetString(errCode);
				if (strHdl) {
					Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
					PToCString((unsigned char *)strBuf);
					strNum = (errInfo>0? 15 : 16);	/* "%s (heap object type=%d)." : "%s (error code=%d)." */
					GetIndCString(fmtStr, FILEIO_STRS, strNum);
					sprintf(aStr, fmtStr, strBuf, errInfo);
				}
				else {
					strNum = (errInfo>0? 17 : 18);	/* "Error ID=%d (heap object type=%d)." : "Error ID=%d (error code=%d)." */
					GetIndCString(fmtStr, FILEIO_STRS, strNum);
					sprintf(aStr, fmtStr, errCode, errInfo);
				}
		}
#ifndef PUBLIC_VERSION
		LogPrintf(LOG_NOTICE, aStr); LogPrintf(LOG_NOTICE, "\n");
#endif
		CParamText(aStr, "", "", "");
		StopInform(READ_ALRT);
	}
}
	

/* -------------------------------------------------------- Functions for SaveFile - */

static enum {
	SF_SafeSave,
	SF_Replace,
	SF_SaveAs,
	SF_Cancel
} E_SaveFileItems;

static long GetOldFileSize(Document *doc);
static long GetFileSize(Document *doc,long vAlBlkSize);
static long GetFreeSpace(Document *doc,long *vAlBlkSize);
static short AskSaveType(Boolean canContinue);
static short GetSaveType(Document *doc,Boolean saveAs);
static short WriteFile(Document *doc,short refNum);
static Boolean SFChkScoreOK(Document *doc);
static Boolean GetOutputFile(Document *doc);

/* Return, in bytes, the physical size of the file in which doc was previously
saved: physical length of data fork + physical length of resource fork.

??This does not quite give the information needed to tell how much disk space the
file takes: the relevant thing is numbers of allocation blocks, not bytes, and
that number must be computed independently for the data fork and resource fork.
PreflightFileCopySpace in FileCopy.c in Apple DTS' More Files package appears
to do a much better job. */

static long GetOldFileSize(Document *doc)
{
	HParamBlockRec fInfo;
	short err;
	long fileSize;

	fInfo.fileParam.ioCompletion = NULL;
	fInfo.fileParam.ioFVersNum = 0;
	fInfo.fileParam.ioResult = noErr;
	fInfo.fileParam.ioVRefNum = doc->vrefnum;
	fInfo.fileParam.ioDirID = doc->fsSpec.parID;
	fInfo.fileParam.ioFDirIndex = 0;							/* ioNamePtr is INPUT, not output */
	fInfo.fileParam.ioNamePtr = (unsigned char *)doc->name;
	PBHGetFInfo(&fInfo,FALSE);
	
	err = fInfo.fileParam.ioResult;
	if (err==noErr) {
		fileSize = fInfo.fileParam.ioFlPyLen;
		fileSize += fInfo.fileParam.ioFlRPyLen;
		return fileSize;
	} 

	return (long)err;
}

/* Get the physical size of the file as it will be saved from memory: the
total size of all objects written to disk, rounded up to sector size, plus
the total size of all resources saved, rounded up to sector size. */

static long GetFileSize(Document *doc, long vAlBlkSize)
{
	unsigned short objCount[LASTtype], i, nHeaps;
	long fileSize=0L, strHdlSize, blkMod, rSize;
	LINK pL;
	Handle stringHdl;

	/* Get the total number of objects of each type and the number of note
		modifiers in the data structure. */

	CountInHeaps(doc, objCount, TRUE);

	for (pL = doc->headL; pL!=RightLINK(doc->tailL); pL = RightLINK(pL))
		fileSize += objLength[ObjLType(pL)];

	for (pL = doc->masterHeadL; pL!=RightLINK(doc->masterTailL); pL = RightLINK(pL))
		fileSize += objLength[ObjLType(pL)];

	for (i=FIRSTtype; i<LASTtype-1; i++) {
		fileSize += subObjLength[i]*objCount[i];
	}
	
	fileSize += 2*sizeof(long);						/* version & file time */	

	fileSize += sizeof(DOCUMENTHDR);
	
	fileSize += sizeof(SCOREHEADER);
	
	fileSize += sizeof((short)LASTtype);

	stringHdl = (Handle)GetStringPool();
	strHdlSize = GetHandleSize(stringHdl);

	fileSize += sizeof(strHdlSize);
	fileSize += strHdlSize;
	
	nHeaps = LASTtype-FIRSTtype+1;

	/* Total number of objects of type heapIndex plus sizeAllObjects, 1
		for each heap. */

	fileSize += (sizeof(short)+sizeof(long))*nHeaps;
	
	/* The HEAP struct header, 1 for each heap. */
	fileSize += sizeof(HEAP)*nHeaps;

	fileSize += sizeof(long);							/* end marker */
	
	/* Round up to the next higher multiple of allocation blk size */

	blkMod = fileSize % vAlBlkSize;
	if (blkMod)
		fileSize += vAlBlkSize - blkMod;
	
	/* Add size of file's resource fork here, rounded up to the next higher
		multiple of allocation block size. */
	
	if (doc->hPrint) {
		rSize = GetHandleSize((Handle)doc->hPrint);

		fileSize += rSize;

		blkMod = rSize % vAlBlkSize;
		if (blkMod)
			fileSize += vAlBlkSize - blkMod;
	}
	
	return fileSize;
}

/* Get the amount of free space on the volume: number of free blks * blk size */

static long GetFreeSpace(Document *doc, long *vAlBlkSize)
{
	HParamBlockRec vInfo;
	short err;
	long vFreeSpace;

	vInfo.volumeParam.ioCompletion = NULL;
	vInfo.volumeParam.ioResult = noErr;
	vInfo.volumeParam.ioVRefNum = doc->vrefnum;
	vInfo.volumeParam.ioVolIndex = 0;					/* Use ioVRefNum alone to find vol. */
	vInfo.volumeParam.ioNamePtr = NULL;
	PBHGetVInfo(&vInfo,FALSE);
	
	err = vInfo.volumeParam.ioResult;
	if (err==noErr) {
		vFreeSpace = ((unsigned short)vInfo.volumeParam.ioVFrBlk) * 
                                   vInfo.volumeParam.ioVAlBlkSiz;
		*vAlBlkSize = vInfo.volumeParam.ioVAlBlkSiz;
		return vFreeSpace;
	} 

	return (long)err;
}

/* Tell the user we can't do a safe save and ask what they want to do. Two different
alerts, one if we can save on the specified volume at all, one if not. Coded so that
the two alerts must have identical item nos. for Save As and Cancel. */

static short AskSaveType(Boolean canContinue)
{
	short saveType;

	if (canContinue)
		saveType = StopAlert(SAFESAVE_ALRT,NULL);
	else
		saveType = StopAlert(SAFESAVE1_ALRT,NULL);

	if (saveType==1) return SF_SaveAs;
	if (saveType==3) return SF_Replace;
	return SF_Cancel;
}

/* Check to see if a file can be saved safely, saved only unsafely, or not saved
at all, onto the current volume. If it cannot be saved safely, ask user what
to do. */

static short GetSaveType(Document *doc, Boolean saveAs)
{
	Boolean canContinue;
	long fileSize,freeSpace,oldFileSize,vAlBlkSize;
	
	/* If doc->new, no previous document to protect from the save operation.
		If saveAs, the previous doc will not be replaced, but a new one will
		be created. */

	if (doc->docNew || saveAs)
		oldFileSize = 0L;
	else {
		/* Get the amount of space physically allocated to the old file,
			and the amount of space available on doc's volume. Return value
			< 0 indicates FS Error: should forget safe saving. ??GetOldFileSize
			doesn't do a very good job: see comments on it. */
	
		oldFileSize = GetOldFileSize(doc);
		if (oldFileSize<0L) {
			GetIndCString(strBuf, FILEIO_STRS, 13);    /* "Can't get the existing file's size." */
			CParamText(strBuf, "", "", "");
			StopInform(SAFESAVEINFO_ALRT);
			return AskSaveType(TRUE);					/* Let them try to save on this vol. anyway */
		}
	}

	freeSpace = GetFreeSpace(doc,&vAlBlkSize);
	if (freeSpace<0L) {
		GetIndCString(strBuf, FILEIO_STRS, 14);    /* "Can't get the disk free space." */
		CParamText(strBuf, "", "", "");
		StopInform(SAFESAVEINFO_ALRT);
		return AskSaveType(TRUE);						/* Let them try to save on this vol. anyway */
	}

	fileSize = GetFileSize(doc,vAlBlkSize);

	/* If we have enough space to save safely, we're done (although, if the doc
		is new or it's a Save As, we tell the caller they don't NEED to do a safe
		save). If not, ask the user what to do. If canContinue, they can still
		replace the previous file; otherwise, they can only Save As or Cancel. */

	if (fileSize <= freeSpace)
		return ((doc->docNew || saveAs)? SF_Replace : SF_SafeSave);
	
	canContinue = (fileSize <= freeSpace + oldFileSize);
	return AskSaveType(canContinue);
}

/* Check validity of doc about to be saved to avoid disasterous problems, mostly
the danger of overwriting a valid file with a bad one. Originally written
specifically to check nEntries fields in object list because of "Mackey's Disease",
which made it impossible to open some saved files! */

static Boolean SFChkScoreOK(Document */*doc*/)
{
#ifndef PUBLIC_VERSION
#ifdef CURE_MACKEYS_DISEASE
	if (DCheckNEntries(doc)) {
		SysBeep(30);
		AlwaysErrMsg("INCONSISTENT SUBOBJ COUNT: can't write a readable file! FILE NOT SAVED. Call Don Byrd at 413-268-7313.");
		return FALSE;
	}
#endif
#endif

	return TRUE;
}

/* Actually write the file. Write header info, write the string pool, call WriteHeaps
to write data structure objects, and write the EOF marker. If any error is
encountered, return that error without continuing. Otherwise, return noErr. */

static short WriteFile(Document *doc, short refNum)
{
	short			errCode;
	short			lastType=LASTtype;
	long			count, blockSize, strHdlSize;
	unsigned long	fileTime;
	Handle			stringHdl;
	OMSSignature	omsDevHdr;
	long			omsDevSize, fmsDevHdr;
	long			cmDevSize, cmHdr;

	version = THIS_VERSION;											/* Write version code */
	count = sizeof(version);
	errCode = FSWrite(refNum, &count, &version);
	if (errCode) return VERSIONobj;

	GetDateTime(&fileTime);											/* Write current date and time */
#define VERSION_KLUDGE
#ifdef VERSION_KLUDGE
	if (version=='N101') fileTime &= 0x7FFFFFFF;				/* Fake "sub-version" N101+1/2 */
#endif
	count = sizeof(fileTime);
	errCode = FSWrite(refNum, &count, &fileTime);
	if (errCode) return VERSIONobj;

	count = sizeof(DOCUMENTHDR);
	errCode = FSWrite(refNum, &count, &doc->origin);
	if (errCode) return HEADERobj;
	
	count = sizeof(SCOREHEADER);
	errCode = FSWrite(refNum, &count, &doc->headL);
	if (errCode) return HEADERobj;
	
	count = sizeof(lastType);
	errCode = FSWrite(refNum, &count, &lastType);
	if (errCode) return HEADERobj;

	stringHdl = (Handle)GetStringPool();
	HLock(stringHdl);

	strHdlSize = GetHandleSize(stringHdl);
	count = sizeof(strHdlSize);
	errCode = FSWrite(refNum, &count, &strHdlSize);
	if (errCode) return STRINGobj;

	errCode = FSWrite(refNum, &strHdlSize, *stringHdl);
	HUnlock(stringHdl);
	if (errCode) return STRINGobj;

	errCode = WriteHeaps(doc, refNum);
	if (errCode) return errCode;

	/* Write info for OMS */
	count = sizeof(OMSSignature);
	omsDevHdr = 'devc';
	errCode = FSWrite(refNum, &count, &omsDevHdr);
	if (errCode) return SIZEobj;
	count = sizeof(long);
	omsDevSize = (MAXSTAVES+1) * sizeof(OMSUniqueID);
	errCode = FSWrite(refNum, &count, &omsDevSize);
	if (errCode) return SIZEobj;
	errCode = FSWrite(refNum, &omsDevSize, &(doc->omsPartDeviceList[0]));
	if (errCode) return SIZEobj;

	/* Write info for FreeMIDI input device:
			1) long having the value 'FMS_' (just a marker)
			2) fmsUniqueID (unsigned short) giving input device ID
			3) destinationMatch union giving info about input device */
	count = sizeof(long);
	fmsDevHdr = FreeMIDISelector;
	errCode = FSWrite(refNum, &count, &fmsDevHdr);
	if (errCode) return SIZEobj;
	count = sizeof(fmsUniqueID);
	errCode = FSWrite(refNum, &count, &doc->fmsInputDevice);
	if (errCode) return SIZEobj;
	count = sizeof(destinationMatch);
	errCode = FSWrite(refNum, &count, &doc->fmsInputDestination);
	if (errCode) return SIZEobj;
	
	/* Write info for CoreMIDI [version >= N105] */
	
	count = sizeof(long);
	cmHdr = 'cmdi';
	errCode = FSWrite(refNum, &count, &cmHdr);
	if (errCode) return SIZEobj;
	count = sizeof(long);
	cmDevSize = (MAXSTAVES+1) * sizeof(MIDIUniqueID);
	errCode = FSWrite(refNum, &count, &cmDevSize);
	if (errCode) return SIZEobj;
	errCode = FSWrite(refNum, &cmDevSize, &(doc->cmPartDeviceList[0]));
	if (errCode) return SIZEobj;	

	blockSize = 0L;													/* mark end w/ 0x00000000 */
	count = sizeof(blockSize);
	errCode = FSWrite(refNum, &count, &blockSize);
	if (errCode) return SIZEobj;

	return noErr;
}

static Boolean GetOutputFile(Document *doc)
{
	Str255 name; short vrefnum;
	OSErr result;
	FInfo fInfo;
	FSSpec fsSpec;
	NSClientData nscd;
	
	Pstrcpy(name,doc->name);
	if (GetOutputName(MiscStringsID,3,name,&vrefnum,&nscd)) {
	
		fsSpec = nscd.nsFSSpec;
		//result = FSMakeFSSpec(vrefnum, 0, name, &fsSpec);
		
		//result = GetFInfo(name, vrefnum, &fInfo);
		//if (result == noErr)
			result = FSpGetFInfo (&fsSpec, &fInfo);
		
		/* This check and error message also appear in DoSaveAs. */
		if (result == noErr && fInfo.fdType != documentType) {
			GetIndCString(strBuf, FILEIO_STRS, 11);    /* "You can replace only files created by Nightingale." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}

		//HSetVol(NULL,vrefnum,0);
		result = HSetVol(NULL,fsSpec.vRefNum,fsSpec.parID);
		/* Save the file under this name */
		Pstrcpy(doc->name,name);
		doc->vrefnum = vrefnum;
		SetWTitle((WindowPtr)doc,name);
		doc->changed = FALSE;
		doc->named = TRUE;
		doc->readOnly = FALSE;
	}
	
	return TRUE;
}


/* ----------------------------------------------------------------------- SaveFile -- */
/*	Routine that actually saves the specifed file.  If there's an error, gives an
error message and returns <errCode>; else returns 0 if successful, CANCEL_INT if
operation is cancelled. */

#define TEMP_FILENAME "\p**NightTemp**"

/* Given a filename and a suffix for a variant of the file, append the suffix to the
filename, truncating if necessary to make the result a legal filename. NB: for
systems whose filenames have extensions, like MS Windows, we should probably replace
the extension, though even here some disagree. For other systems, like MacOS and UNIX,
it's really unclear what to do: this is particularly unfortunate because if a file
exists with the resulting name, the calling routine may be planning to overwrite it!
Return TRUE if we can do the job without truncating the given filename, FALSE if we
have to truncate (this is the more dangerous case).

??This should be used elsewhere, e.g., DoPostScript, SaveNotelist, NameMFScore. */

#define FILENAME_MAXLEN 31		/* MacOS Finder's limit is 31 */

Boolean MakeVariantFilename(Str255 filename, char *suffix, Str255 bkpName);
Boolean MakeVariantFilename(
			Str255 filename,		/* Pascal string */
			char *suffix,			/* C string */
			Str255 bkpName			/* Pascal string */
			)
{
	short bkpNameLen; Boolean truncated = FALSE;
	
	Pstrcpy(bkpName, filename);
	PToCString(bkpName);

	if (strlen((char *)bkpName)+strlen(suffix)>FILENAME_MAXLEN) {
		bkpNameLen = FILENAME_MAXLEN-strlen(suffix);
		bkpName[bkpNameLen] = '\0';
		truncated = TRUE;
	}

	strcat((char *)bkpName, suffix);
	CToPString((char *)bkpName);
	
	return truncated;
}

static short RenameAsBackup(Str255 filename, short vRefNum, Str255 bkpName);
static short RenameAsBackup(Str255 filename, short vRefNum, Str255 bkpName)
{
	short errCode;
	FSSpec fsSpec;
	FSSpec bkpFSSpec;

	MakeVariantFilename(filename, ".bkp", bkpName);
		
	errCode = FSMakeFSSpec(vRefNum, 0, bkpName, &bkpFSSpec);
	if (errCode && errCode!=fnfErr)
		return errCode;

	//errCode = FSDelete(bkpName, vRefNum);						/* Delete old file */
	errCode = FSpDelete(&bkpFSSpec);								/* Delete old file */
	if (errCode && errCode!=fnfErr)								/* Ignore "file not found" */
		return errCode;

	errCode = FSMakeFSSpec(vRefNum, 0, filename, &fsSpec);
	if (errCode)
		return errCode;

	//errCode = Rename(filename,vRefNum,bkpName);
	errCode =  FSpRename (&fsSpec, bkpName);

	return errCode;
}


short SaveFile(
			Document *doc,
			Boolean saveAs			/* TRUE if a Save As operation, FALSE if a Save */
			)
{
	Str255			filename, bkpName;
	short			vRefNum;
	short			errCode, refNum;
	short 			errInfo=noErr;				/* Type of object being read or other info on error */
	short			saveType;
	Boolean			fileOpened;
	const unsigned char	*tempName;
	ScriptCode		scriptCode = smRoman;
	FSSpec 			fsSpec;
	FSSpec 			tempFSSpec;
	
		
	if (!SFChkScoreOK(doc))
		{ errInfo = NENTRIESerr; goto Error; }

	Pstrcpy(filename,doc->name);
	vRefNum = doc->vrefnum;
	fsSpec = doc->fsSpec;
	fileOpened = FALSE;

TryAgain:
	WaitCursor();
	saveType = GetSaveType(doc,saveAs);

	if (saveType==SF_Cancel) return CANCEL_INT;					/* User cancelled or FSErr */

	if (saveType==SF_SafeSave) {
		/* Create and open a temporary file */
		tempName = TEMP_FILENAME;

		//errCode = FSMakeFSSpec(vRefNum, 0, tempName, &tempFSSpec);
		errCode = FSMakeFSSpec(vRefNum, fsSpec.parID, tempName, &tempFSSpec);
		if (errCode && errCode!=fnfErr)
			{ errInfo = MAKEFSSPECcall; goto Error; }
		
		/* Without the following, if TEMP_FILENAME exists, Safe Save gives an error. */
		//errCode = FSDelete(tempName, vRefNum);					/* Delete old file */
		errCode = FSpDelete(&tempFSSpec);							/* Delete any old temp file */
		if (errCode && errCode!=fnfErr)								/* Ignore "file not found" */
			{ errInfo = DELETEcall; goto Error; }

		//errCode = Create(tempName, vRefNum, creatorType, documentType); /* Create new file */
		errCode = FSpCreate (&tempFSSpec, creatorType, documentType, scriptCode);
		if (errCode) { errInfo = CREATEcall; goto Error; }
		
		//errCode = FSOpen(tempName, vRefNum, &refNum);				/* Open it */
		errCode = FSpOpenDF (&tempFSSpec, fsRdWrPerm, &refNum );	/* Open the temp file */
		if (errCode) { errInfo = OPENcall; goto Error; }
	}
	else if (saveType==SF_Replace) {
		//errCode = FSMakeFSSpec(vRefNum, 0, filename, &fsSpec);
		//if (errCode && errCode!=fnfErr)
		//	{ errInfo = MAKEFSSPECcall; goto Error; }
		fsSpec = doc->fsSpec;
		
		//errCode = FSDelete(filename, vRefNum);						/* Delete old file */
		errCode = FSpDelete(&fsSpec);									/* Delete old file */
		if (errCode && errCode!=fnfErr)								/* Ignore "file not found" */
			{ errInfo = DELETEcall; goto Error; }
			
		//errCode = Create(filename, vRefNum, creatorType, documentType); /* Create new file */
		errCode = FSpCreate (&fsSpec, creatorType, documentType, scriptCode);
		if (errCode) { errInfo = CREATEcall; goto Error; }
		
		//errCode = FSOpen(filename, vRefNum, &refNum);			/* Open it */
		errCode = FSpOpenDF (&fsSpec, fsRdWrPerm, &refNum );	/* Open the file */
		if (errCode) { errInfo = OPENcall; goto Error; }
	}
	else if (saveType==SF_SaveAs) {
		if (!GetOutputFile(doc)) return CANCEL_INT;				/* User cancelled or FSErr */

		/* The user may have just told us a new disk to save on, so start over. */
		
		saveAs = TRUE;
		Pstrcpy(filename,doc->name);
		vRefNum = doc->vrefnum;
		fsSpec = doc->fsSpec;
		goto TryAgain;
	}

	/* At this point, the file we want to write (either the temporary file or the
		"real" one) is open and <refNum> is set for it. */
		
	doc->docNew = FALSE;
	doc->vrefnum = vRefNum;
	doc->fsSpec	= fsSpec;
	fileOpened = TRUE;

	errCode = WriteFile(doc,refNum);
	if (errCode) { errInfo = WRITEcall; goto Error; };

	if (saveType==SF_SafeSave) {
		/* Close the temporary file; rename as backup or simply delete file at filename
		 * on vRefNum (the old version of the file); and rename the temporary file to
		 * filename and vRefNum. Note that, from the user's standpoint, this replaces
		 * the file's creation date with the current date--unfortunate.
		 * 
		 * ??Inside Mac VI, 25-9ff, points out that System 7 introduces FSpExchangeFiles
		 * and PBExchangeFiles, which simplify a safe save by altering the catalog entries
		 * for two files to swap their contents. However, if we're keeping a backup copy,
		 * this would result in the backup having the current date as its creation date!
		 * One solution would be to start the process of saving the new file by making a
		 * copy of the old version instead of a brand-new file; this would result in a
		 * fair amount of extra I/O, but it might be the best solution.
		 */

		if (config.makeBackup) {
			errCode = RenameAsBackup(filename,vRefNum,bkpName);
			if (errCode && errCode!=fnfErr)								/* Ignore "file not found" */
				{ errInfo = BACKUPcall; goto Error; }
		}
		else {
			FSSpec oldFSSpec;
		
			//errCode = FSMakeFSSpec(vRefNum, 0, filename, &oldFSSpec);
			//if (errCode && errCode!=fnfErr)
			//	{ errInfo = MAKEFSSPECcall; goto Error; }
			
			oldFSSpec = fsSpec;
		
			//errCode = FSDelete(filename, vRefNum);						/* Delete old file */
			errCode = FSpDelete(&oldFSSpec);									/* Delete old file */
			if (errCode && errCode!=fnfErr)								/* Ignore "file not found" */
				{ errInfo = DELETEcall; goto Error; }
		}

		errCode = FSClose(refNum);										/* Close doc's file */
		if (errCode) { errInfo = CLOSEcall; goto Error; }
		
		//errCode = Rename(tempName,vRefNum,filename);
		errCode =  FSpRename (&tempFSSpec, filename);
		if (errCode) { errInfo = RENAMEcall; goto Error; }		/* Rename temp to old filename */
			
	}
	else if (saveType==SF_Replace) {
		errCode = FSClose(refNum);										/* Close doc's file */
		if (errCode) { errInfo = CLOSEcall; goto Error; }
	}
	else if (saveType==SF_SaveAs) {
		errCode = FSClose(refNum);										/* Close the newly created file */
		if (errCode) { errInfo = CLOSEcall; goto Error; }
	}

	doc->changed = FALSE;												/* Score isn't dirty... */
	doc->saved = TRUE;													/* ...and it's been saved */

	/* Save any print record or other resources in the resource fork */

	WritePrintHandle(doc);												/* Ignore any errors */
	
	/* Save the midi map in the resource fork */

	SaveMidiMap(doc);														/* Ignore any errors */
	

	FlushVol(NULL, vRefNum);
	return 0;

Error:
	SaveError(fileOpened, refNum, errCode, errInfo);
	return errCode;
}

/* --------------------------------------------------------------------- SaveError -- */
/* Handle errors occurring while writing a file. Parameters are the same as those
for OpenError(). */

void SaveError(Boolean fileOpened,
					short refNum,
					short errCode,
					short errInfo)		/* More info: at what step error happened, type of obj being written, etc. */
{
	char aStr[256], fmtStr[256]; StringHandle strHdl;
	short strNum;

	if (fileOpened) FSClose(refNum);

	/*
	 * We expect descriptions of the common errors stored by code (negative
	 * values, for system errors; positive ones for our own I/O errors) in
	 * individual 'STR ' resources. If we find one for this error, print it,
	 * else just print the raw code.
	 */
	strHdl = GetString(errCode);
	if (strHdl) {
		Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
		PToCString((unsigned char *)strBuf);
		strNum = (errInfo>0? 15 : 16);	/* "%s (heap object type=%d)." : "%s (error code=%d)." */
		GetIndCString(fmtStr, FILEIO_STRS, strNum);
		sprintf(aStr, fmtStr, strBuf, errInfo);
	}
	else {
		strNum = (errInfo>0? 17 : 18);	/* "Error ID=%d (heap object type=%d)." : "Error ID=%d (error code=%d)." */
		GetIndCString(fmtStr, FILEIO_STRS, strNum);
		sprintf(aStr, fmtStr, errCode, errInfo);
	}

#ifndef PUBLIC_VERSION
	LogPrintf(LOG_NOTICE, aStr); LogPrintf(LOG_NOTICE, "\n");
#endif
	CParamText(aStr, "", "", "");
	StopInform(SAVE_ALRT);
}
