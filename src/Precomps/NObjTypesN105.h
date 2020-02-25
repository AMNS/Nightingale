/* NObjTypesN105.h for Nightingale - typedefs for objects, subobjects, and related things
in files of format 'N105'. We have to get rid of bit fields in files for compatibility
between Nightingale running on Intel CPUs vs. PowerPCs. But for backward compatibility,
we must convert stuff in old files from the below definitions to those in NObjTypes.h. */

#pragma options align=mac68k

/* Macros in MemMacros.h depend on the positions of the first five fields of the object
header, which MUST NOT be re-positioned! In the future, this may apply to other fields
as well. */

#define OBJECTHEADER_5 \
	LINK		right, left;		/* links to left and right objects */					\
	LINK		firstSubObj;		/* link to first subObject */							\
	DDIST		xd, yd;				/* position of object */								\
	SignedByte	type;				/* (.+#10) object type */								\
	Boolean		selected:1;			/* True if any part of object selected */				\
	Boolean		visible:1;			/* True if any part of object is visible */				\
	Boolean		soft:1;				/* True if object is program-generated */				\
	Boolean		valid:1;			/* True if objRect (for Measures, measureBBox also) valid */ \
	Boolean		tweaked:1;			/* True if object dragged or position edited with Get Info */ \
	Boolean		spareFlag:1;		/* available for general use */							\
	char		ohdrFiller1:2;		/* unused; could use for specific "tweak" flags */		\
	Rect		objRect;			/* (.+#12) object's enclosing rectangle (paper-rel.pixels) */ \
	SignedByte	relSize;			/* (unused) size rel. to normal for object & context */	\
	SignedByte	ohdrFiller2;		/* unused */											\
	Byte		nEntries;			/* (.+#22) number of subobjects in object */
	
#define SUBOBJHEADER_5 \
	LINK		next;				/* index of next subobj */								\
	SignedByte	staffn;				/* staff no. For cross-stf objs, top stf (Slur,Beamset) or 1st stf (Tuplet) */	\
	SignedByte	subType;			/* subobject subtype. N.B. Signed--see ANOTE. */		\
	Boolean		selected:1;			/* True if subobject is selected */						\
	Boolean		visible:1;			/* True if subobject is visible */						\
	Boolean		soft:1;				/* True if subobject is program-generated */


/* ------------------------------------------------------------------------------ TAIL -- */

typedef struct {
	OBJECTHEADER_5
} TAIL_5, *PTAIL_5;


/* ------------------------------------------------------------------------------ PAGE -- */

typedef struct {
	OBJECTHEADER_5
	LINK		lPage,				/* Links to left and right Pages */
				rPage;
	short		sheetNum;			/* Sheet number: indexed from 0 */
	StringPtr	headerStrOffset,	/* (unused; when used, should be STRINGOFFSETs) */
				footerStroffset;
} PAGE_5, *PPAGE_5;


/* ---------------------------------------------------------------------------- SYSTEM -- */

typedef struct {
	OBJECTHEADER_5
	LINK		lSystem,			/* Links to left and right Systems */
				rSystem;
	LINK		pageL;				/* Link to previous (enclosing) Page */
	short		systemNum;			/* System number: indexed from 1 */
	DRect		systemRect;			/* DRect enclosing entire system, rel to Page */
	Ptr			sysDescPtr;			/* (unused) ptr to data describing left edge of System */
} SYSTEM_5, *PSYSTEM_5;


/* ----------------------------------------------------------------------------- STAFF -- */

#define SHOW_ALL_LINES	15

typedef struct {
	LINK		next;				/* index of next subobj */
	SignedByte	staffn;				/* staff number */
	Boolean		selected:1;			/* True if subobject is selected */
	Boolean		visible:1;			/* True if object is visible */
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
				showLedgers:1,		/* True if drawing ledger lines of notes on this staff (the default if showLines>0) */
				showLines:4;		/* 0=show 0 staff lines, 1=only middle line (of 5-line staff), 2-14 unused,
										SHOW_ALL_LINES=show all lines (default) */
} ASTAFF_5, *PASTAFF_5;

typedef struct {
	OBJECTHEADER_5
	LINK			lStaff,				/* links to left and right Staffs */
					rStaff;
	LINK			systemL;			/* link to previous (enclosing) System */
} STAFF_5, *PSTAFF_5;


/* --------------------------------------------------------------------------- CONNECT -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	Boolean		selected:1;			/* True if subobject is selected */
	Byte 		filler:1;
	Byte		connLevel:3;		/* Code from list below */
	Byte		connectType:2;		/* Code from list below */
	SignedByte	staffAbove;			/* upper staff no. (top of line or curly) (valid if connLevel!=0) */
	SignedByte	staffBelow;			/* lower staff no. (bottom of " ) (valid if connLevel!=0) */
	DDIST		xd;					/* DDIST position */
	LINK		firstPart;			/* (Unused) LINK to first part of group or connected part if not a group */
	LINK		lastPart;			/* (Unused) LINK to last part of group or NILINK if not a group */
} ACONNECT_5, *PACONNECT_5;

typedef struct {
	OBJECTHEADER_5
	LINK		connFiller;
} CONNECT_5, *PCONNECT_5;


/* ----------------------------------------------------------------------- ACLEF, CLEF -- */

typedef struct {
	SUBOBJHEADER_5
	Byte		filler1:3;
	Byte		small:2;			/* True to draw in small characters */
	Byte		filler2;
	DDIST		xd, yd;				/* DDIST position */
} ACLEF_5, *PACLEF_5;

typedef struct {
	OBJECTHEADER_5
	Boolean	inMeasure:1;			/* True if object is in a Measure, False if not */
} CLEF_5, *PCLEF_5;

enum {								/* clef subTypes: */
	TREBLE8_CLEF_5=1,					/* unused */
	FRVIOLIN_CLEF_5,					/* unused */
	TREBLE_CLEF_5,
	SOPRANO_CLEF_5,
	MZSOPRANO_CLEF_5,
	ALTO_CLEF_5,
	TRTENOR_CLEF_5,
	TENOR_CLEF_5,
	BARITONE_CLEF_5,
	BASS_CLEF_5,
	BASS8B_CLEF_5,					/* unused */
	PERC_CLEF_5
};

#define LOW_CLEF_5 TREBLE8_CLEF_5
#define HIGH_CLEF_5 PERC_CLEF_5


/* ---------------------------------------------------------------------------- KEYSIG -- */

typedef struct {
	SUBOBJHEADER_5					/* subType=no. of naturals, if nKSItems==0 */
	Byte			nonstandard:1;	/* True if not a standard CMN key sig. */
	Byte			filler1:2;
	Byte			small:2;		/* (unused so far) True to draw in small characters */
	SignedByte		filler2;
	DDIST			xd;				/* DDIST horizontal position */
	WHOLE_KSINFO
} AKEYSIG_5, *PAKEYSIG_5;

typedef struct {
	OBJECTHEADER_5
	Boolean	inMeasure:1;			/* True if object is in a Measure, False if not */
} KEYSIG_5, *PKEYSIG_5;


/* --------------------------------------------------------------------------- TIMESIG -- */

typedef struct {
	SUBOBJHEADER_5
	Byte		filler:3;			/* Unused--put simple/compound/other:2 here? */
	Byte		small:2;			/* (unused so far) True to draw in small characters */
	SignedByte	connStaff;			/* (unused so far) bottom staff no. */
	DDIST		xd, yd;				/* DDIST position */
	SignedByte	numerator;			/* numerator */
	SignedByte	denominator;		/* denominator */
} ATIMESIG_5, *PATIMESIG_5;

typedef struct {
	OBJECTHEADER_5
	Boolean		inMeasure:1;		/* True if object is in a Measure, False if not */
} TIMESIG_5, *PTIMESIG_5;


/* --------------------------------------------------------------------------- MEASURE -- */

typedef struct {
	SUBOBJHEADER_5					/* subType=barline type (see enum below) */
	Boolean		measureVisible:1;	/* True if measure contents are visible */
	Boolean		connAbove:1;		/* True if connected to barline above */
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
} AMEASURE_5, *PAMEASURE_5;

typedef struct {
	OBJECTHEADER_5
	SignedByte	fillerM;
	LINK			lMeasure,		/* links to left and right Measures */
					rMeasure;
	LINK			systemL;		/* link to owning System */
	LINK			staffL;			/* link to owning Staff */
	short			fakeMeas:1,		/* True=not really a measure (i.e., barline ending system) */
					spacePercent:15;	/* Percentage of normal horizontal spacing used */
	Rect			measureBBox;	/* enclosing Rect of all measure subObjs, in pixels, paper-rel. */
	long			lTimeStamp;		/* P: PDURticks since beginning of score */
} MEASURE_5, *PMEASURE_5;


/* ------------------------------------------------------------------------ PSEUDOMEAS -- */
/* Pseudomeasures are symbols that have similar graphic appearance to barlines
but have no semantics: dotted barlines and double bars that don't coincide with
"real" barlines. */

typedef struct {
	SUBOBJHEADER_5					/* subType=barline type (see enum below) */
	Boolean		connAbove:1;		/* True if connected to barline above */
	char		filler1:4;			/* (unused) */
	SignedByte	connStaff;			/* staff to connect to (valid if >0 and !connAbove) */
} APSMEAS_5, *PAPSMEAS_5;

typedef struct 	{
	OBJECTHEADER_5
	SignedByte	filler;
} PSMEAS_5, *PPSMEAS_5;


/* ------------------------------------------------------------------------ NOTE, SYNC -- */
/* A "note" is a normal or small note or rest, perhaps a cue note, but not a grace
note. (The main reason is that grace notes have no logical duration.) */

typedef struct {
	SUBOBJHEADER_5					/* subType (l_dur): LG: <0=n measure rest, 0=unknown, >0=Logical (CMN) dur. code */
	Boolean		inChord:1;			/* True if note is part of a chord */
	Boolean		rest:1;				/* LGP: True=rest (=> ignore accident, ystem, etc.) */
	Boolean		unpitched:1;		/* LGP: True=unpitched note */
	Boolean		beamed:1;			/* True if beamed */
	Boolean		otherStemSide:1;	/* G: True if note goes on "wrong" side of stem */
	SHORTQD		yqpit;				/* LG: clef-independent dist. below middle C ("pitch") (unused for rests) */
	DDIST		xd, yd;				/* G: head position */
	DDIST		ystem;				/* G: endpoint of stem (unused for rests) */
	short		playTimeDelta;		/* P: PDURticks before/after timeStamp when note starts */
	short		playDur;			/* P: PDURticks that note plays for */
	short		pTime;				/* P: PDURticks play time; for internal use by Tuplet routines */
	Byte		noteNum;			/* P: MIDI note number (unused for rests) */
	Byte		onVelocity;			/* P: MIDI note-on velocity, normally loudness (unused for rests) */
	Byte		offVelocity;		/* P: MIDI note-off (release) velocity (unused for rests) */
	Boolean		tiedL:1;			/* True if tied to left */
	Boolean		tiedR:1;			/* True if tied to right */
	Byte		ymovedots:2;		/* G: Y-offset on aug. dot pos. (halflines, 2=same as note, except 0=invisible) */
	Byte		ndots:4;			/* LG: No. of aug. dots */
	SignedByte	voice;				/* L: Voice number */
	Byte		rspIgnore:1;		/* True if note's chord should not affect automatic spacing (unused as of v. 5.5) */
	Byte		accident:3;			/* LG: 0=none, 1--5=dbl. flat--dbl. sharp (unused for rests) */
	Boolean		accSoft: 1;			/* L: Was accidental generated by Nightingale? */
	Boolean		playAsCue: 1;		/* L: True = play note as cue, ignoring dynamic marks (unused as of v. 5.7) */
	Byte		micropitch:2;		/* LP: Microtonal pitch modifier (unused as of v. 5.5) */
	Byte		xmoveAcc:5;			/* G: X-offset to left on accidental position */
	Byte		merged:1;			/* temporary flag for Merge functions */
	Byte		courtesyAcc:1;		/* G: Accidental is a "courtesy accidental" */
	Byte		doubleDur:1;		/* G: Draw as if double the actual duration */
	Byte		headShape:5;		/* G: Special notehead or rest shape; see list below */
	Byte		xmovedots:3;		/* G: X-offset on aug. dot position */
	LINK		firstMod;			/* LG: Note-related symbols (articulation, fingering, etc.) */
	Byte		slurredL:2;			/* True if endpoint of slur to left (extra bit for future use) */
	Byte		slurredR:2;			/* True if endpoint of slur to right (extra bit for future use) */
	Byte		inTuplet:1;			/* True if in a tuplet */
	Byte		inOttava:1;			/* True if in an octave sign */
	Byte		small:1;			/* True if a small (cue, etc.) note */
	Byte		tempFlag:1;			/* temporary flag for benefit of functions that need it */
	SignedByte	fillerN;
} ANOTE_5, *PANOTE_5;

typedef struct {
	OBJECTHEADER_5
	unsigned short timeStamp;		/* P: PDURticks since beginning of measure */
} SYNC_5, *PSYNC_5;


/* --------------------------------------------------------------------------- BEAMSET -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	LINK		bpSync;				/* link to Sync containing note/chord */
	SignedByte	startend;			/* No. of beams to start/end (+/-) on note/chord */
	Byte		fracs:3;			/* No. of fractional beams on note/chord */ 
	Byte		fracGoLeft:1;		/* Fractional beams point left? */
	Byte		filler:4;			/* unused */
} ANOTEBEAM_5, *PANOTEBEAM_5;

typedef struct {
	OBJECTHEADER_5
	EXTOBJHEADER
	SignedByte	voice;				/* Voice number */
	Byte		thin:1;				/* True=narrow lines, False=normal width */
	Byte		beamRests:1;		/* True if beam can contain rests */
	Byte		feather:2;			/* (unused) 0=normal,1=feather L end (accel.), 2=feather R (decel.) */
	Byte		grace:1;			/* True if beam consists of grace notes */
	Byte		firstSystem:1;		/* True if on first system of cross-system beam */	
	Byte		crossStaff:1;		/* True if the beam is cross-staff: staffn=top staff */
	Byte		crossSystem:1;		/* True if the beam is cross-system */
} BEAMSET_5, *PBEAMSET_5;

#ifdef NOMORE
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
#endif


/* ---------------------------------------------------------------------------- TUPLET -- */

typedef struct {
	OBJECTHEADER_5
	EXTOBJHEADER
	Byte			accNum;			/* Accessory numeral (numerator) for Tuplet */
	Byte			accDenom;		/* Accessory denominator */
	SignedByte		voice;			/* Voice number */
	Byte			numVis:1,
					denomVis:1,
					brackVis:1,
					small:2,		/* (unused so far) True to draw in small characters */
					filler:3;
	DDIST			acnxd, acnyd;	/* DDIST position of accNum (now unused) */
	DDIST			xdFirst, ydFirst,	/* DDIST position of bracket */
					xdLast, ydLast;
} TUPLET_5, *PTUPLET_5;


/* ------------------------------------------------------------------------- REPEATEND -- */

typedef struct {
	SUBOBJHEADER_5					/* subType is in object so unused here */
	Byte			connAbove:1;	/* True if connected above */
	Byte			filler:4;		/* (unused) */
	SignedByte		connStaff;		/* staff to connect to; valid if connAbove True */
} ARPTEND_5, *PARPTEND_5;

typedef struct {
	OBJECTHEADER_5
	LINK			firstObj;		/* Beginning of ending or NILINK */
	LINK			startRpt;		/* Repeat start point or NILINK */
	LINK			endRpt;			/* Repeat end point or NILINK */
	SignedByte		subType;		/* Code from enum below */
	Byte			count;			/* Number of times to repeat */
} RPTEND_5, *PRPTEND_5;


/* ---------------------------------------------------------------------------- ENDING -- */

typedef struct {
	OBJECTHEADER_5
	EXTOBJHEADER
	LINK		firstObjL;			/* Object left end of ending is attached to */
	LINK		lastObjL;			/* Object right end of ending is attached to or NILINK */
	Byte		noLCutoff:1;		/* True to suppress cutoff at left end of Ending */	
	Byte		noRCutoff:1;		/* True to suppress cutoff at right end of Ending */	
	Byte		endNum:6;			/* 0=no ending number or label, else code for the ending label */
	DDIST		endxd;				/* Position offset from lastObjL */
} ENDING_5, *PENDING_5;


/* ----------------------------------------------------------------- ADYNAMIC, DYNAMIC -- */

typedef struct {
	SUBOBJHEADER_5					/* subType is unused */
	Byte			mouthWidth:5;	/* Width of mouth for hairpin */
	Byte			small:2;		/* True to draw in small characters */
	Byte			otherWidth:6;	/* Width of other (non-mouth) end for hairpin */
	DDIST			xd;				/* (unused) */
	DDIST			yd;				/* Position offset from staff top */
	DDIST			endxd;			/* Position offset from lastSyncL for hairpins. */
	DDIST			endyd;			/* Position offset from staff top for hairpins. */
} ADYNAMIC_5, *PADYNAMIC_5;

typedef struct {
	OBJECTHEADER_5
	SignedByte	dynamicType;		/* Code for dynamic marking (see enum below) */
	Boolean		filler:7;
	Boolean		crossSys:1;			/* (unused) Whether crossSystem */
	LINK		firstSyncL;			/* Sync dynamic or hairpin start is attached to */
	LINK		lastSyncL;			/* Sync hairpin end is attached to or NILINK */
} DYNAMIC_5, *PDYNAMIC_5;


/* ---------------------------------------------------------------------------- AMODNR -- */

typedef struct {
	LINK		next;					/* index of next subobj */
	Boolean		selected:1;				/* True if subobject is selected */
	Boolean		visible:1;				/* True if subobject is visible */
	Boolean		soft:1;					/* True if subobject is program-generated */
	unsigned char xstd:5;				/* FIXME: use "Byte"? Note-relative position (really signed STDIST: see below) */
	Byte		modCode;				/* Which note modifier */
	SignedByte	data;					/* Modifier-dependent */
	SHORTSTD	ystdpit;				/* Clef-independent dist. below middle C ("pitch") */
} AMODNR_5, *PAMODNR_5;


/* --------------------------------------------------------------------------- GRAPHIC -- */

typedef struct {
	LINK next;
	STRINGOFFSET strOffset;			/* index return by String Manager library. */
} AGRAPHIC_5, *PAGRAPHIC_5;

typedef struct {
	OBJECTHEADER_5
	EXTOBJHEADER					/* N.B. staff number can be 0 here */
	SignedByte	graphicType;		/* graphic class (subtype) */
	SignedByte	voice;				/* Voice number (only with some types of relObjs) */
	Byte		enclosure:2;		/* Enclosure type; see list below */
	Byte		justify:3;			/* (unused) justify left/center/right */
	Boolean		vConstrain:1;		/* (unused) True if object is vertically constrained */
	Boolean		hConstrain:1;		/* (unused) True if object is horizontally constrained */
	Byte		multiLine:1;		/* True if string contains multiple lines of text (delimited by CR) */
	short		info;				/* PICT res. ID (GRPICT); char (GRChar); length (GRArpeggio); */
									/*   ref. to text style (FONT_R1, etc) (GRString,GRLyric); */
									/*	  2nd x (GRDraw); draw extension parens (GRChordSym) */
	union {
		Handle		handle;			/* handle to resource, or NULL */
		short		thickness;
	} gu;
	SignedByte	fontInd;			/* index into font name table (GRChar,GRString only) */
	Byte		relFSize:1;			/* True if size is relative to staff size (GRChar,GRString only) */ 
	Byte		fontSize:7;			/* if relSize, small..large code, else point size (GRChar,GRString only) */
	short		fontStyle;			/* (GRChar,GRString only) */
	short		info2;				/* sub-subtype (GRArpeggio), 2nd y (GRDraw), _expanded_ (GRString) */
	LINK		firstObj;			/* link to object left end is relative to, or NULL */
	LINK		lastObj;			/* link to object right end is relative to, or NULL */
} GRAPHIC_5, *PGRAPHIC_5;



/* ---------------------------------------------------------------------------- OTTAVA -- */

typedef struct {
	LINK		next;					/* index of next subobj */
	LINK		opSync;					/* link to Sync containing note/chord (not rest) */
} ANOTEOTTAVA_5, *PANOTEOTTAVA_5;

typedef struct {
	OBJECTHEADER_5
	EXTOBJHEADER
	Byte		noCutoff:1;				/* True to suppress cutoff at right end of octave sign */	
	Byte		crossStaff:1;			/* (unused) True if the octave sign is cross-staff */
	Byte		crossSystem:1;			/* (unused) True if the octave sign is cross-system */	
	Byte		octSignType:5;			/* class of octave sign */
	SignedByte	filler;					/* unused */
	Boolean		numberVis:1,
				unused1:1,
				brackVis:1,
				unused2:5;
	DDIST		nxd, nyd;				/* DDIST position of number */
	DDIST		xdFirst, ydFirst,		/* DDIST position of bracket */
				xdLast, ydLast;
} OTTAVA_5, *POTTAVA_5;


/* ------------------------------------------------------------------------------ SLUR -- */

typedef struct {
	LINK		next;				/* index of next subobj */
	Boolean		selected:1;			/* True if subobject is selected */
	Boolean		visible:1;			/* True if subobject is visible */
	Boolean		soft:1;				/* True if subobject is program-generated */
	Boolean		dashed:2;			/* True if slur should be shown as dashed line */
	Boolean		filler:3;
	Rect		bounds;				/* Bounding box of whole slur */
	SignedByte	firstInd,lastInd;	/* Starting, ending note indices in chord of tie */
	long		reserved;			/* For later expansion (e.g., to multi-segment slurs) */
	SplineSeg	seg;				/* For now, one slur spline segment always defined */
	Point		startPt, endPt;		/* Base points (note positions), paper-rel.; GetSlurContext returns Points */
	DPoint		endKnot;			/* End point of last spline segment, relative to endPt */
	
} ASLUR_5, *PASLUR_5;

typedef struct {
	OBJECTHEADER_5
	EXTOBJHEADER
	SignedByte	voice;				/* Voice number */
	char		filler:2;
	char		crossStaff:1;		/* True if the slur is cross-staff: staffn=top staff(?) */
	char		crossStfBack:1;		/* True if the slur goes from a lower (position, not no.) stf to higher */
	char		crossSystem:1;		/* True if the slur is cross-system */	
	Boolean		tempFlag:1;			/* temporary flag for benefit of functions that need it */
	Boolean 	used:1;				/* True if being used */
	Boolean		tie:1;				/* True if tie, else slur */
	LINK		firstSyncL;			/* Link to sync with 1st slurred note or to slur's system's init. measure */
	LINK		lastSyncL;			/* Link to sync with last slurred note or to slur's system */
} SLUR_5, *PSLUR_5;


/* -------------------------------------------------------------------- GRNOTE, GRSYNC -- */

typedef ANOTE_5 AGRNOTE_5;				/* Same struct, though not all fields are used here */
typedef PANOTE_5 PAGRNOTE_5;

typedef struct {
	OBJECTHEADER_5
} GRSYNC_5, *PGRSYNC_5;


/* ----------------------------------------------------------------------------- TEMPO -- */

typedef struct {
	OBJECTHEADER_5
	EXTOBJHEADER
	SignedByte		subType;		/* beat: same units as note's l_dur */
	Boolean			expanded:1;
	Boolean			noMM:1;			/* False = play at _tempoMM_ BPM, True = ignore it */
	char			filler:4;
	Boolean			dotted:1;
	Boolean			hideMM:1;
	short			tempoMM;		/* new playback speed in beats per minute */	
	STRINGOFFSET	strOffset;		/* "tempo" string index return by String Manager */
	LINK			firstObjL;		/* object tempo depends on */
	STRINGOFFSET	metroStrOffset;	/* "metronome mark" index return by String Manager */
} TEMPO_5, *PTEMPO_5;


/* ---------------------------------------------------------------------------- SPACER -- */

typedef struct {
	OBJECTHEADER_5
	EXTOBJHEADER
	SignedByte	bottomStaff;		/* Last staff on which space to be left */
	STDIST		spWidth;			/* Amount of blank space to leave */
} SPACER_5, *PSPACER_5;


/* ---------------------------------------------------------------------------- HEADER -- */

typedef struct {
	OBJECTHEADER_5
} HEADER_5, *PHEADER_5;


/* -------------------------------------------------------------------------- SUPEROBJ -- */
/* This struct is the union of all Object structs, and its size is the maximum size of
any record kept in the object heap. */

typedef union uSUPEROBJ_5 {	
		HEADER_5	header;
		TAIL_5		tail;
		SYNC_5		sync;
		RPTEND_5	rptEnd;
		PAGE_5		page;
		SYSTEM_5	system;
		STAFF_5		staff;
		MEASURE_5	measure;
		CLEF_5		clef;
		KEYSIG_5	keysig;
		TIMESIG_5	timesig;
		BEAMSET_5	beamset;
		CONNECT_5	connect;
		DYNAMIC_5	dynamic;
		GRAPHIC_5	graphic;
		OTTAVA_5	ottava;
		SLUR_5		slur;
		TUPLET_5	tuplet;
		GRSYNC_5	grSync;
		TEMPO_5		tempo;
		SPACER_5	spacer;
		} SUPEROBJ_5;

typedef struct {
	Byte bogus[sizeof(SUPEROBJ_5)];
} SUPEROBJECT_N105, *PSUPEROBJECT_N105;


/* --------------------------------------------------------------------------- CONTEXT -- */

typedef struct {
	Boolean		visible:1;			/* True if (staffVisible && measureVisible) */
	Boolean		staffVisible:1;		/* True if staff is visible */
	Boolean		measureVisible:1;	/* True if measure is visible */
	Boolean		inMeasure:1;		/* True if currently in measure */
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
} CONTEXT_5, *PCONTEXT_5;


#pragma options align=reset
