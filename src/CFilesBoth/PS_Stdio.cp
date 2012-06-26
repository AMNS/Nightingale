/*	PS_Stdio.c for Nightingale - medium- and low-level PostScript output, rev. for v.2000. */

/*										NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROPERTY OF
 *	ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE SECRET AND IS
 *	NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED WRITTEN
 *	AUTHORIZATION FROM THE OWNER.
 *
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* QuickDraw comment codes */

#define PostScriptBegin			190
#define PostScriptEnd			191
#define PostScriptHandle		192
#define PostScriptFile			193
#define TextIsPostScript		194
#define ResourcePS				195
#define PostScriptBeginNoSave	196

/* Parameters */

#define PSBUFSIZE	256

#define DDFact		pt2d(1)		 /* points-to-DDIST factor */

/*
 *	These are used to remember the type of last font used, so that we can optimize
 *	font changes.
 */

enum {
	F_None,
	F_Music,
	F_Other
	};

/* Prototypes for private routines */

static void	PS_InitGlobals(Document *doc);
static void	PS_RestartPageVars(void);
static Boolean PS_SetMusicFont(Document *doc, short sizePercent);
static void	PS_FontRunAround(Document *doc, short fontNum, short fontSize, short fontStyle);
static void	PS_Char(short ch);
static char	*PS_PrtLong(unsigned long arg, Boolean negArg, Boolean doSign,
						Boolean noSign, short base, short width, short *len);
static void	PS_Recompute(void);
static OSErr	PS_Comment(char *buffer, long nChars);
static Byte 	*PS_NthString(Byte *stringList, short n);

/* Private globals */

static Document *psDoc;
static Boolean usingPrinter;		/* Send buffered text straight to printer */
static Boolean usingFile;			/* Send buffered text to file */
static Boolean usingHandle;			/* Send buffered text to a handle */
static Boolean inQD;				/* TRUE when in QuickDraw world; FALSE in PostScript */
static short thisFile;				/* ID of currently open file */
static short thisVolume;			/* Current directory ID */
static OSErr thisError;				/* Latest report from the front */
static Boolean fileOpened;			/* TRUE when there is an opened file */
static Boolean handleOpened;		/* TRUE when there is an opened handle */
static Handle prec103;				/* The resource to be downloaded once per job */
static Boolean skipHeader;			/* TRUE if PS_Header has been used to build PREC */
static Handle theTextHandle;
static char *buffer,*bufTop,*bp;	/* For buffered output to file */
static short thisFont;				/* Latest set font */
static short musicPtSize;			/* Latest point size of latest music font */
static short musicSizePercent;		/* Latest percent scaling of music font */
static short numPages;				/* Number of pages to be printed */
static Boolean pageEnded;			/* TRUE if we've not printing since last page */
static Boolean doEscapes;			/* TRUE when outputting a PostScript string */
static Boolean printLinewidths;		/* TRUE to print linewidths at top of page */
static short unknownFonts;			/* Number of references to unknown fonts */
static DDIST oldBar,				/* For optimizing PostScript versions */
			 oldStem,
			 oldLedger,
			 oldStaff;

static DDIST pageWidth,				/* Sizes of page in whatever coordinates */
			 pageHeight,
			 wStaff,wLedger,		/* Widths of various lines to be drawn */
			 wBar,wStem,
			 wConnect,
			 lineSpace,				/* Distance between adjacent staff lines */
			 stemFudge;				/* For shortening stems at notehead end */


/* ============================================================== PUBLIC ROUTINES == */

/* Open file and prepare to output PostScript.  If another file is already 
open, then we close it before opening the new one.

If usingWhat is USING_FILE, PostScript text we create is sent to the
given filename and directory.
If usingWhat is USING_PRINTER, the PostScript text is sent to the
current printer, and the other arguments are ignored.
If usingWhat is USING_HANDLE, the PostScript text is stored in the buffer
as a handle, to be retrieved later, and other args are ignored. */

OSErr PS_Open(Document *doc, unsigned char */*fileName*/, short vRefNum,
										short usingWhat, ResType fileType, FSSpec *fsSpec)
	{
		if (usingWhat == USING_FILE) {
			
			/* Don't open anything new until old open is shut first */
			
			if (fileOpened)
				if (thisError = PS_Close()) return(thisError);
			
			usingFile = TRUE;
			usingPrinter = usingHandle = FALSE;

			HSetVol(NULL,fsSpec->vRefNum,fsSpec->parID);
			FSpDelete(fsSpec);									/* Don't care if notFound error */
			thisError = FSpCreate (fsSpec, creatorType, 
									(ShiftKeyDown()||OptionKeyDown())?'TEXT':fileType, smRoman);
			if (thisError==noErr || thisError==dupFNErr) {
				thisError = FSpOpenDF(fsSpec, fsRdWrPerm, &thisFile);
	 			if (thisError) return(thisError);
	 			}
	 		 else
	 			return(thisError);
 			}
 		 else if (usingWhat == USING_PRINTER) {
 			usingPrinter = TRUE;
 			usingFile = usingHandle = FALSE;
 			}
 		 else if (usingWhat == USING_HANDLE) {
 			if (handleOpened)
 				if (thisError = PS_Close()) return(thisError);
 			
 			usingHandle = TRUE;
 			usingPrinter = usingFile = FALSE;
 			
 			/* Create the place to accumulate text (must be 0 in size initially) */
 			theTextHandle = NewHandle(0L);
 			thisError = MemError();
 			if (thisError) return(thisError);
 			}
 		
 		/*
 		 *	Now try to allocate temporary buffer for text output.
 		 *	This may be a different call for MultiFinder, and could
 		 *	just as well be a locked handle somewhere high in memory.
 		 */
 		
 		buffer = (char *)NewPtr(PSBUFSIZE);
 		if (thisError = MemError()) return(thisError);
 		
 		bufTop = buffer + PSBUFSIZE;	/* First char above buffer */
 		bp = buffer;					/* Next char to be placed */
 		
 		if (usingFile) {
 			/* File is open and empty at this point */
			thisVolume = vRefNum;
			fileOpened = TRUE;
			}
		 else if (usingHandle) {
			handleOpened = TRUE;
			}
		
		printLinewidths = (CapsLockKeyDown() && OptionKeyDown());

		PS_InitGlobals(doc);

 		return(thisError);
 	}

/*
 *	Close the currently open PostScript output file and/or printer connection,
 *	writing whatever tail information needed to wind things down.  If no currently
 *	open output file, does nothing.
 */

OSErr PS_Close()
	{
		PS_Flush();
		
		if (usingFile && fileOpened) {
			thisError = FSClose(thisFile);
			FlushVol(NULL,thisVolume);
			fileOpened = FALSE;
			}
		 else if (usingHandle && handleOpened) {
			if (theTextHandle) DisposeHandle(theTextHandle);
			theTextHandle = NULL;
			handleOpened = FALSE;
			thisError = noErr;
			}
		
		if (buffer) {
			DisposePtr(buffer);
			buffer = bufTop = bp = NULL;
			}
		
		return(thisError);
	}

/*
 *	Deliver the handle to the current text being formatted using PS_Open() with
 *	USING_HANDLE.  Caller should not dispose of the reference delivered by this
 *	routine; PS_Close is for doing that.
 */

Handle PS_GetTextHandle()
	{
		PS_Flush();
		return(theTextHandle);
	}

/*
 *	Write out the PostScript preambles and definitions for every Nightingale
 *	print file.  If we're being called with usingPrinter, then most of the
 *	arguments here are ignored, since the Print Manager is automatically taking
 *	care of landscape, etc.
 */

OSErr PS_Header(Document *doc, const unsigned char *docName, short nPages, FASTFLOAT scaleFactor,
						Boolean landscape, Boolean doEncoding, Rect *bBox, Rect *paper)
	{
		short percent,width,height,paperWidth,paperHeight,resID;
		Rect box; unsigned long dateTime; Str255 dateTimeStr;
		static Rect thisImageRect;
		char fmtStr[256];
		char str[256];
		Byte glyph;
		double dPercent;
		double dSdcf;
		long lPercent;
		long sdcf;        
		short temp;
		
		if (paper) thisImageRect = *paper;		/* thisImageRect must be static */

		numPages = nPages;
		unknownFonts = 0;
		
		percent = scaleFactor * 100.0;
		if (percent <= 0) percent = 100;
		
		if (usingFile) {
			PS_Print("%%!PS-Adobe-2.0 EPSF-1.2\r");
			PS_Print("%%%%Title: %p\r",docName);
			PS_Print("%%%%Creator: %s %s\r",PROGRAM_NAME,applVerStr);
			GetDateTime(&dateTime);
			//IUDateString(dateTime,abbrevDate,dateTimeStr);
			DateString(dateTime,abbrevDate,dateTimeStr,NULL);
			PS_Print("%%%%CreationDate: %p",dateTimeStr);
			//IUTimeString(dateTime,TRUE,dateTimeStr);
			TimeString(dateTime,TRUE,dateTimeStr,NULL);
			PS_Print(" [%p]\r",dateTimeStr);
			PS_Print("%%%%Pages: %ld\r",(long)nPages);
			}
		
		paperWidth = scaleFactor * (thisImageRect.right - thisImageRect.left);
		paperHeight= scaleFactor * (thisImageRect.bottom - thisImageRect.top);
		
		if (landscape) {
			short tmp;
			tmp = paperHeight; paperHeight = paperWidth; paperWidth = tmp;
			}
		
		box = *bBox;
		width = box.right - box.left;
		height = box.bottom - box.top;
		/*
		 *	bBox is delivered in screen coordinates, which are upside down
		 *	compared to PostScript's default coordinates, which BoundingBox
		 *	expects them to be in, so we flip w/r/t the page.
		 */
		box.top = paperHeight - bBox->top;
		box.bottom = box.top - (scaleFactor*height);
		box.right *= scaleFactor;
		
		if (landscape) {
			short t,l,r,b;
			l = paperWidth - (height*scaleFactor);		/* Paper width minus image height */
			r = paperWidth;
			t = paperHeight;
			b = paperHeight - (width*scaleFactor);
			SetRect(&box,l,t,r,b);
			}

		if (usingFile) {
			PS_Print("%%%%BoundingBox: %ld %ld %ld %ld\r",(long)box.left,(long)box.bottom,(long)box.right,(long)box.top);
			PS_Print("%%%%DocumentFonts: (atend)\r");
			PS_Print("%%%%EndComments\r\r");
			}
		/*
		 *	Read TEXT resources in which we keep our music drawing procedures as
		 *	well as a Sonata encoding (for homebrew PostScript output). The
		 *	following is a table of contents of words we can call upon:
		 *		BK	Bracket
		 *		BL	Barline
		 *		BM	Beam, or other thickened line with vertical cutoff
		 *		BP	Begin Page
		 *		BR	Brace
		 *		EP	End Page
		 *		LHT	Thickened line with horizontal cutoff
		 *		LL	Ledgerline
		 *		MC	Move and curveto
		 *		MF	MusicFont (sets size, quarternote width, etc.)
		 *		ML	Move and Line
		 *		MS	Move and Show
		 *		NF	New Font
		 *		NMF	New Music Font (changes size only)
		 *		SD	Notehead with Stem Down: see details below
		 *		SDI	Invisible notehead with Stem Down: see details below
		 *		SFL	Staffline
		 *		SL	Slur (curveto and closepath)
		 *		SU	Notehead with Stem Up
		 */
		 
		if (usingFile)
 			PS_Print("/NightTopSave save def\r\r");				/* restore in Trailer */
					 
		// MAS: Declare ddFact before first jump
		short ddFact;
		
		if (PS_Resource(-1,'TEXT',resID=128)) goto PSRErr;		/* Start Preamble (1) */

		//PS_Print("%%••••\r");
		/* /qw needs stringwidth of a char whose code depends on the music font.
			Formerly, this was 'q', but MCH_quarterNoteHead is same width as 'q' in
			Sonata, and MCH_quarterNoteHead is a better choice for other fonts. */
		glyph = MapMusChar(doc->musFontInfoIndex, MCH_quarterNoteHead);
		sprintf(str, "\\%o", glyph);
		PS_Print("/SQW {/qw (%s)stringwidth pop def} def\r", str);

		if (PS_Resource(-1,'TEXT',resID=129)) goto PSRErr;		/* Start Preamble (2) */

		//PS_Print("%%••••\r");
		/*	Curly braces. For fonts that have curly brace chars, we use them, but also
			provide a homebrew method that draws the braces without relying on the font.
			(Why was this provided before?) For fonts, like Petrucci, that don't have
			the brace chars, we use only the homebrew method.  -JGG */
		if (PS_Resource(-1,'TEXT',resID=130)) goto PSRErr;		/* Curly brace (1) */
		if (MusFontHasCurlyBraces(doc->musFontInfoIndex)) {
			/* This code is here, instead of in resources, so that we can map brace chars. */
			glyph = MapMusChar(doc->musFontInfoIndex, MCH_braceup);
			sprintf(str, "\\%o", glyph);
			PS_Print("   (%s)CH/cBot XD/cTop XD\r", str);
			if (PS_Resource(-1,'TEXT',resID=131)) goto PSRErr;	/* Curly brace (2), both */
			PS_Print("      0 cBot abs neg rmoveto(%s)show\r", str);
			PS_Print("      grestore\r");
			glyph = MapMusChar(doc->musFontInfoIndex, MCH_bracedown);
			sprintf(str, "\\%o", glyph);
			PS_Print("      (%s)CH/cBot XD/cTop XD\r", str);
			PS_Print("      /cht cTop cBot sub abs def\r");
			PS_Print("      0 cTop abs rmoveto(%s)show\r", str);
			PS_Print("   }\r");
			PS_Print("   ifelse\r\r");
			PS_Print("   grestore\r");
			PS_Print("} BD\r");
		}
		else {
			if (PS_Resource(-1,'TEXT',resID=132)) goto PSRErr;	/* Curly brace (2), homebrew only */
		}

		//PS_Print("%%••••\r");
		if (doEncoding)
			if (PS_Resource(-1,'TEXT',resID=133)) goto PSRErr;	/* Encoding */

		//PS_Print("%%••••\r");
		/* Now directly print stuff that needs information from here */
		
		PS_Print("/MFS %ld def\r",(long)musicPtSize);
		PS_Print("/DCF %ld def\r",(long)DDFact);
		ddFact = DDFact;
		if ((ddFact != 16) || (percent != 100))
		{
			temp = 0;
			temp++;
		}
		dPercent = percent/100.0;
		dSdcf = dPercent * DDFact;
		lPercent = dPercent;
		sdcf = dSdcf;
		//PS_Print("/SDCF %ld %ld mul def\r", lPercent);
		PS_Print("/SDCF %ld 100 %ld div mul def\r",(long)DDFact,(long)percent);
		
		/*
		 *	Transformation matrix that we will multiply the current transformation
		 *	matrix by.  After the multiplication, PostScript's user coordinate system
		 *	should be the same as our paper-relative DDIST upper-left origin system.
		 */
		
		if (usingFile)
			if (landscape) 
				PS_Print("/MX [0 -1 SDCF div -1 SDCF div 0 %l %l] def\r",
								paperWidth,paperHeight);
			 else
				PS_Print("/MX [1 SDCF div 0 0 -1 SDCF div 0 %ld] def\r",(long)paperHeight);
		 else {
		 	/*
		 	 *	LaserPrep "kindly" makes the simulated QuickDraw origin the first
		 	 *	imageable point on the page, which is about 30 pts in from the upper
		 	 *	left corner.  We undo this "favor" here by adding back their translation
		 	 *	component, since Nightingale gives the user the illusion that all
		 	 *	parts of a piece of paper are printable on, and since its origin is
		 	 *	the upper left corner of the sheet.  Actually, Nightingale
		 	 *	should not allow the user to set the margins (in Master Page) to
		 	 *	something less than the current imageable area.
		 	 */
			PS_Print("/MX [1 SDCF div 0 0 1 SDCF div %ld %ld] def\r",
								(long)thisImageRect.left,(long)thisImageRect.top);
			}

		/* And finish up with final matrix-related stuff */
		
		if (PS_Resource(-1,'TEXT',resID=134)) goto PSRErr;	/* End Preamble */
		//PS_Print("%%••••\r");

		/*	Need the PostScript font name of our music font, which can differ from the screen font
			name. IM Font Mgr. Ref., p. 102, gives code for doing this for a 'FOND' rsrc.  Hints 
			about how to do it for 'sfnt' appear earlier.  But all this is *way* more complicated 
			than you'd expect, so we just store the PS font name in the 'MFEx' resource. You can 
			get this name using the old Varityper ToolKit program. */
		PS_Print("/MF {/%p findfont exch DCF mul dup neg FMX scale makefont setfont} BD\r",
											musFontInfo[doc->musFontInfoIndex].postscriptFontName);

		PS_Print("\rend         %% NightingaleDict\r\r");
		//PS_Print("%%••••\r");

		/*
		 *	Once the final transformation is settled, we'll push it onto the graphics
		 *	state stack, to be restored and pushed again after every showpage.  This
		 *	keeps it the same on every page, since showpage resets it.
		 */
		
		if (usingFile)
			PS_Print("%%%%EndProlog\r");

		return thisError;

PSRErr:
		GetIndCString(fmtStr, PRINTERRS_STRS, 5);    /* "Can't get PostScript resource 'TEXT' %d." */
		sprintf(strBuf, fmtStr, resID); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return thisError;

	}
	
OSErr PS_HeaderHdl(Document *doc, unsigned char *docName, short nPages, FASTFLOAT scaleFactor,
						Boolean landscape, Boolean doEncoding, Rect *bBox, Rect *paper)
	{
		short percent,width,height,paperWidth,paperHeight,resID;
		Rect box; unsigned long dateTime; Str255 dateTimeStr;
		static Rect thisImageRect;
		char fmtStr[256];
		char str[256];
		Byte glyph;
		
		if (paper) thisImageRect = *paper;		/* thisImageRect must be static */

		numPages = nPages;
		unknownFonts = 0;
		
		percent = scaleFactor * 100.0;
		if (percent <= 0) percent = 100;
		
		if (usingFile) {
			PS_Print("%%!PS-Adobe-2.0 EPSF-1.2\r");
			PS_Print("%%%%Title: %p\r",docName);
			PS_Print("%%%%Creator: %s %s\r",PROGRAM_NAME,applVerStr);
			GetDateTime(&dateTime);
			//IUDateString(dateTime,abbrevDate,dateTimeStr);
			DateString(dateTime,abbrevDate,dateTimeStr,NULL);
			PS_Print("%%%%CreationDate: %p",dateTimeStr);
			//IUTimeString(dateTime,TRUE,dateTimeStr);
			TimeString(dateTime,TRUE,dateTimeStr,NULL);
			PS_Print(" [%p]\r",dateTimeStr);
			PS_Print("%%%%Pages: %ld\r",(long)nPages);
			}
		
		paperWidth = scaleFactor * (thisImageRect.right - thisImageRect.left);
		paperHeight= scaleFactor * (thisImageRect.bottom - thisImageRect.top);
		
		if (landscape) {
			short tmp;
			tmp = paperHeight; paperHeight = paperWidth; paperWidth = tmp;
			}
		
		box = *bBox;
		width = box.right - box.left;
		height = box.bottom - box.top;
		/*
		 *	bBox is delivered in screen coordinates, which are upside down
		 *	compared to PostScript's default coordinates, which BoundingBox
		 *	expects them to be in, so we flip w/r/t the page.
		 */
		box.top = paperHeight - bBox->top;
		box.bottom = box.top - (scaleFactor*height);
		box.right *= scaleFactor;
		
		if (landscape) {
			short t,l,r,b;
			l = paperWidth - (height*scaleFactor);		/* Paper width minus image height */
			r = paperWidth;
			t = paperHeight;
			b = paperHeight - (width*scaleFactor);
			SetRect(&box,l,t,r,b);
			}

		if (usingFile) {
			PS_Print("%%%%BoundingBox: %ld %ld %ld %ld\r",(long)box.left,(long)box.bottom,(long)box.right,(long)box.top);
			PS_Print("%%%%DocumentFonts: (atend)\r");
			PS_Print("%%%%EndComments\r\r");
			}
		/*
		 *	Read TEXT resources in which we keep our music drawing procedures as
		 *	well as a Sonata encoding (for homebrew PostScript output). The
		 *	following is a table of contents of words we can call upon:
		 *		BK	Bracket
		 *		BL	Barline
		 *		BM	Beam, or other thickened line with vertical cutoff
		 *		BP	Begin Page
		 *		BR	Brace
		 *		EP	End Page
		 *		LHT	Thickened line with horizontal cutoff
		 *		LL	Ledgerline
		 *		MC	Move and curveto
		 *		MF	MusicFont (sets size, quarternote width, etc.)
		 *		ML	Move and Line
		 *		MS	Move and Show
		 *		NF	New Font
		 *		NMF	New Music Font (changes size only)
		 *		SD	Notehead with Stem Down: see details below
		 *		SDI	Invisible notehead with Stem Down: see details below
		 *		SFL	Staffline
		 *		SL	Slur (curveto and closepath)
		 *		SU	Notehead with Stem Up
		 */
		 
		if (usingFile)
 			PS_Print("/NightTopSave save def\r\r");				/* restore in Trailer */
		
		if (PS_Resource(-1,'TEXT',resID=128)) goto PSRErr;		/* Start Preamble (1) */
		//PS_Print("%%••••\r");
		/* /qw needs stringwidth of a char whose code depends on the music font.
			Formerly, this was 'q', but MCH_quarterNoteHead is same width as 'q' in
			Sonata, and MCH_quarterNoteHead is a better choice for other fonts. */
		glyph = MapMusChar(doc->musFontInfoIndex, MCH_quarterNoteHead);
		sprintf(str, "\\%o", glyph);
		PS_Print("/SQW {/qw (%s)stringwidth pop def} def\r", str);

		if (PS_Resource(-1,'TEXT',resID=129)) goto PSRErr;		/* Start Preamble (2) */

		//PS_Print("%%••••\r");
		/*	Curly braces. For fonts that have curly brace chars, we use them, but also
			provide a homebrew method that draws the braces without relying on the font.
			(Why was this provided before?) For fonts, like Petrucci, that don't have
			the brace chars, we use only the homebrew method.  -JGG */
		if (PS_Resource(-1,'TEXT',resID=130)) goto PSRErr;		/* Curly brace (1) */
		if (MusFontHasCurlyBraces(doc->musFontInfoIndex)) {
			/* This code is here, instead of in resources, so that we can map brace chars. */
			glyph = MapMusChar(doc->musFontInfoIndex, MCH_braceup);
			sprintf(str, "\\%o", glyph);
			PS_Print("   (%s)CH/cBot XD/cTop XD\r", str);
			if (PS_Resource(-1,'TEXT',resID=131)) goto PSRErr;	/* Curly brace (2), both */
			PS_Print("      0 cBot abs neg rmoveto(%s)show\r", str);
			PS_Print("      grestore\r");
			glyph = MapMusChar(doc->musFontInfoIndex, MCH_bracedown);
			sprintf(str, "\\%o", glyph);
			PS_Print("      (%s)CH/cBot XD/cTop XD\r", str);
			PS_Print("      /cht cTop cBot sub abs def\r");
			PS_Print("      0 cTop abs rmoveto(%s)show\r", str);
			PS_Print("   }\r");
			PS_Print("   ifelse\r\r");
			PS_Print("   grestore\r");
			PS_Print("} BD\r");
		}
		else {
			if (PS_Resource(-1,'TEXT',resID=132)) goto PSRErr;	/* Curly brace (2), homebrew only */
		}

		//PS_Print("%%••••\r");
		if (doEncoding)
			if (PS_Resource(-1,'TEXT',resID=133)) goto PSRErr;	/* Encoding */

		//PS_Print("%%••••\r");
		/* Now directly print stuff that needs information from here */
		
		PS_Print("/MFS %ld def\r",(long)musicPtSize);
		PS_Print("/DCF %ld def\r",(long)DDFact);
		PS_Print("/SDCF 16 def\r");
		/*
		 *	Transformation matrix that we will multiply the current transformation
		 *	matrix by.  After the multiplication, PostScript's user coordinate system
		 *	should be the same as our paper-relative DDIST upper-left origin system.
		 */
		
		if (usingFile)
			if (landscape) 
				PS_Print("/MX [0 -1 SDCF div -1 SDCF div 0 %l %l] def\r",
								paperWidth,paperHeight);
			 else
				PS_Print("/MX [1 SDCF div 0 0 -1 SDCF div 0 %ld] def\r",(long)paperHeight);
		 else {
		 	/*
		 	 *	LaserPrep "kindly" makes the simulated QuickDraw origin the first
		 	 *	imageable point on the page, which is about 30 pts in from the upper
		 	 *	left corner.  We undo this "favor" here by adding back their translation
		 	 *	component, since Nightingale gives the user the illusion that all
		 	 *	parts of a piece of paper are printable on, and since its origin is
		 	 *	the upper left corner of the sheet.  Actually, Nightingale
		 	 *	should not allow the user to set the margins (in Master Page) to
		 	 *	something less than the current imageable area.
		 	 */
			PS_Print("/MX [1 SDCF div 0 0 1 SDCF div %ld %ld] def\r",
								(long)thisImageRect.left,(long)thisImageRect.top);
			}

		/* And finish up with final matrix-related stuff */
		
		if (PS_Resource(-1,'TEXT',resID=134)) goto PSRErr;	/* End Preamble */
		//PS_Print("%%••••\r");

		/*	Need the PostScript font name of our music font, which can differ from the screen font
			name. IM Font Mgr. Ref., p. 102, gives code for doing this for a 'FOND' rsrc.  Hints 
			about how to do it for 'sfnt' appear earlier.  But all this is *way* more complicated 
			than you'd expect, so we just store the PS font name in the 'MFEx' resource. You can 
			get this name using the old Varityper ToolKit program. */
		PS_Print("/MF {/%p findfont exch DCF mul dup neg FMX scale makefont setfont} BD\r",
											musFontInfo[doc->musFontInfoIndex].postscriptFontName);
		PS_Print("\rend         %% NightingaleDict\r\r");
		//PS_Print("%%••••\r");

		/*
		 *	Once the final transformation is settled, we'll push it onto the graphics
		 *	state stack, to be restored and pushed again after every showpage.  This
		 *	keeps it the same on every page, since showpage resets it.
		 */
		
		if (usingFile)
			PS_Print("%%%%EndProlog\r");

		return thisError;
PSRErr:
		GetIndCString(fmtStr, PRINTERRS_STRS, 5);    /* "Can't get PostScript resource 'TEXT' %d." */
		sprintf(strBuf, fmtStr, resID); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return thisError;
	}


/*
 *	This routine must be called prior to opening the Print Driver.  It pulls in the
 *	stub PREC 103 resource, and replaces its contents with whatever definitions are
 *	going to be constant over all pages and font changes.  Prior to using this, we
 *	were calling PS_Header to insert the Nightingale dictionary into the PostScript
 *	stream at the start of the job, as well as after every font change, due to the
 *	fact that the PostScript mechanism does a Restore/Save every time a font is
 *	re-encoded, which would throw our dictionary away.  The following code implements
 *	the Apple-recommended method instead.
 *
 *	After calling PS_PreparePrintDict, you should call PS_FinishPrintDict in order to
 *	throw the PREC 103 resource away and save any memory.
 *	We gate all of this with the global prec103, which will be NULL if something goes
 *	wrong in PS_PreparePrintDict(), thereby causing the code to revert to our old
 *	method.
 */

void PS_PreparePrintDict(Document *doc, Rect *imageRect)
	{
		Handle text; long size; Rect box;
		short oldFile = CurResFile();
		FSSpec fsSpec;
		
		UseResFile(appRFRefNum);
		prec103 = Get1Resource('PREC',103);
		UseResFile(oldFile);
		
		if (GoodResource(prec103)) {
			HNoPurge(prec103);
			thisError = PS_Open(doc, NULL, 0, USING_HANDLE, 0L, &fsSpec);
			if (thisError) { ReleaseResource(prec103); return; }
			
			/* Now stuff prec103 with stuff from our resources */
			
			SetRect(&box,0,0,0,0);
			PS_Header(doc,"\p??",1,1.0,FALSE,TRUE,&box,imageRect);
			SetHandleSize(prec103,size=GetHandleSize(text = PS_GetTextHandle()));
			BlockMove(*text,*prec103,size);
#ifdef FOR_DEBUGGING_ONLY
			ChangedResource(prec103);
			WriteResource(prec103);
			UpdateResFile(appRFRefNum);
			ExitToShell();
#endif
			}
	}

void PS_FinishPrintDict()
	{
		if (prec103)
			ReleaseResource(prec103);
	}


typedef struct {
	char	*macFont;
	short	style;
	char	*postFont;
} FontNameMap;

#define FONTSUBST	TRUE	/* Do standard font substitution? */

/* Given a Macintosh font name (as a P string) and style bits, replace the font name
with the equivalent PostScript font name, based on information in the FOND resource.
For example, given useFont = "Helvetica Narrow" and style (bold + italic), deliver
useFont as "Helvetica-Narrow-BoldOblique". */

static Boolean Res2FontName(unsigned char *,short);

OSStatus GetFontFamilyResource(FMFontFamily iFontFamily, Handle* oHandle) 
{
	FMFont font;
	Str255 fontFamilyName;
	SInt16 rsrcFRefNum;
	Handle rsrcHandle;
	FSSpec rsrcFSSpec;
	FSRef rsrcFSRef;
	HFSUniStr255 forkName;
	OSStatus status;

	font = kInvalidFont;
	rsrcFRefNum = -1;
	rsrcHandle = NULL;
	status = noErr;


	/* Get the font family name to use with the Resource
		Manager when grabbing the 'FOND' resource. */
	status = FMGetFontFamilyName(iFontFamily, fontFamilyName);
	require(status == noErr, FMGetFontFamilyName_Failed);

	/* Get a component font of the font family to obtain
		the file specification of the container of the font
		family. */
	status = FMGetFontFromFontFamilyInstance(iFontFamily, 0, &font, nil);
	require(status == noErr && font != kInvalidFont, FMGetFontFromFontFamilyInstance_Failed);

	status = FMGetFontContainer(font, &rsrcFSSpec);
	require(status == noErr, FMGetFontContainer_Failed);

	/* Open the resource fork of the file. */
	rsrcFRefNum = FSpOpenResFile(&rsrcFSSpec, fsRdPerm);

	/* If the font is based on the ".dfont" file format,
		we need to open the data fork of the file. */
	if ( rsrcFRefNum == -1 ) {

		/* The standard fork name is required to open
			the data fork of the file. */
		status = FSGetDataForkName(&forkName);
		require(status == noErr, FSGetDataForkName_Failed);

		/* The file specification (FSSpec) must be converted
			to a file reference (FSRef) to open the data fork
			of the file. */
		status = FSpMakeFSRef(&rsrcFSSpec, &rsrcFSRef);
		require(status == noErr, FSpMakeFSRef_Failed);

		status = FSOpenResourceFile(&rsrcFSRef,
												forkName.length, forkName.unicode,
												fsRdPerm, &rsrcFRefNum);
		require(status == noErr, FSOpenResourceFile_Failed);
	}

	UseResFile(rsrcFRefNum);

	/* On Mac OS X, the font family identifier may not
		match the resource identifier after resolution of
		conflicting and duplicate fonts. */
	rsrcHandle = Get1NamedResource(FOUR_CHAR_CODE('FOND'), fontFamilyName);
	require_action(rsrcHandle != NULL,
		Get1NamedResource_Failed, status = ResError());

	DetachResource(rsrcHandle);

Get1NamedResource_Failed:

	if ( rsrcFRefNum != -1 )
		CloseResFile(rsrcFRefNum);

FSOpenResourceFile_Failed:
FSpMakeFSRef_Failed:
FSGetDataForkName_Failed:
FMGetFontContainer_Failed:
FMGetFontFromFontFamilyInstance_Failed:
FMGetFontFamilyName_Failed:

	if ( oHandle != NULL )
		*oHandle = rsrcHandle;

	return status;
} 


static Boolean Res2FontName(unsigned char *useFont, short style)
	{
		Handle resH; 
		unsigned char *deFont;
		
		FMFontFamily 	iFontFamily;
		OSStatus 		status = noErr;
		/*
		 *  Get the suffixes for the desired style from the style-mapping table in the
		 *	FOND resource. This is (poorly) documented in the LaserWriter Reference
		 *	manual, p. 27ff.
		 */
		iFontFamily = FMGetFontFamilyFromName(useFont); 
		
		//resH = GetNamedResource('FOND', useFont);
		//if (ResError()==noErr) {
		status = GetFontFamilyResource( iFontFamily, &resH);
		
		if (status == noErr) 
		{
			short count,index,newStyle,nSuffices,state;
			/* Careful: <suffix>, at least, points to a P string some of the time. */
			Byte *p,*styleIndex,*suffix,*stringList;
			unsigned char *baseName;
			FamRec *fond;
			
			state = HGetState(resH);
			HLock(resH);
			fond = (FamRec *)(*resH);
			
			p = (Byte *)(*resH + fond->ffStylOff);		/* Start of style-mapping table */
			p += sizeof(short) + 2*sizeof(long);		/* Past header to 48 style indices */
			styleIndex = p; p += 48;					/* Save table start for later */
			stringList = p;								/* Save start of string list */
			count = *(short *)p; p += sizeof(short);	/* Get past string count */
			baseName = p; p += 1 + *p;					/* Get font base name from 1st string */
			
			/* Convert from Mac style bits to 'FOND' style-mapping table style bits */
			/* See p. 32, LaserWriter Reference Manual. */
			
			newStyle = 0;
			if (style & bold) newStyle += bold;
			if (style & italic) newStyle += italic;
			
			/* The FOND-brand style skips the underline bit, so we do too */

			if (style & outline) newStyle += (outline>>1);
			if (style & shadow) newStyle += (shadow>>1);
			/*
			 *	If both condense and extend are on, they would cancel out, and the FOND
			 *	style-mapping table doesn't even have slots for styles that have both
			 *	on, so avoid the situation.
			 */
			if ((style & (condense+extend)) == (condense+extend))
				style &= ~(condense+extend);
			if (style & condense) newStyle += (condense>>1);
			if (style & extend) newStyle += (extend>>1);
			
			/* Now we can use newStyle as an index into the style index tables at styleIndex */
			
			index = styleIndex[newStyle];
			/* Get pseudo-string that index points at in string list */
			suffix = PS_NthString(stringList,index);
			/* Each "character" in suffix is an index into stringList for appendage */
			nSuffices = *suffix;

			/* Start with base name */
			
			PStrCopy((StringPtr)baseName,useFont);
			for (p=suffix+1; nSuffices>0; nSuffices--,p++) {
				short lenSuff,lenName;
				suffix = PS_NthString(stringList,*p);
				lenName = *useFont;		/* What we've built so far */
				lenSuff = *(unsigned char *)suffix;			/* What we're about to append */
				if ((lenName+lenSuff) <= 255)
					PStrCat(useFont,suffix);
				 else
					break;
				}
			HSetState(resH,state);
			return TRUE;		
			}
			else {
			
			/* We couldn't get the FOND: use a default */
			
			if (style & bold) deFont = (unsigned char *)((style & italic) ? "\pTimes-BoldItalic" : "\pTimes-Bold");
			 else			  deFont = (unsigned char *)((style & italic) ? "\pTimes-Italic" : "\pTimes-Roman");
			 PStrCopy(deFont,useFont);
			 return FALSE;
			}
	}

/* Given a Pascal string of the Mac font name, convert it to the PostScript version,
with any style (usually italic or bold) modifications, and print the result as a
PostScript literal name.  Conversion is needed because the PostScript font names
are, shall we say, less than systematically named with respect to the Mac names.
The PostScript names appear in the style-mapping table at the end of FOND resources
in a complex format. Returns (in *known) TRUE=font name known; FALSE=unknown, using
the default. */

static OSErr PS_PrintFontName(const unsigned char *font, short style, Boolean *known);
static OSErr PS_PrintFontName(const unsigned char *font, short style, Boolean *known)
	{
		//FontNameMap *fnt; 
		char *pn = NULL;
		Str255 useFont;
		
		PStrCopy(font, useFont);
		if (FONTSUBST) {
			if (PStrCmp((StringPtr)"\pGeneva", useFont))
				PStrCopy((StringPtr)"\pHelvetica", useFont);
			if (PStrCmp((StringPtr)"\pNew York", useFont))
				PStrCopy((StringPtr)"\pTimes", useFont);
			if (PStrCmp((StringPtr)"\pMonaco", useFont))
				PStrCopy((StringPtr)"\pCourier", useFont);
			}

		*known = Res2FontName(useFont,style);
		PS_Print("%p",useFont);
		
		return(thisError);
	}

static Byte *PS_NthString(Byte *stringList, short n)
	{
		short count; Byte *p;
		
		BlockMove(stringList,&count,sizeof(short));		/* In case of odd address */
		if (n>0 && n<=count) {
			p = stringList + sizeof(short);
			while (n-- > 1) p += 1 + *p;
			return(p);
			}
		 else
			return((Byte *)"\p");
	}

/*
 *	Write out the PostScript trailer stuff. This should be called for writing
 *	PostScript files only, not when using the Print Manager.
 */

OSErr PS_Trailer(Document *doc, short nfontsUsed, FONTUSEDITEM *fontUsedTbl, unsigned char *measNumFont,
					Rect */*bBox*/)
	{
		short	j, k;
		Boolean	fontKnown;
		char	fmtStr[256];
		
		PS_Print("\r%%%%Trailer\r\r");
		PS_Print("NightTopSave restore\r\r");
		PS_Print("%%%%Pages: %ld\r",(long)numPages);
		
		/* Correct "DocumentFonts" are needed for EPSF files. */
		
		PS_Print("%%%%DocumentFonts: %p\r", musFontInfo[doc->musFontInfoIndex].postscriptFontName);

		if (measNumFont) {
			/*
			 *	??This is a mess. First, we don't know what style measNumFont is, but
			 *	in any case, what's special about the measure no. font?
			 */
			PS_Print("%%%%+");
			PS_PrintFontName(measNumFont,0,&fontKnown);			/* ??what style? */
			PS_Print("\r");
		}
		for (j = 0; j<nfontsUsed; j++)
			for (k = 0; k<4; k++) {
				if (fontUsedTbl[j].style[k]) {
					PS_Print("%%%%+");
					PS_PrintFontName(fontUsedTbl[j].fontName,k,&fontKnown);
					PS_Print("\r");
				}
		}

		/*
		 * If any unknown fonts were used in the score, complain. We ignore any
		 * unknown fonts found in the fontUsedTbl by the loop above, since the user
		 * probably only cares about fonts they actually used.
		 */
		if (unknownFonts!=0) {
			GetIndCString(fmtStr, PRINTERRS_STRS, 6);    /* "%d piece(s) of text in PostScript fonts Nightingale doesn't know will be printed in the default font." */
			sprintf(strBuf, fmtStr, unknownFonts); 
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			}
		return(thisError);
	}

/*
 *	Declare a new page to begin.  The page is known under the name/number as
 *	found in the C string page (which can be NULL or empty); the number that
 *	this page is in the sequence of printed pages must be in n and must be
 *	1 through n in an n-page document.
 */

OSErr PS_NewPage(Document *doc, char *page, short n)
	{
		if (page==NULL || *page=='\0') page = "?";
		
		if (usingFile) {
			PS_Print("%%%%Page: %s %ld\r",page,(long)n);
			PS_Print("NightingaleDict begin\rBP\rMX concat\r\r");
			}

		/* Make sure PS_MusSize initializes everything related to music font size. */
		PS_MusSize(doc, -1);
		
		return(thisError);
	}


static void PS_StrEncrypted(unsigned char *, unsigned char *);

static void PS_StrEncrypted(unsigned char *string, unsigned char *key)
	{
	   	short i,k,len,klen;
		/*
		 *  Decrypt text string a character at a time, and then encrypt again
		 *  after sending the character out to PostScript.  Because we're using
		 *  a simple XOR, this leaves the string in the same state it was before.
		 */
		k = 1; klen = *key; len = *string;
		for (i=1; i<=len; i++) {
			string[i] ^= key[k];
			PS_Char(string[i]);
			string[i] ^= key[k++];
			if (k > klen) k = 1;
		}
	}

/*
 *	Do what needs to be done to end processing the current page.
 */

OSErr PS_EndPage()
	{
		if (usingFile)
			PS_Print("\rshowpage\rEP\rend        %% NightingaleDict\r");
		
		PS_RestartPageVars();
		pageEnded = TRUE;
		return(thisError);
	}

/*
 *	Send a given C string out to the open file.  This could be made more
 *	efficient later by doing it a roomleft-sized chunk at a time instead
 *	of the simpler character at a time.
 */

OSErr PS_String(char *str)
	{

		while (*str) PS_Char(*(unsigned char *)str++);
		return(thisError);
	}

/*
 *	Send a given Pascal string out to the open file.  This may not be
 *	necessary, but I've added it for completeness.
 */

OSErr PS_PString(unsigned char *str)
	{
		unsigned short len;
		
		len = *str++;
		while (len-- > 0) PS_Char(*str++);
		return(thisError);
	}

/*
 *	Print the characters found in a given resource of a given open resource
 *	file, presumably containing PostScript code.  Make sure that we leave
 *	the current resource file the same.  If resFile is -1, then the current
 *	resource file is searched first.
 */

OSErr PS_Resource(short resFile, ResType type, short id)
	{
		short thisResFile; Handle rsrc;
		long len;
		char *p;
		
		thisResFile = CurResFile();
		if (resFile != -1) UseResFile(resFile);
		
		rsrc = GetResource(type,id);
		if (rsrc == NULL) return(-1);
		LoadResource(rsrc);
		if ((thisError=ResError()) != noErr) return(thisError);
		
		/* Lock it down while we're reading from it to the file */
		
		HNoPurge(rsrc); MoveHHi(rsrc); HLock(rsrc);
		
		/* Copy it verbatim to output file */
		
		len = GetHandleSize(rsrc);
		p = *rsrc;
		while (len-- > 0) PS_Char(*(unsigned char *)p++);
		PS_Flush();
		
		/* Get rid of it and restore old resource path */
		
		HUnlock(rsrc); HPurge(rsrc);
		ReleaseResource(rsrc);
		
		UseResFile(thisResFile);
		
		return(thisError);
	}
	
void PS_Handle()
{
		Size len = GetHandleSize(theTextHandle);
		char *p = *theTextHandle;
		while (len-- > 0) PS_Char(*(unsigned char *)p++);
		PS_Flush();
}
	
/* Here is a simpler, shorter version of printf for printing directly to the
the currently open file.  This is included here in case you want to be self-
self-sufficient library-wise and want to avoid loading someone else's sprintf().
But it also incorporates some non-standard features. According to ANSI/ISO,
"%p" is a pointer to void, and the result is the value of the pointer; PS_Print
assumes it's a pointer to a Pascal string, and the result is the string. Also,
PS_Print accepts "%P" as a variation on "%p" that generates escape codes for
non-standard PostScript characters (see PS_Char). If you need the stdio library
anyway, you could use sprintf to format strings and then call PS_String() to
print them, but you'd have to do something about PS_Print's non-standard features.

PS_Print handles:

	Modifiers: - + 0 # *
	Field width
	Precision
	Length: h l
	Base/sign: b o x u d
	Other: p s c %

 */

#include <stdarg.h>

void PS_Print(char *msg, ...)
	{
		va_list nxtArg;
		short base, precision, len;
		short width,n,fieldLength;
		long arg; unsigned long argu;
		char *p;
		Boolean wideArg,pascalStr,doPrecision,noSign,doSpace;
		Boolean leftJustify,doSign,negArg,doZero,doAlternate;
		
		va_start(nxtArg, msg);
		
		while (*msg)
		
			if (*msg != '%') PS_Char(*(unsigned char *)msg++);
			 else {
			 	
			 	leftJustify = doSign = doSpace = doZero = doAlternate =
			 				  doPrecision = FALSE;
			 	width = -1; precision = 0;
			 	
			 	 /* First pick off the possible modifiers */
			 	
				while (TRUE)
			 		switch(*++msg) {
			 			case ' ':
			 					doSpace = !doSign; break;
			 			case '-':
			 					leftJustify = TRUE; doZero = FALSE; break;
			 			case '+':
			 					doSign = TRUE; doSpace = FALSE; break;
			 			case '0':
			 					doZero = !leftJustify; break;
			 			case '#':
			 					doAlternate = TRUE; break;
			 			case '*':
			 					width = va_arg(nxtArg, short);
			 					if (width < 0) width = 0;
			 					break;
			 			default:
			 					goto getwidth;
			 					break;
			 			}

	getwidth:					 	
			 	 /* Now get the field width, if not already read */
 	
			 	if (width < 0) {
			 		width = 0;
			 		while (*msg>='0' && *msg<='9')
			 			width = 10*width + (*msg++ - '0');
			 		}
			 	
			 	 /* Get the precision, if a period is next */
			 	
			 	if (doPrecision = (*msg == '.'))
			 		if (*++msg == '*') {
			 			precision = va_arg(nxtArg, short);
			 			msg++;
			 			}
			 		 else 
			 			while (*msg>='0' && *msg<='9')
			 				precision = 10*precision  + (*msg++ - '0');
			 		
			 	 /* Note whether this is a long argument: ignore short */
			 	
			 	if (*msg == 'h') msg++;
			 	if (wideArg = (*msg == 'l')) msg++;
			 	
			 	negArg = pascalStr = FALSE; noSign = TRUE; fieldLength = 0;

			 	switch(*msg) {
			 		case 'b':
			 			base = 2; goto outInt;
			 		case 'o':
			 			base = 8; goto outInt;
			 		case 'x':
			 			base = 16; goto outInt;
			 		case 'u':
			 			base = 10; goto outInt;
			 		case 'd':
			 			base = 10; noSign = FALSE;
			 outInt:	
			 			if (noSign)
			 				if (wideArg)
			 					argu = va_arg(nxtArg, unsigned long);
			 				else
			 					argu = va_arg(nxtArg, unsigned short);
			 			else {
			 				if (wideArg)
			 					arg = va_arg(nxtArg, long);
			 				 else
			 					arg = va_arg(nxtArg, short);
			 				
			 				/* Split the signed value into value <argu> and sign <negArg>. */
			 				argu = (unsigned long)arg;
			 				negArg = (arg < 0);
			 				if (negArg) argu = -argu;
			 				}
			 			p = PS_PrtLong(argu,negArg,doSign,noSign,base,width,&len);
			 			fieldLength = len;
			 			break;
			 		case 'P':
			 			doEscapes = TRUE;
			 			/* Fall thru */
			 		case 'p':
						p = va_arg(nxtArg, char *);
			 			if (p == NULL)
			 				{ p = "<NULL>"; width = 0; fieldLength = strlen(p); }
			 			 else {
			 				fieldLength = *(unsigned char *)p++;
			 				if (doPrecision) fieldLength = precision;
			 				}
			 			pascalStr = TRUE;
			 			break;
			 		case 's':
						p = va_arg(nxtArg, char *);
			 			if (p == NULL)
			 				{ p = "<NULL>"; width = 0; fieldLength = strlen(p); }
			 			 else {
			 				fieldLength = strlen(p);
			 				if (doPrecision) fieldLength = precision;
			 				}
			 			break;
			 		case 'c':
						p = " ";
						*p = (char)va_arg(nxtArg, short);	 /* chars passed as shorts */
			 			fieldLength = 1;
			 			break;
			 		case '%':
			 			p = "%"; fieldLength = 1;
			 			break;
			 		default:
			 			p = "<% :BadCmdChar>"; p[2] = *msg; width = 0;
			 			fieldLength = strlen(p);
			 			break;
			 		}
			 	
				va_end(nxtArg);								/* Clean up va_ stuff */
				
			 	/*
			 	 *	Finally add string to output: fieldLength is actual string
			 	 *	length, width is requested or default width.
			 	 */
			 	
			 	n = fieldLength;
			 	
		 		if (width!=0 && width>fieldLength) {
		 			if (!leftJustify) while (n++ < width) PS_Char(' ');
		 			while (fieldLength-- > 0) PS_Char(*(unsigned char *)p++);
		 			if (leftJustify) while (n++ < width) PS_Char(' ');
		 			}
		 		 else
		 			while (fieldLength-- > 0) PS_Char(*(unsigned char *)p++);
		 			
				doEscapes = FALSE;
			 	msg++;
				}
	}

/*
 *	Change the default coordinate system to a given one.  This routine will
 *	only affect things if it is called prior to PS_Header().  The origin is
 *	assumed to be at the upper left when the actual PostScript code
 *	is generated.
 */

void PS_PageSize(DDIST x, DDIST y)
	{
		pageWidth = (x < 0) ? -x : x;
		pageHeight = (y < 0) ? -y : y;
	}

/*
 *	Set any or all of the current line widths for staff lines, ledger lines,
 *	note stems, or bar lines.  Only those that are non-negative are reset.
 */

void PS_SetWidths(DDIST staff, DDIST ledger, DDIST stem, DDIST bar)
	{
		if (staff >= 0) wStaff = staff;
		if (ledger >= 0) wLedger = ledger;
		if (stem >= 0) wStem = stem;
		if (bar >= 0) wBar = bar;
	}

/*
 *	Change the current line width in PostScript to whatever
 */

OSErr PS_SetLineWidth(DDIST width)
	{
		PS_Print("%ld setlinewidth\r",(long)width);
		return(thisError);
	}

/*
 *	Line-drawing routines:
 *		PS_Line			Draw a solid line, thickened perpendicular to its direction
 *		PS_LineVT		Draw a solid line, thickened vertically (like a beam)
 *		PS_LineHT       Draw a solid line, thickened horizontally
 *		PS_HDashedLine	Draw a horizontal dashed line
 *		PS_VDashedLine	Draw a vertical dashed line
 *		PS_FrameRect	Frame a rectangle
 */
 
OSErr PS_Line(DDIST x0, DDIST y0, DDIST x1, DDIST y1, DDIST width)
	{
		PS_Print("%ld %ld %ld %ld %ld ML\r",
								(long)x1,(long)y1,(long)x0,(long)y0,(long)width);
		return(thisError);
	}

OSErr PS_LineVT(DDIST x0, DDIST y0, DDIST x1, DDIST y1, DDIST width)
	{
		/* y-coordinates refer to the TOP of the line! */
		PS_Print("%ld %ld %ld %ld %ld BM\r",(long)x0,(long)y0,(long)x1,(long)y1,
					(long)width);
		return(thisError);
	}

OSErr PS_LineHT(DDIST x0, DDIST y0, DDIST x1, DDIST y1, DDIST width);

OSErr PS_LineHT(DDIST x0, DDIST y0, DDIST x1, DDIST y1, DDIST width)
	{
		/*
            The new definition in the 'TEXT' preamble for this is:

            /LHT {/th XD newpath 2 copy XF moveto exch th add exch XF lineto
                                 2 copy exch th add exch XF lineto XF lineto
                                 closepath fill} BD
		*/
		/* y-coordinates refer to the TOP of the line! */
		PS_Print("%ld %ld %ld %ld %ld LHT\r",(long)x0,(long)y0,(long)x1,(long)y1,
					(long)width);
		return(thisError);
	}

/*
 * The dashed-line drawing routines below really should use the PostScript
 * "setdash" operator instead of drawing lots of tiny lines.
 */

OSErr PS_HDashedLine(DDIST x0, DDIST y, DDIST x1, DDIST width, DDIST dashLen)
	{
		DDIST xstart,xend,spaceLen;
		
		spaceLen = dashLen;		

		for (xstart = x0; xstart+dashLen<=x1; xstart += dashLen+spaceLen) {
			xend = xstart+dashLen;
			PS_Print("%ld %ld %ld %ld %ld ML\r",(long)xend,(long)y,
						(long)xstart,(long)y,(long)width);
			}
		if (xend<x1)								/* If necessary, extend final dash */
			PS_Print("%ld %ld %ld %ld %ld ML\r",(long)x1,(long)y,
						(long)xend,(long)y,(long)width);
		return(thisError);
	}

OSErr PS_VDashedLine(DDIST x, DDIST y0, DDIST y1, DDIST width, DDIST dashLen)
	{
		DDIST ystart,yend,spaceLen;
		
		spaceLen = dashLen;
		
		for (ystart = y0; ystart+dashLen<=y1; ystart += dashLen+spaceLen) {
			yend = ystart+dashLen;
			PS_Print("%ld %ld %ld %ld %ld ML\r",(long)x,(long)yend,
						(long)x,(long)ystart,(long)width);
			}

		if (ystart<y1)									/* Draw short final dash */
			PS_Print("%ld %ld %ld %ld %ld ML\r",(long)x,(long)y1,
						(long)x,(long)ystart,(long)width);
		return(thisError);
	}

OSErr PS_FrameRect(DRect *dr, DDIST width)
	{
		PS_Line(dr->left, dr->top, dr->right, dr->top, width);
		PS_Line(dr->right, dr->top, dr->right, dr->bottom, width);
		PS_Line(dr->right, dr->bottom, dr->left, dr->bottom, width);
		PS_Line(dr->left, dr->bottom, dr->left, dr->top, width);

		return(thisError);
	}

/*
 *	Given the Y distance from the origin and the starting and ending X values,
 *	draw the line in PostScript using the current line width.
 */

OSErr PS_StaffLine(DDIST height, DDIST x0, DDIST x1)
	{
		PS_Print("%ld %ld %ld %ld SFL\r",(long)x1,(long)height,
											(long)x0,(long)height);
		return(thisError);
	}

/*
 *	Given the Y distance from the origin and the starting and ending X values,
 *	and the number of staff lines and the distance between them, draw them.
 *	Note that if the distance between them is undefined, then it is assigned
 *	the current default distance, which is computed from the current music
 *	font point size.  If the pt size is changed later, you'd have to find out
 *	the new lineSpace value computed from it (by setting *dy to -1 and nLines
 *	to 0 and calling this).
 */

OSErr PS_Staff(DDIST height, DDIST x0, DDIST x1, short nLines, DDIST *dy)
	{
		DDIST y;
		
		if (*dy <= 0) *dy = lineSpace; else lineSpace = *dy;
		for (y=height; nLines-- > 0; y+=lineSpace) PS_StaffLine(y,x0,x1);
		return(thisError);
	}

/*
 *	Draw barline on page at given x using current bar width. type is
 *	BAR_SINGLE, BAR_DOUBLE, or BAR_FINALDBL.
 */

/* ??These should be user-definable (via the CNFG). */

#define INTERLNSPACE(lnSpace)	((lnSpace)/2)		/* Space between components of dbl. bar */ 
#define THICKBARLINE(lnSpace)	((lnSpace)/2)		/* Thickness of thick barline */

OSErr PS_BarLine(DDIST top, DDIST bot,DDIST x, char type)
	{
		short thickWidth, thickx;

		switch (type) {
			case BAR_SINGLE:
				PS_Print("%ld %ld %ld %ld BL\r",(long)x,(long)bot,(long)x,(long)top);
				break;
			case BAR_DOUBLE:
				PS_Print("%ld %ld %ld %ld BL\r",(long)x,(long)bot,(long)x,(long)top);
				PS_Print("%ld %ld %ld %ld BL\r",(long)(x+INTERLNSPACE(lineSpace)),(long)bot,
												(long)(x+INTERLNSPACE(lineSpace)),(long)top);
				break;
			case BAR_FINALDBL:
				thickWidth = THICKBARLINE(lineSpace);
				thickx = x+INTERLNSPACE(lineSpace)+thickWidth/2;
				PS_Print("%ld %ld %ld %ld BL\r",(long)x,(long)bot,(long)x,(long)top);
				PS_Print("%ld %ld %ld %ld %ld ML\r",(long)thickx,(long)bot,
													(long)thickx,(long)top,
													(long)thickWidth);
				break;
		}
		return(thisError);
	}

/*
 *	Draw vertical thin connect line on page at given x using current bar width.
 *	This differs from barline above in that the connect line has to be slightly
 *	extended to make a sharp corner with the left edge of the staff.
 */

OSErr PS_ConLine(DDIST top, DDIST bot, DDIST x)
	{
		PS_Print("%ld %ld %ld %ld %ld ML\r",(long)x,(long)bot+(wStaff>>1),
											(long)x,(long)top-(wStaff>>1),(long)wBar);
		return(thisError);
	}

/*
 *	Draw Repeat bar on page at given x from given top to bottom, using current bar
 *	width. We need <botNorm> because the Sonata repeat-dots character has an origin
 *	appropriate for a 5-line staff. type is RPT_L, RPT_R, or RPT_LR.
 *	top and bot are coords of the barline to draw.
 *	botNorm is bottom of barline "if it had 5 lines".
 *	sizePercent scales normal font size when drawing dots (>100 when using 2 aug. dots).
 *	dotsOnly TRUE means don't draw the barline proper, only repeat dots.
 */

OSErr PS_Repeat(Document *doc, DDIST top, DDIST bot, DDIST botNorm, DDIST x,
										char type, short sizePercent, Boolean dotsOnly)
	{
		short thickWidth, thickx, x2;
		DDIST xLDots, xRDots, ydDots, dxGlyph;
		Byte dotsGlyph;
		Boolean hasRptDots = MusFontHasRepeatDots(doc->musFontInfoIndex);
		
		if (hasRptDots) {
			dotsGlyph = MapMusChar(doc->musFontInfoIndex, MCH_rptDots);
			ydDots = botNorm + MusCharYOffset(doc->musFontInfoIndex, dotsGlyph, lineSpace);
		}
		else {	/* We have to use two augmentation dots, at a larger font size. */
			dotsGlyph = MapMusChar(doc->musFontInfoIndex, MCH_dot);
			/* Origin of MCH_rptDots is bottom staff line, so have to use higher ydDots for MCH_dot.
				5*lnSpace/2 is 2 and a half spaces -- where the top dot should go. */
			ydDots = (botNorm-(5*lineSpace/2))
						+ SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, dotsGlyph, lineSpace));
		}

		xLDots = x+INTERLNSPACE(lineSpace)+(4*lineSpace)/10;
		xRDots = x-(8*lineSpace)/10;

		dxGlyph = SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, dotsGlyph, lineSpace));
		xLDots += dxGlyph;
		xRDots += dxGlyph;

		switch (type) {
			case RPT_L:
				PS_MusChar(doc, xLDots, ydDots, dotsGlyph, TRUE, sizePercent);
				if (!hasRptDots)
					PS_MusChar(doc, xLDots, ydDots+lineSpace, dotsGlyph, TRUE, sizePercent);

				if (!dotsOnly) {
					/* ??Thick line is just a bit too far to left, compared with RPT_R */
					thickWidth = THICKBARLINE(lineSpace);
					thickx = x-thickWidth/2;
					PS_Print("%ld %ld %ld %ld %ld ML\r",(long)thickx,(long)bot,
														(long)thickx,(long)top,
														(long)thickWidth);
					PS_Print("%ld %ld %ld %ld BL\r",(long)(x+INTERLNSPACE(lineSpace)),(long)bot,
													(long)(x+INTERLNSPACE(lineSpace)),(long)top);
				}
				break;
			case RPT_R:
				if (!dotsOnly) {
					thickWidth = THICKBARLINE(lineSpace);
					thickx = x+INTERLNSPACE(lineSpace)+thickWidth/2;
					PS_Print("%ld %ld %ld %ld BL\r",(long)x,(long)bot,(long)x,(long)top);
					PS_Print("%ld %ld %ld %ld %ld ML\r",(long)thickx,(long)bot,
														(long)thickx,(long)top,
														(long)thickWidth);
				}
				PS_MusChar(doc, xRDots, ydDots, dotsGlyph, TRUE, sizePercent);
				if (!hasRptDots)
					PS_MusChar(doc, xRDots, ydDots+lineSpace, dotsGlyph, TRUE, sizePercent);
				break;
			case RPT_LR:
				PS_MusChar(doc, xLDots, ydDots, dotsGlyph, TRUE, sizePercent);
				if (!dotsOnly) {
					thickWidth = THICKBARLINE(lineSpace);
					/* ??Drawing these at full width makes the barlines collide with the
						dots. Obviously, the solution is to move the dots, but I don't
						want to do that now, full width is probably too wide in engraving
						style, and the width should be under user control anyway. For
						now, kludge. */
					thickWidth = (7*thickWidth)/10;
					x2 = x+INTERLNSPACE(lineSpace)+thickWidth/4;
					PS_Print("%ld %ld %ld %ld %ld ML\r",(long)x,(long)bot,
													(long)x,(long)top,
													(long)thickWidth);
					PS_Print("%ld %ld %ld %ld %ld ML\r",(long)x2,(long)bot,
														(long)x2,(long)top,
														(long)thickWidth);
				}
				PS_MusChar(doc, xRDots, ydDots, dotsGlyph, TRUE, sizePercent);
				if (!hasRptDots) {
					PS_MusChar(doc, xLDots, ydDots+lineSpace, dotsGlyph, TRUE, sizePercent);
					PS_MusChar(doc, xRDots, ydDots+lineSpace, dotsGlyph, TRUE, sizePercent);
				}
				break;
		}
		return(thisError);
	}

/*
 *	Draw a horizontal ledger line of current thickness
 */

OSErr PS_LedgerLine(DDIST height, DDIST x0, DDIST dx)
	{
		PS_Print("%ld %ld %ld %ld LL\r",(long)(x0+dx),(long)height,
											(long)x0,(long)height);
		return(thisError);
	}

/*
 *	Draw a beam or similar vertically-thickened line of given thickness between two
 *	points. If either end is attached to an up stem, move that end of the beam left
 *	because up note stems have been pushed left by half the stem width. y-coordinates
 *	refer to the TOP of the line.
 *	upOrDown0 and upOrDown1 are 1=stemp up, -1=down
 */

OSErr PS_Beam(DDIST x0, DDIST y0, DDIST x1, DDIST y1, DDIST thick,
												short upOrDown0, short upOrDown1)
	{
		if (upOrDown0 > 0) x0 -= (wStem + 4);
		if (upOrDown1 > 0) x1 -= wStem;
		PS_Print("%ld %ld %ld %ld %ld BM\r",(long)x0,(long)y0,(long)x1,(long)y1,
					(long)thick);
		return(thisError);
	}


/*
 *	Draw a notehead with a stem.  We draw the notehead first if the stem is up,
 *	the stem first (in procedure <SD>)if it's down, so as to get the stem (and
 *	head?) at the correct horizontal position. Print the head at <sizePercent> of
 *	normal size.
 *
 * 	The <xNorm> parameter is so we can compensate for the fact the the harmonic
 *	notehead in Sonata is too small and we have to draw it oversize and positioned
 *	to the left a bit of where it would normally be.
 *
 *	<stemShorten> is for head shapes like "X" and chord slash, in which the point
 *	where the stem touches the head (and ends) is non-standard.
 * 
 *	If <sym> is a null byte, then the notehead is a chord slash symbol.  In this
 *	case, we don't have to (possibly) transfer into the music font, and we draw
 *	the notehead as a graphic command.  Then we pretend the notehead was
 *	invisible and call the usual stem machinery.
 */

#define SLASH_XTWEAK (DDIST)2		/* Experimental constant */

OSErr PS_NoteStem(
	Document *doc,
	DDIST x, DDIST y,	/* Notehead origin (x=left edge) */
	DDIST xNorm,		/* If stem up, draw stem as if left edge of note were here */
	char sym,			/* Notehead glyph, or null = chord slash */
	DDIST stemLen,		/* Length not considering <stemShorten>: >0 means down */
	DDIST stemShorten,	/* Start notehead end of stem this far from normal position */
	Boolean beamed,
	Boolean headVisible,
	short sizePercent)
	{
		unsigned char str[2];						/* Pascal string */
		DDIST dhalfLn, thick, xoff, yoff, yShorten=0;
		Boolean drawStem, stemDown;
		
		str[0] = 1; str[1] = sym;					/* Not a good string if sym is '\0' */
		dhalfLn = lineSpace/2;
		drawStem = (ABS(stemLen) > pt2d(3) && ABS(stemLen)-stemShorten > pt2d(3));
		stemDown = (stemLen>(DDIST)0);
		PS_SetMusicFont(doc, sizePercent);

		if (!sym) {									/* Chord slash */
			yoff = y+2*dhalfLn;
			thick = 3*dhalfLn/2;
			if (stemDown)
				xoff = x-SLASH_XTWEAK;
			else
				xoff = x-(3*thick)/4+SLASH_XTWEAK;
			if (headVisible)
				PS_LineHT(xoff, yoff, xoff + 2*dhalfLn, yoff - 4*dhalfLn, thick);
			headVisible = FALSE;
			}
		
		/* We use PostScript words SD and SDI to draw stem-down noteheads and stems:
		 *   glyph xHeadOrigin yHeadOrigin x2 y2 xStemEnd yStemEnd SD
		 *   xHeadOrigin yHeadOrigin x2 y2 xStemEnd yStemEnd SDI
		 * Ngale always passes xHeadOrigin = x2 = xStemEnd.
		 * x2 = y2 = -1 (actually probably x2<0) means don't draw stem. ??OK,
		 * BUT WHAT ARE x2 AND y2?
		 */
		
		if (stemDown) {
		
			if (sym) {
				/* Standard stem-down notehead */
				if (headVisible) PS_Print("(%P)",str);
				}
			 else {
				/* Chord slash: notehead already drawn above */
				}
			/* If stem is extremely short or zero length, don't draw it. */
			if (drawStem) {
				yShorten = y+stemShorten;
				stemLen -= stemShorten;
				PS_Print("%ld %ld %ld %ld %ld %ld ",
						(long)x,(long)y,(long)x,(long)yShorten+stemFudge,
						(long)x,(long)(yShorten+stemLen));
				}
			else
				PS_Print("%ld %ld -1 -1 %ld %ld ", (long)x,(long)y,
						(long)x,(long)yShorten+stemFudge);
			PS_Print((char*)(headVisible ? "SD\r" : "SDI\r"));
			}
			
		 else {										/* Nope, it's stem up */
		 
			if (!sym) sym = 'q';					/* Chord slash: notehead already drawn above */
			PS_MusChar(doc,x,y,sym,headVisible,sizePercent);
			/* If stem is extremely short or zero length, don't draw it. */
			if (drawStem) {
				yShorten = y-stemShorten;
				stemLen += stemShorten;
				PS_Print("%ld %ld %ld SU\r",
							(long)xNorm, (long)yShorten+stemLen+(DDIST)(beamed ? 8 : 0),
							(long)(-stemShorten));
				}
			}

		return(thisError);
	}


/*
 *	Draw an arpeggio sign of the given height but composed of segments <sizePercent>
 *	of normal size.
 */
 
OSErr PS_ArpSign(Document *doc, DDIST x, DDIST y, DDIST height, short sizePercent,
														Byte glyph, DDIST /*lnSpace*/)
	{
		DDIST charHeight, yHere;

		charHeight = pt2d(musicPtSize)/2;			/* Sonata arpeggio char. is 2 spaces high */

		for (yHere = y; yHere-y<height; yHere += charHeight)
			PS_MusChar(doc,x,yHere+charHeight,glyph,TRUE,sizePercent);
		return(thisError);
	}


/*
 * Set the minimum "safe" linewidth. At 1270 DPI, sfw=4 is fine, sfw=3 is too thin to
 * photocopy well. But at 300 DPI and rastral 5, with our old XF operator, which always
 * rounded line thicknesses to an even no. of device pixels (see "setlinewidth" in the
 * first edition of the Red Book or the Adobe tech note on line widths), sfw=0 was the
 * ONLY setting that wasn't too thick! With the new, improved XF of Sept. '91, things
 * seem to be better.
 */
 
#define MIN_LW 4	/* minimum "safe" linewidth */


/*
 *	If <ptSize> is different from the previous value, change the current music font to
 *	be a given point size and set linewidths accordingly. If <ptSize> is negative, just
 *	re-initialize so the next call will always assume <ptSize> is indeed different.
 *
 *	NB: It's not obvious why, but apparently this needs to be done at the start of
 *	every page: if it's not, linewidths get reset to default.
 */

OSErr PS_MusSize(Document *doc, short ptSize)
	{
		static short prevPtSize=-1;
		
		if (ptSize<=0) {
			prevPtSize = ptSize;				/* Re-initialize */
			return(thisError);
			}
		
		if (ptSize!=prevPtSize) {
			PS_Print("/MFS %ld def\r",(long)ptSize);
			thisFont = F_None;					/* Force PS_SetMusicFont to call MF */
			musicPtSize = ptSize;				/* Must be prior to PS_SetMusicFont() */
			PS_SetMusicFont(doc, 100);
			PS_Recompute();
			
			if (wStem<MIN_LW || wBar<MIN_LW || wLedger<MIN_LW || wStaff<MIN_LW)
				thinLines = TRUE;
				
			if (wStem != oldStem) PS_Print("/stw %ld def\r",(long)wStem);
			if (wBar  != oldBar)  PS_Print("/blw %ld def\r",(long)wBar);
			if (wLedger != oldLedger) PS_Print("/ldw %ld def\r",(long)wLedger);
			if (wStaff != oldStaff) PS_Print("/sfw %ld def\r",(long)wStaff);
			
			/*
			 *	If requested, print linewidths at top of music page. Unfortunately, many
			 *	printers have rather large unprintable margins, so we have to position
			 *	the text fairly well in--half an inch or so from the top and left.
			 */
			if (printLinewidths)
				if (wStaff != oldStaff) {
					sprintf(strBuf, "ptSize=%d stw=%d blw=%d ldw=%d sfw=%d (%s %s)",
							ptSize, wStem, wBar, wLedger, wStaff, PROGRAM_NAME, applVerStr);
					PS_FontString(doc,pt2d(36), pt2d(30+10), CToPString(strBuf), "\pCourier", 10, 0);
					}

			oldStem = wStem;
			oldBar = wBar;
			oldLedger = wLedger;
			oldStaff = wStaff;
			prevPtSize = ptSize;
			}
		return(thisError);
	}


/*
 *	Given a position on the page and a symbol from the music font, print it
 *	at <sizePercent> of normal size.
 */

OSErr PS_MusChar(Document *doc, DDIST x, DDIST y, char sym, Boolean visible, short sizePercent)
	{
		unsigned char str[2];
		
		str[0] = 1; str[1] = sym;
		
		PS_SetMusicFont(doc, sizePercent);
		PS_Print("(%P)%ld %ld ",str,(long)x,(long)y);
		PS_Print((char*)(visible?"MS\r":"MSI\r"));
		
		return(thisError);
	}


/*
 *	Given a position and a Pascal string, print it from that position in the given
 *	font, point size, and style. The only styles recognized are plain, bold, italic,
 *	and bold italic, except for Sonata, where style is ignored and plain is always
 *	used.
 */

OSErr PS_FontString(Document *doc, DDIST x, DDIST y, const unsigned char *str, const unsigned char *font,
					short ptSize, short style)
	{
		Boolean fontKnown; short fontNum;
		
		/*
		 *	We treat Sonata differently from all other fonts because Sonata can't use
		 *	our standard encoding vector.
		 */
		
		if (PStrCmp(font,(StringPtr)"\pSonata")) {
			if (usingFile)
				PS_Print(" %ld MF\r",(long)ptSize);
			 else
				PS_FontRunAround(doc,doc->musicFontNum,ptSize,0);
			PS_Print("SQW\r");
			PS_Print("(%P)%ld %ld MS\r",str,(long)x,(long)y);
			thisFont = F_None;
			}
		else {
			if (usingFile) {
				PS_Print("/");
				PS_PrintFontName(font,style,&fontKnown);
				PS_Print(" %ld NF\r",(long)ptSize);
				}
			 else {
			 	GetFNum(font,&fontNum);
				PS_FontRunAround(doc,fontNum,ptSize,style);
				}
			PS_Print("(%P)%ld %ld MS\r",str,(long)x,(long)y);
			thisFont = F_Other;
			if (!fontKnown) unknownFonts += 1;
			}
			
		return(thisError);
	}

/*
 *	Given a position and a Pascal string, print it from that position in the
 *	music font at <sizePercent> of normal size.
 */

OSErr PS_MusString(Document *doc, DDIST x, DDIST y, unsigned char *str, short sizePercent)
	{
		PS_SetMusicFont(doc, sizePercent);
		PS_Print("(%P)%ld %ld MS\r",str,(long)x,(long)y);
		return(thisError);
	}

/*
 *	Given a position, print a do-it-yourself colon from that position at <sizePercent>
 *	of normal size. Useful because Sonata has no colon character.
 */

OSErr PS_MusColon(Document *doc, DDIST x, DDIST y, short sizePercent, DDIST lnSpace, Boolean italic)
	{
		Byte glyph = MapMusChar(doc->musFontInfoIndex, MCH_dot);

		x += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace));
		y += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace));

		PS_SetMusicFont(doc, sizePercent);
		PS_Print("(.)%ld %ld MS\r",(long)x,(long)y);
		PS_Print("(.)%ld %ld MS\r",(long)(x+(italic? lineSpace/6 : 0)),
								   (long)(y-lineSpace/2));
		return(thisError);
	}


/*
 *	Given positions of the endpoints and control points, if !dashed, draw a slur as
 *	a pair of Bezier cubics filled in between. If dashed, draw a single dashed
 *	Bezier.
 */

/* Slur thickness of lineSpace/4 looks OK at 300 DPI but too thin at 1270 DPI. */
#define SLURTHICK(lnSpace)	(config.slurMidLW*(lnSpace)/100)

OSErr PS_Slur(DDIST p0x, DDIST p0y, DDIST c1x, DDIST c1y,
				DDIST c2x, DDIST c2y, DDIST p3x, DDIST p3y, Boolean dashed)
	{
		DDIST thick; Boolean up;
		
		if (dashed) {
			/* ??We should really define a PostScript operator for this. */
			PS_Print("[%ld %ld] 0 setdash\r", (long)pt2d(config.slurDashLen),
					(long)pt2d(config.slurSpaceLen));
			PS_Print("%ld %ld moveto %ld %ld %ld %ld  %ld %ld curveto stroke\r",
					(long)p0x,(long)p0y,
					(long)c1x,(long)c1y,(long)c2x,(long)c2y,(long)p3x,(long)p3y);
			PS_Print("[] 0 setdash\r");
		}
		else {
			PS_Print("%ld %ld %ld %ld %ld %ld  %ld %ld MC\r",
					(long)c1x,(long)c1y,(long)c2x,(long)c2y,(long)p3x,(long)p3y,
					(long)p0x,(long)p0y);
			up = c1y < p0y;
			thick = (up ? SLURTHICK(lineSpace) : SLURTHICK(-lineSpace));
			PS_Print("%ld %ld %ld %ld %ld %ld SL\r",
					(long)c2x,(long)(c2y+thick),(long)c1x,(long)(c1y+thick),
					(long)p0x,(long)p0y);
		}

		return(thisError);
	}

/*
 *	Draw a Connecting bracket, using the appropriate ending symbols from the
 *	music font.
 */
						
OSErr PS_Bracket(Document *doc, DDIST x, DDIST yTop,DDIST yBot)
	{
		PS_SetMusicFont(doc, 100);
		PS_Print("%ld %ld %ld BK\r",(long)(yBot-yTop),(long)x,(long)yTop);
		
		return(thisError);
	}

/*
 *	Draw a Connecting brace, using the appropriate upper and lower halves
 *	from the music font.  (x,yTop) is the upper left corner of the box the
 *	symbol should be drawn in, and yBot is the bottom.
 */

OSErr PS_Brace(Document *doc, DDIST x, DDIST yTop, DDIST yBot)
	{
		PS_SetMusicFont(doc, 100);
		PS_Print("%ld %ld %ld BR\r",(long)x,(long)yTop,(long)(yBot-yTop)/2);
		
		return(thisError);
	}


/*  ============================================================ PRIVATE ROUTINES == */

/*
 *	Set all non-zero local global variables
 */

static void PS_InitGlobals(Document *doc)
	{
		PS_PageSize((DDIST)(doc->origPaperRect.right*DDFact),
					(DDIST)(doc->origPaperRect.bottom*DDFact));
		musicPtSize = 24; thisFont = F_None;
		thinLines = FALSE;
		PS_Recompute();
		pageEnded = TRUE;
		PS_RestartPageVars();
		inQD = TRUE;
		psDoc = doc;
	}

/*
 *	Reset various widths so that they will be defined again in PostScript
 */

static void PS_RestartPageVars()
	{
		oldBar = oldStaff = oldLedger = oldStem = -32000; 
	}

/*
 *	Whenever the Music Font point size changes, this function should be called to
 *	recompute everything that depends on it. The distance between staff lines is
 *	1/4 the point size, scaled to our 1/16 point unit system.
 */

static void PS_Recompute()
	{
		DDIST staffLW, ledgerLW, stemLW, barlineLW;
		long lineSpLong;
		
		lineSpace = (DDIST)((DDFact * musicPtSize) / 4);
		stemFudge = (3 * lineSpace) / 20;
		
		/* 
		 * Settings that work OK at 300 DPI can lead, at high resolution and small
		 * staff sizes, to staff lines that are too thin. Specifically, at 1270
		 * DPI, sfw=4 is fine, sfw=3 is too thin to photocopy well. But at 300 DPI
		 * and rastral 5, with our old XF operator, which always rounded line
		 * thicknesses to an even no. of device pixels (see "setlinewidth" in the
		 * first edition of the Red Book), sfw=0 was the ONLY setting that wasn't
		 * too thick! With the new, improved XF of Sept. '91, should be better but
		 * I don't know.
		 */
		lineSpLong = lineSpace;
		staffLW = (config.staffLW * lineSpLong) / 100L;			/* staff */
		ledgerLW = (config.ledgerLW * lineSpLong) / 100L;		/* ledger line */
		stemLW = (config.stemLW * lineSpLong) / 100L;			/* stem */
		barlineLW = (config.barlineLW* lineSpLong) / 100L;		/* barline */
					  
		PS_SetWidths(staffLW, ledgerLW, stemLW, barlineLW);
		wConnect = (DDIST)(lineSpace * 35.0/64.0);
	}

/*
 *	Set the font to the music font scaled to <sizePercent> of <musicPtSize>. This is
 *	different from changing the music font size to <sizePercent>*<musicPtSize> as
 *	PS_MusSize does because it doesn't affect line widths; this effect is appropriate
 *	for cue or grace notes and such.
 */

Boolean PS_SetMusicFont(Document *doc, short sizePercent)
	{
		short ptSize;
		
		/* Add 50 before dividing by 100 in order to round. */
		ptSize = (short)((sizePercent*(long)musicPtSize+50L)/100);
		
		if (sizePercent!=musicSizePercent || thisFont!=F_Music) {
			if (usingFile) {
				PS_Print("%ld MF SQW\r",(long)ptSize);
				musicSizePercent = sizePercent;
				thisFont = F_Music;
				}
			 else {			
				PS_FontRunAround(doc, doc->musicFontNum, ptSize, 0);		/* plain */
				PS_Print("SQW\r");
				musicSizePercent = sizePercent;
				thisFont = F_Music;
				}
		}
		return TRUE;
	}

/*
 *	Call this to change to given font and size.
 *	We must be inside PicComment mode, which we get out of temporarily to do the 
 *	font stuff in QuickDraw mode.
 */

static void PS_FontRunAround(Document *doc, short fontNum, short fontSize, short fontStyle)
	{
		Point penLoc;
//		short fontSz;
//		short ddf;
		
		/* Get out of our dictionary context and back down to LaserPrep */
		PS_IntoQD(doc,FALSE);
		
		GetPen(&penLoc);
		
		/* Change font and size through QuickDraw/LaserPrep */
		TextFont(fontNum);
		TextSize(fontSize*DDFact);		/* Since current matrix is scaled by DDFact */
		TextFace(fontStyle);
		DrawChar(' ');						/* Space forces font download, even in System 7.0 */
		MoveTo(penLoc.h,penLoc.v);		/* Restore to old position */
		
		/* And back to our own world */
		PS_OutOfQD(doc,FALSE,NULL);
		
		TextSize(fontSize);				/* Reset QD locally to non-scaled pt size */
	}

/*
 *	Finished calling Quickdraw stuff, begin a PicComment to start sending our own
 *	stuff.
 *
 *	Calls to this routine must be matched by calls to PS_IntoQD to end the PicComment.
 *	We use inQD as a reality check, but it should not be necessary.
 *
 *	If this is the first time we're opening a PicComment on the current page,
 *	first should be TRUE; otherwise FALSE.
 */

OSErr PS_OutOfQD(Document *doc, short /*first*/, Rect *imageRect)
	{
		Rect box;
		
		if (inQD) {
#if TARGET_API_MAC_CARBON
			PMSessionPostScriptBegin(doc->docPrintInfo.docPrintSession);
#else
			PicComment( PostScriptBeginNoSave, 0, NULL);
			PicComment( TextIsPostScript, 0, NULL);
#endif
			PS_Print("\r%%%% Into PostScript %%%%\r\r");
			
			/*
			 *	Install our NightingaleDict header stuff in userdict.
			 *	We have to do this every time because LaserPrep dips down
			 *	a save level when we change fonts, which has the side effect
			 *	of throwing away all definitions we might have entered into
			 *	userdict the last time.  This isn't too bad, since our dictionary
			 *	isn't very large (compared to "md" anyway), but it means that
			 *	DrawPageContent should try to minimize font switching as much as
			 *	possible.
			 *
			 *	NOTE: This now happens only once using the PREC 103 mechanism,
			 *	documented in Tech Note 296.  If there is no PREC resource while
			 *	printing, then we do things the old way, which generates a whole
			 *	lot more PostScript traffic.
			 */
			
			if (prec103 == NULL) {
				SetRect(&box,0,0,0,0);
				PS_Header(doc,"\p??",1,1.0,FALSE,FALSE,&box,imageRect);
				/* This leaves dictionary closed */
				}
			
			/* Send stuff while in our own dictionary context */
			
			PS_Print("\r\rNightingaleDict begin\r");

			/*
			 *	Push graphics context so we can restore when we go back to QuickDraw,
			 *	and convert LaserPrep's original coordinate system back into DDISTs.
			 */
			
			PS_Print("gsave MX concat\r");
			PS_RestartPageVars();
			
			inQD = FALSE;
			}
		return(thisError);
	}

/*
 *	After sending pure PostScript within the context of our own dictionary, call this
 *	to bring us back to LaserPrep QuickDraw level.
 *	If last is TRUE, then this is the last time we'll be called before the page ends.
 */

OSErr PS_IntoQD(Document *doc,short last)
	{
		if (!inQD) {
		
			/*
			 *	Save current position while we move around in QuickDraw, pop our
			 *	dictionary context, and pop our graphics state back to LaserPrep.
			 */
			
			PS_Print("grestore end\r");
			
			/*
			 *	Our header definitions are still in MD in userdict, but LaserPrep
			 *	will restore the previous context when it closes the page,
			 *	which will make everything we've defined all go away.
			 */
			
			if (last) {
				}

			/* Make sure everything buffered is sent before closing comment */
			
			PS_Print("\r%%%% Into QuickDraw %%%%\r\r");
			PS_Flush();
			
#if TARGET_API_MAC_CARBON
			PMSessionPostScriptEnd(doc->docPrintInfo.docPrintSession);
#else
			PicComment( PostScriptEnd, 0, NULL);
#endif
			inQD = TRUE;
			}
		return(thisError);
	}

/*
 *	The PostScript driver needs to be able to format simple strings and
 *	then append them to the current file.  These are buffered in a block
 *	allocated at open time, and added to as we call various printing
 *	routines.  When the buffer gets full it needs to be flushed before
 *	we can continue adding stuff to it.
 */

#define DSKFUL_ERR -34			/* Disk full (N.B. FileMgr.h has enum for result codes.) */
#define FNF_ERR -43				/* File not found */
#define WPR_ERR -44				/* Volume write-protected by hardware setting */

void PS_Flush()
	{
		long nChars;
		char fmtStr[256];
		
		/* Get actual number of chars so far in buffer and send them out */
		
		nChars = bp - buffer;
		if (usingFile) {
			thisError = FSWrite(thisFile, &nChars, buffer);
			if (thisError) {
				switch (thisError) {
					case DSKFUL_ERR:
						GetIndCString(strBuf, PRINTERRS_STRS, 7);	/* "Error writing PostScript file: Disk is full." */
						break;
					case WPR_ERR:
						GetIndCString(strBuf, PRINTERRS_STRS, 8);	/* "Error writing PostScript file: Disk is write-protected." */
						break;
					default:
						GetIndCString(fmtStr, PRINTERRS_STRS, 9);	/* "Error writing PostScript file: ID=%d" */
						sprintf(strBuf, fmtStr, thisError); 
					}
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				/* ??These errors should be fatal: should return error code now. */
				}
			}
			
		 else if (usingHandle) {
		 	/* Append our buffer to theTextHandle */
			PtrAndHand(buffer,theTextHandle,nChars);
			thisError = MemError();
			if (thisError) {
				switch(thisError) {
					case memFullErr:
						GetIndCString(strBuf, PRINTERRS_STRS, 10);    /* "No more memory while preparing printing." */
						break;
					default:
						GetIndCString(fmtStr, PRINTERRS_STRS, 11);    /* "Error preparing printing: ID=%d" */
						sprintf(strBuf, fmtStr,thisError); 
						break;
					}
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				/* ??These errors should be fatal: should return error code now. */
				}
			}
			
		if (usingPrinter)
			thisError = PS_Comment(buffer,nChars);
	
		/* Reset to an empty buffer */
		
		bp = buffer;
	}

/*
 *	Add a character to the buffer, flushing if necessary.  If the character
 *	is outside the range of "printable" characters (ASCII graphic chars.
 *	plus Return and Tab) and we're printing a PostScript string, we turn
 *	the char. into the appropriate escape sequence, then call ourselves
 *	recursively to output the sequence.
 */

static void PS_Char(short ch)
	{
		if (bp >= bufTop) PS_Flush();
		
		if (doEscapes) {
			doEscapes = FALSE;
			if ((ch<' ' && (ch!='\r' && ch!='\t')) || (ch > 127)) {
				/* Convert character to octal code escape sequence */
				PS_Char('\\');
				PS_Char('0' + ((ch >> 6)&3));
				PS_Char('0' + ((ch >> 3)&7));
				ch = '0' + (ch & 7);
				}
			 else if (ch=='(' || ch==')' || ch=='\\') {
				PS_Char('\\');
				}
			PS_Char(ch);
			doEscapes = TRUE;
			}
		 else
		 	if (usingFile || usingHandle)
		 		*bp++ = ch;
		 	 else {
					*bp++ = ch;
				if (ch == '\r') PS_Flush();
				}
	}

/*
 *	Convert a long integer to a static string and deliver pointer
 *	to string.  Gets digits in reverse order, and then reverses them.
 *	This would be better in-line in PS_Print(), so we wouldn't need
 *	so many arguments.
 */

static char *PS_PrtLong(unsigned long arg,
						Boolean negArg, Boolean doSign, Boolean noSign,
						short base, short /*width*/, short *len)
	{
		char *dst,*start,*dig,tmp;
		static char *digit = "0123456789abcdef";
		static char number[64];
		
		dst = number; dig = digit;		/* Register copies of static stuff */
		
		 /* Get string of digits in reverse order */
		 
		start = dst;
		if (arg == 0L) *dst++ = dig[0];
		 else {
		 	if (doSign) {
		 		if (negArg) *dst++ = '-'; else *dst++ = '+';
		 		start = dst;
		 		}
		 	 else
		 		if (!noSign) if (negArg) { *dst++ = '-'; start = dst; }
		 	
			while (arg) { *dst++ = dig[arg % base]; arg /= base; }
			}
		
		*len = dst - number;
		*dst-- = '\0';
		
		 /* Now reverse digits, and return pointer to string of digits */
		 
		while (dst > start) { tmp = *dst; *dst-- = *start; *start++ = tmp; }
		return(number);
	}


/*
 *	This sends the given buffer of text out to the printer via a DrawString call
 *	within PicComment mode.  We do this by byting off consecuting Pascal string-sized
 *	chunks of the given buffer, and "drawing" them within the confines of an open
 *	comment.
 *
 *	NOTE: DrawString appears to append carriage returns when using PostScriptBegin,
 *		  but not when using PostScriptBeginNoSave.
 */

static OSErr PS_Comment(char *buffer, long remaining)
	{
		Str255 str; long offset = 0L,size,temp=0L;
		
		if (remaining <= 0) return(noErr);
		
		while (remaining > 0L) {
			/* Process the next up-to-255 chars of buffer */
			size = remaining;
			if (size > 255) size = 255;
			/* Put it in local string storage, and force length to be correct */
			BlockMove(buffer+offset,str+1,size);
			str[0] = size;
			/* Prepare for next iteration, if any */
			offset += size;
			remaining -= size;
			/* Send the next chunk of PostScript on its way */


#if TARGET_API_MAC_CARBON
			//OSStatus status = PMSessionPostScriptData (psDoc->docPrintInfo.docPrintSession, (char*)&str[1], (long)str[0]);
			OSStatus status = PMSessionPostScriptData (psDoc->docPrintInfo.docPrintSession, (char*)str+1, size);
			if (status!=0) {
				temp--;			// Homemade conditional breakpoint
			}
			status = PMSessionError(psDoc->docPrintInfo.docPrintSession);
			if (status!=0) {
				temp--;			// Homemade conditional breakpoint
			}
#else
			DrawString(str);
#endif
			}
			
		return(noErr);
	}

