/*	FileConversion.h - data structure elements for older file versions */

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-2001 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */

#define MAX_SCOREFONTS_N102	10
#define MAX_COMMENT_LEN_N102	35

//#if TARGET_CPU_PPC
	#pragma options align=mac68k
//#endif

#define NIGHTSCOREHEADER_N102																				\
	LINK 			headL,				/* links to header and tail objects */						\
					tailL,																						\
					selStartL,			/* currently selected range. */								\
					selEndL;				/*		Also see the <selStaff> field. */					\
																													\
	INT16			nstaves,				/* number of staves in a system */							\
					nsystems;			/* number of systems in score */								\
	unsigned char comment[MAX_COMMENT_LEN_N102+1]; /* User comment on score */				\
	char			feedback:1;			/* TRUE if we want feedback on note insert */			\
	char			dontSendPatches:1; /* 0 = when playing, send patch changes for channels */	\
	char			saved:1;				/* TRUE if score has been saved */							\
	char			named:1;				/* TRUE if file has been named */							\
	char 			used:1;				/* TRUE if score contains any nonstructural info */	\
	char			transposed:1;		/* TRUE if transposed score, else C score */				\
	char			lyricText:1;		/* (no longer used) TRUE if last text entered was lyric */	\
	char			polyTimbral:1;		/* TRUE for one part per MIDI channel */					\
	Byte			currentPage;		/* (no longer used) */											\
	INT16			spacePercent,		/* Percentage of normal horizontal spacing used */		\
					srastral,			/* Staff size rastral no.--score */							\
					altsrastral,		/* (unused) Staff size rastral no.--parts */				\
					tempo,				/* playback speed in beats per minute */					\
					channel,				/* Basic MIDI channel number */								\
					velocity;			/* global playback velocity offset */						\
	STRINGOFFSET headerStr;			/* index returned by String Manager */	\
	STRINGOFFSET footerStr;			/* index returned by String Manager */	\
	char			topPGN:1;			/* TRUE=page numbers at top of page, else bottom */	\
	char			hPosPGN:3;			/* 1=page numbers at left, 2=center, 3=at right */		\
	char			alternatePGN:1;	/* TRUE=page numbers alternately left and right */		\
	char			useHeaderFooter:1; /* TRUE=use header/footer text, not simple pagenum */\
	char			fillerPGN:2;		/* unused */														\
	SignedByte	fillerMB;			/* unused */														\
	DDIST			filler2,				/* unused */														\
					otherIndent;		/* Amount to indent Systems other than first */			\
	SignedByte	firstNames,			/* Code for drawing part names: see enum above */		\
					otherNames,			/* Code for drawing part names: see enum above */		\
					lastGlobalFont,	/* Header index of most recent text style used */		\
					xMNOffset,			/* Horiz. pos. offset for meas. nos. (half-spaces) */	\
					yMNOffset,			/* Vert. pos. offset for meas. nos. (half-spaces) */	\
					xSysMNOffset;		/* Horiz. pos. offset for meas.nos.if 1st meas.in system */ \
	short			aboveMN:1,			/* TRUE=measure numbers above staff, else below */		\
					sysFirstMN:1,		/* TRUE=indent 1st meas. of system by xMNOffset */		\
					startMNPrint1:1,	/* TRUE=First meas. number to print is 1, else 2 */	\
					firstMNNumber:13;	/* Number of first measure */									\
	LINK			masterHeadL,		/* Head of Master Page object list */						\
					masterTailL;		/* Tail of Master Page object list */						\
	SignedByte	filler1,																						\
					nFontRecords;		/* Always 11 for now */											\
	/* Eleven identical TEXTSTYLE records. Fontnames are Pascal strings. */					\
																													\
	unsigned char fontNameMN[32];	/* MEASURE NO. FONT: default name, size and style */	\
	unsigned short	fillerMN:5;																				\
	unsigned short	lyricMN:1;		/* TRUE if spacing like lyric (ignored) */				\
	unsigned short	enclosureMN:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSizeMN:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizeMN:7;	/* if relFSizeMN, small..large code, else point size */ \
	INT16				fontStyleMN;																			\
																													\
	unsigned char fontNamePN[32];	/* PART NAME FONT: default name, size and style */		\
	unsigned short	fillerPN:5;																				\
	unsigned short	lyricPN:1;		/* TRUE if spacing like lyric (ignored) */				\
	unsigned short	enclosurePN:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSizePN:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizePN:7;	/* if relFSizePN, small..large code, else point size */ \
	INT16				fontStylePN;																			\
																													\
	unsigned char fontNameRM[32];	/* REHEARSAL MARK FONT: default name, size and style */	\
	unsigned short	fillerRM:5;																				\
	unsigned short	lyricRM:1;		/* TRUE if spacing like lyric (ignored) */				\
	unsigned short	enclosureRM:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSizeRM:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizeRM:7;	/* if relFSizeRM, small..large code, else point size */ \
	INT16				fontStyleRM;																			\
																													\
	unsigned char fontName1[32];	/* REGULAR FONT 1: default name, size and style */		\
	unsigned short	fillerR1:5;																				\
	unsigned short	lyric1:1;		/* TRUE if spacing like lyric	*/								\
	unsigned short	enclosure1:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSize1:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize1:7;	/* if relFSize1, small..large code, else point size */ \
	INT16				fontStyle1;																				\
																													\
	unsigned char fontName2[32];	/* REGULAR FONT 2: default name, size and style */		\
	unsigned short	fillerR2:5;																				\
	unsigned short	lyric2:1;		/* TRUE if spacing like lyric	*/								\
	unsigned short	enclosure2:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSize2:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize2:7;	/* if relFSize2, small..large code, else point size */ \
	INT16				fontStyle2;																				\
																													\
	unsigned char fontName3[32];	/* REGULAR FONT 3: default name, size and style */		\
	unsigned short	fillerR3:5;																				\
	unsigned short	lyric3:1;		/* TRUE if spacing like lyric	*/								\
	unsigned short	enclosure3:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSize3:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize3:7;	/* if relFSize3, small..large code, else point size */ \
	INT16				fontStyle3;																				\
																													\
	unsigned char fontName4[32];	/* REGULAR FONT 4: default name, size and style */		\
	unsigned short	fillerR4:5;																				\
	unsigned short	lyric4:1;		/* TRUE if spacing like lyric	*/								\
	unsigned short	enclosure4:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSize4:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize4:7;	/* if relFSizeR4, small..large code, else point size */ \
	INT16				fontStyle4;																				\
																													\
	unsigned char fontNameTM[32];	/* TEMPO MARK FONT: default name, size and style */	\
	unsigned short	fillerTM:5;																				\
	unsigned short	lyricTM:1;		/* TRUE if spacing like lyric (ignored) */				\
	unsigned short	enclosureTM:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSizeTM:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizeTM:7;	/* if relFSizeTM, small..large code, else point size */ \
	INT16				fontStyleTM;																			\
																													\
	unsigned char fontNameCS[32];	/* CHORD SYMBOL FONT: default name, size and style */	\
	unsigned short	fillerCS:5;																				\
	unsigned short	lyricCS:1;		/* TRUE if spacing like lyric (ignored) */				\
	unsigned short	enclosureCS:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSizeCS:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizeCS:7;	/* if relFSizeCS, small..large code, else point size */ \
	INT16				fontStyleCS;																			\
																													\
	unsigned char fontNamePG[32];	/* PAGE HEADER/FOOTER/NO.FONT: default name, size and style */	\
	unsigned short	fillerPG:5;																				\
	unsigned short	lyricPG:1;		/* TRUE if spacing like lyric (ignored) */				\
	unsigned short	enclosurePG:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSizePG:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSizePG:7;	/* if relFSizePG, small..large code, else point size */ \
	INT16				fontStylePG;																			\
																													\
	unsigned char fontName5[32];	/* REGULAR FONT 5: default name, size and style */		\
	unsigned short	fillerR5:5;																				\
	unsigned short	lyric5:1;		/* TRUE if spacing like lyric (ignored) */				\
	unsigned short	enclosure5:2;	/* Enclosure: whether box, circular or none */			\
	unsigned short	relFSize5:1;	/* TRUE if size is relative to staff size */				\
	unsigned short	fontSize5:7;	/* if relFSize5, small..large code, else point size */ \
	INT16				fontStyle5;																				\
																													\
	/* End of TEXTSTYLE records */																		\
																													\
	INT16			nfontsUsed;			/* no. of entries in fontTable */							\
	FONTITEM		fontTable[MAX_SCOREFONTS_N102]; /* To convert stored to system font nos. */ \
																													\
	short			magnify,				/* Current reduce/enlarge magnification, 0=none */		\
					selStaff;			/* If sel. is empty, insertion pt's staff, else undefined */ \
	SignedByte	otherMNStaff,		/* (not yet used) staff no. for meas. nos. besides stf 1 */ \
					numberMeas;			/* Show measure nos.: -=every system, 0=never, n=every nth meas. */ \
	short			currentSystem,		/* systemNum of system which contains caret or, if non-empty sel, selStartL (but currently not defined) */	\
					spaceTable,			/* ID of 'SPTB' resource having spacing table to use */ \
					htight,				/* Percent tightness */											\
					fillerInt,			/* unused */														\
					lookVoice,			/* Voice to look at, or -1 for all voices */				\
					fillerHP,			/* (unused) */														\
					fillerLP,			/* (unused) */														\
					ledgerYSp,			/* Extra space above/below staff for max. ledger lines (half-spaces) */ \
					deflamTime;			/* Maximum time between consec. attacks in a chord (millsec.) */ \
																													\
	Boolean		autoRespace,		/* Respace on symbol insert, or leave things alone? */\
					insertMode,			/* Graphic insertion logic (else temporal)? */			\
					beamRests,			/* In beam handling, treat rests like notes? */			\
					pianoroll,			/* Display everything in pianoroll, not CMN, form? */	\
					showSyncs,			/* Show (w/HiliteInsertNode) lines on every sync? */	\
					frameSystems,		/* Frame systemRects (for debugging)? */					\
					fillerEM:4,			/* unused */														\
					colorVoices:2,		/* 0=normal, 1=show non-dflt voices in color */			\
					showInvis:1,		/* Display invisible objects? */								\
					showDurProb:1,		/* Show measures with duration/time sig. problems? */	\
					recordFlats;		/* TRUE if black-key notes recorded should use flats */ \
																													\
	long			spaceMap[MAX_L_DUR];	/* Ideal spacing of basic (undotted, non-tuplet) durs. */ \
	DDIST			firstIndent,		/* Amount to indent first System */							\
					yBetweenSys;		/* obsolete, was vert. "dead" space btwn Systems */	\
	VOICEINFO	voiceTab[MAXVOICES+1];	/* Descriptions of voices in use */					\
	short			expansion[256-(MAXVOICES+1)];


typedef struct {
	NIGHTSCOREHEADER_N102
} SCOREHEADER_N102, *PSCOREHEADER_N102;

// MAS: reset alignment
#pragma options align=reset

