/*	NTypes.h - #defines and typedefs for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#ifndef TypesIncluded
#define TypesIncluded

#pragma options align=mac68k

/* ----------------------------- Data-structure Things ----------------------------- */

/* We assume already-included headers (on the Macintosh, Types.h) say:
	typedef unsigned char Byte;
	typedef signed char SignedByte;
*/

#define INT16 short				/* 2 bytes; no longer used */
#define STDIST short			/* range +-4096 staffLines, resolution 1/8 staffLine */
#define LONGSTDIST long			/* range +-268,435,456 staffLines, resolution 1/8 staffLine */
#define QDIST short				/* range +-8192 staffLines, resolution 1/4 staffLine */
#define SHORTSTD SignedByte		/* range +-16 staffLines, resolution 1/8 staffLine */
#define SHORTQD SignedByte		/* range +-32 staffLines, resolution 1/4 staffLine */
#define DDIST short				/* range +-2048 points (+-28 in), resolution 1/16 point */
#define SHORTDDIST SignedByte	/* range +-8 points, resolution 1/16 point */
#define LONGDDIST long			/* range +-134,217,728 points (+-155344 ft), resolution 1/16 point */
#define STD_LINEHT 8			/* STDIST scale: value for standard staff interline space */
								/* N.B. Before changing, consider roundoff in conversion macros. */
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
	DDIST	top,left,bottom,right;
} DRect;

typedef struct						/* Key signature item: */
{
	char letcode:7;					/* LG: Code for letter name: A=5,B=4,C=3,D=2,E=1,F=0,G=6  */
	Boolean sharp:1;				/* LG: Is it a sharp? (FALSE=flat) */
} KSITEM;

#define WHOLE_KSINFO				/* Complete key sig. w/o context. NB: w/TC5, 15 bytes! */	\
	KSITEM		KSItem[MAX_KSITEMS];	/* The sharps and flats */								\
	SignedByte	nKSItems;			/* No. of sharps and flats in key sig. */
	
typedef struct						/* NB: w/TC5, 16 bytes: KSINFO is larger than WHOLE_KSINFO! */
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
	FULLNAMES
};


/* Generic subObj functions FirstSubObj(), NextSubObj() in Objects.c are 
dependent on the order of items in the following enum. Arrays subObjLength[] 
and objLength[], in vars.h, MUST be updated if enum is changed. */

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


/* --------------------------------------------------------------------------------- */
/* Macros in MemMacros.h depend on the positions of the first five fields of the
object header, which must not be re-positioned. In the future, this may apply to
other fields as well. */

#define OBJECTHEADER \
	LINK		right, left;		/* links to left and right objects */					\
	LINK		firstSubObj;		/* link to first subObject */							\
	DDIST		xd, yd;				/* position of object */								\
	SignedByte	type;				/* (.+#10) object type */								\
	Boolean		selected:1;			/* TRUE if any part of object selected */				\
	Boolean		visible:1;			/* TRUE if any part of object is visible */				\
	Boolean		soft:1;				/* TRUE if object is program-generated */				\
	Boolean		valid:1;			/* TRUE if objRect (for Measures, measureBBox also) valid. */ \
	Boolean		tweaked:1;			/* TRUE if object dragged or position edited with Get Info */ \
	Boolean		spareFlag:1;		/* available for general use */							\
	char		ohdrFiller1:2;		/* unused; could use for specific "tweak" flags */		\
	Rect		objRect;			/* enclosing rectangle of object (paper-rel.pixels) */ 	\
	SignedByte	relSize;			/* (unused) size rel. to normal for object & context */	\
	SignedByte	ohdrFiller2;		/* unused */											\
	Byte		nEntries;			/* (.+#22) number of subobjects in object */
	
#define SUBOBJHEADER \
	LINK		next;				/* index of next subobj */								\
	SignedByte	staffn;				/* staff number. For cross-stf objs, top stf (Slur,Beamset) or 1st stf (Tuplet) */									\
	SignedByte	subType;			/* subobject subtype. N.B. Signed--see ANOTE. */		\
	Boolean		selected:1;			/* TRUE if subobject is selected */						\
	Boolean		visible:1;			/* TRUE if subobject is visible */						\
	Boolean		soft:1;				/* TRUE if subobject is program-generated */

#define EXTOBJHEADER \
	SignedByte	staffn;				/* staff number: for cross-staff objs, of top staff FIXME: except tuplets! */


/* ----------------------------------------------------------------------- MEVENT -- */

typedef struct sMEVENT {
	OBJECTHEADER
} MEVENT, *PMEVENT;


/* ----------------------------------------------------------------------- EXTEND -- */

typedef struct sEXTEND {
	OBJECTHEADER
	EXTOBJHEADER
} EXTEND, *PEXTEND;


/* -------------------------------------------------------------------- TEXTSTYLE -- */

typedef struct xTEXTSTYLE {
	unsigned char	fontName[32];	/* default font name: Pascal string */
	unsigned short	filler2:5;
	unsigned short	lyric:1;		/* TRUE means lyric spacing */
	unsigned short	enclosure:2;
	unsigned short relFSize:1;		/* TRUE if size is relative to staff size */ 
	unsigned short	fontSize:7;		/* if relFSize, small..large code, else point size */
	short			fontStyle;
} TEXTSTYLE, *PTEXTSTYLE;


/* -------------------------------------------------------------------- VOICEINFO -- */

typedef struct {
	Byte		partn;				/* No. of part using voice, or 0 if voice is unused */
	Byte		voiceRole:3;		/* upper, lower, single, or cross-stf voice */
	Byte		relVoice:5;			/* Voice no. within the part, >=1 */
} VOICEINFO;


/* ----------------------------------------------------------------- SCORE HEADER -- */

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

	/* The following are for FreeMIDI support. We use an fmsUniqueID to identify
		the output device while running. We use a destinationMatch to allow FreeMIDI
		to match the characteristics of a stored device with devices that are currently
		available to the system. (This is recommended by "FreeMIDI API.pdf.") E.g.,
		if we just save an fmsUniqueID, then this may be invalid when opening the
		file on another system or on the user's own reconfigured system. FreeMIDI
		can make a good guess about what to do by analyzing info about the device
		in the destinationMatch union, so we save that also. */
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
	char		feedback:1;			/* TRUE if we want feedback on note insert */				\
	char		dontSendPatches:1; /* 0 = when playing, send patch changes for channels */		\
	char		saved:1;			/* TRUE if score has been saved */							\
	char		named:1;			/* TRUE if file has been named */							\
	char 		used:1;				/* TRUE if score contains any nonstructural info */			\
	char		transposed:1;		/* TRUE if transposed score, else C score */				\
	char		lyricText:1;		/* (no longer used) TRUE if last text entered was lyric */	\
	char		polyTimbral:1;		/* TRUE for one part per MIDI channel */					\
	Byte		currentPage;		/* (no longer used) */										\
	short		spacePercent,		/* Percentage of normal horizontal spacing used */			\
				srastral,			/* Standard staff size rastral no. */						\
				altsrastral,		/* (unused) Alternate staff size rastral no. */				\
				tempo,				/* playback speed in beats per minute */					\
				channel,			/* Basic MIDI channel number */								\
				velocity;			/* global playback velocity offset */						\
	STRINGOFFSET headerStrOffset;	/* index returned by String Manager */						\
	STRINGOFFSET footerStrOffset;	/* index returned by String Manager */						\
	char		topPGN:1;			/* TRUE=page numbers at top of page, else bottom */			\
	char		hPosPGN:3;			/* 1=page numbers at left, 2=center, 3=at right */			\
	char		alternatePGN:1;		/* TRUE=page numbers alternately left and right */			\
	char		useHeaderFooter:1;	/* TRUE=use header/footer text, not simple pagenum */		\
	char		fillerPGN:2;		/* unused */												\
	SignedByte	fillerMB;			/* unused */												\
	DDIST		filler2,			/* unused */												\
				otherIndent;		/* Amount to indent Systems other than first */				\
	SignedByte	firstNames,			/* Code for drawing part names: see enum above */			\
				otherNames,			/* Code for drawing part names: see enum above */			\
				lastGlobalFont,		/* Header index of most recent text style used */			\
				xMNOffset,			/* Horiz. pos. offset for meas. nos. (half-spaces) */		\
				yMNOffset,			/* Vert. pos. offset for meas. nos. (half-spaces) */		\
				xSysMNOffset;		/* Horiz. pos. offset for meas.nos.if 1st meas.in system */ \
	short		aboveMN:1,			/* TRUE=measure numbers above staff, else below */			\
				sysFirstMN:1,		/* TRUE=indent 1st meas. of system by xMNOffset */			\
				startMNPrint1:1,	/* TRUE=First meas. number to print is 1, else 2 */			\
				firstMNNumber:13;	/* Number of first measure */								\
	LINK		masterHeadL,		/* Head of Master Page object list */						\
				masterTailL;		/* Tail of Master Page object list */						\
	SignedByte	filler1,																		\
				nFontRecords;		/* Always 15 for now */										\
	/* Fifteen identical TEXTSTYLE records. Fontnames are Pascal strings. */					\
																								\
	unsigned char fontNameMN[32];	/* MEASURE NO. FONT: default name, size and style */		\
	unsigned short	fillerMN:5;																	\
	unsigned short	lyricMN:1;		/* TRUE if spacing like lyric (ignored) */					\
	unsigned short	enclosureMN:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeMN:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizeMN:7;	/* if relFSizeMN, small..large code, else point size */		\
	short			fontStyleMN;																\
																								\
	unsigned char fontNamePN[32];	/* PART NAME FONT: default name, size and style */			\
	unsigned short	fillerPN:5;																	\
	unsigned short	lyricPN:1;		/* TRUE if spacing like lyric (ignored) */					\
	unsigned short	enclosurePN:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizePN:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizePN:7;	/* if relFSizePN, small..large code, else point size */		\
	short			fontStylePN;																\
																								\
	unsigned char fontNameRM[32];	/* REHEARSAL MARK FONT: default name, size and style */		\
	unsigned short	fillerRM:5;																	\
	unsigned short	lyricRM:1;		/* TRUE if spacing like lyric (ignored) */					\
	unsigned short	enclosureRM:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeRM:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizeRM:7;	/* if relFSizeRM, small..large code, else point size */		\
	short			fontStyleRM;																\
																								\
	unsigned char fontName1[32];	/* REGULAR FONT 1: default name, size and style */			\
	unsigned short	fillerR1:5;																	\
	unsigned short	lyric1:1;		/* TRUE if spacing like lyric	*/							\
	unsigned short	enclosure1:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize1:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize1:7;	/* if relFSize1, small..large code, else point size */		\
	short			fontStyle1;																	\
																								\
	unsigned char fontName2[32];	/* REGULAR FONT 2: default name, size and style */			\
	unsigned short	fillerR2:5;																	\
	unsigned short	lyric2:1;		/* TRUE if spacing like lyric	*/							\
	unsigned short	enclosure2:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize2:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize2:7;	/* if relFSize2, small..large code, else point size */		\
	short			fontStyle2;																	\
																								\
	unsigned char fontName3[32];	/* REGULAR FONT 3: default name, size and style */			\
	unsigned short	fillerR3:5;																	\
	unsigned short	lyric3:1;		/* TRUE if spacing like lyric	*/							\
	unsigned short	enclosure3:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize3:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize3:7;	/* if relFSize3, small..large code, else point size */		\
	short			fontStyle3;																	\
																								\
	unsigned char fontName4[32];	/* REGULAR FONT 4: default name, size and style */			\
	unsigned short	fillerR4:5;																	\
	unsigned short	lyric4:1;		/* TRUE if spacing like lyric	*/							\
	unsigned short	enclosure4:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize4:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize4:7;	/* if relFSizeR4, small..large code, else point size */		\
	short			fontStyle4;																	\
																								\
	unsigned char fontNameTM[32];	/* TEMPO MARK FONT: default name, size and style */			\
	unsigned short	fillerTM:5;																	\
	unsigned short	lyricTM:1;		/* TRUE if spacing like lyric (ignored) */					\
	unsigned short	enclosureTM:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeTM:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizeTM:7;	/* if relFSizeTM, small..large code, else point size */		\
	short			fontStyleTM;																\
																								\
	unsigned char fontNameCS[32];	/* CHORD SYMBOL FONT: default name, size and style */		\
	unsigned short	fillerCS:5;																	\
	unsigned short	lyricCS:1;		/* TRUE if spacing like lyric (ignored) */					\
	unsigned short	enclosureCS:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizeCS:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizeCS:7;	/* if relFSizeCS, small..large code, else point size */		\
	short			fontStyleCS;																\
																								\
	unsigned char fontNamePG[32];	/* PAGE HEADER/FOOTER/NO.FONT: default name, size and style */	\
	unsigned short	fillerPG:5;																	\
	unsigned short	lyricPG:1;		/* TRUE if spacing like lyric (ignored) */					\
	unsigned short	enclosurePG:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSizePG:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizePG:7;	/* if relFSizePG, small..large code, else point size */		\
	short			fontStylePG;																\
																								\
	unsigned char fontName5[32];	/* REGULAR FONT 5: default name, size and style */			\
	unsigned short	fillerR5:5;																	\
	unsigned short	lyric5:1;		/* TRUE if spacing like lyric (ignored) */					\
	unsigned short	enclosure5:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize5:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize5:7;	/* if relFSize5, small..large code, else point size */		\
	short			fontStyle5;																	\
																								\
	unsigned char fontName6[32];	/* REGULAR FONT 6: default name, size and style */			\
	unsigned short	fillerR6:5;																	\
	unsigned short	lyric6:1;		/* TRUE if spacing like lyric	*/							\
	unsigned short	enclosure6:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize6:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize6:7;	/* if relFSizeR6, small..large code, else point size */		\
	short			fontStyle6;																	\
																								\
	unsigned char fontName7[32];	/* REGULAR FONT 7: default name, size and style */			\
	unsigned short	fillerR7:5;																	\
	unsigned short	lyric7:1;		/* TRUE if spacing like lyric	*/							\
	unsigned short	enclosure7:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize7:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize7:7;	/* if relFSizeR7, small..large code, else point size */		\
	short			fontStyle7;																	\
																								\
	unsigned char fontName8[32];	/* REGULAR FONT 8: default name, size and style */			\
	unsigned short	fillerR8:5;																	\
	unsigned short	lyric8:1;		/* TRUE if spacing like lyric	*/							\
	unsigned short	enclosure8:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize8:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize8:7;	/* if relFSizeR8, small..large code, else point size */		\
	short			fontStyle8;																	\
																								\
	unsigned char fontName9[32];	/* REGULAR FONT 9: default name, size and style */			\
	unsigned short	fillerR9:5;																	\
	unsigned short	lyric9:1;		/* TRUE if spacing like lyric	*/							\
	unsigned short	enclosure9:2;	/* Enclosure: whether box, circular or none */				\
	unsigned short	relFSize9:1;	/* TRUE if size is relative to staff size */				\
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
					showSyncs,			/* Show (w/HiliteInsertNode) lines on every sync? */	\
					frameSystems,		/* Frame systemRects (for debugging)? */				\
					fillerEM:4,			/* unused */											\
					colorVoices:2,		/* 0=normal, 1=show non-dflt voices in color */			\
					showInvis:1,		/* Display invisible objects? */						\
					showDurProb:1,		/* Show measures with duration/time sig. problems? */	\
					recordFlats;		/* TRUE if black-key notes recorded should use flats */ \
																									\
	long			spaceMap[MAX_L_DUR];	/* Ideal spacing of basic (undotted, non-tuplet) durs. */ \
	DDIST			firstIndent,		/* Amount to indent first System */						\
					yBetweenSys;		/* obsolete, was vert. "dead" space btwn Systems */		\
	VOICEINFO	voiceTab[MAXVOICES+1];	/* Descriptions of voices in use */						\
	short			expansion[256-(MAXVOICES+1)];


typedef struct {
	NIGHTSCOREHEADER
} SCOREHEADER, *PSCOREHEADER;


/* ------------------------------------------------------------------------- TAIL -- */

typedef struct {
	OBJECTHEADER
} TAIL, *PTAIL;


/* ------------------------------------------------------------------------- PAGE -- */

typedef struct sPAGE {
	OBJECTHEADER
	LINK		lPage,				/* Links to left and right Pages */
				rPage;
	short		sheetNum;			/* Sheet number: indexed from 0 */
	StringPtr	headerStrOffset,	/* (unused; when used, should be STRINGOFFSETs) */
				footerStroffset;
} PAGE, *PPAGE;


/* ----------------------------------------------------------------------- SYSTEM -- */

typedef struct sSYSTEM {
	OBJECTHEADER
	LINK		lSystem,			/* Links to left and right Systems */
				rSystem;
	LINK		pageL;				/* Link to previous (enclosing) Page */
	short		systemNum;			/* System number: indexed from 1 */
	DRect		systemRect;			/* DRect enclosing entire system, rel to Page */
	Ptr			sysDescPtr;			/* (unused) ptr to data describing left edge of System */
} SYSTEM, *PSYSTEM;


/* ------------------------------------------------------------------------ STAFF -- */

#define SHOW_ALL_LINES	15

typedef struct {
	LINK		next;				/* index of next subobj */
	SignedByte	staffn;				/* staff number */
	Boolean		selected:1;			/* TRUE if subobject is selected */
	Boolean		visible:1;			/* TRUE if object is visible */
	Boolean		fillerStf:6;		/* unused */
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
	unsigned char filler:3,			/* unused */
				showLedgers:1,		/* TRUE if drawing ledger lines of notes on this staff (the default if showLines>0) */
				showLines:4;		/* 0=show 0 staff lines, 1=only middle line (of 5-line staff), 2-14 unused,
										15=show all lines (default) (use SHOW_ALL_LINES for this) */
#ifdef STAFFRASTRAL
// FIXME: maybe this should be drSize (DDIST) for the staff's current rastral?
	short			srastral;		/* rastral for this staff */
#endif
} ASTAFF, *PASTAFF;

typedef struct sSTAFF {
	OBJECTHEADER
	LINK			lStaff,				/* links to left and right Staffs */
					rStaff;
	LINK			systemL;			/* link to previous (enclosing) System */
} STAFF, *PSTAFF;


/* ---------------------------------------------------------------------- CONNECT -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	Boolean		selected:1;			/* TRUE if subobject is selected */
	Byte 		filler:1;
	Byte		connLevel:3;		/* Code from list below */
	Byte		connectType:2;		/* Code from list below */
	SignedByte	staffAbove;			/* upper staff no. (top of line or curly) (valid if connLevel!=0) */
	SignedByte	staffBelow;			/* lower staff no. (bottom of " ) (valid if connLevel!=0) */
	DDIST		xd;					/* DDIST position */
	LINK		firstPart;			/* (Unused) LINK to first part of group or connected part if not a group */
	LINK		lastPart;			/* (Unused) LINK to last part of group or NILINK if not a group */
} ACONNECT, *PACONNECT;

typedef struct {
	OBJECTHEADER
	LINK			connFiller;
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


/* ------------------------------------------------------------------ ACLEF, CLEF -- */

typedef struct {
	SUBOBJHEADER
	Byte		filler1:3;
	Byte		small:2;			/* TRUE to draw in small characters */
	Byte		filler2;
	DDIST		xd, yd;				/* DDIST position */
} ACLEF, *PACLEF;

typedef struct {
	OBJECTHEADER
	Boolean	inMeasure:1;			/* TRUE if object is in a Measure, FALSE if not */
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


/* ----------------------------------------------------------------------- KEYSIG -- */

typedef struct {
	SUBOBJHEADER					/* subType=no. of naturals, if nKSItems==0 */
	Byte			nonstandard:1;	/* TRUE if not a standard CMN key sig. */
	Byte			filler1:2;
	Byte			small:2;		/* (unused so far) TRUE to draw in small characters */
	SignedByte		filler2;
	DDIST			xd;				/* DDIST horizontal position */
	WHOLE_KSINFO
} AKEYSIG, *PAKEYSIG;

typedef struct {
	OBJECTHEADER
	Boolean	inMeasure:1;			/* TRUE if object is in a Measure, FALSE if not */
} KEYSIG, *PKEYSIG;


/* --------------------------------------------------------------------- TIMESIG -- */

typedef struct {
	SUBOBJHEADER
	Byte		filler:3;			/* Unused--put simple/compound/other:2 here? */
	Byte		small:2;			/* (unused so far) TRUE to draw in small characters */
	SignedByte	connStaff;			/* (unused so far) bottom staff no. */
	DDIST		xd, yd;				/* DDIST position */
	SignedByte	numerator;			/* numerator */
	SignedByte	denominator;		/* denominator */
} ATIMESIG, *PATIMESIG;

typedef struct {
	OBJECTHEADER
	Boolean		inMeasure:1;		/* TRUE if object is in a Measure, FALSE if not */
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


/* ---------------------------------------------------------------------- MEASURE -- */

typedef struct {
	SUBOBJHEADER					/* subType=barline type (see enum below) */
	Boolean		measureVisible:1;	/* TRUE if measure contents are visible */
	Boolean		connAbove:1;		/* TRUE if connected to barline above */
	char		filler1:3;
	SignedByte	filler2;
	short		oldFakeMeas:1,		/* OBSOLETE: now at the object level, so this is to be removed */
				measureNum:15;		/* internal measure number; first is always 0 */
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
	short			fakeMeas:1,		/* TRUE=not really a measure (i.e., barline ending system) */
					spacePercent:15;	/* Percentage of normal horizontal spacing used */
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


/* -------------------------------------------------------------------- PSEUDOMEAS -- */
/* Pseudomeasures are symbols that have similar graphic appearance to barlines
but have no semantics: dotted barlines and double bars that don't coincide with
"real" barlines. */

typedef struct {
	SUBOBJHEADER					/* subType=barline type (see enum below) */
	Boolean		connAbove:1;		/* TRUE if connected to barline above */
	char		filler1:4;			/* (unused) */
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


/* -------------------------------------------------------------------- NOTE, SYNC -- */
/* A "note" is a normal- or cue-sized note or rest, but not a grace note. */

typedef struct {
	SUBOBJHEADER					/* subType (l_dur): LG: <0=n measure rest, 0=unknown, >0=Logical (CMN) dur. code */
	Boolean		inChord:1;			/* TRUE if note is part of a chord */
	Boolean		rest:1;				/* LGP: TRUE=rest (=> ignore accident, ystem, etc.) */
	Boolean		unpitched:1;		/* LGP: TRUE=unpitched note */
	Boolean		beamed:1;			/* TRUE if beamed */
	Boolean		otherStemSide:1;	/* G: TRUE if note goes on "wrong" side of stem */
	SHORTQD		yqpit;				/* LG: clef-independent dist. below middle C ("pitch") (unused for rests) */
	DDIST		xd, yd;				/* G: head position */
	DDIST		ystem;				/* G: endpoint of stem (unused for rests) */
	short		playTimeDelta;		/* P: PDURticks before/after timeStamp when note starts */
	short		playDur;			/* P: PDURticks that note plays for */
	short		pTime;				/* P: PDURticks play time; for internal use by Tuplet routines */
	Byte		noteNum;			/* P: MIDI note number (unused for rests) */
	Byte		onVelocity;			/* P: MIDI note-on velocity, normally loudness (unused for rests) */
	Byte		offVelocity;		/* P: MIDI note-off (release) velocity (unused for rests) */
	Boolean		tiedL:1;			/* TRUE if tied to left */
	Boolean		tiedR:1;			/* TRUE if tied to right */
	Byte		ymovedots:2;		/* G: Y-offset on aug. dot pos. (halflines, 2=same as note, except 0=invisible) */
	Byte		ndots:4;			/* LG: No. of aug. dots */
	SignedByte	voice;				/* L: Voice number */
	Byte		rspIgnore:1;		/* TRUE if note's chord should not affect automatic spacing (unused as of v. 5.5) */
	Byte		accident:3;			/* LG: 0=none, 1--5=dbl. flat--dbl. sharp (unused for rests) */
	Boolean		accSoft : 1;		/* L: Was accidental generated by Nightingale? */
	Byte		micropitch:3;		/* LP: Microtonal pitch modifier (unused as of v. 5.5) */
	Byte		xmoveAcc:5;			/* G: X-offset to left on accidental position */
	Byte		merged:1;			/* temporary flag for Merge functions */
	Byte		courtesyAcc:1;		/* G: Accidental is a "courtesy accidental" */
	Byte		doubleDur:1;		/* G: Draw as if double the actual duration */
	Byte		headShape:5;		/* G: Special notehead or rest shape; see list below */
	Byte		xmovedots:3;		/* G: X-offset on aug. dot position */
	LINK		firstMod;			/* LG: Note-related symbols (articulation, fingering, etc.) */
	Byte		slurredL:2;			/* TRUE if endpoint of slur to left (extra bit for future use) */
	Byte		slurredR:2;			/* TRUE if endpoint of slur to right (extra bit for future use) */
	Byte		inTuplet:1;			/* TRUE if in a tuplet */
	Byte		inOttava:1;			/* TRUE if in an octave sign */
	Byte		small:1;			/* TRUE if a small (cue, etc.) note */
	Byte		tempFlag:1;			/* temporary flag for benefit of functions that need it */
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


/* ---------------------------------------------------------------------- BEAMSET -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	LINK		bpSync;				/* link to Sync containing note/chord */
	SignedByte	startend;			/* No. of beams to start/end (+/-) on note/chord */
	Byte		fracs:3;			/* No. of fractional beams on note/chord */ 
	Byte		fracGoLeft:1;		/* Fractional beams point left? */
	Byte		filler:4;			/* unused */
} ANOTEBEAM, *PANOTEBEAM;

typedef struct sBeam {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte	voice;				/* Voice number */
	Byte		thin:1;				/* TRUE=narrow lines, FALSE=normal width */
	Byte		beamRests:1;		/* TRUE if beam can contain rests */
	Byte		feather:2;			/* (unused) 0=normal,1=feather L end (accel.), 2=feather R (decel.) */
	Byte		grace:1;			/* TRUE if beam consists of grace notes */
	Byte		firstSystem:1;		/* TRUE if on first system of cross-system beam */	
	Byte		crossStaff:1;		/* TRUE if the beam is cross-staff: staffn=top staff */
	Byte		crossSystem:1;		/* TRUE if the beam is cross-system */
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


/* ----------------------------------------------------------------------- TUPLET -- */

/* This struct is used to get information from TupletDialog. */

typedef struct {
	Byte			accNum;			/* Accessory numeral (numerator) for Tuplet */
	Byte			accDenom;		/* Accessory denominator */
	short			durUnit;		/* Duration units of denominator */
	Boolean			numVis:1,
					denomVis:1,
					brackVis:1,
					isFancy:1;
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
	Byte			numVis:1,
					denomVis:1,
					brackVis:1,
					small:2,		/* (unused so far) TRUE to draw in small characters */
					filler:3;
	DDIST			acnxd, acnyd;	/* DDIST position of accNum (now unused) */
	DDIST			xdFirst, ydFirst,	/* DDIST position of bracket */
					xdLast, ydLast;
} TUPLET, *PTUPLET;


/* -------------------------------------------------------------------- REPEATEND -- */

typedef struct {
	SUBOBJHEADER					/* subType is in object so unused here */
	Byte			connAbove:1;	/* TRUE if connected above */
	Byte			filler:4;		/* (unused) */
	SignedByte		connStaff;		/* staff to connect to; valid if connAbove TRUE */
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


/* ----------------------------------------------------------------------- ENDING -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	LINK		firstObjL;			/* Object left end of ending is attached to */
	LINK		lastObjL;			/* Object right end of ending is attached to or NILINK */
	Byte		noLCutoff:1;		/* TRUE to suppress cutoff at left end of Ending */	
	Byte		noRCutoff:1;		/* TRUE to suppress cutoff at right end of Ending */	
	Byte		endNum:6;			/* 0=no ending number or label, else code for the ending label */
	DDIST		endxd;				/* Position offset from lastObjL */
} ENDING, *PENDING;


/* ------------------------------------------------------------ ADYNAMIC, DYNAMIC -- */

typedef struct {
	SUBOBJHEADER					/* subType is unused */
	Byte			mouthWidth:5;	/* Width of mouth for hairpin */
	Byte			small:2;		/* TRUE to draw in small characters */
	Byte			otherWidth:6;	/* Width of other (non-mouth) end for hairpin */
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


/* ----------------------------------------------------------------------- AMODNR -- */

typedef struct {
	LINK		next;					/* index of next subobj */
	Boolean		selected:1;				/* TRUE if subobject is selected */
	Boolean		visible:1;				/* TRUE if subobject is visible */
	Boolean		soft:1;					/* TRUE if subobject is program-generated */
	unsigned char xstd:5;				/* FIXME: use "Byte"? Note-relative position (really signed STDIST: see below) */
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


/* ---------------------------------------------------------------------- GRAPHIC -- */

typedef struct {
	LINK next;
	STRINGOFFSET strOffset;			/* index return by String Manager library. */
} AGRAPHIC, *PAGRAPHIC;

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER					/* N.B. staff number can be 0 here */
	SignedByte	graphicType;		/* graphic class (subtype) */
	SignedByte	voice;				/* Voice number (only with some types of relObjs) */
	Byte		enclosure:2;		/* Enclosure type; see list below */
	Byte		justify:3;			/* (unused) justify left/center/right */
	Boolean		vConstrain:1;		/* (unused) TRUE if object is vertically constrained */
	Boolean		hConstrain:1;		/* (unused) TRUE if object is horizontally constrained */
	Byte		multiLine:1;		/* TRUE if string contains multiple lines of text (delimited by CR) */
	short		info;				/* PICT res. ID (GRPICT); char (GRChar); length (GRArpeggio); */
									/*   ref. to text style (FONT_R1, etc) (GRString,GRLyric); */
									/*	  2nd x (GRDraw); draw extension parens (GRChordSym) */
	union {
		Handle		handle;			/* handle to resource, or NULL */
		short		thickness;
	} gu;
	SignedByte	fontInd;			/* index into font name table (GRChar,GRString only) */
	Byte		relFSize:1;			/* TRUE if size is relative to staff size (GRChar,GRString only) */ 
	Byte		fontSize:7;			/* if relSize, small..large code, else point size (GRChar,GRString only) */
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
	GRMIDISustainOn,
	GRMIDISustainOff,
	GRLastType=GRMIDISustainOff
};

/* Our internal codes for sustain on/off. FIXME: Should these be here or in a header file
 (MIDIGeneral.h)? */

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


/* ----------------------------------------------------------------------- OTTAVA -- */

typedef struct {
	LINK		next;						/* index of next subobj */
	LINK		opSync;					/* link to Sync containing note/chord (not rest) */
} ANOTEOTTAVA, *PANOTEOTTAVA;

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	Byte		noCutoff:1;				/* TRUE to suppress cutoff at right end of octave sign */	
	Byte		crossStaff:1;			/* (unused) TRUE if the octave sign is cross-staff */
	Byte		crossSystem:1;			/* (unused) TRUE if the octave sign is cross-system */	
	Byte		octSignType:5;			/* class of octave sign */
	SignedByte	filler;					/* unused */
	Boolean		numberVis:1,
				unused1:1,
				brackVis:1,
				unused2:5;
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


/* ------------------------------------------------------------------------- SLUR -- */

/* Types of Slursor behavior: */

enum { S_New=0, S_Start, S_C0, S_C1, S_End, S_Whole, S_Extend };

typedef struct dknt {
	DPoint knot;						/* Coordinates of knot relative to startPt */
	DPoint c0;							/* Coordinates of first control point relative to knot */
	DPoint c1;							/* Coordinates of second control pt relative to endpoint */
} SplineSeg;

typedef struct {
	LINK		next;				/* index of next subobj */
	Boolean		selected:1;			/* TRUE if subobject is selected */
	Boolean		visible:1;			/* TRUE if subobject is visible */
	Boolean		soft:1;				/* TRUE if subobject is program-generated */
	Boolean		dashed:2;			/* TRUE if slur should be shown as dashed line */
	Boolean		filler:3;
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
	char		filler:2;
	char		crossStaff:1;		/* TRUE if the slur is cross-staff: staffn=top staff(?) */
	char		crossStfBack:1;		/* TRUE if the slur goes from a lower (position, not no.) stf to higher */
	char		crossSystem:1;		/* TRUE if the slur is cross-system */	
	Boolean		tempFlag:1;			/* temporary flag for benefit of functions that need it */
	Boolean 	used:1;				/* TRUE if being used */
	Boolean		tie:1;				/* TRUE if tie, else slur */
	LINK		firstSyncL;			/* Link to sync with 1st slurred note or to slur's system's init. measure */
	LINK		lastSyncL;			/* Link to sync with last slurred note or to slur's system */
} SLUR, *PSLUR;


/* --------------------------------------------------------------- GRNOTE, GRSYNC -- */

typedef ANOTE AGRNOTE;				/* Same struct, though not all fields are used here */
typedef PANOTE PAGRNOTE;

typedef struct {
	OBJECTHEADER
} GRSYNC, *PGRSYNC;


/* ------------------------------------------------------------------------ TEMPO -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte		subType;		/* beat: same units as note's l_dur */
	Boolean			expanded:1;
	Boolean			noMM:1;			/* FALSE = play at _tempoMM_ BPM, TRUE = ignore it */
	char			filler:4;
	Boolean			dotted:1;
	Boolean			hideMM:1;
	short			tempoMM;		/* new playback speed in beats per minute */	
	STRINGOFFSET	strOffset;		/* "tempo" string index return by String Manager */
	LINK			firstObjL;		/* object tempo depends on */
	STRINGOFFSET	metroStrOffset;	/* "metronome mark" index return by String Manager */
} TEMPO, *PTEMPO;


/* ----------------------------------------------------------------------- SPACER -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte	bottomStaff;		/* Last staff on which space to be left */
	STDIST		spWidth;			/* Amount of blank space to leave */
} SPACER, *PSPACER;


/* --------------------------------------------------------------------- SUPEROBJ -- */
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


/* ---------------------------------------------------------------------- CONTEXT -- */

typedef struct {
	Boolean		visible:1;			/* TRUE if (staffVisible && measureVisible) */
	Boolean		staffVisible:1;		/* TRUE if staff is visible */
	Boolean		measureVisible:1;	/* TRUE if measure is visible */
	Boolean		inMeasure:1;		/* TRUE if currently in measure */
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
	Boolean		showLedgers;		/*			TRUE=show ledger lines for notes on this staff */
#ifdef STAFFRASTRAL
	short		srastral;			/*			rastral for this staff */
#endif
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


/* ---------------------------------------------------------------------- STFRANGE -- */

typedef struct {					/* Staff range for staffRect selection mode. */
	short		topStaff;
	short		bottomStaff;
} STFRANGE;


/* --------------------------------------------------------------------- Clipboard -- */

enum {								/* Most recently copied objects */
	COPYTYPE_CONTENT=0,
	COPYTYPE_SYSTEM,
	COPYTYPE_PAGE
};

typedef struct {
	LINK srcL;
	LINK dstL;
} COPYMAP;


/* ------------------------------------------------------------ Structs for Merge -- */

typedef struct {
	long		startTime;				/* Start time in this voice */
	short		firstStf,				/* First staff occupied by objs in this voice */
				lastStf;				/* Last staff occupied by objs in this voice */
	unsigned short singleStf:1,			/* Whether voice in this sys is on more than 1 stf */
				hasV:1,
				vOK:1,					/* TRUE if there is enough space in voice to merge */
				vBad:1,					/* TRUE if check of this voice caused abort */
				overlap:1,
				unused:11;
} VInfo;

typedef struct {
	long		startTime;				/* Start time in this voice */
	long		clipEndTime;			/* End time of clipboard in this voice */
	short		firstStf,				/* First staff occupied by objs in this voice */
				lastStf;				/* Last staff occupied by objs in this voice */
	short		singleStf:1,			/* Whether voice in this sys on more than 1 stf */
				hasV:1,
				vBad:1,
				unused:13;
} ClipVInfo;


/* -------------------------------------------------------------------- CHORDNOTE -- */

typedef struct {
	SHORTQD		yqpit;
	Byte		noteNum;	
	LINK		noteL;
} CHORDNOTE;


/* ------------------------------------------------------------- SYMDATA, OBJDATA -- */

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
	Boolean		objRectOrdered;		/* TRUE=objRect meaningful & its .left should be in order */
} OBJDATA;


/* ------------------------------------------------------ JUSTTYPE, SPACETIMEINFO -- */

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


/* ---------------------------------------------------- Reconstruction algorithms -- */
	
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


/* ------------------------------------------------------------------ SEARCHPARAM -- */
/*	PARAMETER BLOCK FOR Search functions. If you change it, be sure to change
InitSearchParam accordingly! */

typedef struct {
	short		id;					/* target staff number (for Staff, Measure, Sync, etc) */
									/*	or page number (for Page) */
	Boolean		needSelected;		/* TRUE if we only want selected items */
	Boolean		needInMeasure;		/* TRUE if we only want items inMeasure */
	Boolean		inSystem;			/* TRUE if we want to stay within the System */
	Boolean		optimize;			/* TRUE if use optimized search functions (LPageSearch, etc.) */
	short		subtype;			/* ANYSUBTYPE or code for subtype wanted */
	short		voice;				/* target voice number (for Sync, Beamset, etc.) */
	short		entryNum;			/* output: index of subitem found */
	LINK		pEntry;				/* output: link to subitem found */
} SearchParam;


/* ---------------------------------------------------------------------- UNDOREC -- */
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
	unsigned short redo : 1;			/* undo flag for menu: TRUE if redo, FALSE if undo */
	unsigned short hasUndo : 1;			/* TRUE if Undo object list contains a System */
	unsigned short canUndo : 1;			/* TRUE if operation is undoable */
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
	U_NumOps
};


/* -------------------------------------------------------------------- MPCHANGED -- */
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


/* ---------------------------------------------------------------- CHANGE_RECORD -- */
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
	short		staff;				/*	Starting staff no. */
	short		p1;					/* Operation-dependent */
	short		p2;					/* Operation-dependent */
	short		p3;					/* Operation-dependent */
} CHANGE_RECORD;


/* ---------------------------------------------------------------- CharRectCache -- */

typedef struct {
	short		fontNum, fontSize;
	Rect		charRect[256];
} CharRectCache, *CharRectCachePtr;


/* ----------------------------------------------------------------- FONTUSEDITEM -- */

typedef struct {
	Boolean			style[4];		/* styles used (bold and italic attribs. only) */
	unsigned char	fontName[32];	/* font name (Pascal string) */	
} FONTUSEDITEM;


/* ----------------------------------------------------- MIDI-file-handling stuff -- */

/* Information on a MIDI file track */

typedef struct {
	Byte			*pChunk;		/* Contents of the track */
	Word			len;			/* Track length */
	Word			loc;			/* Current position in the track (offset into *pChunk) */
	DoubleWord		now;			/* Time at position <loc> in the track */
	Boolean			okay;			/* TRUE=no problems parsing the track */
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

/* ----------------------------------------------------- Document printing stuff -- */

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
