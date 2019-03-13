/*	NTypes.h - #defines and typedefs for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

#ifndef TypesIncluded
#define TypesIncluded

#pragma options align=mac68k

/* -------------------------------- Data-structure Things ------------------------------- */

/* We assume already-included headers (on the Macintosh, Types.h) say:
	typedef unsigned char Byte;
	typedef signed char SignedByte;
*/

#define INT16 short				/* 2 bytes; no longer used */
#define STDIST short			/* range +-4096 staffLines, resolution 1/8 (STD_LINEHT) staffLine */
#define LONGSTDIST long			/* range +-268,435,456 staffLines, resolution 1/8 staffLine */
#define QDIST short				/* range +-8192 staffLines, resolution 1/4 staffLine */
#define SHORTSTD SignedByte		/* range +-16 staffLines, resolution 1/8 staffLine */
#define SHORTQD SignedByte		/* range +-32 staffLines, resolution 1/4 staffLine */
#define DDIST short				/* range +-2048 points (+-28 in), resolution 1/16 point */
#define SHORTDDIST SignedByte	/* range +-8 points, resolution 1/16 point */
#define LONGDDIST long			/* range +-134,217,728 points (+-155344 ft), resolution 1/16 point */
#define STD_LINEHT 8			/* STDIST scale: value for standard staff interline space */
								/* NB: Before changing, consider roundoff in conversion macros! */
#define FASTFLOAT double		/* For floating-point vars. that don't need much range or precision */
#define STRINGOFFSET long		/* So we don't have to include "StringManager.h" everywhere */

typedef unsigned short Word;
typedef unsigned long DoubleWord;

typedef struct {
	Rect box, bounds, shadow, prompt;
	short currentChoice, menuID;
	MenuHandle menu;
	unsigned char str[40];		/* Pascal string */
} UserPopUp;

typedef struct {
	DDIST	v, h;
} DPoint;

typedef struct {
	DDIST	top, left, bottom, right;
} DRect;

typedef struct						/* Key signature item: */
{
	char letcode:7;					/* LG: Code for letter name: A=5,B=4,C=3,D=2,E=1,F=0,G=6  */
	Boolean sharp:1;				/* LG: Is it a sharp? (False=flat) */
} KSITEM;

/* NB: We #define a WHOLE_KSINFO macro, then a struct that consists of nothing but an
invocation of that macro; why? I'm don't remember, but old comments here pointed out that
with THINK C 5 the macro takes 15 bytes, but the struct takes 16. */

#define WHOLE_KSINFO					/* Complete key sig. w/o context. */			\
	KSITEM		KSItem[MAX_KSITEMS];	/* The sharps and flats */						\
	SignedByte	nKSItems;				/* No. of sharps and flats in key sig. */
	
typedef struct						/* Complete key sig. */
{
	WHOLE_KSINFO
} KSINFO, *PKSINFO;

typedef struct
{
	SignedByte	TSType;				/* Time signature context */
	SignedByte	numerator;
	SignedByte	denominator;
} TSINFO, *PTSINFO;

enum {								/* Window types */
	DETAIL=1,
	PALETTE,
	KEYBOARD
};

enum {								/* Types of part names (to label left ends of systems) */
	NONAMES=0,
	ABBREVNAMES,
	FULLNAMES,
	MAX_NAMES_TYPE=FULLNAMES
};


/* Generic subObj functions FirstSubObj(), NextSubObj() in Objects.c are dependent on
the order of items in the following enum. Arrays subObjLength[]  and objLength[], in
vars.h, MUST be updated if this enum is changed. */

enum								/* Object types: */
{
			FIRSTtype = 0,
/* 0 */		HEADERtype=FIRSTtype,
			TAILtype,
			SYNCtype,				/* Note/rest sync */
			RPTENDtype,
			PAGEtype,

/* 5 */		SYSTEMtype,
			STAFFtype,
			MEASUREtype,
			CLEFtype,
			KEYSIGtype,

/* 10 */	TIMESIGtype,
			BEAMSETtype,
			CONNECTtype,
			DYNAMtype,
			MODNRtype,

/* 15 */	GRAPHICtype,
			OTTAVAtype,
			SLURtype,				/* Slur or set of ties */
			TUPLETtype,
			GRSYNCtype,				/* Grace note sync */
	
/* 20 */	TEMPOtype,
			SPACERtype,
			ENDINGtype,
			PSMEAStype,
			OBJtype,				/* Object heap MUST be the last heap. */
			LASTtype
};

#define LOWtype HEADERtype
#define HIGHtype LASTtype

typedef unsigned short LINK;

typedef struct {
	Handle block;					/* Handle to floating array of objects */
	short objSize;					/* Size in bytes of each object in array */
	short type;						/* Type of object for this heap */
	LINK firstFree;					/* Index of head of free list */
	unsigned short nObjs;			/* Maximum number of objects in heap block */
	unsigned short nFree;			/* Size of the free list */
	short lockLevel;				/* Nesting lock level: >0 ==> locked */
} HEAP;


/* -------------------------------------------------------------------------------------- */
/* Macros in MemMacros.h depend on the positions of the first five fields of the
object header, which MUST NOT be re-positioned! In the future, this may apply to
other fields as well. */

#define OBJECTHEADER \
	LINK		right, left;		/* links to left and right objects */					\
	LINK		firstSubObj;		/* link to first subObject */							\
	DDIST		xd, yd;				/* position of object */								\
	SignedByte	type;				/* (.+#10) object type */								\
	Boolean		selected;			/* True if any part of object selected */				\
	Boolean		visible;			/* True if any part of object is visible */				\
	Boolean		soft;				/* True if object is program-generated */				\
	Boolean		valid;				/* True if objRect (for Measures, measureBBox also) valid. */ \
	Boolean		tweaked;			/* True if object dragged or position edited with Get Info */ \
	Boolean		spareFlag;			/* available for general use */							\
	char		ohdrFiller1;		/* unused; could use for specific "tweak" flags */		\
	Rect		objRect;			/* enclosing rectangle of object (paper-rel.pixels) */ 	\
	SignedByte	relSize;			/* (unused) size rel. to normal for object & context */	\
	SignedByte	ohdrFiller2;		/* unused */											\
	Byte		nEntries;			/* (.+#22??) number of subobjects in object */
	
#define SUBOBJHEADER \
	LINK		next;				/* index of next subobj */								\
	SignedByte	staffn;				/* staff number. For cross-stf objs, top stf (Slur,Beamset) or 1st stf (Tuplet) */									\
	SignedByte	subType;			/* subobject subtype. N.B. Signed--see ANOTE. */		\
	Boolean		selected;			/* True if subobject is selected */						\
	Boolean		visible;			/* True if subobject is visible */						\
	Boolean		soft;				/* True if subobject is program-generated */

#define EXTOBJHEADER \
	SignedByte	staffn;				/* staff number: for cross-staff objs, of top staff FIXME: except tuplets! */


/* ---------------------------------------------------------------------------- MEVENT -- */

typedef struct sMEVENT {
	OBJECTHEADER
} MEVENT, *PMEVENT;


/* ---------------------------------------------------------------------------- EXTEND -- */

typedef struct sEXTEND {
	OBJECTHEADER
	EXTOBJHEADER
} EXTEND, *PEXTEND;


/* ------------------------------------------------------------------------- TEXTSTYLE -- */

typedef struct xTEXTSTYLE {
	unsigned char	fontName[32];	/* default font name: Pascal string */
	unsigned short	filler2;
	unsigned short	lyric;			/* True means lyric spacing */
	unsigned short	enclosure;
	unsigned short	relFSize;		/* True if size is relative to staff size */ 
	unsigned short	fontSize;		/* if relFSize, small..large code, else point size */
	short			fontStyle;
} TEXTSTYLE, *PTEXTSTYLE;


/* ------------------------------------------------------------------------- VOICEINFO -- */

typedef struct {
	Byte		partn;				/* No. of part using voice, or 0 if voice is unused */
	Byte		voiceRole;			/* upper, lower, single, or cross-stf voice */
	Byte		relVoice;			/* Voice no. within the part, >=1 */
} VOICEINFO;


/* ---------------------------------------------------------------------- SCORE HEADER -- */

typedef struct						/* A part (for an instrument or voice): */
{
	LINK		next;				/* index of next subobj */
	SignedByte	partVelocity;		/* MIDI playback velocity offset */
	SignedByte	firstStaff;			/* Index of first staff in the part */
	Byte		patchNum;			/* MIDI program no. */
	SignedByte	lastStaff;			/* Index of last staff in the part (>= first staff) */
	Byte		channel;			/* MIDI channel no. */
	SignedByte	transpose;			/* Transposition, in semitones (0=none) */
	short		loKeyNum;			/* MIDI note no. of lowest playable note */
	short		hiKeyNum;			/* MIDI note no. of highest playable note  */
	char		name[32];			/* Full name, e.g., to label 1st system (C string) */
	char		shortName[12];		/* Short name, e.g., for systems after 1st (C string) */
	char		hiKeyName;			/* Name and accidental of highest playable note */
	char		hiKeyAcc;
	char		tranName;			/* ...of transposition */
	char		tranAcc;
	char		loKeyName;			/* ...of lowest playable note */
	char		loKeyAcc;

	/* New fields for N103 file format  -JGG, 8/4/01 */
	Byte		bankNumber0;		/* If device uses cntl 0 for bank select msgs */
	Byte		bankNumber32;		/* If device uses cntl 32 for bank select msgs */
									/* NB: some devices use both cntl 0 and cntl 32 */

	/* The following fields were for FreeMIDI support. FreeMIDI is obsolete -- it's a
		pre-OS X technology -- but we keep the fields for file compatability. */
	fmsUniqueID	fmsOutputDevice;
	destinationMatch fmsOutputDestination;	/* NB: about 280 bytes */
} PARTINFO, *PPARTINFO;

typedef struct {
	short	fontID;					/* font ID number for TextFont */
	unsigned char fontName[32]; 	/* font name: Pascal string */
} FONTITEM;

typedef struct {
	OBJECTHEADER
} HEADER, *PHEADER;


#define NIGHTSCOREHEADER																		\
	LINK 		headL,				/* links to header and tail objects */						\
				tailL,																			\
				selStartL,			/* currently selected range. */								\
				selEndL;			/*		Also see the <selStaff> field. */					\
																								\
	short		nstaves,			/* number of staves in a system */							\
				nsystems;			/* number of systems in score */							\
	unsigned char comment[MAX_COMMENT_LEN+1]; /* User comment on score */						\
	char		feedback;			/* True if we want feedback on note insert */				\
	char		dontSendPatches; 	/* 0 = when playing, send patch changes for channels */		\
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
				tempo,				/* playback speed in beats per minute */					\
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


/* ------------------------------------------------------------------------------ TAIL -- */

typedef struct {
	OBJECTHEADER
} TAIL, *PTAIL;


/* ------------------------------------------------------------------------------ PAGE -- */

typedef struct sPAGE {
	OBJECTHEADER
	LINK		lPage,				/* Links to left and right Pages */
				rPage;
	short		sheetNum;			/* Sheet number: indexed from 0 */
	StringPtr	headerStrOffset,	/* (unused; when used, should be STRINGOFFSETs) */
				footerStroffset;
} PAGE, *PPAGE;


/* ---------------------------------------------------------------------------- SYSTEM -- */

typedef struct sSYSTEM {
	OBJECTHEADER
	LINK		lSystem,			/* Links to left and right Systems */
				rSystem;
	LINK		pageL;				/* Link to previous (enclosing) Page */
	short		systemNum;			/* System number: indexed from 1 */
	DRect		systemRect;			/* DRect enclosing entire system, rel to Page */
	Ptr			sysDescPtr;			/* (unused) ptr to data describing left edge of System */
} SYSTEM, *PSYSTEM;


/* ----------------------------------------------------------------------------- STAFF -- */

#define SHOW_ALL_LINES	15

typedef struct {
	LINK		next;				/* index of next subobj */
	SignedByte	staffn;				/* staff number */
	Boolean		selected;			/* True if subobject is selected */
	Boolean		visible;			/* True if object is visible */
	Boolean		fillerStf;			/* unused */
	DDIST		staffTop,			/* relative to systemRect.top */
				staffLeft,			/* always 0 now; rel to systemRect.left */
				staffRight;			/* relative to systemRect.left */
	DDIST		staffHeight;		/* staff height */
	SignedByte	staffLines;			/* number of lines in staff: 0..6 (always 5 for now) */
	short		fontSize;			/* preferred font size for this staff */
	DDIST		flagLeading;		/* (unused) vertical space between flags */
	DDIST		minStemFree;		/* (unused) min. flag-free length of note stem */
	DDIST		ledgerWidth;		/* (unused) Standard ledger line length */
	DDIST		noteHeadWidth;		/* width of common note heads */
	DDIST		fracBeamWidth;		/* Fractional beam length */
	DDIST		spaceBelow;			/* Vert space occupied by stf; stored in case stf made invis */
	SignedByte	clefType;			/* clef context */
	SignedByte	dynamicType;		/* dynamic marking context */
	WHOLE_KSINFO					/* key signature context */
	SignedByte	timeSigType,		/* time signature context */
				numerator,
				denominator;
	unsigned char filler,			/* unused */
				showLedgers,		/* True if drawing ledger lines of notes on this staff (the default if showLines>0) */
				showLines;			/* 0=show 0 staff lines, 1=only middle line (of 5-line staff), 2-14 unused,
										SHOW_ALL_LINES=show all lines (default) */
} ASTAFF, *PASTAFF;

typedef struct sSTAFF {
	OBJECTHEADER
	LINK			lStaff,				/* links to left and right Staffs */
					rStaff;
	LINK			systemL;			/* link to previous (enclosing) System */
} STAFF, *PSTAFF;


/* --------------------------------------------------------------------------- CONNECT -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	Boolean		selected;			/* True if subobject is selected */
	Byte 		filler;
	Byte		connLevel;			/* Code from list below */
	Byte		connectType;		/* Code from list below */
	SignedByte	staffAbove;			/* upper staff no. (top of line or curly) (valid if connLevel!=0) */
	SignedByte	staffBelow;			/* lower staff no. (bottom of " ) (valid if connLevel!=0) */
	DDIST		xd;					/* DDIST position */
	LINK		firstPart;			/* (Unused) LINK to first part of group or connected part if not a group */
	LINK		lastPart;			/* (Unused) LINK to last part of group or NILINK if not a group */
} ACONNECT, *PACONNECT;

typedef struct {
	OBJECTHEADER
	LINK		connFiller;
} CONNECT, *PCONNECT;

enum {								/* Codes for connectType */
	CONNECTLINE=1,
	CONNECTBRACKET,
	CONNECTCURLY
};

enum {								/* Codes for connLevel */
	SystemLevel=0,
	GroupLevel,
	PartLevel=7
};


/* ----------------------------------------------------------------------- ACLEF, CLEF -- */

typedef struct {
	SUBOBJHEADER
	Byte		filler1;
	Byte		small;				/* True to draw in small characters */
	Byte		filler2;
	DDIST		xd, yd;				/* DDIST position */
} ACLEF, *PACLEF;

typedef struct {
	OBJECTHEADER
	Boolean	inMeasure;			/* True if object is in a Measure, False if not */
} CLEF, *PCLEF;

enum {								/* clef subTypes: */
	TREBLE8_CLEF=1,					/* unused */
	FRVIOLIN_CLEF,					/* unused */
	TREBLE_CLEF,
	SOPRANO_CLEF,
	MZSOPRANO_CLEF,
	ALTO_CLEF,
	TRTENOR_CLEF,
	TENOR_CLEF,
	BARITONE_CLEF,
	BASS_CLEF,
	BASS8B_CLEF,					/* unused */
	PERC_CLEF
};

#define LOW_CLEF TREBLE8_CLEF
#define HIGH_CLEF PERC_CLEF


/* ---------------------------------------------------------------------------- KEYSIG -- */

typedef struct {
	SUBOBJHEADER					/* subType=no. of naturals, if nKSItems==0 */
	Byte			nonstandard;	/* True if not a standard CMN key sig. */
	Byte			filler1;
	Byte			small;			/* (unused so far) True to draw in small characters */
	SignedByte		filler2;
	DDIST			xd;				/* DDIST horizontal position */
	WHOLE_KSINFO
} AKEYSIG, *PAKEYSIG;

typedef struct {
	OBJECTHEADER
	Boolean	inMeasure;				/* True if object is in a Measure, False if not */
} KEYSIG, *PKEYSIG;


/* --------------------------------------------------------------------------- TIMESIG -- */

typedef struct {
	SUBOBJHEADER
	Byte		filler;				/* Unused--put simple/compound/other here? */
	Byte		small;				/* (unused so far) True to draw in small characters */
	SignedByte	connStaff;			/* (unused so far) bottom staff no. */
	DDIST		xd, yd;				/* DDIST position */
	SignedByte	numerator;			/* numerator */
	SignedByte	denominator;		/* denominator */
} ATIMESIG, *PATIMESIG;

typedef struct {
	OBJECTHEADER
	Boolean		inMeasure;			/* True if object is in a Measure, False if not */
} TIMESIG, *PTIMESIG;

enum {								/* subtypes: */
	N_OVER_D=1,
	C_TIME,
	CUT_TIME,
	N_ONLY,
	ZERO_TIME,
	N_OVER_QUARTER,
	N_OVER_EIGHTH,
	N_OVER_HALF,
	N_OVER_DOTTEDQUARTER,
	N_OVER_DOTTEDEIGHTH
};

#define LOW_TStype N_OVER_D
#define HIGH_TStype N_OVER_DOTTEDEIGHTH


/* --------------------------------------------------------------------------- MEASURE -- */

typedef struct {
	SUBOBJHEADER					/* subType=barline type (see enum below) */
	Boolean		measureVisible;		/* True if measure contents are visible */
	Boolean		connAbove;			/* True if connected to barline above */
	char		filler1;
	SignedByte	filler2;
	short		oldFakeMeas,		/* OBSOLETE: now at the object level, so this is to be removed */
				measureNum;			/* internal measure number; first is always 0 */
	DRect		measSizeRect;		/* enclosing Rect of measure, V rel. to System top & H to meas. xd  */
	SignedByte	connStaff;			/* staff to connect to (valid if >0 and !connAbove) */
	SignedByte	clefType;			/* clef context */
	SignedByte	dynamicType;		/* dynamic marking context */
	WHOLE_KSINFO					/* key signature context */
	SignedByte	timeSigType,		/* time signature context */
				numerator,
				denominator;
	SHORTSTD	xMNStdOffset;		/* horiz. offset on measure number position */
	SHORTSTD	yMNStdOffset;		/* vert. offset on measure number position */
} AMEASURE, *PAMEASURE;

typedef struct sMEASURE	{
	OBJECTHEADER
	SignedByte	fillerM;
	LINK			lMeasure,		/* links to left and right Measures */
					rMeasure;
	LINK			systemL;		/* link to owning System */
	LINK			staffL;			/* link to owning Staff */
	short			fakeMeas,		/* True=not really a measure (i.e., barline ending system) */
					spacePercent;	/* Percentage of normal horizontal spacing used */
	Rect			measureBBox;	/* enclosing Rect of all measure subObjs, in pixels, paper-rel. */
	long			lTimeStamp;		/* P: PDURticks since beginning of score */
} MEASURE, *PMEASURE;

enum {								/* barline types */
	BAR_SINGLE=1,
	BAR_DOUBLE,
	BAR_FINALDBL,
	BAR_HEAVYDBL,					/* (unused) */
	BAR_RPT_L,						/* Codes must be the same as equivalent RPTENDs! */
	BAR_RPT_R,
	BAR_RPT_LR,
	BAR_LAST=BAR_RPT_LR
};


/* ------------------------------------------------------------------------ PSEUDOMEAS -- */
/* Pseudomeasures are symbols that have similar graphic appearance to barlines
but have no semantics: dotted barlines and double bars that don't coincide with
"real" barlines. */

typedef struct {
	SUBOBJHEADER					/* subType=barline type (see enum below) */
	Boolean		connAbove;			/* True if connected to barline above */
	char		filler1;			/* (unused) */
	SignedByte	connStaff;			/* staff to connect to (valid if >0 and !connAbove) */
} APSMEAS, *PAPSMEAS;

typedef struct 	{
	OBJECTHEADER
	SignedByte	filler;
} PSMEAS, *PPSMEAS;

enum {								/* pseudomeasure types: codes follow those for MEASUREs */
	PSM_DOTTED=BAR_LAST+1,
	PSM_DOUBLE,
	PSM_FINALDBL					/* unused */
};


/* ------------------------------------------------------------------------ NOTE, SYNC -- */
/* A "note" is a normal or small note or rest, perhaps a cue note, but not a grace
note. (The main reason is that grace notes have no logical duration.) */

typedef struct {
	SUBOBJHEADER					/* subType (l_dur): LG: <0=n measure rest, 0=unknown, >0=Logical (CMN) dur. code */
	Boolean		inChord;			/* True if note is part of a chord */
	Boolean		rest;				/* LGP: True=rest (=> ignore accident, ystem, etc.) */
	Boolean		unpitched;			/* LGP: True=unpitched note */
	Boolean		beamed;				/* True if beamed */
	Boolean		otherStemSide;		/* G: True if note goes on "wrong" side of stem */
	SHORTQD		yqpit;				/* LG: clef-independent dist. below middle C ("pitch") (unused for rests) */
	DDIST		xd, yd;				/* G: head position */
	DDIST		ystem;				/* G: endpoint of stem (unused for rests) */
	short		playTimeDelta;		/* P: PDURticks before/after timeStamp when note starts */
	short		playDur;			/* P: PDURticks that note plays for */
	short		pTime;				/* P: PDURticks play time; for internal use by Tuplet routines */
	Byte		noteNum;			/* P: MIDI note number (unused for rests) */
	Byte		onVelocity;			/* P: MIDI note-on velocity, normally loudness (unused for rests) */
	Byte		offVelocity;		/* P: MIDI note-off (release) velocity (unused for rests) */
	Boolean		tiedL;				/* True if tied to left */
	Boolean		tiedR;				/* True if tied to right */
	Byte		ymovedots;			/* G: Y-offset on aug. dot pos. (halflines, 2=same as note, except 0=invisible) */
	Byte		ndots;				/* LG: No. of aug. dots */
	SignedByte	voice;				/* L: Voice number */
	Byte		rspIgnore;			/* True if note's chord should not affect automatic spacing (unused as of v. 5.5) */
	Byte		accident;			/* LG: 0=none, 1--5=dbl. flat--dbl. sharp (unused for rests) */
	Boolean		accSoft;			/* L: Was accidental generated by Nightingale? */
	Boolean		playAsCue;			/* L: True = play note as cue, ignoring dynamic marks (unused as of v. 5.7) */
	Byte		micropitch;			/* LP: Microtonal pitch modifier (unused as of v. 5.5) */
	Byte		xmoveAcc;			/* G: X-offset to left on accidental position */
	Byte		merged;				/* temporary flag for Merge functions */
	Byte		courtesyAcc;		/* G: Accidental is a "courtesy accidental" */
	Byte		doubleDur;			/* G: Draw as if double the actual duration */
	Byte		headShape;			/* G: Special notehead or rest shape; see list below */
	Byte		xmovedots;			/* G: X-offset on aug. dot position */
	LINK		firstMod;			/* LG: Note-related symbols (articulation, fingering, etc.) */
	Byte		slurredL;			/* True if endpoint of slur to left (extra bit for future use) */
	Byte		slurredR;			/* True if endpoint of slur to right (extra bit for future use) */
	Byte		inTuplet;			/* True if in a tuplet */
	Byte		inOttava;			/* True if in an octave sign */
	Byte		small;				/* True if a small (cue, etc.) note */
	Byte		tempFlag;			/* temporary flag for benefit of functions that need it */
	SignedByte	fillerN;
} ANOTE, *PANOTE;

typedef struct {
	OBJECTHEADER
	unsigned short timeStamp;		/* P: PDURticks since beginning of measure */
} SYNC, *PSYNC;

enum {								/* Notehead and rest appearances: */
	NO_VIS=0,						/* Notehead/rest invisible */
	NORMAL_VIS,						/* Normal appearance */
	X_SHAPE,						/* "X" head (notes only) */
	HARMONIC_SHAPE,					/* "Harmonic" hollow head (notes only) */
	SQUAREH_SHAPE,					/* Square hollow head (notes only) */
	SQUAREF_SHAPE,					/* Square filled head (notes only) */
	DIAMONDH_SHAPE,					/* Diamond-shaped hollow head (notes only) */
	DIAMONDF_SHAPE,					/* Diamond-shaped filled head (notes only) */
	HALFNOTE_SHAPE,					/* Halfnote head (for Schenker, etc.) (notes only) */
	SLASH_SHAPE,					/* Chord slash */
	NOTHING_VIS						/* EVERYTHING (head/rest + stem, aug.dots, etc.) invisible */
};


/* --------------------------------------------------------------------------- BEAMSET -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	LINK		bpSync;				/* link to Sync containing note/chord */
	SignedByte	startend;			/* No. of beams to start/end (+/-) on note/chord */
	Byte		fracs;			/* No. of fractional beams on note/chord */ 
	Byte		fracGoLeft;		/* Fractional beams point left? */
	Byte		filler;			/* unused */
} ANOTEBEAM, *PANOTEBEAM;

typedef struct sBeam {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte	voice;				/* Voice number */
	Byte		thin;				/* True=narrow lines, False=normal width */
	Byte		beamRests;			/* True if beam can contain rests */
	Byte		feather;			/* (unused) 0=normal,1=feather L end (accel.), 2=feather R (decel.) */
	Byte		grace;				/* True if beam consists of grace notes */
	Byte		firstSystem;		/* True if on first system of cross-system beam */	
	Byte		crossStaff;			/* True if the beam is cross-staff: staffn=top staff */
	Byte		crossSystem;		/* True if the beam is cross-system */
} BEAMSET, *PBEAMSET;

enum {
	NoteBeam,
	GRNoteBeam
};

typedef struct {
	SignedByte	start;
	SignedByte	stop;
	SignedByte	startLev;			/* Vertical slot no. (0=end of stem; -=above) */
	SignedByte	stopLev;			/* Vertical slot no. (0=end of stem; -=above) */
	SignedByte	fracGoLeft;
} BEAMINFO;


/* ---------------------------------------------------------------------------- TUPLET -- */

/* This struct is used to get information from the user interface (TupletDialog). */

typedef struct {
	Byte			accNum;			/* Accessory numeral (numerator) for Tuplet */
	Byte			accDenom;		/* Accessory denominator */
	short			durUnit;		/* Duration units of denominator */
	Boolean			numVis,
					denomVis,
					brackVis,
					isFancy;
} TupleParam;

typedef struct {
	LINK			next;			/* index of next subobj */
	LINK			tpSync;			/* link to Sync containing note/chord */
} ANOTETUPLE, *PANOTETUPLE;

typedef struct sTuplet {
	OBJECTHEADER
	EXTOBJHEADER
	Byte			accNum;			/* Accessory numeral (numerator) for Tuplet */
	Byte			accDenom;		/* Accessory denominator */
	SignedByte		voice;			/* Voice number */
	Byte			numVis,
					denomVis,
					brackVis,
					small,			/* (unused so far) True to draw in small characters */
					filler;
	DDIST			acnxd, acnyd;	/* DDIST position of accNum (now unused) */
	DDIST			xdFirst, ydFirst,	/* DDIST position of bracket */
					xdLast, ydLast;
} TUPLET, *PTUPLET;


/* ------------------------------------------------------------------------- REPEATEND -- */

typedef struct {
	SUBOBJHEADER					/* subType is in object so unused here */
	Byte			connAbove;		/* True if connected above */
	Byte			filler;			/* (unused) */
	SignedByte		connStaff;		/* staff to connect to; valid if connAbove True */
} ARPTEND, *PARPTEND;

typedef struct {
	OBJECTHEADER
	LINK			firstObj;		/* Beginning of ending or NILINK */
	LINK			startRpt;		/* Repeat start point or NILINK */
	LINK			endRpt;			/* Repeat end point or NILINK */
	SignedByte		subType;		/* Code from enum below */
	Byte			count;			/* Number of times to repeat */
} RPTEND, *PRPTEND;

enum {
	RPT_DC=1,
	RPT_DS,
	RPT_SEGNO1,
	RPT_SEGNO2,
	RPT_L,							/* Codes must be the same as equivalent MEASUREs! */
	RPT_R,
	RPT_LR
};


/* ---------------------------------------------------------------------------- ENDING -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	LINK		firstObjL;			/* Object left end of ending is attached to */
	LINK		lastObjL;			/* Object right end of ending is attached to or NILINK */
	Byte		noLCutoff;			/* True to suppress cutoff at left end of Ending */	
	Byte		noRCutoff;			/* True to suppress cutoff at right end of Ending */	
	Byte		endNum;				/* 0=no ending number or label, else code for the ending label */
	DDIST		endxd;				/* Position offset from lastObjL */
} ENDING, *PENDING;


/* ----------------------------------------------------------------- ADYNAMIC, DYNAMIC -- */

typedef struct {
	SUBOBJHEADER					/* subType is unused */
	Byte			mouthWidth;		/* Width of mouth for hairpin */
	Byte			small;			/* True to draw in small characters */
	Byte			otherWidth;		/* Width of other (non-mouth) end for hairpin */
	DDIST			xd;				/* (unused) */
	DDIST			yd;				/* Position offset from staff top */
	DDIST			endxd;			/* Position offset from lastSyncL for hairpins. */
	DDIST			endyd;			/* Position offset from staff top for hairpins. */
} ADYNAMIC, *PADYNAMIC;

typedef struct {
	OBJECTHEADER
	SignedByte	dynamicType;		/* Code for dynamic marking (see enum below) */
	Boolean		filler:7;
	Boolean		crossSys:1;			/* (unused) Whether crossSystem */
	LINK		firstSyncL;			/* Sync dynamic or hairpin start is attached to */
	LINK		lastSyncL;			/* Sync hairpin end is attached to or NILINK */
} DYNAMIC, *PDYNAMIC;

enum {								/* FIXME: NEED MODIFIER BIT(S), E.G. FOR mpp, poco piu f */
	PPPP_DYNAM=1,
	PPP_DYNAM,
	PP_DYNAM,
	P_DYNAM,
	MP_DYNAM,							/* 5 */
	MF_DYNAM,
	F_DYNAM,
	FF_DYNAM,
	FFF_DYNAM,
	FFFF_DYNAM,							/* 10 */
	FIRSTREL_DYNAM,
	PIUP_DYNAM=FIRSTREL_DYNAM,
	MENOP_DYNAM,
	MENOF_DYNAM,
	PIUF_DYNAM,
	FIRSTSF_DYNAM,
	SF_DYNAM=FIRSTSF_DYNAM,				/* 15 */
	FZ_DYNAM,
	SFZ_DYNAM,
	RF_DYNAM,
	RFZ_DYNAM,
	FP_DYNAM,							/* 20 */
	SFP_DYNAM,
	FIRSTHAIRPIN_DYNAM,					/* ONLY hairpins after this! */
	DIM_DYNAM=FIRSTHAIRPIN_DYNAM,		/* Hairpin open at left ("diminuendo") */
	CRESC_DYNAM,						/* Hairpin open at right ("crescendo") */
	LAST_DYNAM
};


/* ---------------------------------------------------------------------------- AMODNR -- */

typedef struct {
	LINK		next;					/* index of next subobj */
	Boolean		selected;				/* True if subobject is selected */
	Boolean		visible;				/* True if subobject is visible */
	Boolean		soft;					/* True if subobject is program-generated */
	unsigned char xstd;					/* FIXME: use "Byte"? Note-relative position (really signed STDIST: see below) */
	Byte		modCode;				/* Which note modifier */
	SignedByte	data;					/* Modifier-dependent */
	SHORTSTD	ystdpit;				/* Clef-independent dist. below middle C ("pitch") */
} AMODNR, *PAMODNR;

#define XSTD_OFFSET 16				/* 2**(xstd fieldwidth-1) to fake signed value */

enum {								/* modCode values */
	MOD_FERMATA=10,					/* Leave 0 thru 9 for digits */
	MOD_TRILL,
	MOD_ACCENT,
	MOD_HEAVYACCENT,
	MOD_STACCATO,
	MOD_WEDGE,
	MOD_TENUTO,
	MOD_MORDENT,
	MOD_INV_MORDENT,
	MOD_TURN,
	MOD_PLUS,
	MOD_CIRCLE,
	MOD_UPBOW,
	MOD_DOWNBOW,
	MOD_TREMOLO1,
	MOD_TREMOLO2,
	MOD_TREMOLO3,
	MOD_TREMOLO4,
	MOD_TREMOLO5,
	MOD_TREMOLO6,
	MOD_HEAVYACC_STACC,
	MOD_LONG_INVMORDENT,
	MOD_FAKE_AUGDOT=127				/* Augmentation dot is not really a MODNR */
};


/* --------------------------------------------------------------------------- GRAPHIC -- */

typedef struct {
	LINK next;
	STRINGOFFSET strOffset;			/* index return by String Manager library. */
} AGRAPHIC, *PAGRAPHIC;

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER					/* N.B. staff number can be 0 here */
	SignedByte	graphicType;		/* graphic class (subtype) */
	SignedByte	voice;				/* Voice number (only with some types of relObjs) */
	Byte		enclosure;			/* Enclosure type; see list below */
	Byte		justify;			/* (unused) justify left/center/right */
	Boolean		vConstrain;			/* (unused) True if object is vertically constrained */
	Boolean		hConstrain;			/* (unused) True if object is horizontally constrained */
	Byte		multiLine;			/* True if string contains multiple lines of text (delimited by CR) */
	short		info;				/* PICT res. ID (GRPICT); char (GRChar); length (GRArpeggio); */
									/*   ref. to text style (FONT_R1, etc) (GRString,GRLyric); */
									/*	  2nd x (GRDraw); draw extension parens (GRChordSym) */
	union {
		Handle		handle;			/* handle to resource, or NULL */
		short		thickness;
	} gu;
	SignedByte	fontInd;			/* index into font name table (GRChar,GRString only) */
	Byte		relFSize;			/* True if size is relative to staff size (GRChar,GRString only) */ 
	Byte		fontSize;			/* if relSize, small..large code, else point size (GRChar,GRString only) */
	short		fontStyle;			/* (GRChar,GRString only) */
	short		info2;				/* sub-subtype (GRArpeggio), 2nd y (GRDraw), _expanded_ (GRString) */
	LINK		firstObj;			/* link to object left end is relative to, or NULL */
	LINK		lastObj;			/* link to object right end is relative to, or NULL */
} GRAPHIC, *PGRAPHIC;

#define ARPINFO(info2)	((unsigned short)(info2) >>13) 

enum {								/* graphicType values: */
	GRPICT=1,						/* 	(unimplemented) PICT */
	GRChar,							/* 	(unimplemented) single character */
	GRString,						/* 	character string */
	GRLyric,						/* 	lyric character string */
	GRDraw,							/* 	Pure graphic: so far, only lines; someday, MiniDraw */
	GRMIDIPatch,					/*	(unimplemented) MIDI program change */
	GRRehearsal,					/* 	rehearsal mark */
	GRChordSym,						/* 	chord symbol */
	GRArpeggio,						/*	arpeggio or non-arpeggio sign */
	GRChordFrame,					/*	chord frame (for guitar, etc.) */
	GRMIDIPan,
	GRSusPedalDown,
	GRSusPedalUp,
	GRLastType=GRSusPedalUp
};

/* Our internal codes for sustain on/off. FIXME: Should these be here or in a header file
 (MIDIGeneral.h)? (Are these really MIDI specific, or do they apply to notation too?) */

#define MIDISustainOn	127
#define MIDISustainOff	0

enum {
	GRJustLeft=1,					/* Graphic is left justified */
	GRJustRight,					/* Graphic is right justified */
	GRJustBoth,						/* Graphic is left and right justified */
	GRJustCenter					/* Graphic is centered */
};

enum {								/* Staff-relative text size codes */
	GRTiny=1,
	GRVSmall,
	GRSmall,
	GRMedium,
	GRLarge,
	GRVLarge,
	GRJumbo,
	GR_____1,
	GRStaffHeight,
	GRLastSize=GRStaffHeight
};

enum {								/* Enclosure types */
	ENCL_NONE=0,
	ENCL_BOX,
	ENCL_CIRCLE
};

enum {								/* GRArpeggio sub-subtypes */
	ARP=0,
	NONARP
};


/* ---------------------------------------------------------------------------- OTTAVA -- */

typedef struct {
	LINK		next;					/* index of next subobj */
	LINK		opSync;					/* link to Sync containing note/chord (not rest) */
} ANOTEOTTAVA, *PANOTEOTTAVA;

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	Byte		noCutoff;				/* True to suppress cutoff at right end of octave sign */	
	Byte		crossStaff;				/* (unused) True if the octave sign is cross-staff */
	Byte		crossSystem;			/* (unused) True if the octave sign is cross-system */	
	Byte		octSignType;			/* class of octave sign */
	SignedByte	filler;					/* unused */
	Boolean		numberVis,
				unused1,
				brackVis,
				unused2;
	DDIST		nxd, nyd;				/* DDIST position of number */
	DDIST		xdFirst, ydFirst,		/* DDIST position of bracket */
				xdLast, ydLast;
} OTTAVA, *POTTAVA;

enum {
	OTTAVA8va = 1,
	OTTAVA15ma,
	OTTAVA22ma,
	OTTAVA8vaBassa,
	OTTAVA15maBassa,
	OTTAVA22maBassa
};


/* ------------------------------------------------------------------------------ SLUR -- */

/* Types of Slursor behavior: */

enum { S_New=0, S_Start, S_C0, S_C1, S_End, S_Whole, S_Extend };

typedef struct dknt {
	DPoint knot;						/* Coordinates of knot relative to startPt */
	DPoint c0;							/* Coordinates of first control point relative to knot */
	DPoint c1;							/* Coordinates of second control pt relative to endpoint */
} SplineSeg;

typedef struct {
	LINK		next;				/* index of next subobj */
	Boolean		selected;			/* True if subobject is selected */
	Boolean		visible;			/* True if subobject is visible */
	Boolean		soft;				/* True if subobject is program-generated */
	Boolean		dashed;				/* True if slur should be shown as dashed line */
	Boolean		filler;
	Rect		bounds;				/* Bounding box of whole slur */
	SignedByte	firstInd,lastInd;	/* Starting, ending note indices in chord of tie */
	long		reserved;			/* For later expansion (e.g., to multi-segment slurs) */
	SplineSeg	seg;				/* For now, one slur spline segment always defined */
	Point		startPt, endPt;		/* Base points (note positions), paper-rel.; GetSlurContext returns Points */
	DPoint		endKnot;			/* End point of last spline segment, relative to endPt */
	
} ASLUR, *PASLUR;

typedef struct sSLUR{
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte	voice;				/* Voice number */
	char		filler;
	char		crossStaff;			/* True if the slur is cross-staff: staffn=top staff(?) */
	char		crossStfBack;		/* True if the slur goes from a lower (position, not no.) stf to higher */
	char		crossSystem;		/* True if the slur is cross-system */	
	Boolean		tempFlag;			/* temporary flag for benefit of functions that need it */
	Boolean 	used;				/* True if being used */
	Boolean		tie;				/* True if tie, else slur */
	LINK		firstSyncL;			/* Link to sync with 1st slurred note or to slur's system's init. measure */
	LINK		lastSyncL;			/* Link to sync with last slurred note or to slur's system */
} SLUR, *PSLUR;


/* -------------------------------------------------------------------- GRNOTE, GRSYNC -- */

typedef ANOTE AGRNOTE;				/* Same struct, though not all fields are used here */
typedef PANOTE PAGRNOTE;

typedef struct {
	OBJECTHEADER
} GRSYNC, *PGRSYNC;


/* ----------------------------------------------------------------------------- TEMPO -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte		subType;		/* beat: same units as note's l_dur */
	Boolean			expanded;
	Boolean			noMM;			/* False = play at _tempoMM_ BPM, True = ignore it */
	char			filler;
	Boolean			dotted;
	Boolean			hideMM;
	short			tempoMM;		/* new playback speed in beats per minute */	
	STRINGOFFSET	strOffset;		/* "tempo" string index return by String Manager */
	LINK			firstObjL;		/* object tempo depends on */
	STRINGOFFSET	metroStrOffset;	/* "metronome mark" index return by String Manager */
} TEMPO, *PTEMPO;


/* ---------------------------------------------------------------------------- SPACER -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte	bottomStaff;		/* Last staff on which space to be left */
	STDIST		spWidth;			/* Amount of blank space to leave */
} SPACER, *PSPACER;


/* -------------------------------------------------------------------------- SUPEROBJ -- */
/* This struct is the union of all Object structs, and its size is the maximum size of
any record kept in the object heap. */

typedef union uSUPEROBJ {	
		HEADER		header;
		TAIL		tail;
		SYNC		sync;
		RPTEND		rptEnd;
		PAGE		page;
		SYSTEM		system;
		STAFF		staff;
		MEASURE		measure;
		CLEF		clef;
		KEYSIG		keysig;
		TIMESIG		timesig;
		BEAMSET		beamset;
		CONNECT		connect;
		DYNAMIC		dynamic;
		GRAPHIC		graphic;
		OTTAVA		ottava;
		SLUR		slur;
		TUPLET		tuplet;
		GRSYNC		grSync;
		TEMPO		tempo;
		SPACER		spacer;
		} SUPEROBJ;

typedef struct {
	Byte bogus[sizeof(SUPEROBJ)];
} SUPEROBJECT, *PSUPEROBJECT;


/* --------------------------------------------------------------------------- CONTEXT -- */

typedef struct {
	Boolean		visible;			/* True if (staffVisible && measureVisible) */
	Boolean		staffVisible;		/* True if staff is visible */
	Boolean		measureVisible;		/* True if measure is visible */
	Boolean		inMeasure;			/* True if currently in measure */
	Rect		paper;				/* SHEET:	paper rect in window coords */ 
	short		sheetNum;			/* PAGE:		sheet number */
	short		systemNum;			/* SYSTEM:	number (unused) */
	DDIST		systemTop;			/* 			page relative top */
	DDIST		systemLeft;			/* 			page relative left edge */
	DDIST		systemBottom;		/* 			page relative bottom */
	DDIST		staffTop;			/* STAFF:	page relative top */
	DDIST		staffLeft;			/* 			page relative left edge */
	DDIST		staffRight;			/* 			page relative right edge */
	DDIST		staffHeight;		/* 			height */
	DDIST		staffHalfHeight;	/* 			height divided by 2 */
	SignedByte	staffLines;			/* 			number of lines */
	SignedByte	showLines;			/*			0=show no lines, 1=only middle line, or SHOW_ALL_LINES=show all */
	Boolean		showLedgers;		/*			True=show ledger lines for notes on this staff */
	short		fontSize;			/* 			preferred font size */
	DDIST		measureTop;			/* MEASURE:	page relative top */
	DDIST		measureLeft;		/* 			page relative left */
	SignedByte	clefType;			/* MISC:	current clef type */
	SignedByte	dynamicType;		/* 			dynamic marking */
	WHOLE_KSINFO					/*			key signature */
	SignedByte	timeSigType,		/* 			current time signature */
				numerator,
				denominator;
} CONTEXT, *PCONTEXT;


/* -------------------------------------------------------------------------- STFRANGE -- */

typedef struct {					/* Staff range for staffRect selection mode. */
	short		topStaff;
	short		bottomStaff;
} STFRANGE;


/* ------------------------------------------------------------------------- Clipboard -- */

enum {								/* Most recently copied objects */
	COPYTYPE_CONTENT=0,
	COPYTYPE_SYSTEM,
	COPYTYPE_PAGE
};

typedef struct {
	LINK srcL;
	LINK dstL;
} COPYMAP;


/* ----------------------------------------------------------------- Structs for Merge -- */

typedef struct {
	long		startTime;				/* Start time in this voice */
	short		firstStf,				/* First staff occupied by objs in this voice */
				lastStf;				/* Last staff occupied by objs in this voice */
	unsigned short singleStf,			/* Whether voice in this sys is on more than 1 stf */
				hasV,
				vOK,					/* True if there is enough space in voice to merge */
				vBad,					/* True if check of this voice caused abort */
				overlap,
				unused;
} VInfo;

typedef struct {
	long		startTime;				/* Start time in this voice */
	long		clipEndTime;			/* End time of clipboard in this voice */
	short		firstStf,				/* First staff occupied by objs in this voice */
				lastStf;				/* Last staff occupied by objs in this voice */
	short		singleStf,				/* Whether voice in this sys on more than 1 stf */
				hasV,
				vBad,
				unused;
} ClipVInfo;


/* ------------------------------------------------------------------------- CHORDNOTE -- */

typedef struct {
	SHORTQD		yqpit;
	Byte		noteNum;	
	LINK		noteL;
} CHORDNOTE;


/* ------------------------------------------------------------------ SYMDATA, OBJDATA -- */

typedef struct						/* Symbol table data for a CMN symbol: */
{
	short		cursorID;			/* Resource ID of cursor */
	SignedByte	objtype;			/* Object type for symbol's opcode */
	SignedByte	subtype;			/* subtype */
	char		symcode;			/* Input char. code for symbol (0=none) */
	SignedByte	durcode;			/* Duration code for notes and rests */
} SYMDATA;

typedef struct						/* Symbol table data for an object-list object: */
{
	SignedByte	objType;			/* mEvent type for symbol's opcode */
	short		justType;
	short		minEntries;
	short		maxEntries;
	Boolean		objRectOrdered;		/* True=objRect meaningful & its .left should be in order */
} OBJDATA;


/* ----------------------------------------------------------- JUSTTYPE, SPACETIMEINFO -- */

enum {
	J_IT=1,							/* Justification type Independent, Totally ordered */
	J_IP,							/* Justification type Independent, Partially ordered */
	J_D,							/* Justification type Dependent */
	J_STRUC							/* Structural object, no justification type */
};

typedef struct
{
	long		startTime;			/* Logical start time for object */
	SignedByte	justType;			/* Justification type */
	LINK		link;
	Boolean		isSync;
	long		dur;				/* Duration that controls space after (Gourlay) */
	FASTFLOAT	frac;				/* Fraction of controlling duration to use (Gourlay) */
} SPACETIMEINFO;

typedef struct {
	LINK		measL;
	long		lxd;
	DDIST		width;
} RMEASDATA;


/* --------------------------------------------------------- Reconstruction algorithms -- */
	
typedef struct {
	LINK startL;
	LINK endL;
} SELRANGE;

typedef struct {
	LINK objL;
	LINK subL;
	LINK newObjL;					/* New objects for updating cross links */
	LINK newSubL;
	LINK beamL;						/* Owning links */
	LINK octL;
	LINK tupletL;
	LINK slurFirstL;				/* slurL if sync is first sync */
	LINK slurLastL;					/* slurL if sync is last sync */
	LINK tieFirstL;					/* tieL if sync is first sync */
	LINK tieLastL;					/* tieL if sync is last sync */
	short mult;						/* The number of notes in a staff (voice) */
	long playDur;
	long pTime;
} PTIME;


/* ----------------------------------------------------------------------- SEARCHPARAM -- */
/*	PARAMETER BLOCK FOR Search functions. If you change it, be sure to change
InitSearchParam accordingly! */

typedef struct {
	short		id;					/* target staff number (for Staff, Measure, Sync, etc) */
									/*	or page number (for Page) */
	Boolean		needSelected;		/* True if we only want selected items */
	Boolean		needInMeasure;		/* True if we only want items inMeasure */
	Boolean		inSystem;			/* True if we want to stay within the System */
	Boolean		optimize;			/* True if use optimized search functions (LPageSearch, etc.) */
	short		subtype;			/* ANYSUBTYPE or code for subtype wanted */
	short		voice;				/* target voice number (for Sync, Beamset, etc.) */
	short		entryNum;			/* output: index of subitem found */
	LINK		pEntry;				/* output: link to subitem found */
} SearchParam;


/* --------------------------------------------------------------------------- UNDOREC -- */
/* struct and constants for use by Undo routines */

typedef struct {				
	Byte		lastCommand;			/* operation to be undone */
	Byte		subCode;				/* sub-operation */
	LINK		headL;					/* Head of Undo object list */
	LINK		tailL;					/* Tail of Undo object list */
	LINK		selStartL;				/* selStartL in Undo object list */
	LINK		selEndL;				/* selEndL in Undo object list */
	LINK		scorePrevL;				/* link before range in main object list */
	LINK		scoreEndL;				/* end of range in main object list */
	LINK		insertL;				/* Location in score of last operation */
	short		param1;					/* parameter for an operation */
	short		param2;					/* another parameter for an operation which requires 2 */
	Handle		undoRecord;				/* space for undo's private storage */
	char		menuItem[64];			/* menu command: C string */
	unsigned short redo : 1;			/* undo flag for menu: True if redo, False if undo */
	unsigned short hasUndo : 1;			/* True if Undo object list contains a System */
	unsigned short canUndo : 1;			/* True if operation is undoable */
	unsigned short unused : 13;			/* space for more flags */
	unsigned short expand[6];			/*	room to move */
} UNDOREC;

enum									/* Undo op codes */
{
	U_NoOp = 0,
	U_Insert,
	U_InsertClef,
	U_InsertKeySig,
	U_EditBeam,
	U_EditSlur,
	U_Cut,
	U_Paste,
	U_PasteSystem,
	U_Copy,
	U_Clear,
	U_Merge,
	U_GetInfo,
	U_Set,
	U_LeftEnd,
	U_GlobalText,
	U_MeasNum,
	U_AddSystem,
	U_AddPage,
	U_ChangeTight,
	U_Respace,
	U_Justify,
	U_BreakSystems,
	U_MoveMeasUp,
	U_MoveMeasDown,
	U_MoveSysUp,
	U_MoveSysDown,
	U_SetDuration,
	U_Transpose,
	U_DiatTransp,
	U_Respell,
	U_DelRedundAcc,
	U_Dynamics,
	U_AutoBeam,
	U_Beam,
	U_Unbeam,
	U_GRBeam,
	U_GRUnbeam,
	U_Tuple,
	U_Untuple,
	U_Ottava,
	U_UnOttava,
	U_AddMods,
	U_StripMods,
	U_MultiVoice,
	U_FlipSlurs,
	U_CompactVoice,
	U_Record,
	U_Quantize,
	U_TapBarlines,
	U_TranspKey,
	U_Reformat,
	U_Double,
	U_RecordMerge,
	U_ClearSystem,
	U_ClearPages,
	U_AddRedundAcc,
	U_FillEmptyMeas,
	U_AddCautionaryTS,
	U_EditText,
	U_PasteAsCue,
	U_NUM_OPS
};


/* ------------------------------------------------------------------------- MPCHANGED -- */
/*	Changed flags for exporting Master Page. */

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


/* --------------------------------------------------------------------- CharRectCache -- */

typedef struct {
	short		fontNum, fontSize;
	Rect		charRect[256];
} CharRectCache, *CharRectCachePtr;


/* ---------------------------------------------------------------------- FONTUSEDITEM -- */

typedef struct {
	Boolean			style[4];		/* styles used (bold and italic attribs. only) */
	unsigned char	fontName[32];	/* font name (Pascal string) */	
} FONTUSEDITEM;


/* ---------------------------------------------------------- MIDI-file-handling stuff -- */

/* Information on a MIDI file track */

typedef struct {
	Byte			*pChunk;		/* Contents of the track */
	Word			len;			/* Track length */
	Word			loc;			/* Current position in the track (offset into *pChunk) */
	DoubleWord		now;			/* Time at position <loc> in the track */
	Boolean			okay;			/* True=no problems parsing the track */
} TRACKINFO;


typedef struct MEASINFO {
	short			count;
	short			numerator;
	short			denominator;
} MEASINFO;

typedef struct TEMPOINFO {
	long			tStamp;			/* in units specified by <timeBase> in file header */
	unsigned long 	microsecPQ;
} TEMPOINFO;

typedef struct CTRLINFO {
	long			tStamp;			/* in units specified by <timeBase> in file header */
	Byte			channel;
	Byte 			ctrlNum;
	Byte 			ctrlVal;
	short			track;
} CTRLINFO;

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

#pragma options align=reset

#endif /* TypesIncluded */
