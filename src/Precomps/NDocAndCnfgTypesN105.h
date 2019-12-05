/* NDocAndCnfgTypes.h (formerly part of NMiscTypes.h) for Nightingale: declarations for
the Document and headers that are part of it, plus some Preference info and a few other
things  - version for the 'N105' file format. NB: Many of these appear in Nightingale
score files, so changing them may be a problem for backward compatibility. In fact, in
format 'N105' files (as well as in-memory data structures of earlier versions of
Nightingale), our objects and subobjects as well as file headers are jammed with bit
fields. In the words of Harbison & Steele's _C: A Reference Manual_, bit fields are
"likely to be nonportable". Indeed, handling of bit fields in code generated by the C
compiler in Xcode 2.5 for Intel CPUs vs. PowerPCs is not compatible. Nightingale has
always run on PowerPCs, so we need to get bit fields out of files to let Nightingale on
Intel read older files. */

// MAS
#include <CoreMIDI/MIDIServices.h>		/* for MIDIUniqueID */
// MAS

// MAS we want to /always/ use mac68k alignment
#pragma options align=mac68k

/* ------------------------------------------------------------------------- MPCHANGED -- */
/*	Changed flags for exporting Master Page. ??CAN THESE AFFECT FILES?? */

#ifdef NOMORE
#define MPCHANGEDFLAGS				\
	short	partChangedMP : 1,		\
			sysVChangedMP : 1,		\
			sysHChangedMP : 1,		\
			stfChangedMP : 1,		\
			margVChangedMP : 1,		\
			margHChangedMP : 1,		\
			indentChangedMP : 1,	\
			fixMeasRectYs : 1,		\
			grpChangedMP : 1,		\
			stfLinesChangedMP : 1,	\
			unusedChgdMP : 6;

typedef struct {
	MPCHANGEDFLAGS
} MPCHGDFLAGS;
#endif

/* -------------------------------------------------------------------- Major headers -- */

#define NIGHTSCOREHEADER_N105																		\
	LINK 		headL,				/* links to header and tail objects */						\
				tailL,																			\
				selStartL,			/* currently selected range. */								\
				selEndL;			/*		Also see the <selStaff> field. */					\
																								\
	short		nstaves,			/* number of staves in a system */							\
				nsystems;			/* number of systems in score */							\
	unsigned char comment[MAX_COMMENT_LEN+1]; /* User comment on score */						\
	char		feedback:1;			/* True if we want feedback on note insert */				\
	char		dontSendPatches:1; /* 0 = when playing, send patch changes for channels */		\
	char		saved:1;			/* True if score has been saved */							\
	char		named:1;			/* True if file has been named */							\
	char 		used:1;				/* True if score contains any nonstructural info */			\
	char		transposed:1;		/* True if transposed score, else C score */				\
	char		lyricText:1;		/* (no longer used) True if last text entered was lyric */	\
	char		polyTimbral:1;		/* True for one part per MIDI channel */					\
	Byte		currentPage;		/* (no longer used) */										\
	short		spacePercent,		/* Percentage of normal horizontal spacing used */			\
				srastral,			/* Standard staff size rastral no. */						\
				altsrastral,		/* (unused) Alternate staff size rastral no. */				\
				tempo,				/* playback speed in beats per minute */					\
				channel,			/* Basic MIDI channel number */								\
				velocity;			/* global playback velocity offset */						\
	STRINGOFFSET headerStrOffset;	/* index returned by String Manager */						\
	STRINGOFFSET footerStrOffset;	/* index returned by String Manager */						\
	char		topPGN:1;			/* True=page numbers at top of page, else bottom */			\
	char		hPosPGN:3;			/* 1=page numbers at left, 2=center, 3=at right */			\
	char		alternatePGN:1;		/* True=page numbers alternately left and right */			\
	char		useHeaderFooter:1;	/* True=use header/footer text, not simple pagenum */		\
	char		fillerPGN:2;		/* unused */												\
	SignedByte	fillerMB;			/* unused */												\
	DDIST		filler2,			/* unused */												\
				dIndentOther;		/* Amount to indent Systems other than first */				\
	SignedByte	firstNames,			/* Code for drawing part names: see enum above */			\
				otherNames,			/* Code for drawing part names: see enum above */			\
				lastGlobalFont,		/* Header index of most recent text style used */			\
				xMNOffset,			/* Horiz. pos. offset for meas. nos. (half-spaces) */		\
				yMNOffset,			/* Vert. pos. offset for meas. nos. (half-spaces) */		\
				xSysMNOffset;		/* Horiz. pos. offset for meas.nos.if 1st meas.in system */ \
	short		aboveMN:1,			/* True=measure numbers above staff, else below */			\
				sysFirstMN:1,		/* True=indent 1st meas. of system by xMNOffset */			\
				startMNPrint1:1,	/* True=First meas. number to print is 1, else 2 */			\
				firstMNNumber:13;	/* Number of first measure */								\
	LINK		masterHeadL,		/* Head of Master Page object list */						\
				masterTailL;		/* Tail of Master Page object list */						\
	SignedByte	filler1,																		\
				nFontRecords;		/* Always 15 for now */										\
	/* Fifteen identical TEXTSTYLE records. Fontnames are Pascal strings. */					\
																								\
	unsigned char fontNameMN[32];	/* MEASURE NO. FONT: default name, size and style */		\
	unsigned short	fillerMN:5;																	\
	unsigned short	lyricMN:1;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosureMN:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeMN:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSizeMN:7;	/* if relFSizeMN, small..large code, else point size */		\
	short			fontStyleMN;																\
																								\
	unsigned char fontNamePN[32];	/* PART NAME FONT: default name, size and style */			\
	unsigned short	fillerPN:5;																	\
	unsigned short	lyricPN:1;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosurePN:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizePN:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSizePN:7;	/* if relFSizePN, small..large code, else point size */		\
	short			fontStylePN;																\
																								\
	unsigned char fontNameRM[32];	/* REHEARSAL MARK FONT: default name, size and style */		\
	unsigned short	fillerRM:5;																	\
	unsigned short	lyricRM:1;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosureRM:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeRM:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSizeRM:7;	/* if relFSizeRM, small..large code, else point size */		\
	short			fontStyleRM;																\
																								\
	unsigned char fontName1[32];	/* REGULAR FONT 1: default name, size and style */			\
	unsigned short	fillerR1:5;																	\
	unsigned short	lyric1:1;		/* True if spacing like lyric	*/							\
	unsigned short	enclosure1:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize1:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize1:7;	/* if relFSize1, small..large code, else point size */		\
	short			fontStyle1;																	\
																								\
	unsigned char fontName2[32];	/* REGULAR FONT 2: default name, size and style */			\
	unsigned short	fillerR2:5;																	\
	unsigned short	lyric2:1;		/* True if spacing like lyric	*/							\
	unsigned short	enclosure2:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize2:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize2:7;	/* if relFSize2, small..large code, else point size */		\
	short			fontStyle2;																	\
																								\
	unsigned char fontName3[32];	/* REGULAR FONT 3: default name, size and style */			\
	unsigned short	fillerR3:5;																	\
	unsigned short	lyric3:1;		/* True if spacing like lyric	*/							\
	unsigned short	enclosure3:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize3:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize3:7;	/* if relFSize3, small..large code, else point size */		\
	short			fontStyle3;																	\
																								\
	unsigned char fontName4[32];	/* REGULAR FONT 4: default name, size and style */			\
	unsigned short	fillerR4:5;																	\
	unsigned short	lyric4:1;		/* True if spacing like lyric	*/							\
	unsigned short	enclosure4:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize4:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize4:7;	/* if relFSizeR4, small..large code, else point size */		\
	short			fontStyle4;																	\
																								\
	unsigned char fontNameTM[32];	/* TEMPO MARK FONT: default name, size and style */			\
	unsigned short	fillerTM:5;																	\
	unsigned short	lyricTM:1;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosureTM:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeTM:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSizeTM:7;	/* if relFSizeTM, small..large code, else point size */		\
	short			fontStyleTM;																\
																								\
	unsigned char fontNameCS[32];	/* CHORD SYMBOL FONT: default name, size and style */		\
	unsigned short	fillerCS:5;																	\
	unsigned short	lyricCS:1;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosureCS:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeCS:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSizeCS:7;	/* if relFSizeCS, small..large code, else point size */		\
	short			fontStyleCS;																\
																								\
	unsigned char fontNamePG[32];	/* PAGE HEADER/FOOTER/NO.FONT: default name, size and style */	\
	unsigned short	fillerPG:5;																	\
	unsigned short	lyricPG:1;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosurePG:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizePG:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSizePG:7;	/* if relFSizePG, small..large code, else point size */		\
	short			fontStylePG;																\
																								\
	unsigned char fontName5[32];	/* REGULAR FONT 5: default name, size and style */			\
	unsigned short	fillerR5:5;																	\
	unsigned short	lyric5:1;		/* True if spacing like lyric (ignored) */					\
	unsigned short	enclosure5:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize5:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize5:7;	/* if relFSize5, small..large code, else point size */		\
	short			fontStyle5;																	\
																								\
	unsigned char fontName6[32];	/* REGULAR FONT 6: default name, size and style */			\
	unsigned short	fillerR6:5;																	\
	unsigned short	lyric6:1;		/* True if spacing like lyric	*/							\
	unsigned short	enclosure6:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize6:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize6:7;	/* if relFSizeR6, small..large code, else point size */		\
	short			fontStyle6;																	\
																								\
	unsigned char fontName7[32];	/* REGULAR FONT 7: default name, size and style */			\
	unsigned short	fillerR7:5;																	\
	unsigned short	lyric7:1;		/* True if spacing like lyric	*/							\
	unsigned short	enclosure7:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize7:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize7:7;	/* if relFSizeR7, small..large code, else point size */		\
	short			fontStyle7;																	\
																								\
	unsigned char fontName8[32];	/* REGULAR FONT 8: default name, size and style */			\
	unsigned short	fillerR8:5;																	\
	unsigned short	lyric8:1;		/* True if spacing like lyric	*/							\
	unsigned short	enclosure8:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize8:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize8:7;	/* if relFSizeR8, small..large code, else point size */		\
	short			fontStyle8;																	\
																								\
	unsigned char fontName9[32];	/* REGULAR FONT 9: default name, size and style */			\
	unsigned short	fillerR9:5;																	\
	unsigned short	lyric9:1;		/* True if spacing like lyric	*/							\
	unsigned short	enclosure9:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize9:1;	/* True if size is relative to staff size */				\
	unsigned short	fontSize9:7;	/* if relFSizeR9, small..large code, else point size */		\
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
	SignedByte		otherMNStaff,			/* (not yet used) staff no. for meas. nos. besides stf 1 */ \
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
					fillerEM:4,			/* unused */											\
					colorVoices:2,		/* 0=normal, 1=show non-dflt voices in color, 2=show all but voice 1 in color */ \
					showInvis:1,		/* Display invisible objects? */						\
					showDurProb:1,		/* Show measures with duration/time sig. problems? */	\
					recordFlats;		/* True if black-key notes recorded should use flats */ \
																								\
	long			spaceMap[MAX_L_DUR];	/* Ideal spacing of basic (undotted, non-tuplet) durs. */ \
	DDIST			dIndentFirst,		/* Amount to indent first System */						\
					yBetweenSys;		/* obsolete, was vert. "dead" space btwn Systems */		\
	VOICEINFON105	voiceTab[MAXVOICES+1];	/* Descriptions of voices in use */					\
	short			expansion[256-(MAXVOICES+1)];


typedef struct {
	NIGHTSCOREHEADER_N105
} SCOREHEADER_N105, *PSCOREHEADER_N105;


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
	
	unsigned short inUse:1,				/* This slot in table is in use */
					changed:1,			/* True if Document should be saved on close */
					canCutCopy:1,		/* True if something is cuttable/copyiable */
					active:1,			/* True when Document is active window */
					docNew:1,
					masterView:1,		/* True when editing master sheet */
					overview:1,			/* True when viewing all pages from on high */
					hasCaret:1,			/* True when the Document's caret is initialized */
					cvisible:1,			/* True when the Document's caret is visible */
					idleOff:1,			/* True when caret not idle */
					caretOn:1,			/* True if caret is now in its visible blink state */
					caretActive:1,		/* True if caret should be blinking at all */
					fillSI:1,			/* unused */
					readOnly:1,			/* True if Document was opened Read Only */
					masterChanged:1,	/* Always False outside masterView */
					showFormat:1;		/* True when showing format rather than content */
	unsigned short	enterFormat:1,		/* True when entering show format */
					converted:1,		/* True if Document converted from older file format */
					locFmtChanged:1,	/* True if any "local" format changes (via Work on Format) */
					nonstdStfSizes:1,	/* True = not all staves are of size doc->srastral */
					showWaitCurs:1,		/* True means show "please wait" cursor while drawing */
					moreUnused:11;
	UNDOREC			undo;				/* Undo record for the Document */
	
/* Non-Nightingale-specific fields which need to be saved */
	DOCUMENTHEADER

/* Nightingale-specific fields which need to be saved */
	HEAP 			Heap[LASTtype];			/* One HEAP for every type of object */
	NIGHTSCOREHEADER_N105

/* MIDI driver fields. We now support only Core MIDI; OMS and FreeMIDI are pre-OS X. */
	/* OMS (obsolete) */
	OMSUniqueID		omsPartDeviceList[MAXSTAVES];	/* OMS MIDI dev data for each part on MP, */
													/*   indexed by partn associated with PARTINFOheap */
	OMSUniqueID		omsInputDevice;  				/* OMS MIDI dev data for recording inputs */
	/* FreeMIDI (obsolete) */
	fmsUniqueID		fmsInputDevice;					/* FreeMIDI dev data for recording inputs */
	destinationMatch	fmsInputDestination;		/* (See comment at PARTINFO in NBasicTypes.h.) */
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

} DocumentN105;

#pragma options align=reset