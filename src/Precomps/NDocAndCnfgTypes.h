/* NDocAndCnfgTypes.h (formerly part of NMiscTypes.h) for Nightingale: declarations for
the Document and headers that are part of it, plus some Preference info and a few other
things. NB: Many of these appear in Nightingale score files, so changing them may be a
problem for backward compatibility.*/

#include <CoreMIDI/MIDIServices.h>		/* for MIDIUniqueID */

/* An old comment here by MAS: "we want to /always/ use mac68k alignment." Why? I suspect
for compatibility of files containing bitfields, which we no longer use. Still, it
shouldn't cause any problems, so leave as is until there's a reason to change it. */ 
#pragma options align=mac68k

/* ----------------------------------------------------------- Document printing stuff -- */

//typedef struct Document *DocumentPtr;
//typedef OSStatus (*NDocDrawPageProc)(int doc, UInt32 pageNum);
//typedef void (*DebugPrintFunc)(void *data, short width, short precision, short alternate);

//typedef CALLBACK_API(OSStatus, NDocDrawPageProc)
//							(Document 	*doc,
//							UInt32		pageNum);

//typedef struct DocPrintInfo *DocPrintInfoPtr;
/* Document Print info record */

typedef struct DocPrintInfo {
	PMPrintSession 	docPrintSession;
	PMPageFormat		docPageFormat;
	PMPrintSettings	docPrintSettings;
	
	PMSheetDoneUPP	docPageSetupDoneUPP;
	PMSheetDoneUPP	docPrintDialogDoneUPP;
//	NDocDrawPageProc	docDrawPageProc;
} DocPrintInfo, *DocPrintInfoPtr;


/* ------------------------------------------------------------------------- MPCHANGED -- */
/*	Changed flags for exporting Master Page. */

#define MPCHANGEDFLAGS				\
	short	partChangedMP,			\
			sysVChangedMP,			\
			sysHChangedMP,			\
			stfChangedMP,			\
			margVChangedMP,			\
			margHChangedMP,			\
			indentChangedMP,		\
			fixMeasRectYs,			\
			grpChangedMP,			\
			stfLinesChangedMP,		\
			unusedChgdMP : 6;

typedef struct {
	MPCHANGEDFLAGS
} MPCHGDFLAGS;


/* --------------------------------------------------------------------- CHANGE_RECORD -- */
/* "Change part" record for exporting operations (add/delete, group/ungroup, etc.)
from Master Page. */

enum {								/* Opcodes for change part records. */
	SDAdd,
	SDDelete,
	SDGroup,
	SDUngroup,
	SDMake1
};

typedef struct	{
	short		oper;				/* Operation: SDAdd,SDDelete,SDGroup,SDUngroup. */
	short		staff;				/* Starting staff no. */
	short		p1;					/* Operation-dependent */
	short		p2;					/* Operation-dependent */
	short		p3;					/* Operation-dependent */
} CHANGE_RECORD;


/* -------------------------------------------------------------------- Major headers -- */

/* DOCUMENTHEADER is generic: it contains fields appropriate for any program that displays
displays pages of documents. */

#define DOCUMENTHEADER	\
	Point		origin;				/* Current origin of Document window */						\
	Rect		paperRect;			/* Size of virtual paper sheet in current mag.(pixels) */	\
	Rect		origPaperRect;		/* Size of paper sheet (points) */							\
	Point		holdOrigin;			/* Holds position during pick mode */						\
	Rect		marginRect;			/* Size of area within margins on sheet (points) */			\
	Point		sheetOrigin;		/* Where in Quickdraw space to place sheet array */			\
																								\
	short		currentSheet;		/* Internal sheet [0, ..., numSheets) */					\
	short		numSheets;			/* Number of sheets in Document (visible or not) */			\
	short		firstSheet;			/* To be shown in upper left of sheet array */				\
	short		firstPageNumber;	/* Page number of zero'th sheet */							\
	short		startPageNumber;	/* First printed page number */								\
	short		numRows;			/* Size of sheet array */									\
	short		numCols;																		\
	short		pageType;			/* Current standard/custom page size from popup menu */		\
	short		measSystem;			/* Code for measurement system (from popup menu) */			\
																								\
	Rect		headerFooterMargins; /* Header/footer/pagenum margins  */						\
	Rect		currentPaper;		/* Paper rect in window coords for cur.sheet (pixels) */	\
	SignedByte	landscape;			/* Unused; was "Paper wider than it is high" */				\
	SignedByte	philler;			/* Unused */

typedef struct {
	DOCUMENTHEADER
} DOCUMENTHDR, *PDOCUMENTHDR;


typedef struct {
	short	fontID;					/* font ID number for TextFont */
	unsigned char fontName[32]; 	/* font name: Pascal string */
} FONTITEM;


#define NIGHTSCOREHEADER																		\
	LINK 		headL,				/* links to header and tail objects */						\
				tailL,																			\
				selStartL,			/* currently selected range. */								\
				selEndL;			/*		Also see the <selStaff> field. */					\
																								\
	short		nstaves,			/* number of staves in a system */							\
				nsystems;			/* number of systems in score */							\
	char		comment[MAX_COMMENT_LEN+1]; /* User comment on score (C string) */				\
	char		feedback;			/* True if we want feedback on note insert */				\
	char		dontSendPatches;	 /* 0 = when playing, send patch changes for channels */		\
	char		saved;				/* True if score has been saved */							\
	char		named;				/* True if file has been named */							\
	char 		used;				/* True if score contains any nonstructural info */			\
	char		transposed;			/* True if transposed score, else C score */				\
	char		lyricText;			/* (no longer used) True if last text entered was lyric */	\
	char		polyTimbral;		/* True for one part per MIDI channel */					\
	Byte		currentPage;		/* (no longer used) */										\
	short		spacePercent,		/* Percentage of normal horizontal spacing used */			\
				srastral,			/* Standard staff size rastral no. */						\
				altsrastral,		/* (unused) Alternate staff size rastral no. */				\
				tempo,				/* (unused?) playback speed in beats per minute */			\
				channel,			/* Basic MIDI channel number */								\
				velocity;			/* global playback velocity offset */						\
	STRINGOFFSET headerStrOffset;	/* index returned by String Manager */						\
	STRINGOFFSET footerStrOffset;	/* index returned by String Manager */						\
	char		topPGN;				/* True=page numbers at top of page, else bottom */			\
	char		hPosPGN;			/* 1=page numbers at left, 2=center, 3=at right */			\
	char		alternatePGN;		/* True=page numbers alternately left and right */			\
	char		useHeaderFooter;	/* True=use header/footer text, not simple pagenum */		\
	char		fillerPGN;			/* unused */												\
	SignedByte	fillerMB;			/* unused */												\
	DDIST		filler2,			/* unused */												\
				dIndentOther;		/* Amount to indent Systems other than first */				\
	SignedByte	firstNames,			/* Code for drawing part names: see enum above */			\
				otherNames,			/* Code for drawing part names: see enum above */			\
				lastGlobalFont,		/* Header index of most recent text style used */			\
				xMNOffset,			/* Horiz. pos. offset for meas. nos. (half-spaces) */		\
				yMNOffset,			/* Vert. pos. offset for meas. nos. (half-spaces) */		\
				xSysMNOffset;		/* Horiz. pos. offset for meas.nos.if 1st meas.in system */ \
	short		aboveMN,			/* True=measure numbers above staff, else below */			\
				sysFirstMN,			/* True=indent 1st meas. of system by xMNOffset */			\
				startMNPrint1,		/* True=First meas. number to print is 1, else 2 */			\
				firstMNNumber;		/* Number of first measure */								\
	LINK		masterHeadL,		/* Head of Master Page object list */						\
				masterTailL;		/* Tail of Master Page object list */						\
	SignedByte	filler1,																		\
				nFontRecords;		/* Always 15 for now */										\
	/* Fifteen identical TEXTSTYLE records. Fontnames are Pascal strings. */					\
																								\
	unsigned char fontNameMN[32];	/* MEASURE NO. FONT: default name, size and style */		\
	unsigned short	fillerMN;																	\
	unsigned short	lyricMN;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosureMN;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeMN;		/* True if size is relative to staff size */				\
	unsigned short	fontSizeMN;		/* if relFSizeMN, small..large code, else point size */		\
	short			fontStyleMN;																\
																								\
	unsigned char fontNamePN[32];	/* PART NAME FONT: default name, size and style */			\
	unsigned short	fillerPN;																	\
	unsigned short	lyricPN;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosurePN;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizePN;		/* True if size is relative to staff size */				\
	unsigned short	fontSizePN;		/* if relFSizePN, small..large code, else point size */		\
	short			fontStylePN;																\
																								\
	unsigned char fontNameRM[32];	/* REHEARSAL MARK FONT: default name, size and style */		\
	unsigned short	fillerRM;																	\
	unsigned short	lyricRM;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosureRM;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeRM;		/* True if size is relative to staff size */				\
	unsigned short	fontSizeRM;		/* if relFSizeRM, small..large code, else point size */		\
	short			fontStyleRM;																\
																								\
	unsigned char fontName1[32];	/* REGULAR FONT 1: default name, size and style */			\
	unsigned short	fillerR1;																	\
	unsigned short	lyric1;			/* True if spacing like lyric	*/							\
	unsigned short	enclosure1;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize1;		/* True if size is relative to staff size */				\
	unsigned short	fontSize1;		/* if relFSize1, small..large code, else point size */		\
	short			fontStyle1;																	\
																								\
	unsigned char fontName2[32];	/* REGULAR FONT 2: default name, size and style */			\
	unsigned short	fillerR2;																	\
	unsigned short	lyric2;			/* True if spacing like lyric	*/							\
	unsigned short	enclosure2;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize2;		/* True if size is relative to staff size */				\
	unsigned short	fontSize2;		/* if relFSize2, small..large code, else point size */		\
	short			fontStyle2;																	\
																								\
	unsigned char fontName3[32];	/* REGULAR FONT 3: default name, size and style */			\
	unsigned short	fillerR3;																	\
	unsigned short	lyric3;			/* True if spacing like lyric	*/							\
	unsigned short	enclosure3;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize3;		/* True if size is relative to staff size */				\
	unsigned short	fontSize3;		/* if relFSize3, small..large code, else point size */		\
	short			fontStyle3;																	\
																								\
	unsigned char fontName4[32];	/* REGULAR FONT 4: default name, size and style */			\
	unsigned short	fillerR4;																	\
	unsigned short	lyric4;			/* True if spacing like lyric	*/							\
	unsigned short	enclosure4;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize4;		/* True if size is relative to staff size */				\
	unsigned short	fontSize4;		/* if relFSizeR4, small..large code, else point size */		\
	short			fontStyle4;																	\
																								\
	unsigned char fontNameTM[32];	/* TEMPO MARK FONT: default name, size and style */			\
	unsigned short	fillerTM;																	\
	unsigned short	lyricTM;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosureTM;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeTM;		/* True if size is relative to staff size */				\
	unsigned short	fontSizeTM;		/* if relFSizeTM, small..large code, else point size */		\
	short			fontStyleTM;																\
																								\
	unsigned char fontNameCS[32];	/* CHORD SYMBOL FONT: default name, size and style */		\
	unsigned short	fillerCS;																	\
	unsigned short	lyricCS;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosureCS;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeCS;		/* True if size is relative to staff size */				\
	unsigned short	fontSizeCS;		/* if relFSizeCS, small..large code, else point size */		\
	short			fontStyleCS;																\
																								\
	unsigned char fontNamePG[32];	/* PAGE HEADER/FOOTER/NO.FONT: default name, size and style */	\
	unsigned short	fillerPG;																	\
	unsigned short	lyricPG;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosurePG;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizePG;		/* True if size is relative to staff size */				\
	unsigned short	fontSizePG;		/* if relFSizePG, small..large code, else point size */		\
	short			fontStylePG;																\
																								\
	unsigned char fontName5[32];	/* REGULAR FONT 5: default name, size and style */			\
	unsigned short	fillerR5;																	\
	unsigned short	lyric5;			/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosure5;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize5;		/* True if size is relative to staff size */				\
	unsigned short	fontSize5;		/* if relFSize5, small..large code, else point size */		\
	short			fontStyle5;																	\
																								\
	unsigned char fontName6[32];	/* REGULAR FONT 6: default name, size and style */			\
	unsigned short	fillerR6;																	\
	unsigned short	lyric6;			/* True if spacing like lyric	*/							\
	unsigned short	enclosure6;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize6;		/* True if size is relative to staff size */				\
	unsigned short	fontSize6;		/* if relFSizeR6, small..large code, else point size */		\
	short			fontStyle6;																	\
																								\
	unsigned char fontName7[32];	/* REGULAR FONT 7: default name, size and style */			\
	unsigned short	fillerR7;																	\
	unsigned short	lyric7;			/* True if spacing like lyric	*/							\
	unsigned short	enclosure7;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize7;		/* True if size is relative to staff size */				\
	unsigned short	fontSize7;		/* if relFSizeR7, small..large code, else point size */		\
	short			fontStyle7;																	\
																								\
	unsigned char fontName8[32];	/* REGULAR FONT 8: default name, size and style */			\
	unsigned short	fillerR8;																	\
	unsigned short	lyric8;			/* True if spacing like lyric	*/							\
	unsigned short	enclosure8;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize8;		/* True if size is relative to staff size */				\
	unsigned short	fontSize8;		/* if relFSizeR8, small..large code, else point size */		\
	short			fontStyle8;																	\
																								\
	unsigned char fontName9[32];	/* REGULAR FONT 9: default name, size and style */			\
	unsigned short	fillerR9;																	\
	unsigned short	lyric9;			/* True if spacing like lyric	*/							\
	unsigned short	enclosure9;		/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize9;		/* True if size is relative to staff size */				\
	unsigned short	fontSize9;		/* if relFSizeR9, small..large code, else point size */		\
	short			fontStyle9;																	\
																								\
	/* End of TEXTSTYLE records */																\
																								\
	short			nfontsUsed;			/* no. of entries in fontTable */						\
	FONTITEM		fontTable[MAX_SCOREFONTS]; /* To convert stored to system font nos. */		\
																								\
	unsigned char musFontName[32]; /* name of this document's music font */						\
																								\
	short			magnify,			/* Current reduce/enlarge magnification, 0=none */		\
					selStaff;			/* If sel. is empty, insertion pt's staff, else undefined */ \
	SignedByte		otherMNStaff,		/* (not yet used) staff no. for meas. nos. besides stf 1 */ \
					numberMeas;			/* Show measure nos.: -=every system, 0=never, n=every nth meas. */ \
	short			currentSystem,		/* systemNum of system which contains caret or, if non-empty sel, selStartL (but currently not defined) */	\
					spaceTable,			/* ID of 'SPTB' resource having spacing table to use */ \
					htight,				/* Percent tightness */									\
					fillerInt,			/* unused */											\
					lookVoice,			/* Voice to look at, or -1 for all voices */			\
					fillerHP,			/* (unused) */											\
					fillerLP,			/* (unused) */											\
					ledgerYSp,			/* Extra space above/below staff for max. ledger lines (half-spaces) */ \
					deflamTime;			/* Maximum time between consec. attacks in a chord (millsec.) */ \
																								\
	Boolean			autoRespace,		/* Respace on symbol insert, or leave things alone? */	\
					insertMode,			/* Graphic insertion logic (else temporal)? */			\
					beamRests,			/* In beam handling, treat rests like notes? */			\
					pianoroll,			/* Display everything in pianoroll, not CMN, form? */	\
					showSyncs,			/* Show (with InvertSymbolHilite) lines on every Sync? */	\
					frameSystems,		/* Frame systemRects (for debugging)? */				\
					fillerEM,			/* unused */											\
					colorVoices,		/* 0=normal, 1=show non-dflt voices in color, 2=show all but voice 1 in color */ \
					showInvis,			/* Display invisible objects? */						\
					showDurProb,		/* Show measures with duration/time sig. problems? */	\
					recordFlats;		/* True if black-key notes recorded should use flats */ \
																								\
	long			spaceMap[MAX_L_DUR];	/* Ideal spacing of basic (undotted, non-tuplet) durs. */ \
	DDIST			dIndentFirst,		/* Amount to indent first System */						\
					yBetweenSys;		/* obsolete, was vert. "dead" space btwn Systems */		\
	VOICEINFO		voiceTab[MAXVOICES+1];	/* Descriptions of voices in use */					\
	short			expansion[256-(MAXVOICES+1)];


typedef struct {
	NIGHTSCOREHEADER
} SCOREHEADER, *PSCOREHEADER;


/* The <Document> struct includes a DOCUMENTHEADER, a set of Heaps, and a 
NIGHTSCOREHEADER, along with many other fields. Each open Document file is represented
on the desktop by a single window with which the user edits the Document. This
Document's window record is the first field of an extended Document struct, so that
pointers to Documents can be used anywhere a WindowPtr can.  Each Document consists of
a standard page size and margin for all pages, and between 1 and numSheets pages,
called <sheets>.

 Sheets, the internal form of pages, are always numbered from 0 to (numSheets-1);
 pages are numbered according to the user's whim.  Sheets are laid out to tile the space
 in the window, extending first horizontally and then vertically into the window space. 
 There is always a currentSheet that we're editing, and whose upper left corner is always
 (0,0) with respect to any drawing routines that draw on the sheets. However, sheets are
 kept in an array whose upper left origin (sheetOrigin) is usually chosen very negative
 so that we can use as much of Quickdraws 16-bits space as possible. The coordinate
 system of the window is continually danced around as the current sheet changes, as well
 as during scrolling.  The bounding box of all sheets is used to compute the scrolling
 bounds.  The background region is used to paint a background pattern behind all sheets
 in the array. */

typedef struct {
/* These first fields don't need to be saved. */
	WindowPtr		theWindow;			/* The window being used to show this doc */
	Str255			name;				/* File name */
	FSSpec			fsSpec;
	short			vrefnum;			/* Directory file name is local to */
	Rect			viewRect;			/* Port rect minus scroll bars */
	Rect			growRect;			/* Grow box */

	Rect			allSheets;			/* Bounding box of all visible sheets */
	RgnHandle		background;			/* Everything except sheets */

	ControlHandle	vScroll;			/* Vertical scroll bar */
	ControlHandle	hScroll;			/* Horizontal scroll bar */
	DDIST			yBetweenSysMP;		/* (always 0) Vertical sp. between Systems for Master Page */
	THPrint			hPrint;				/* Handle to Document print record */
	StringPoolRef	stringPool;			/* Strings for this Document */

	Rect			caret;				/* Rect of Document's caret */
	long			nextBlink;			/* When to blink the caret */

	Point			scaleCenter;		/* Window coord of last mouse click to enlarge from */
	
	unsigned short inUse,				/* This slot in table is in use */
					changed,			/* True if Document should be saved on close */
					canCutCopy,			/* True if something is cuttable/copyiable */
					active,				/* True when Document is active window */
					docNew,
					masterView,			/* True when editing master sheet */
					overview,			/* True when viewing all pages from on high */
					hasCaret,			/* True when the Document's caret is initialized */
					cvisible,			/* True when the Document's caret is visible */
					idleOff,			/* True when caret not idle */
					caretOn,			/* True if caret is now in its visible blink state */
					caretActive,		/* True if caret should be blinking at all */
					fillSI,				/* unused */
					readOnly,			/* True if Document was opened Read Only */
					masterChanged,		/* Always False outside masterView */
					showFormat;			/* True when showing format rather than content */
	unsigned short	enterFormat,		/* True when entering show format */
					converted,			/* True if Document converted from older file format */
					locFmtChanged,		/* True if any "local" format changes (via Work on Format) */
					nonstdStfSizes,		/* True = not all staves are of size doc->srastral */
					showWaitCurs,		/* True means show "please wait" cursor while drawing */
					moreUnused:11;
	UNDOREC			undo;				/* Undo record for the Document */
	
/* Non-Nightingale-specific fields which need to be saved */
	DOCUMENTHEADER

/* Nightingale-specific fields which need to be saved */
	HEAP 			Heap[LASTtype];			/* One HEAP for every type of object */
	NIGHTSCOREHEADER

/* MIDI driver fields. We now support only Core MIDI; OMS and FreeMIDI are pre-OS X. */
	/* OMS (obsolete) */
	OMSUniqueID		omsPartDeviceList[MAXSTAVES];	/* OMS MIDI dev data for each part on MP, */
													/*   indexed by partn associated with PARTINFOheap */
	OMSUniqueID		omsInputDevice;  				/* OMS MIDI dev data for recording inputs */
	/* FreeMIDI (obsolete) */
	fmsUniqueID		fmsInputDevice;					/* FreeMIDI dev data for recording inputs */
	fmsDestinationMatch	fmsInputDestination;		/* (See comment at PARTINFO in NBasicTypes.h.) */
	/* Core MIDI */
	MIDIUniqueID	cmPartDeviceList[MAXSTAVES];	/* CoreMIDI dev data for each part on MP, */
													/*   indexed by partn associated with PARTINFOheap */
	MIDIUniqueID	cmInputDevice;  				/* CoreMIDI dev data for recording inputs */

/* The remaining fields don't need to be saved. */
	Rect			prevMargin;			/* doc->marginRect upon entering Master Page */
	short			srastralMP;			/* srastral set inside Master Page */
	short			nSysMP;				/* Number of systems which can fit on a page */
	DDIST			dIndentFirstMP,		/* Amount to indent first System */
					dIndentOtherMP;		/* Amount to indent Systems other than first */	
	SignedByte		firstNamesMP,		/* 0=show none, 1=show abbrev., 2=show full names */
					otherNamesMP;		/* 0=show none, 1=show abbrev., 2=show full names */
	short			nstavesMP;			/* Number of staves in Master Page */
	DDIST			*staffTopMP;		/* staffTop array for Master Page. 1-based indexing (staffn-based indexing) */
	MPCHANGEDFLAGS						/* partChangedMP,sysChangedMP,stfChangedMP,margVChangedMP,margHChangedMP,indentChangedMP. */
	LINK			oldMasterHeadL,		/* Master Page data structure used to revert Master Page */
					oldMasterTailL;

	short			mutedPartNum;		/* Number of currently-muted part, or 0 = none */
	short			musicFontNum;		/* Font ID number of font to use for music chars */
	short			musFontInfoIndex;	/* Index into <musFontInfo> of current music font */

	PARTINFO		*partFiller;		/* Unused; formerly <part>, used in MPImportExport.c */
	DDIST			*staffTopFiller;	/* Unused; formerly <staffTop>, used in MPImportExport.c */
	short			nChangeMP,
					npartsFiller,		/* Unused; formerly <nparts>, used in MPImportExport.c */
					nstaves1Filler;		/* Unused; formerly <nstaves>, used in MPImportExport.c */
	CHANGE_RECORD	change[MAX_MPCHANGES];

	LINK			oldSelStartL,
					oldSelEndL;

	DDIST			staffSize[MAXSTAVES+1];	/* Staff size */
	
	DocPrintInfo	docPrintInfo;
	Handle			flatFormatHandle;
	
	Handle			midiMapFSSpecHdl;
	Handle			midiMap;

} Document;


/* -------------------------------------------------------------------- Configuration -- */

/* General configuration information is kept in a 'CNFG' resource, with the structure
 given below; its size is 256 bytes.
 
 To add a field (assuming there's room for it):
 
 1. Add its declaration just before the unused[] array below.
 2. Reduce the size of the unused[] array accordingly. NB: if the size is even, its
 starting offset is odd: this can lead certain C compilers to put a byte of padding
 in front of it, leading to subtle and potentially nasty problems! To avoid this
 the new size being even, include or comment out (but do not remove!) the
 <unusedOddByte> field, as appropriate.
 3. In GetConfig(), add code to initialize the new field and, if possible, to check for
 illegal values.
 
 In comments on the struct declaration below, each field is tagged by the type of
 information it contains, viz. "G" (Graphic), "L" (Logical), "P" (Performance), "S"
 (System), or "U" (User interface). */

typedef struct {
	short		maxDocuments;		/* S: Size of Document table to allocate */

	SignedByte	stemLenNormal;		/* G: Standard stem length (quarter-spaces) */
	SignedByte	stemLen2v;			/* G: Standard stem length if >=2 voices/staff (qtr-spaces) */
	SignedByte	stemLenOutside;		/* G: Standard stem length if entire stem outside of staff (qtr-spaces) */
	SignedByte	stemLenGrace;		/* G: Standard grace note stem length (qtr-spaces) */

	SignedByte	spAfterBar;			/* G: For Respace, "normal" space between barline and next symbol (eighth-spaces) */
	SignedByte	minSpBeforeBar;		/* G: For Respace, min. space between barline and previous symbol (eighth-spaces) */
	SignedByte	minRSpace;			/* G: For Respace, minimum space between symbols (eighth-spaces) */
	SignedByte	minLyricRSpace;		/* G: For Respace, minimum space between lyrics (eighth-spaces) */
	SignedByte	minRSpace_KSTS_N;	/* G: For Respace, min. space btwn keysig or timesig and note (eighth-spaces) */

	SignedByte	hAccStep;			/* G: Step size for moving accidentals to avoid overlap */

	SignedByte	stemLW;				/* G: PostScript linewidth for note stems (% of a space) */
	SignedByte	barlineLW;			/* G: PostScript linewidth for regular barlines (% of a space) */
	SignedByte	ledgerLW;			/* G: PostScript linewidth for ledger lines (% of a space) */
	SignedByte	staffLW;			/* G: PostScript linewidth for staff lines (% of a space) */
	SignedByte	enclLW;				/* G: PostScript linewidth for text/meas.no. enclosure (qtr-points) */

	SignedByte	beamLW;				/* G: (+)PostScript linewidth for normal-note beams (% of a space) */
	SignedByte	hairpinLW;			/* G: +PostScript linewidth for hairpins (% of a space) */
	SignedByte	dottedBarlineLW;	/* G: +PostScript linewidth for dotted barlines (% of a space) */
	SignedByte	mbRestEndLW;		/* G: +PostScript linewidth for multibar rest end lines (% of a space) */
	SignedByte	nonarpLW;			/* G: +PostScript nonarpeggio sign linewidth (% of a space) */

	SignedByte	graceSlashLW;		/* G: PostScript linewidth of slashes on grace-notes stems (% of a space) */
	SignedByte	slurMidLW;			/* G: PostScript linewidth of middle of slurs and ties (% of a space) */
	SignedByte	tremSlashLW;		/* G: +Linewidth of tremolo slashes (% of a space) */

	SignedByte	slurCurvature;		/* G: Initial slur curvature (% of "normal") */
	SignedByte	tieCurvature;		/* G: Initial tie curvature (% of "normal") */
	SignedByte	relBeamSlope;		/* G: Relative slope for new beams (%) */

	SignedByte	hairpinMouthWidth;	/* G: Initial hairpin mouth width (quarter-spaces) */
	SignedByte	mbRestBaseLen;		/* G: Length of 2-meas. multibar rest (qtr-spaces) */
	SignedByte	mbRestAddLen;		/* G: Additional length per meas. of multibar rest (qtr-spaces) */
	SignedByte	barlineDashLen;		/* G: Length of dashes in dotted barlines (eighth-spaces) */
	SignedByte	crossStaffBeamExt;	/* G: Dist. cross-staff beams extend past end stem (qtr-spaces) */

	SignedByte	slashGraceStems;	/* G: !0 = slash stems of (normally unbeammed 8th) grace notes */
	SignedByte	bracketsForBraces;	/* G: !0 = substitute square brackets for curly braces */

	SignedByte	titleMargin;		/* G: Additional margin at top of first page, in points */
	
	Rect		paperRect;			/* G: Default paper size, in points */
	Rect		pageMarg;			/* G: Default page top/left/bottom/right margins, in points */
	Rect		pageNumMarg;		/* G: Page number top/left/bottom/right margins, in points */

	SignedByte	defaultLedgers;		/* G: Default max. no. of ledger lines to position systems for */

	SignedByte	defaultTSNum;		/* L: Default time signature numerator */
	SignedByte	defaultTSDenom;		/* L: Default time signature denominator */

	SignedByte	defaultRastral;		/* G: Default staff size rastral */
	SignedByte	rastral0size;		/* G: Height of rastral 0 staff (points) */

	SignedByte	minRecVelocity;		/* P: Softer (lower-velocity) recorded notes are ignored */
	SignedByte	minRecDuration;		/* P: Shorter recorded notes are ignored (milliseconds) */
	SignedByte	midiThru;			/* P: MIDI Thru: 0 = off, 1 = to modem port */

	short		defaultTempoMM;		/* P: Default tempo (quarters per min.) */

	short		lowMemory;			/* S: Number of free Kbytes below which to warn once */
	short		minMemory;			/* S: Number of free Kbytes below which to warn always */

	Point		toolsPosition;		/* U: Palette offset from left/right of screen (plus/minus) */

	short		numRows;			/* U: Initial layout of sheets */
	short		numCols;			/* U: ditto */
	short		maxRows;			/* U: No more than this number of rows in sheet array */
	short		maxCols;			/* U: ditto */
	short		vPageSep;			/* U: Amount to separate pages horizontally */
	short		hPageSep;			/* U: And vertically */
	short		vScrollSlop;		/* U: Amount of paper to keep in view, vertically */
	short		hScrollSlop;		/* U: Amount of paper to keep in view, horizontally */
	Point		origin;				/* U: Of upper left corner sheet in array */

	SignedByte	powerUser;			/* U: Enable power user feature(s)? */
	SignedByte	maxSyncTolerance;	/* U: Max. horiz. tolerance to sync on note insert (% of notehead width) */
	SignedByte	dblBarsBarlines;	/* U: Double bars/rpt bars inserted become: 0=ask, 1=MEASUREs, 2=PSEUDOMEAS/RPTENDs */
	SignedByte	enlargeHilite;		/* U: Enlarge all selection hiliting areas, horiz. and vert. (pixels) */
	SignedByte	disableUndo;		/* U: !0 = disable the Undo command */
	SignedByte	assumeTie;			/* U: Assume slur/tie of same pitch is: 0=ask, 1=tie, 2=slur */
	SignedByte	delRedundantAccs;	/* U: On record, automatically Delete Redundant Accidentals? */
	SignedByte	strictContin; 		/* U: Use strict (old) rules for continous selections? */
	SignedByte	turnPagesInPlay;	/* U: !0 = "turn pages" on screen during playback */
	SignedByte	infoDistUnits;		/* U: Code for units for Get Info to display distances */
	SignedByte	earlyMusic;			/* U: !0 = allow early music clef positions */
	SignedByte	mShakeThresh;		/* U: Max. number of ticks between connected mouse shakes */

	short		musicFontID;		/* S: Font ID of music font (0=use Sonata) */
	short		numMasters;			/* S: Size of extra MasterPtr blocks */

	SignedByte	indentFirst;		/* G: Default 1st system indent (qtr-spaces) */
	SignedByte	mbRestHeight;		/* G: In qtr-spaces: 2 for engraving, 4 for "manuscript" style */
	short		chordSymMusSize;	/* G: Size of music chars. in chord syms. (% of text size) */
	SignedByte	enclMargin;			/* G: Margin between text bboxes and enclosures (points) */
	SignedByte	musFontSizeOffset;	/* G: Offset on PostScript size for music font (points) */
	
	SignedByte	fastScreenSlurs;	/* U: Code for faster-drawing but less beautiful slurs on screen */
	SignedByte	legatoPct;			/* P: For notes, play duration/note duration (%) */
	short		defaultPatch;		/* P: MIDI default patch number, 1 to MAXPATCHNUM */
	SignedByte	whichMIDI;			/* P: Use MIDI Mgr, built-in MIDI, or none: see InitMIDI */
	SignedByte	restMVOffset; 		/* G: Rest multivoice offset from normal ht. (half-spaces) */
	SignedByte	autoBeamOptions; 	/* U: Control beam breaks for Autobeam */
	SignedByte	dontAskSavePrefs; 	/* U: !0 = save Preference and palette changes without asking */
	SignedByte	noteOffVel;			/* P: Default MIDI Note Off velocity */
	SignedByte	feedbackNoteOnVel;	/* P: Note On velocity to use for feedback on input, etc. */
	SignedByte	defaultChannel;		/* P: MIDI default channel number, 1 to MAXCHANNEL */
	SignedByte	enclWidthOffset;	/* G: Offset on width of text enclosures (points) */
	short		rainyDayMemory;		/* S: Number of Kbytes in "rainy day" memory fund */
	short		tryTupLevels;		/* L: When quantizing, beat levels to look for tuplets at */
	SignedByte	solidStaffLines;	/* U: !0 = (non-Master Page) staff lines are solid, else gray */
	SignedByte	moveModNRs;			/* U: !0 = when notes move vertically, move modifiers too */
	SignedByte	minSpAfterBar;		/* G: For Respace, min. space between barline and next symbol (eighth-spaces) */
	SignedByte	leftJustPN;			/* G: !0=left-justify part names, else center them */
	SignedByte	show1StfSysConn;	/* G: Draw system connect if only 1 staff visible? */
	SignedByte	show1StfPartConn;	/* G: Draw part connect if only 1 staff visible? */
	SignedByte	midiFeedback;		/* U: Set MIDI feedback initially on? */
	SignedByte	alwaysCtrWholeMR;	/* G: Center whole-meas. rests entered even if auto-respace is off? */
	SignedByte	barTapSlop;			/* U: Tap-in barline tolerance to put before next note (100ths of sec.) */
	SignedByte	justifyWarnThresh;	/* U: Min. stretching for Justify to warn about (mult. of 10%) */
	SignedByte	leastSquares;		/* L: Least-squares fit to decide tuplets from real time (else least-linear)? */
	SignedByte	altPitchCtl;		/* U: Use alternate accidental-pitch-entry method? */

	SignedByte	metroViaMIDI;		/* U: True=use MIDI for metronome, False=internal sound */
	SignedByte	metroChannel;		/* U: MIDI metronome channel number, 1 to MAXCHANNEL */
	SignedByte	metroNote;			/* U: MIDI metronome note number */
	SignedByte	metroVelo;			/* U: MIDI metronome note velocity */
	short		metroDur;			/* U: MIDI metronome note duration (millisec.) */
						
	SignedByte	chordSymSmallSize;	/* G: Size of font used in all components of chord symbol*/
											/* except root string. (% of CS font size) */
	SignedByte	chordSymSuperscr;	/* G: Superscript of chord sym extensions above root baseline (% of CS font size) */
	SignedByte	chordSymDrawPar;	/* G: !0=draw parentheses around each level of ext stack in chord sym */
	SignedByte	endingHeight;		/* G: Height above staff top of new endings (half-spaces) */
	SignedByte	drawStemlets;		/* G: !0=draw stemlets on beamed rests */
	SignedByte	noTripletBias;		/* U: When quantizing, bias against recognizing tuplets */
	SignedByte	chordSymStkLead;	/* G: Extra leading of chord sym extensions (%) */
	SignedByte	ignoreRecVel;		/* U: On record, ignore recorded note velocity & set it from context? */

	/* Following fields were added after Nightingale 1.0. */

	SignedByte	tempoMarkHGap;		/* G: Extra gap between tempo mark & metronome mark */
	SignedByte	conFiller;

	short		trebleVOffset;		/* G: Vertical offset on treble clef from Sonata pos. (% of a space) */
	short		cClefVOffset;		/* G: Vertical offset on C clef from Sonata pos. (% of a space) */
	short		bassVOffset;		/* G: Vertical offset on bass clef from Sonata pos. (% of a space) */

	SignedByte	tupletNumSize;		/* G: PostScript size of accessory numerals in tuplets (% of text size) */
	SignedByte	tupletColonSize;	/* G: PostScript size of colons between tuplet numerals (% of text size) */
	SignedByte	octaveNumSize;		/* G: PostScript size of number on octave signs (% of text size) */
	SignedByte	noteScanEpsXD;		/* U: Open NoteScan File tolerance for combining subobjs (points) */
	SignedByte	lineLW;				/* G: Linewidth of line Graphics (% of a space) */
	SignedByte	ledgerLLen;			/* G: Length of ledger lines on notehead's side of stem (32nds of a space) */
	SignedByte	ledgerLOtherLen;	/* G: Length of ledger lines on other side of stem (32nds of a space) */
	SignedByte	printJustWarn;		/* U: !0=on printing, warn if all systems aren't right-justified */

	/* Following fields were added after Nightingale 2.0. */
	
	SignedByte	slurDashLen;		/* G: PostScript length of dashes in dashed slurs (points) */	
	SignedByte	slurSpaceLen;		/* G: PostScript length of spaces in dashed slurs (points) */	
	SignedByte	courtesyAccLXD;		/* G: Left paren. H offset from courtesy accidental (8th-spaces) */
	SignedByte	courtesyAccRXD;		/* G: Courtesy accidental H offset from right paren. (8th-spaces) */
	SignedByte	courtesyAccYD;		/* G: Paren. V offset for courtesy accidentals (8th-spaces) */
	SignedByte	courtesyAccPSize;	/* G: Paren. size for courtesy accidentals (% of normal) */

	/* Following fields were added after Nightingale 2.5. */
	
	OMSUniqueID metroDevice;			/* P: OMS identifier for metronome synth (no longer used) */
	OMSUniqueID defaultInputDevice;		/* P: OMS identifier for default input synth (no longer used) */
	OMSUniqueID defaultOutputDevice;	/* P: OMS identifier for default output synth (no longer used) */
	SignedByte	defaultOutputChannel;	/* P: channel for dflt out device; input uses defaultChannel (no longer used) */

	/* Following fields were added after Nightingale 3.0. */

	SignedByte	wholeMeasRestBreve;		/* U: !0 = whole-measure rests can look like breve rests */
	SignedByte	quantizeBeamYPos;		/* G: !0 = force beam vertical positions to sit, straddle, or hang */
	SignedByte	makeBackup;				/* U: !0 = on Save, rename and keep old file as backup */

	short		chordFrameFontID;		/* G: Font ID of chord-frame font (0=use Seville) */
	SignedByte	chordFrameRelFSize;		/* G: !0 = size is relative to staff size */ 
	SignedByte	chordFrameFontSize;		/* G: if chordFrameRelFSize, small..large code, else point size */
	SignedByte	notelistWriteBeams;		/* G: !0 = include beamsets in Notelist files */
	SignedByte	extractAllGraphics;		/* U: !0 = extract to all parts all Graphics attached to suitable objs */

	/* Following fields were added after Nightingale 3.5. */

	Byte		maxHyphenDist;			/* G: For flow in text, max. space a hyphen can span (half-spaces; 0 = 1/2 in.) */
	SignedByte	enlargeNRHiliteH;		/* U: Further enlarge note/rest sel. hiliting area, horiz. (pixels) */
	SignedByte	enlargeNRHiliteV;		/* U: Further enlarge note/rest sel. hiliting area, vertical (pixels) */
	SignedByte	pianorollLenFact;		/* G: Relative length of note bars in pianoroll (%; 0 = just round blobs) */

	/* Following fields were added after Nightingale 4.0. */

	SignedByte	useModNREffects;		/* P: Use modifier prefs for velocity, duration, etc. changes when playing? */
	SignedByte	thruChannel;			/* U: Channel for MIDI thru output */
	unsigned short thruDevice;			/* U: Device for MIDI thru output */

	/* Following fields were added after Nightingale 4.0, file format version "N104". */
	
	MIDIUniqueID cmMetroDev;			/* P: Core MIDI identifier for metronome synth */
	MIDIUniqueID cmDfltInputDev;		/* P: Core MIDI identifier for default input synth */
	MIDIUniqueID cmDfltOutputDev;		/* P: Core MIDI identifier for default output synth */
	SignedByte	 cmDfltOutputChannel;	/* P: channel for dflt out device; input uses defaultChannel */

	/* Padding and final stuff. NOTE: Cf. comments on <unused> above before adding fields! */
	
	SignedByte	unusedOddByte;			/* S: Room for expansion: must be 0 in resource. */

	SignedByte	unused[32];				/* S: Room for expansion: must be 0's in resource */
	SignedByte	fastLaunch;				/* S: SECRET FIELD: Don't show splash screen, MIDI init ALRT, or font warnings? FOR TESTING ONLY! */
	SignedByte	noExpire;				/* S: SECRET FIELD: defeat expiration date (unused since v. 5.5) */
	
	} Configuration;


/*
 *	MIDI configuration information (currently, just the dynamic-mark-to-velocity table)
 * is kept in a 'MIDI' resource, with this structure.
 */

typedef struct {
	SignedByte	velocities[23];		/* Dynamic velocities */
	SignedByte	unused;				/* Filler to make even */
	SignedByte	roomToGrow[104];	/* Room for expansion: resource is 128 bytes long */
} MIDIPreferences;

/* MIDI modifier information, kept in a 'MIDM' resource, with this structure. Array
indices are ModNR identifiers (MOD_FERMATA, etc.). New in v.4.1b6.  -JGG, 6/24/01  */

typedef struct {
	short		velocityOffsets[32];	/* Velocity offset from note's value */
	short		durationFactors[32];	/* % change from note's play duration */
	short		timeFactors[32];		/* (unused) supposed to affect time offset */
	short		filler[32];
} MIDIModNRPreferences;


/* For support of music fonts other than Sonata */
typedef struct {
	short			fontID;
	Str31			fontName;

	/* from 'MFEx' rsrc */
	Boolean			sonataFlagMethod;		/* (unused) Draw flags like Sonata? */
	Boolean			has16thFlagChars;		/* Has 16th flag chars? */
	Boolean			hasCurlyBraceChars;		/* Has curly brace chars (as in Sonata) */
	Boolean			hasRepeatDotsChar;		/* Has repeat dot char (as in Sonata) */
	Boolean			upstemFlagsHaveXOffset;	/* Upstem flags start to the right of their origin by width of "stem-space" char? */
	Boolean			hasStemSpaceChar;		/* Has "stem-space" char giving with of notehead minus stem (as in Sonata)? */
	short			stemSpaceWidth;			/* Width of note head minus stem (i.e., of "stem-space" char) (% of space) */
	short			upstemExtFlagLeading;	/* Up-stem extension flag leading (% of space) */
	short			downstemExtFlagLeading;	/* Down-stem extension flag leading (% of space) */
	short			upstem8thFlagLeading;	/* Up-stem 8th flag leading (% of space) */
	short			downstem8thFlagLeading;	/* Down-stem 8th flag leading (% of space) */
	Str31			postscriptFontName;		/* PostScript font name of this music font (not printer font file name). This
												is usually same as font name, but (e.g.) "Mozart 2001" uses "Mozart-MMI". */
	Rect			cBBox[256];				/* character bounding boxes (from 'BBX#' rsrc) */
	unsigned char	cMap[256];				/* map Sonata char (index) to char in this font (from 'MCMp' rsrc) */
	short			xd[256];				/* Offset (in % of a space) relative to Sonata chars (index) (from 'MCOf' rsrc) */
	short			yd[256];				/* ditto */
} MusFontRec;

/* Navigation Services client data */
typedef struct NSClientData {
	char 		nsFileName[256];		/* Name of file for open or save dialog */
	FSSpec 		nsFSSpec;				/* FSSpec to uniquely id the file */
	Boolean		nsIsClosing;			/* True if the window is closing [not supported or used] */
	Boolean		nsOpCancel;				/* True if cancel is issued */
	OSStatus	nsStatus;				/* Status of navservices operation [not supported or used] */
} NSClientData,*NSClientDataPtr;

struct SysEnvRec {
  short               environsVersion;
  short               machineType;
  short               systemVersion;
  short               processor;
  Boolean             hasFPU;
  Boolean             hasColorQD;
  short               keyBoardType;
  short               atDrvrVersNum;
  short               sysVRefNum;
};
typedef struct SysEnvRec	SysEnvRec;


/* ---------------------------------------------------------------------- FONTUSEDITEM -- */

typedef struct {
	Boolean			style[4];		/* styles used (bold and italic attribs. only) */
	unsigned char	fontName[32];	/* font name (Pascal string) */	
} FONTUSEDITEM;

#pragma options align=reset
