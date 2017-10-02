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

#define DOAUTOBEAM					1
#ifdef PUBLIC_VERSION
	#define PRINTNOTELIST			0
#else
	#define PRINTNOTELIST			1
#endif
#define CNTLKEYFORPRINTNOTELIST		1


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

#define NLOBJHEADER					/* 8 bytes: */												\
	long		lStartTime;			/* positive 32-bit integer */								\
	char		objType;			/* see enum above */										\
	char		uVoice;				/* part-relative ("user") voice: 1-31, or -2 (NOONE) */		\
	char		part;				/* 1-64, or -2 (NOONE) */									\
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


/* The NL_NRGR struct represents either a normal Note, a Rest or a GRace note,
	depending on the value of <objType> (either NR_TYPE or GRACE_TYPE) and the
	<isRest> field. (Note that Ngale does not currently support grace rests.)

	Note the differences in interpretation of grace note and normal note fields:
	- <durCode> has narrower range for grace notes (qtr-32nd)
	- <pDur> unimplemented for grace notes in Ngale
	- grace notes cannot (yet) be tied, slurred or tupleted
	- grace notes cannot (yet) have note modifiers
	
	Note the differences in interpretation of rest and note fields:
	- rests have neither <acc> nor <eAcc> fields in the Notelist
	- rests have no <pDur> field
	- rests have no MIDI note number field
	- rests have no MIDI velocity field
	- rests can have negative durCodes (representing whole- and multi-meas rests),
		but they cannot be of unknown duration (durCode=0)
	- rests never have their mainNote flag set
*/

typedef struct {
	NLOBJHEADER
	unsigned short	isRest:1;		/* note is really a rest */
	unsigned short	inChord:1;		/* note is in a chord */
	unsigned short	mainNote:1;		/* main note (carrying the stem) in a chord */
	unsigned short	inTuplet:1;		/* note/rest is a member of a tuplet group (irrelevant for grace notes) */
	unsigned short	tiedL:1;		/* tied to the preceding note */
	unsigned short	tiedR:1;		/* tied to the following note */
	unsigned short	slurredL:1;		/* slurred to the preceding note */
	unsigned short	slurredR:1;		/* slurred to the following note */
	unsigned short	filler1:2;
	unsigned short	acc:3;			/* code of displayed accidental: 0-5 */
	unsigned short	eAcc:3;			/* code of effective accidental: 0-5 */

	unsigned char	filler2:1;
	unsigned char	appear:4;		/* appearance code: 0-10 (see "NTypes.h") */
	unsigned char	nDots:3;		/* number of augmentation dots: 0-5 */
	char			durCode;		/* code for logical duration: 0-9 (-127 - 9 for rests!) */

	char			noteNum;		/* MIDI note number: 0-127 */
	char			vel;			/* MIDI note on velocity: 0-127 */
	short			pDur;			/* physical duration (480 units/quarter): 1-32000 */
	NLINK			firstMod;		/* index into gHModList of 1st note mod, or 0 if no mods */
} NL_NRGR, *PNL_NRGR;				/* 8 + 10 = 18 bytes */


/* Note the interpretation of NLOBJHEADER fields for tuplets:
	- <lStartTime> is not assigned by Notelist.
	- <staff> is not specified in the Notelist: a tuplet applies to a
	  part-relative voice, so Notelist encodes only <uVoice> and <part>.
	  However, since FICreateTUPLET requires a staff number, we fill
	  in the tuplet's staff in AnalyzeNLTuplet. */
	  
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
	- <uVoice>, <staff> and <part> are irrelevant (currently): a barline.
	  is understood to span the entire system. */
	  
typedef struct {
	NLOBJHEADER
	char		appear;				/* appearance code: 1-7 (see "NTypes.h") */
	char		filler;
	char		data[];				/* (additional bytes in union used by NL_NRGR) */
} NL_BARLINE, *PNL_BARLINE;


/* Note the interpretation of NLOBJHEADER fields for clefs:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice> and <part> are irrelevant; use <staff> only. */
	
typedef struct {
	NLOBJHEADER
	char		type;				/* clef type code: 1-12 (see "NTypes.h") */
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
	char	appear;					/* 1-4 (normal, 'C', cut time, numerator only -- see "NTypes.h") */
	char	filler;
	char	data[];					/* (additional bytes in union used by NL_NRGR) */
} NL_TIMESIG, *PNL_TIMESIG;


/* Note the interpretation of NLOBJHEADER fields for tempo marks:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice> and <part> are irrelevant; use <staff> only. */
	
typedef struct {
	NLOBJHEADER
	NLINK	string;					/* tempo string: index into Notelist string pool, or NILINK */
	NLINK	metroStr;				/* bpm string: index into Notelist string pool, or NILINK */
	Boolean	dotted;					/* is metronome note-value dotted? */
	Boolean	hideMM;					/* not encoded in Notelist; set in ParseTempoMark */
	char	durCode;				/* for metronome note-value */
	char	data[];					/* (additional bytes in union used by NL_NRGR) */
} NL_TEMPO, *PNL_TEMPO;


/* Note the interpretation of NLOBJHEADER fields for graphics:
	- <lStartTime> is not assigned by Notelist.
	- <uVoice>, <part> and <staff> will be -2 if graphic is page-relative.
		Note that if a graphic is attached to a clef, timesig or keysig,
		then its staff will be the same as that of the clef, timesig or
		keysig. However, its user (and internal) voice number, as well
		as its part number, will be -2. */
		
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
	char	type;					/* code (see "NTypes.h") : 1-23 */
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
	char	code;					/* note-modifier code: 1-31 (see "NTypes.h") */
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

