/* NObjTypes.h for Nightingale - typedefs for objects, subobjects, and related things.
NB: Very many of these appear in Nightingale score files, so changing them may be a
problem for backward compatibility.

The infomation represented in Conventional Western Music Notation is extraordinarily
complex and subtle. One reason is that it specifies -- often implicitly -- information
in several _domains_: Logical, Graphical, and Performance/Playback. These are symbolized
in comments below by "L", "G", and "P". See Nightingale Technical Note #1 for some details
and a reference. 
*/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2019 by Avian Music Notation Foundation. All Rights Reserved.
 */

#pragma options align=mac68k

/* Macros in MemMacros.h depend on the positions of the first six fields of the object
header, which MUST NOT be re-positioned! In the future, this may apply to other fields
as well. */

#define OBJECTHEADER \
	LINK		right, left;		/* links to left and right objects */					\
	LINK		firstSubObj;		/* link to first subObject */							\
	DDIST		xd, yd;				/* position of object */								\
	SignedByte	type;				/* (.+#10) object type */								\
	Boolean		selected;			/* True if object or any part of it is selected */		\
	Boolean		visible;			/* True if object or any part of it is visible */		\
	Boolean		soft;				/* True if object is program-generated */				\
	Boolean		valid;				/* True if objRect (for Measures, measureBBox too) valid. */ \
	Boolean		tweaked;			/* True if object dragged or position edited with Get Info */ \
	Boolean		spareFlag;			/* available for general use */							\
	char		ohdrFiller1;		/* unused; could use for specific "tweak" flags */		\
	Rect		objRect;			/* (.+18) enclosing rectangle of object (paper-rel.pixels) */ 	\
	SignedByte	relSize;			/* (unused) size rel. to normal for object & context */	\
	SignedByte	ohdrFiller2;		/* unused */											\
	Byte		nEntries;			/* (.+#28) number of subobjects in object */
	
#define SUBOBJHEADER \
	LINK		next;				/* index of next subobj */								\
	SignedByte	staffn;				/* staff no. For cross-stf objs, top stf (Slur,Beamset) or 1st stf (Tuplet) */									\
	SignedByte	subType;			/* subobject subtype. NB: Signed; see ANOTE. */			\
	Boolean		selected;			/* True if subobject is selected */						\
	Boolean		visible;			/* True if subobject is visible */						\
	Boolean		soft;				/* True if subobject is program-generated */

#define EXTOBJHEADER \
	SignedByte	staffn;				/* staff number: for cross-staff objs, of top staff FIXME: except tuplets! */


/* ------------------------------------------------------------------- Type 0 = HEADER -- */
/* The HEADER's subobject, PARTINFO, is defined in NBasicTypes.h. */

typedef struct {
	OBJECTHEADER
} HEADER, *PHEADER;


/* --------------------------------------------------------------------- Type 1 = TAIL -- */

typedef struct {
	OBJECTHEADER
} TAIL, *PTAIL;


/* --------------------------------------------------------------- Type 2 = NOTE, SYNC -- */
/* A "note" is a normal or small note or rest, perhaps a cue note, but not a grace
note. (The main reason we use a different type for grace notes is they have no logical
duration.) */

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
	Boolean		tiedL;				/* LGP: True if tied to left */
	Boolean		tiedR;				/* LGP: True if tied to right */
	Byte		xMoveDots;			/* G: X-offset on aug. dot position (quarter-spaces) */
	Byte		yMoveDots;			/* G: Y-offset on aug. dot pos. (half-spaces, 2=same as note, except 0=invisible) */
	Byte		ndots;				/* LG: No. of aug. dots */
	SignedByte	voice;				/* L: Voice number */
	Byte		rspIgnore;			/* True if note's chord should not affect automatic spacing (unused as of v. 5.9) */
	Byte		accident;			/* LG: 0=none, 1--5=dbl. flat--dbl. sharp (unused for rests) */
	Boolean		accSoft;			/* L: Was accidental generated by Nightingale? */
	Byte		xmoveAcc;			/* G: X-offset to left on accidental position */
	Boolean		playAsCue;			/* L: True = play note as cue, ignoring dynamic marks (unused as of v. 5.9) */
	Byte		micropitch;			/* LP: Microtonal pitch modifier (unused as of v. 5.9) */
	Byte		merged;				/* temporary flag for Merge functions */
	Byte		courtesyAcc;		/* G: Accidental is a "courtesy accidental" */
	Byte		doubleDur;			/* G: Draw as if double the actual duration */
	Byte		headShape;			/* G: Special notehead or rest shape; see list below */
	LINK		firstMod;			/* LG: Note-related symbols (articulation, fingering, etc.) */
	Boolean		slurredL;			/* G: True if endpoint of slur to left */
	Boolean		slurredR;			/* G: True if endpoint of slur to right */
	Boolean		inTuplet;			/* True if in a tuplet */
	Boolean		inOttava;			/* True if in an octave sign */
	Boolean		small;				/* G: True if a small (cue, cadenza-like, etc.) note */
	Byte		tempFlag;			/* temporary flag for benefit of functions that need it */
	Byte		artHarmonic;		/* Artificial harmonic: stopped, touched, sounding, normal note (unused as of v. 6.0) */
	long		reservedN;			/* For future use, e.g., for user ID number (unused as of v. 6.0) */
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
	NOTHING_VIS						/* EVERYTHING (head/rest, stem, aug. dots, etc.) invisible */
};


/* ---------------------------------------------- Type 3 = ARPTEND, RPTEND (REPEATEND) -- */

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


/* --------------------------------------------------------------------- Type 4 = PAGE -- */

typedef struct {
	OBJECTHEADER
	LINK		lPage,				/* Links to left and right Pages */
				rPage;
	short		sheetNum;			/* Sheet number: indexed from 0 */
	StringPtr	headerStrOffset,	/* (unused; when used, should be STRINGOFFSETs) */
				footerStroffset;
} PAGE, *PPAGE;


/* ------------------------------------------------------------------- Type 5 = SYSTEM -- */

typedef struct {
	OBJECTHEADER
	LINK		lSystem,			/* Links to left and right Systems */
				rSystem;
	LINK		pageL;				/* Link to previous (enclosing) Page */
	short		systemNum;			/* System number: indexed from 1 */
	DRect		systemRect;			/* DRect enclosing entire system, rel to Page */
	Ptr			sysDescPtr;			/* (unused) ptr to data describing left edge of System */
} SYSTEM, *PSYSTEM;


/* ------------------------------------------------------------ Type 6 = ASTAFF, STAFF -- */

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
	Byte		filler,				/* unused */
				showLedgers,		/* True if drawing ledger lines of notes on this staff (the default if showLines>0) */
				showLines;			/* 0=show 0 staff lines, 1=only middle line (of 5-line staff), 2-14 unused,
										SHOW_ALL_LINES=show all lines (default) */
} ASTAFF, *PASTAFF;

typedef struct {
	OBJECTHEADER
	LINK			lStaff,			/* links to left and right Staffs */
					rStaff;
	LINK			systemL;		/* link to previous (enclosing) System */
} STAFF, *PSTAFF;


/* -------------------------------------------------------- Type 7 = AMEASURE, MEASURE -- */

typedef struct {
	SUBOBJHEADER					/* subType=barline type (see enum below) */
	Boolean		measureVisible;		/* True if measure contents are visible */
	Boolean		connAbove;			/* True if connected to barline above */
	char		filler1;
	SignedByte	filler2;
	short		reservedM,			/* formerly <oldFakeMeas>; keep space for future use */
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

typedef struct	{
	OBJECTHEADER
	SignedByte		fillerM;		/* unused */
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


/* -------------------------------------------------------------- Type 8 = ACLEF, CLEF -- */

typedef struct {
	SUBOBJHEADER
	Byte		filler1;
	Byte		small;				/* True to draw in small characters */
	Byte		filler2;
	DDIST		xd, yd;				/* DDIST position */
} ACLEF, *PACLEF;

typedef struct {
	OBJECTHEADER
	Boolean	inMeasure;				/* True if object is in a Measure, False if not */
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


/* ---------------------------------------------------------- Type 9 = AKEYSIG, KEYSIG -- */

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


/* ------------------------------------------------------- Type 10 = ATIMESIG, TIMESIG -- */

typedef struct {
	SUBOBJHEADER
	Byte		filler;				/* Unused--put simple/compound/other here? */
	Byte		small;				/* (unused) True to draw in small characters */
	SignedByte	connStaff;			/* (unused) bottom staff no. */
	DDIST		xd, yd;				/* DDIST position */
	SignedByte	numerator;			/* numerator */
	SignedByte	denominator;		/* denominator */
} ATIMESIG, *PATIMESIG;

typedef struct {
	OBJECTHEADER
	Boolean		inMeasure;		/* True if object is in a Measure, False if not */
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

#define LOW_TSTYPE N_OVER_D
#define HIGH_TSTYPE N_OVER_DOTTEDEIGHTH


/* ------------------------------------------------------ Type 11 = ANOTEBEAM, BEAMSET -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	LINK		bpSync;				/* link to Sync containing note/chord */
	SignedByte	startend;			/* No. of beams to start/end (+/-) on note/chord */
	Byte		fracs;				/* No. of fractional beams on note/chord */ 
	Byte		fracGoLeft;			/* Do fractional beams point left? */
	Byte		filler;				/* unused */
} ANOTEBEAM, *PANOTEBEAM;

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte	voice;				/* Voice number */
	Byte		thin;				/* True=narrow lines, False=normal width */
	Byte		beamRests;			/* True if beam can contain rests */
	Byte		feather;			/* (unused) 0=normal, 1=feather L end (accel.), 2=feather R (decel.) */
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
	SignedByte	start;				/* Index of starting note/rest in BEAMSET */
	SignedByte	stop;				/* Index of ending note/rest in BEAMSET */
	SignedByte	startLev;			/* Vertical slot no. at start (0=end of stem; -=above) */
	SignedByte	stopLev;			/* Vertical slot no. at end (0=end of stem; -=above) */
	SignedByte	fracGoLeft;			/* Do fractional beams point left? */
} BEAMINFO;


/* ------------------------------------------------------- Type 12 = ACONNECT, CONNECT -- */

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
	LINK		connFiller;			/* unused */
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


/* ------------------------------------------------------- Type 13 = ADYNAMIC, DYNAMIC -- */

typedef struct {
	SUBOBJHEADER					/* subType is unused */
	Byte			mouthWidth;		/* Width of mouth for hairpin */
	Byte			small;			/* True to draw in small characters */
	Byte			otherWidth;		/* Width of other (non-mouth) end for hairpin */
	DDIST			xd;				/* (unused) */
	DDIST			yd;				/* Position offset from staff top */
	DDIST			endxd;			/* Position offset from lastSyncL for hairpins. */
	DDIST			endyd;			/* Position offset from staff top for hairpins. */
	Byte			dModCode;		/* Code for modifier (see enum below) (unused) */
	Byte			crossStaff;		/* 0=normal, 1=also affects staff above, 2=also staff below;
										negative=show curly brace if cross-staff (unused) */
} ADYNAMIC, *PADYNAMIC;

typedef struct {
	OBJECTHEADER
	SignedByte	dynamicType;		/* Code for dynamic marking (see enum below) */
	Boolean		filler;
	Boolean		crossSys;			/* (unused) Whether cross-system */
	LINK		firstSyncL;			/* Sync dynamic or hairpin start is attached to */
	LINK		lastSyncL;			/* Sync hairpin end is attached to or NILINK */
} DYNAMIC, *PDYNAMIC;

/* Dynamic marking */
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

/* Dynamic marking modifier */

#ifdef NOTYET
enum {
	Values for adding "subito" before or after, "piu"/"meno" before; also "mpp", etc.?
}
#endif


/* ------------------------------------------------------------------ Type 14 = AMODNR -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	Boolean		selected;			/* True if subobject is selected */
	Boolean		visible;			/* True if subobject is visible */
	Boolean		soft;				/* True if subobject is program-generated */
	Byte		xstd;				/* Note-relative position (FIXME: should be STDIST: see below) */
	Byte		modCode;			/* Which note modifier */
	SignedByte	data;				/* Modifier-dependent */
	SHORTSTD	ystdpit;			/* Clef-independent dist. below middle C ("pitch") */
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


/* ------------------------------------------------------- Type 15 = AGRAPHIC, GRAPHIC -- */

typedef struct {
	LINK next;
	STRINGOFFSET strOffset;			/* index return by String Manager library. */
} AGRAPHIC, *PAGRAPHIC;

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER					/* NB: staff number can be 0 here */
	SignedByte	graphicType;		/* graphic class (subtype) */
	SignedByte	voice;				/* Voice number (but with some types of relObjs, NOONE) */
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
		short		thickness;		/* percent of interline space */
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
	GRPICT=1,						/* 	(unimplemented; anyway PICTs are obsolete) PICT */
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


/* ----------------------------------------------------- Type 16 = ANOTEOTTAVA, OTTAVA -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	LINK		opSync;				/* link to Sync containing note/chord (not rest) */
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
	DDIST		nxd, nyd;				/* (unused) DDIST position of number */
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


/* ------------------------------------------------------------- Type 17 = ASLUR, SLUR -- */

/* Types of Slursor behavior: */

enum { S_New=0, S_Start, S_C0, S_C1, S_End, S_Whole, S_Extend };

typedef struct {
	DPoint knot;					/* Coordinates of knot relative to startPt */
	DPoint c0;						/* Coordinates of first control point relative to knot */
	DPoint c1;						/* Coordinates of second control pt relative to endpoint */
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

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte	voice;				/* Voice number */
	char		philler;			/* A "filler" (unused); funny name to avoid confusion with ASLUR's filler */
	char		crossStaff;			/* True if the slur is cross-staff: staffn=top staff(?) */
	char		crossStfBack;		/* True if the slur goes from a lower (position, not no.) stf to higher */
	char		crossSystem;		/* True if the slur is cross-system */	
	Boolean		tempFlag;			/* temporary flag for benefit of functions that need it */
	Boolean 	used;				/* (Unused :-) ) */
	Boolean		tie;				/* True if tie, else slur */
	LINK		firstSyncL;			/* Link to sync with 1st slurred note or to slur's system's init. measure */
	LINK		lastSyncL;			/* Link to sync with last slurred note or to slur's system */
} SLUR, *PSLUR;


/* ------------------------------------------------------ Type 18 = ANOTETUPLE, TUPLET -- */

/* This struct is used to get information from TupletDialog. */

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
	LINK			tpSync;			/* link to Sync containing note/chord/rest */
} ANOTETUPLE, *PANOTETUPLE;

typedef struct {
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


/* --------------------------------------------------------- Type 19 = AGRNOTE, GRSYNC -- */

typedef ANOTE AGRNOTE;				/* Same struct, though not all fields are used here */
typedef PANOTE PAGRNOTE;

typedef struct {
	OBJECTHEADER
} GRSYNC, *PGRSYNC;


/* ------------------------------------------------------------------- Type 20 = TEMPO -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte		subType;		/* "Beat": same units as note's l_dur */
	Boolean			expanded;		/* Stretch out the text? */
	Boolean			noMM;			/* False = play at _tempoMM_ BPM, True = ignore it */
	char			filler;
	Boolean			dotted;			/* Does beat unit have an augmentation dot? */
	Boolean			hideMM;			/* False = show Metronome mark, True = don't show it */
	short			tempoMM;		/* new playback speed in beats per minute */	
	STRINGOFFSET	strOffset;		/* "tempo" string index return by String Manager */
	LINK			firstObjL;		/* object tempo depends on */
	STRINGOFFSET	metroStrOffset;	/* "metronome mark" index return by String Manager */
} TEMPO, *PTEMPO;


/* ------------------------------------------------------------------ Type 21 = SPACER -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
	SignedByte	bottomStaff;		/* Last staff on which space to be left */
	STDIST		spWidth;			/* Amount of blank space to leave */
} SPACER, *PSPACER;


/* ------------------------------------------------------------------ Type 22 = ENDING -- */

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


/* -------------------------------------------- Type 23 = APSMEAS, PSMEAS (PSEUDOMEAS) -- */
/* Pseudomeasures are symbols that look like barlines but have no semantics, i.e., dotted
barlines and double bars that don't coincide with "real" barlines: they're G domain only,
while Measures are L and G domain. */

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


/* ============================================================= Miscellaneous structs == */
/* The following structs aren't actually object types. */

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
				unused:11;
} VInfo;

typedef struct {
	long		startTime;				/* Start time in this voice */
	long		clipEndTime;			/* End time of clipboard in this voice */
	short		firstStf,				/* First staff occupied by objs in this voice */
				lastStf;				/* Last staff occupied by objs in this voice */
	short		singleStf,				/* Whether voice in this sys on more than 1 stf */
				hasV,
				vBad,
				unused:13;
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


/* ---------------------------------------------------------------------------- MEVENT -- */

typedef struct {
	OBJECTHEADER
} MEVENT, *PMEVENT;


/* ---------------------------------------------------------------------------- EXTEND -- */

typedef struct {
	OBJECTHEADER
	EXTOBJHEADER
} EXTEND, *PEXTEND;

#pragma options align=reset
