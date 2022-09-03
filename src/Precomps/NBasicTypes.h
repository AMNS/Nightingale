/*	NBasicTypes.h - #defines and typedefs for Nightingale's basic datatypes. NB: Many of
these appear in Nightingale score files, so changing them may be a problem for backward
compatibility. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2019 by Avian Music Notation Foundation. All Rights Reserved.
 */

#ifndef TypesIncluded
#define TypesIncluded

#pragma options align=mac68k

/* -------------------------------- Data-structure Things ------------------------------- */

/* We assume already-included headers (on the Macintosh, MacTypes.h) say:
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
	char letcode;					/* LG: Code for letter name: A=5,B=4,C=3,D=2,E=1,F=0,G=6  */
	Boolean sharp;					/* LG: Is it a sharp? (False=flat) */
} KSITEM;

/* NB: We #define a WHOLE_KSINFO macro, then a struct that consists of nothing but an
invocation of that macro; why? I'm don't remember, but ancient comments here pointed out
that the macro takes 15 bytes, but the struct takes 16. So change with care!  --DAB */

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


/* The following enum gives the types of objects and subobjects Nightingale has. It
_must_ agree with the order of HEAPs given in vars.h! In addition, the generic subobjects
functions FirstSubObj(), NextSubObj() in Objects.c depend on the order of items in it.
Arrays subObjLength[] and objLength[], in vars.h, _must_ be updated if the enum is
changed! */

enum								/* Object types: */
{
			FIRSTtype = 0,
/* 0 */		HEADERtype=FIRSTtype,
			TAILtype,
			SYNCtype,				/* Note/rest Sync */
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
			GRSYNCtype,				/* Grace note Sync */
	
/* 20 */	TEMPOtype,
			SPACERtype,
			ENDINGtype,
			PSMEAStype,
			OBJtype,				/* Object heap _must_ be the last heap. */
			LASTtype
};

#define LOW_TYPE HEADERtype
#define HIGH_TYPE LASTtype

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
	   pre-OS X technology -- but we keep the fields for file compatibility. */
		
	fmsUniqueID	fmsOutputDevice;
	fmsDestinationMatch fmsOutputDestination;	/* NB: about 280 bytes */
} PARTINFO, *PPARTINFO;


#pragma options align=reset

#endif /* TypesIncluded */
