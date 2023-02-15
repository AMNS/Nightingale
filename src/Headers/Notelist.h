/* Notelist.h for Nightingale
Definitions and prototypes for routines that read and convert files from
Nightingale's Notelist format to its native format. Written by John Gibson. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#define DOAUTOBEAM_NL				1


/* --------------------------------------------------------------------------------- */
/* Typedefs for creating intermediate Notelist data structure */

/* The intermediate data structure is just an array of NL_NODE unions. Each node is
20 bytes. Notes use all 20; other kinds of node (e.g., time sig.) use fewer. */

enum {								/* used in NLOBJHEADER's objType field */
	HEAD_TYPE=0,
	NR_TYPE,
	GRACE_TYPE,
	TUPLET_TYPE,
	BARLINE_TYPE,
	CLEF_TYPE,
	KEYSIG_TYPE,
	TIMESIG_TYPE,
	TEMPO_TYPE,
	GRAPHIC_TYPE,
	DYNAMIC_TYPE,
	LAST_TYPE
};

typedef unsigned short NLINK;

#define NLOBJHEADER					/* 8 bytes: */											\
	long		lStartTime;			/* positive 32-bit integer */							\
	char		objType;			/* see enum above */									\
	char		uVoice;				/* part-relative ("user") voice: 1-31, or -2 (NOONE) */	\
	char		part;				/* 1-64, or -2 (NOONE) */								\
	char		staff;				/* 1-64, or -2 (NOONE) */

typedef struct {
	NLOBJHEADER
	char		data[];				/* (additional bytes in union used by NL_NRGR) */
} NL_GENERIC, *PNL_GENERIC;


typedef struct {
	NLOBJHEADER
	NLINK		scoreName;			/* index into Notelist string pool, or NILINK */
									/* NB: Finder allows 31 + pascal length byte for filenames. */
	char		data[];				/* (additional bytes in union used by NL_NRGR) */
} NL_HEAD, *PNL_HEAD;


/* The NL_NRGR struct represents either a normal Note, a Rest or a GRace note, depending
on the value of <objType> (either NR_TYPE or GRACE_TYPE) and the <isRest> field.

	Note the differences in interpretation of grace note and normal note fields:
	- <durCode> for grace notes ha a range of only quarter thru 32nd
	- <pDur> is unimplemented for grace notes in Ngale
	- grace notes cannot be tied, slurred or tupleted
	- grace notes cannot have note modifiers
	
	Note also the differences in interpretation of rest and note fields:
	- rests have neither <acc> nor <eAcc> fields
	- rests have no <pDur> field
	- rests have no MIDI note number field
	- rests have no MIDI velocity field
	- rests can have negative durCodes (representing whole and multi-measure rests),
		but they cannot be of unknown duration (durCode=0)
	- rests never have their mainNote flag set
*/

typedef struct {
	NLOBJHEADER
	unsigned short	isRest;			/* "note" is really a rest */
	char			durCode;		/* code for logical duration: 0-9 (-127 - 9 for rests!) */
	unsigned char	nDots;			/* number of augmentation dots: 0-5 */
	char			noteNum;		/* MIDI note number: 0-127 */
	unsigned short	acc;			/* code of displayed accidental: 0-5 */
	unsigned short	eAcc;			/* code of effective accidental: 0-5 */
 	short			pDur;			/* physical duration (480 units/quarter): 1-32000 */
	char			onVel;			/* MIDI note on velocity: 0-127 */
	unsigned short	inChord;		/* note is in a chord */
	unsigned short	mainNote;		/* main note (carrying the stem) in a chord */
	unsigned short	tiedL;			/* tied to the preceding note */
	unsigned short	tiedR;			/* tied to the following note */
	unsigned short	filler1;
	unsigned short	slurredL;		/* slurred to the preceding note */
	unsigned short	slurredR;		/* slurred to the following note */
	unsigned short	inTuplet;		/* note/rest is a member of a tuplet group (irrelevant for grace notes) */
	unsigned char	filler2;
	unsigned char	appear;			/* appearance code: 0-10 (see NObjTypes.h) */
	NLINK			firstMod;		/* index into gHModList of 1st note mod, or 0 if no mods */
	Byte			nhSeg[6];
} NL_NRGR, *PNL_NRGR;


/* Note the interpretation of NLOBJHEADER fields for tuplets:
	- <lStartTime> is not assigned by Notelist.
	- <staff> is not specified in the Notelist: a tuplet applies to a part-relative
	  voice, so Notelist encodes only <uVoice> and <part>. However, since FICreateTUPLET
	  requires a staff number, we fill in the tuplet's staff in AnalyzeNLTuplet. */
	  
typedef struct {
	NLOBJHEADER
	short	num;					/* accessory numeral (numerator) for tuplet: ??range? */
	short	denom;					/* accessory denominator: ??range? */
	short	nInTuple;				/* number of syncs in tuplet, computed by AnalyzeNLTuplet */
	Boolean	numVis;
	Boolean	denomVis;
	Boolean	brackVis;
	char	data[];					/* (additional bytes in union used by NL_NRGR) */
} NL_TUPLET, *PNL_TUPLET;


/* Note the interpretation of NLOBJHEADER fields for barlines:
	- <uVoice>, <staff> and <part> are irrelevant (currently): a barline is assumed to
	  span the entire system. */
	  
typedef struct {
	NLOBJHEADER
	char		appear;				/* appearance code: 1-BAR_LAST (see NObjTypes.h) */
	char		filler;
	char		data[];				/* (additional bytes in union used by NL_NRGR) */
} NL_BARLINE, *PNL_BARLINE;


/* Note the interpretation of NLOBJHEADER fields for clefs:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice> and <part> are irrelevant; use <staff> only. */
	
typedef struct {
	NLOBJHEADER
	char		type;				/* clef type code: 1-HIGH_CLEF (see NObjTypes.h) */
	char		filler;
	char		data[];				/* (additional bytes in union used by NL_NRGR) */
} NL_CLEF, *PNL_CLEF;


/* Note the interpretation of NLOBJHEADER fields for keysigs:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice> and <part> are irrelevant; use <staff> only. */
	
typedef struct {
	NLOBJHEADER
	char	numAcc;					/* number of accidentals in keysig: 0-7 */
	char	filler;
	Boolean	sharp;					/* True if sharps in keysig, False if flats */
	char	data[];					/* (additional bytes in union used by NL_NRGR) */
} NL_KEYSIG, *PNL_KEYSIG;


/* Note the interpretation of NLOBJHEADER fields for timesigs:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice> and <part> are irrelevant; use <staff> only. */
	
typedef struct {
	NLOBJHEADER
	char	num;					/* numerator: 1-99 */
	char	denom;					/* denominator: powers of 2 from 1 to 64 */
	char	appear;					/* 1-4 (normal, 'C', cut time, numerator only -- see NObjTypes.h) */
	char	filler;
	char	data[];					/* (additional bytes in union used by NL_NRGR) */
} NL_TIMESIG, *PNL_TIMESIG;


/* Note the interpretation of NLOBJHEADER fields for tempo/metronome marks:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice> and <part> are irrelevant; use <staff> only. */
	
typedef struct {
	NLOBJHEADER
	NLINK	string;					/* tempo string: index into Notelist string pool, or NILINK */
	NLINK	metroStr;				/* bpm string: index into Notelist string pool, or NILINK */
	Boolean	dotted;					/* is metronome note-value dotted? */
	Boolean	hideMM;					/* not encoded in Notelist; set in ParseTempoMark */
	Boolean	noMM;					/* not encoded in Notelist; set in ParseTempoMark */
	char	durCode;				/* for metronome note-value */
	char	data[];					/* (additional bytes in union used by NL_NRGR) */
} NL_TEMPO, *PNL_TEMPO;


/* Note the interpretation of NLOBJHEADER fields for graphics:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice>, <part> and <staff> will be -2 if graphic is page-relative. Note that
		if a graphic is attached to a clef, timesig or keysig, then its staff will be
		the same as that of the clef, timesig or keysig; but its user (and internal)
		voice number, as well as its part number, will be -2. */
		
typedef struct {
	NLOBJHEADER
	NLINK	string;					/* graphic string: index into Notelist string pool, or NILINK */
	char	type;					/* currently only GRString or GRLyric */
	char	style;					/* style: TSRegular1STYLE to TSRegular5STYLE, or TSNoSTYLE */
	char	data[];					/* (additional bytes in union used by NL_NRGR) */
} NL_GRAPHIC, *PNL_GRAPHIC;


/* Note the interpretation of NLOBJHEADER fields for dynamics:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice> and <part> are irrelevant; use <staff> only. */
	
typedef struct {
	NLOBJHEADER
	char	type;					/* code for dynamic type (see enum in NObjTypes.h) : 1-23 */
	char	filler;
	char	data[];					/* (additional bytes in union used by NL_NRGR) */
} NL_DYNAMIC, *PNL_DYNAMIC;


typedef union {
	NL_GENERIC	generic;
	NL_HEAD		head;				/* only gNodeList[0] will be of HEAD_TYPE */
	NL_NRGR		nrgr;
	NL_TUPLET	tuplet;
	NL_BARLINE	barline;
	NL_CLEF		clef;
	NL_KEYSIG	keysig;
	NL_TIMESIG	timesig;
	NL_TEMPO	tempo;
	NL_GRAPHIC	graphic;
	NL_DYNAMIC	dynamic;
} NL_NODE, *PNL_NODE;


typedef struct {
	NLINK	next;					/* link to next mod attached to owning note, or zero if none */
	char	code;					/* note-modifier code: 1-MOD_LONG_INVMORDENT (see NObjTypes.h) */
	char	data;					/* [currently unused]: -127 - 128 */
} NL_MOD, *PNL_MOD, **HNL_MOD;


/* --------------------------------------------------------------------------------- */
/* Other definitions */

#define MAX_CHARS		256L		/* Max number of chars in a graphic string, including terminating null */
#define MAX_TEMPO_CHARS	64L			/* Max number of chars in either kind of tempo string, including terminating null */
									/* NB: Code assumes MAX_TEMPO_CHARS < MAX_CHARS */
#define FIRST_OFFSET	2			/* Offset into gHStringPool of first string */
#define MAX_OFFSET		65535		/* Max offset into gHStringPool -- constrained by range of NLINK */

#define NOTE_CHAR		'N'
#define GRACE_CHAR		'G'
#define REST_CHAR		'R'
#define TUPLET_CHAR		'P'
#define BAR_CHAR		'/'
#define CLEF_CHAR		'C'
#define KEYSIG_CHAR		'K'
#define TIMESIG_CHAR	'T'
#define METRONOME_CHAR	'M'
#define GRAPHIC_CHAR	'A'
#define DYNAMIC_CHAR	'D'
#define BEAM_CHAR		'B'
#define COMMENT_CHAR	'%'


/* --------------------------------------------------------------------------------- */
/* Data structure access macros */

#define GetPNL_HEAD(link)		(PNL_HEAD)		&gNodeList[link]
#define GetPNL_GENERIC(link)	(PNL_GENERIC)	&gNodeList[link]
#define GetPNL_NRGR(link)		(PNL_NRGR)		&gNodeList[link]
#define GetPNL_TUPLET(link)		(PNL_TUPLET)	&gNodeList[link]
#define GetPNL_BARLINE(link)	(PNL_BARLINE)	&gNodeList[link]
#define GetPNL_CLEF(link)		(PNL_CLEF)		&gNodeList[link]
#define GetPNL_KEYSIG(link)		(PNL_KEYSIG)	&gNodeList[link]
#define GetPNL_TIMESIG(link)	(PNL_TIMESIG)	&gNodeList[link]
#define GetPNL_TEMPO(link)		(PNL_TEMPO)		&gNodeList[link]
#define GetPNL_GRAPHIC(link)	(PNL_GRAPHIC)	&gNodeList[link]
#define GetPNL_DYNAMIC(link)	(PNL_DYNAMIC)	&gNodeList[link]

#define GetNL_TYPE(link)		(GetPNL_GENERIC(link))->objType


/* -------------------------------------------------------------------------------------- */
/* Other definitions */

/* The following codes refer to consecutive error messages that are in the middle of the
string list: this can make adding or changing messages tricky! Cf. ReportParseFailure().
FIXME: ADD NAMES for the first few and the last n! */

enum {
	NLERR_1STLINE_NG=1,		/* Not a valid notelist: 1st line not correct" */
	NLERR_MAYBE_NOMEM,		/* "May be insufficient memory" */
	NLERR_HAIRPINS,			/* "Can't handle hairpins: ignored..." */
	NLERR_PBLM_LINENO,		/* "Problem in line number..." */
	NLERR_MISCELLANEOUS,	/* "Miscellaneous error" */
	NLERR_TOOFEWFIELDS,		/* "Less than the minimum number of fields for the opcode" */
	NLERR_INTERNAL,			/* "Internal error in Notelist parsing" */
	NLERR_BADTIME,			/* "t (time) is illegal" */
	NLERR_BADVOICE,			/* "v (voice no.) is illegal" */
	NLERR_BADPART,			/* (10) "npt (part no.) is illegal" */
	NLERR_BADSTAFF,			/* "stf (staff no.) is illegal" */
	NLERR_BADDURCODE,		/* "dur (duration code) is illegal" */
	NLERR_BADDOTS,			/* "dots (no. of aug. dots) is illegal" */
	NLERR_BADNOTENUM,		/* "nn (note number) is illegal" */
	NLERR_BADACC,			/* "acc (accidental code) is illegal" */
	NLERR_BADEACC,			/* "eAcc (effective accidental code) is illegal" */
	NLERR_BADPDUR,			/* "pDur (play duration) is illegal" */
	NLERR_BADVELOCITY,		/* "vel (MIDI velocity) is illegal" */
	NLERR_BADNOTEFLAGS,		/* "Note flags are illegal" */
	NLERR_BADAPPEAR,		/* (20) "appear (appearance code) is illegal" */
	NLERR_BADMODS,			/* "Note modifiers on grace notes or illegal" */
	NLERR_BADSEGS,			/* "Notehead segments on grace notes or illegal" */
	NLERR_BADNUMDENOM,		/* "num or denom (numerator or denominator) is illegal" */
	NLERR_BADTUPLETVIS,		/* "Numerator/denominator visibility combination is illegal" */
	NLERR_BADTYPE,			/* "type is illegal" */
	NLERR_BADDTYPE,			/* "dType is illegal" */
	NLERR_BADNUMACC,		/* "KS (no. of accidentals) is illegal" */
	NLERR_BADKSACC,			/* "Key signature accidental code is illegal" */
	NLERR_BADDISPL,			/* "displ (time signature appearance) is illegal" */
	NLERR_ILLEGAL_OPCODE,	/* (30) "illegal first character (opcode)..." */
	NLERR_CANT_CONVERT,		/* "Open Notelist can't convert item..." */
	NLERR_BARLINE_TIMES,	/* "Consecutive barline times are inconsistent" */
	NLERR_NOTE_NOTINTUPLET,	/* "Note claims to be in a tuplet, but..." */
	NLERR_NOTE_BARLINE,		/* "Note extends over barline" */
	NLERR_NOTES_OVERLAP,	/* "Consecutive notes in same voice overlap" FIXME: NEVER CHECKED */
	NLERR_TUPLET_1NOTE,		/* "Tuplet has only one note" */
	NLERR_TUPLET_DURATION,	/* "Tuplet total duration doesn't make sense." */
	NLERR_ENTIRENL_NOSAVE,	/* "Entire notelist wasn't saved" */
	NLERR_ILLEGAL_OPCODE_B,	/* "illegal first character (opcode); may be a Beam" */
	NLERR_MM_BADDUR			/* (40) "Metronome mark duration code is illegal." */
};


/* --------------------------------------------------------------------------------- */
/* Prototypes of functions shared between "NotelistConvert.c" and "NotelistParse.c",
but which should not otherwise be public. */

/* in "NotelistConvert.c"... */

/* in "NotelistParse.c"... */
Boolean ParseNotelistFile(Str255 fileName, FSSpec *fsSpec);
NLINK NLSearch(NLINK startL, short type, char part, char staff, char uVoice, Boolean goLeft);
Boolean FetchString(NLINK offset, char str[]);
Boolean FetchModifier(NLINK modL, PNL_MOD pMod);
void DisposNotelistMemory(void);

