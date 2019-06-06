/* NMiscTypes.h (formerly applicationTypes.h) for Nightingale: declarations for user-
interface stuff and a few other things. */

/* An old comment here by MAS: "we want to /always/ use mac68k alignment." Why? I suspect
for compatibility of files containing bitfields, which we no longer use. Still, it
shouldn't cause any problems, so leave as is until there's a reason to change it. */ 
#pragma options align=mac68k

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
	unsigned short redo;				/* undo flag for menu: True if redo, False if undo */
	unsigned short hasUndo;				/* True if Undo object list contains a System */
	unsigned short canUndo;				/* True if operation is undoable */
	unsigned short unused;				/* space for more flags */
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


/*
 * The GridRec holds the information for the tool palette.
 */

typedef struct {
	Point			cell;
	unsigned char	ch;
	} GridRec;


/* ------------------------------------------------------------------------- UserPopUp -- */

typedef struct {
	Rect box, bounds, shadow, prompt;
	short currentChoice, menuID;
	MenuHandle menu;
	unsigned char str[40];		/* Pascal string */
} UserPopUp;


/* ----------------------------------------------------------------------- SEARCHPARAM -- */
/*	Parameter block for Search functions. If you change it, be sure to change
InitSearchParam accordingly! */

typedef struct {
	short		id;					/* target staff number (for Staff, Measure, Sync, etc.) */
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
	LINK slurFirstL;				/* slurL if Sync is first Sync */
	LINK slurLastL;					/* slurL if Sync is last Sync */
	LINK tieFirstL;					/* tieL if Sync is first Sync */
	LINK tieLastL;					/* tieL if Sync is last Sync */
	short mult;						/* The number of notes in a staff (voice) */
	long playDur;
	long pTime;
} PTIME;


/* --------------------------------------------------------------------- CharRectCache -- */

typedef struct {
	short		fontNum, fontSize;
	Rect		charRect[256];
} CharRectCache, *CharRectCachePtr;

#pragma options align=reset

