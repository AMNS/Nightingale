/*	NTypes.h - #defines and typedefs for Nightingale's basic datatypes */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
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


/* ------------------------------------------------------------------------- TEXTSTYLE -- */

typedef struct xTEXTSTYLE {
	unsigned char	fontName[32];	/* default font name: Pascal string */
	unsigned short	filler2:5;
	unsigned short	lyric:1;		/* True means lyric spacing */
	unsigned short	enclosure:2;
	unsigned short relFSize:1;		/* True if size is relative to staff size */ 
	unsigned short	fontSize:7;		/* if relFSize, small..large code, else point size */
	short			fontStyle;
} TEXTSTYLE, *PTEXTSTYLE;


/* ------------------------------------------------------------------------- VOICEINFO -- */

typedef struct {
	Byte		partn;				/* No. of part using voice, or 0 if voice is unused */
	Byte		voiceRole:3;		/* upper, lower, single, or cross-stf voice */
	Byte		relVoice:5;			/* Voice no. within the part, >=1 */
} VOICEINFO;


/* --------------------------------------------------------------------------- PARTINFO -- */

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
