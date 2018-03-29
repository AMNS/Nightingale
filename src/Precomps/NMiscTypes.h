/* NMiscTypes.h (formerly applicationTypes.h) for Nightingale */

// MAS
#include <CoreMIDI/MIDIServices.h>		/* for MIDIUniqueID */
// MAS

// MAS we want to /always/ use mac68k alignment
#pragma options align=mac68k

/* DOCUMENTHEADER is generic: it contains fields appropriate for any program that
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


/* The <Document> struct includes a DOCUMENTHEADER, a set of Heaps, and a
NIGHTSCOREHEADER, along with many other fields. Each open Document file is represented
on the desktop by a single window with which the user edits the Document. This
Document's window record is the first field of an extended Document struct, so that
pointers to Documents can be used anywhere a WindowPtr can.  Each Document consists of
a standard page size and margin for all pages, and between 1 and numSheets pages,
called <sheets>.

Sheets are always numbered from 0 to (numSheets-1) and are the internal form of a
page; pages are numbered according to the user's whim.  Sheets are laid out to tile
the space in the window, extending first horizontally and then vertically into the
window space.  There is always a currentSheet that we're editing, and whose upper left
corner is always (0,0) with respect to any drawing routines that draw on the sheets. 
However, sheets are kept in an array whose upper left origin (sheetOrigin) is usually
chosen very negative so that we can use as much of Quickdraws 16-bits space as
possible. The coordinate system of the window is continually danced around as the
current sheet changes, as well as during scrolling.  The bounding box of all sheets is
used to compute the scrolling bounds.  The background region is used to paint a
background pattern behind all sheets in the array. */

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
	NIGHTSCOREHEADER

/* MIDI driver fields. We now support only Core MIDI; OMS and FreeMIDI are pre-OS X. */
	/* OMS (obsolete) */
	OMSUniqueID		omsPartDeviceList[MAXSTAVES];	/* OMS MIDI dev data for each part on MP, */
													/*   indexed by partn associated with PARTINFOheap */
	OMSUniqueID		omsInputDevice;  				/* OMS MIDI dev data for recording inputs */
	/* FreeMIDI (obsolete) */
	fmsUniqueID		fmsInputDevice;					/* FreeMIDI dev data for recording inputs */
	destinationMatch	fmsInputDestination;		/* (See comment at PARTINFO in NTypes.h.) */
	/* Core MIDI */
	MIDIUniqueID	cmPartDeviceList[MAXSTAVES];	/* CoreMIDI dev data for each part on MP, */
													/*   indexed by partn associated with PARTINFOheap */
	MIDIUniqueID	cmInputDevice;  				/* CoreMIDI dev data for recording inputs */

/* The remaining fields don't need to be saved. */
	Rect			prevMargin;			/* doc->marginRect upon entering Master Page */
	short			srastralMP;			/* srastral set inside Master Page */
	short			nSysMP;				/* Number of systems which can fit on a page */
	DDIST			firstIndentMP,		/* Amount to indent first System */
					otherIndentMP;		/* Amount to indent Systems other than first */	
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
	CHANGE_RECORD	change[MAX_CHANGES];

	LINK			oldSelStartL,
					oldSelEndL;

	DDIST			staffSize[MAXSTAVES+1];	/* Staff size */
	
	DocPrintInfo	docPrintInfo;
	Handle			flatFormatHandle;
	
	Handle			midiMapFSSpecHdl;
	Handle			midiMap;

} Document;


/*
 * General configuration information is kept in a 'CNFG' resource, with the structure
 * given below; its size is 256 bytes.
 *
 * To add a field (assuming there's room for it):
 *
 * 1. Add its declaration just before the unused[] array below.
 * 2. Reduce the size of the unused[] array accordingly. NB: if the size is even, its
 * 	starting offset is odd: this will lead to certain C  compilers sometimes putting a
 * 	byte of padding in front of it, leading to subtle but potentially nasty problems! To
 * 	avoid this the new size being even, include or comment out (but do not remove!) the
 * 	<unusedOddByte> field, as appropriate.
 * 3. In GetConfig(), add code to initialize the new field and, if possible, to check for
 *	 illegal values.
 *
 * In comments on the struct declaration below, each field is tagged by the type of
 * information it contains, viz. "G" (Graphic), "L" (Logical), "P" (Performance), "S"
 * (System), or "U" (User interface).
 */

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

	SignedByte	indent1st;			/* G: Default 1st system indent (qtr-spaces) */
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
	SignedByte	courtesyAccLXD;		/* G: Left paren. H offset for courtesy accidentals (8th-spaces) */
	SignedByte	courtesyAccRXD;		/* G: Right paren. H offset for courtesy accidentals (8th-spaces) */
	SignedByte	courtesyAccYD;		/* G: Paren. V offset for courtesy accidentals (8th-spaces) */
	SignedByte	courtesyAccSize;	/* G: Paren. size for courtesy accidentals (% of normal) */

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

/*
 * MIDI modifier information, kept in a 'MIDM' resource, with this structure. Array
 * indices are ModNR identifiers (MOD_FERMATA, etc.). New in v.4.1b6.  -JGG, 6/24/01
 */

typedef struct {
	short		velocityOffsets[32];	/* Velocity offset from note's value */
	short		durationFactors[32];	/* % change from note's play duration */
	short		timeFactors[32];		/* (unused) supposed to affect time offset */
	short		filler[32];
} MIDIModNRPreferences;


/*
 * The GridRec holds the information for the tool palette.
 */

typedef struct {
	Point			cell;
	unsigned char	ch;
	} GridRec;


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
typedef struct SysEnvRec			SysEnvRec;

#pragma options align=reset
