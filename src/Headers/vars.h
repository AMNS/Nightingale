/******************************************************************************************
*	FILE:	vars.h
*	PROJ:	Nightingale
*	DESC:	global variables
*******************************************************************************************/

/*
#ifdef MAIN
	#define GLOBAL
#else
	#define GLOBAL extern
#endif
*/


/* ============================================================ Without Initialization == */

/* The HEAP pointers below are indexed by type; therefore, the list _must_ agree with the
list of types in NBasicTypes.h, among other things! See comments there. */

GLOBAL HEAP 		*Heap;					/* Pointer to current heap list */
GLOBAL HEAP			*PARTINFOheap,			/* Precomputed pointers into above, indexed by type */
					*TAILheap,
					*NOTEheap,
					*RPTENDheap,
					*PAGEheap,
					*SYSTEMheap,
					*STAFFheap,
					*MEASUREheap,
					*CLEFheap,
					*KEYSIGheap,
					*TIMESIGheap,
					*NOTEBEAMheap,
					*CONNECTheap,
					*DYNAMheap,
					*MODNRheap,
					*GRAPHICheap,
					*NOTEOTTAVAheap,
					*SLURheap,
					*NOTETUPLEheap,
					*GRNOTEheap,
					*TEMPOheap,
					*SPACERheap,
					*ENDINGheap,
					*PSMEASheap,
					*OBJheap;				/* Object heap _must_ be the last heap. */
			
GLOBAL SysEnvRec	thisMac;				/* Our machine environment info */
GLOBAL OSType		creatorType;			/* Application signature ('BYRD') */
GLOBAL OSType		documentType;			/* Document file signature */

GLOBAL LINK			clipFirstMeas;			/* first Measure of clipboard */
GLOBAL Boolean		doneFlag,				/* False until program is done */
					cursorValid,			/* True while cursor is valid */
					bestQualityPrint,		/* "Best Quality Print" requested (on ImageWriter)? */
					toolPalChanged;			/* True if user has rearranged tool palette */
GLOBAL CursHandle	handCursor,				/* various cursors */
					threadCursor,
					genlDragCursor,
					arrowCursor,
					currentCursor,			/* The current cursor */
					oldCursor,				/* for enter key arrow toggle */
					shook,					/* for "mouse shaking" */
					cursorList[MAX_CURSORS]; /* Cursors for inserting music symbols (SIZE MUST BE >=nsyms!) */

GLOBAL char 		palChar,				/* current palette character */
					oldKey,					/* for enter key arrow toggle */
					*strBuf,				/* General temporary string buffer */
					applVerStr[20],			/* Application version no. (C string) */
					*checkDrawStr,			/* Key equivs. for interrupting drawing */
					*endingString;			/* Ending labels */
GLOBAL SignedByte	accTable[MAX_STAFFPOS]; /* Acc./pitch modif. table, by staff ln/space */
GLOBAL char			singleBarInChar,		/* Input character for single barline */
					qtrNoteInChar;			/* Input character for quarter note */
GLOBAL SignedByte	lastCopy;				/* = COPYTYPE_XXX; most recently copied objects */
GLOBAL short		sonataFontNum,			/* font ID number of Sonata font */
					textFontNum,			/* font ID number of normal (non-dialog) text font */
					textFontSize,			/* "Normal" size */
					textFontSmallSize,
					l2p_durs[MAX_L_DUR+1],	/* Logical-to-physical dur. lookup table */
					dynam2velo[LAST_DYNAM],	/* Dynamic-mark-to-MIDI-velocity table */
					modNRVelOffsets[32],	/* Modifier MIDI velocity offset table (mod codes indices) */
					modNRDurFactors[32],	/* Modifier duration factor (% of note's play duration) (mod codes indices) */
				/*	modNRTimeFactors[32],*/	/* (unused) Modifier time factor (% of ??) (mod codes indices) */
					magShift,				/* Shift count for coord. conversion for TopDocument */
					magRound,				/* Addend for rounding in coord. conversion for TopDocument */ 
					magMagnify,				/* Copy of <doc->magnify> for coord. conversion for TopDocument */
					outputTo,				/* Current output device: screen or printer type */
					firstDynSys,lastDynSys; /* firstSys, lastSys for hairpins */

GLOBAL DDIST		drSize[MAXRASTRAL+1];	/* Sizes for staff rastral nos. */
GLOBAL GridRec		*grid;					/* Character grid for Tool Palette */
//GLOBAL short		maxMCharWid, maxMCharHt; /* (unused) Max. size of any music char. in any view */
GLOBAL GrafPtr		toolPalPort;			/* Offscreen port to hold copy of tool palette picture */

GLOBAL Rect			revertWinPosition;		/* Where to replace Document window */
GLOBAL short		theSelectionType;		/* Current selection type (for autoscrolling) */
GLOBAL short		dragOffset;				/* Diff between measureRect and portRect */
GLOBAL CONTEXT		*contextA;				/* Allocate context[MAXSTAVES+1] on the heap */
GLOBAL Boolean		initedBIMIDI;			/* True=hardware/interrupts set for MIDI use, False=normal */
GLOBAL short		portSettingBIMIDI;		/* Port to use for built-in MIDI */	
GLOBAL short		interfaceSpeedBIMIDI;	/* Interface speed for built-in MIDI */
GLOBAL short		maxEndingNum;			/* Maximum Ending number available */

GLOBAL long			appDirID;				/* directory ID of application */
GLOBAL short		appVRefNum,				/* volume refNum of application (new style??) */
		   			appRFRefNum,			/* refNum of application resource file (old style??) */
					setupFileRefNum;		/* refNum of setup file */
GLOBAL LINK			drag1stMeas,			/* bitmap's meas range for Graphics, Tempos, &c. */
					dragLastMeas;

GLOBAL DDIST		initStfTop1, initStfTop2;	/* FIXME: SHOULD BE doc-SPECIFIC */
GLOBAL Pattern		diagonalDkGray,
					diagonalLtGray, otherLtGray;

GLOBAL Byte			useWhichMIDI;			/* MIDIDR_CM is MacOS Carbon MIDI or MIDIDR_NONE */
GLOBAL short		timeMMRefNum;			/* MIDI Manager time base port reference number */ 
GLOBAL short		inputMMRefNum;			/* MIDI Manager input port reference number */ 
GLOBAL short		outputMMRefNum;			/* MIDI Manager output port reference number */
GLOBAL Byte			*mPacketBuffer;			/* Buffer of MIDI Manager-compatible MIDI Packets */
GLOBAL long			mPacketBufferLen;		/* Length of mPacketBuffer */
GLOBAL long			mRecIndex;				/* For our MIDI Mgr readHook or built-in MIDI */		
GLOBAL long			mFirstTime;				/* For our MIDI Mgr readHook: time stamp of 1st data message */
GLOBAL long			mFinalTime;				/* For our MIDI Mgr readHook: time stamp of last data message */
GLOBAL Boolean		recordingNow;			/* True = MIDI recording in progress */
GLOBAL Boolean		recordFlats;			/* Use flats for black key notes from MIDI, else sharps */
GLOBAL short		playTempoPercent;		/* For "variable-speed playback": scale marked tempi by this */
GLOBAL Boolean		thinLines;				/* On PostScript output, linewidth(s) set dangerously thin */
GLOBAL short		debugLevel[5];			/* 0 = Special debug features (mostly logging); 0 = none */

GLOBAL CharRectCache charRectCache;
GLOBAL GrafPtr		underBits, offScrBits, picBits;	/* Bitmaps for dragging */

GLOBAL Rect 		selOldRect;				/* For threader selection mode. */

GLOBAL Byte 		clickMode;				/* Specify which selection mode */

GLOBAL Rect 		ccrossbar;				/* Crossbar rect for inserting notes */

GLOBAL Document		*currentDoc;			/* Document whose heaps are currently installed */

GLOBAL CursHandle	slursorHandle, controlBoxHandle, hairpinHandle;
GLOBAL LINK 		*thisSlur, *restoreSlur, slurTable;
GLOBAL Rect 		outerLimits;

GLOBAL MusFontRec	*musFontInfo;			/* Array of MusFontRec, for support of music fonts other than Sonata */
GLOBAL short		numMusFonts;			/* Number of MusFontRec items in <musFontInfo> array */

GLOBAL ScrapRef 	gNightScrap;

GLOBAL Boolean		gCoreMIDIInited;

GLOBAL Boolean		unisonsOK;				/* If True, don't object to unisons (perfect or augmented) in a chord */

GLOBAL Boolean		ignoreChord[MAX_MEASNODES][MAXVOICES+1];

GLOBAL NBMP			bmpDynamicPal;			/* Bitmaps for palettes */
GLOBAL NBMP			bmpModifierPal;
GLOBAL NBMP			bmpDurationPal;

GLOBAL Rect			durPalCell[DP_NROWS*DP_NCOLS];

/* =============================================================== With Initialization == */

#ifdef MAIN

/* <symtable> gives information for <symcode>, the input key code, i.e., it gives the
mapping from input character codes to musical symbols. A mapping from palette squares to
key codes is specified by a 'PLCH' resource (see Palette.c for details).

Re <symcode>: (1) Note that chars. that aren't in 7-bit ASCII may display differently in
different fonts and may generate different code on different platforms: to avoid these
problems, they should be given in hex. (2) Our palette re-mapping feature can override
the chars. given here (via a 'PLMP' resource), and in fact the default re-mapping as of
v. 3.5b8 makes dozens of changes, so caveat! */
 
SYMDATA symtable[] = {
		/* cursor	objtype   	subtype			symcode durcode */
		{ 99,		0,			0,				'~',	0 },		/* (dummy) */
		{ 201,		SYNCtype,	0,				0xDD,	1 },		/* Double-whole (breve) note */ 
		{ 202,		SYNCtype,	0,				'w',	2 },		/* Whole note */ 
		{ 203,		SYNCtype,	0,				'h',	3 },		/* Half note */
		{ 204,		SYNCtype,	0,				'q',	4 },		/* Quarter note */
		{ 205,		SYNCtype,	0,				'e',	5 },		/* Eighth note */
		{ 206,		SYNCtype,	0,				'x',	6 },		/* 16th note */
		{ 207,		SYNCtype,	0,				'r',	7 },		/* 32nd note */
		{ 208,		SYNCtype,	0,				't',	8 },		/* 64th note */
		{ 209,		SYNCtype,	0,				'y',	9 },		/* 128th note */
		{ 404,		GRSYNCtype,	0,				0xCE,	4 },		/* Quarter grace note */
		{ 405,		GRSYNCtype,	0,				0xE4,	5 },		/* Eighth grace note */
		{ 406,		GRSYNCtype,	0,				0xF4,	6 },		/* 16th grace note */
		{ 407,		GRSYNCtype,	0,				0xE5,	7 },		/* 32nd grace note */
		{ 210,		CLEFtype,	TREBLE_CLEF,	'&',	0 },		/* Treble clef */
		{ 211,		CLEFtype,	ALTO_CLEF,		'B',	0 },		/* C clef */
		{ 212,		CLEFtype,	BASS_CLEF,		'?',	0 },		/* Bass clef */
		{ 213,		CLEFtype,	PERC_CLEF,		'=',	0 },		/* Percussion clef */
		{ 214,		CLEFtype,	TRTENOR_CLEF,	0xE0,	0 },		/* Treble-tenor (treble w/8 below) clef */
		{ 215,		CLEFtype,	TREBLE8_CLEF,	'a',	0 },		/* Treble clef w/8 above */
		{ 216,		CLEFtype,	BASS8B_CLEF,	'b',	0 },		/* Bass clef w/8 below */
		{ 99,		KEYSIGtype,	0,				0xBA,	0 },		/* Double-flat */
		{ 99,		KEYSIGtype,	0,				'b',	0 },		/* (Unused) */
		{ 99,		KEYSIGtype,	0,				'n',	0 },		/* (Unused) */
		{ 217,		KEYSIGtype,	0,				'#',	0 },		/* Key signature */
		{ 99,		KEYSIGtype,	0,				0xDC,	0 },		/* Double-sharp */
		{ 219,		TIMESIGtype,1,				'8',	0 },		/* Time signature. Cf. TIMESIG_PALCHAR! */
		{ 220,		MEASUREtype,BAR_SINGLE,		'l',	0 },		/* Single barline */
		{ 221,		MEASUREtype,BAR_DOUBLE,		0xD5,	0 },		/* Double barline */
		{ 222,		MEASUREtype,BAR_FINALDBL,	0xD3,	0 },		/* Final double barline */
		{ 223,		PSMEAStype, PSM_DOTTED,		0xF1,	0 },		/* Dotted barline */
		{ 226,		DYNAMtype,	PPP_DYNAM,		0xB8,	0 },		/* "ppp" */
		{ 230,		DYNAMtype,	PP_DYNAM,		0xB9,	0 },		/* "pp" */
		{ 231,		DYNAMtype,	P_DYNAM,		'p',	0 },		/* "p" */
		{ 232,		DYNAMtype,	MP_DYNAM,		'P',	0 },		/* "mp" */
		{ 233,		DYNAMtype,	MF_DYNAM,		'F',	0 },		/* "mf" */
		{ 234,		DYNAMtype,	F_DYNAM,		'f',	0 },		/* "f" */
		{ 235,		DYNAMtype,	FF_DYNAM,		0xC4,	0 },		/* "ff" */
		{ 1236,		DYNAMtype,	FFF_DYNAM,		0xEC,	0 },		/* "fff" */
		{ 236,		DYNAMtype,	SF_DYNAM,		'S',	0 },		/* "sf" */
		{ 237,		DYNAMtype,	DIM_DYNAM,		'>', 	0 },		/* Hairpin dynamic (left) */
		{ 238,		DYNAMtype,	CRESC_DYNAM,	'<', 	0 },		/* Hairpin dynamic (right)--MUST immediately follow left */
		{ 240,		MODNRtype,	MOD_FAKE_AUGDOT,'.',	0 },		/* Augmentation dot */
		{ 241,		SYNCtype,	1,				'@',	1 },		/* Double-whole (breve) rest */ 
		{ 242,		SYNCtype,	1,				'W',	2 },		/* Whole rest */ 
		{ 243,		SYNCtype,	1,				'H',	3 },		/* Half rest */
		{ 244,		SYNCtype,	1,				'Q',	4 },		/* Quarter rest */
		{ 245,		SYNCtype,	1,				'E',	5 },		/* Eighth rest */
		{ 246,		SYNCtype,	1,				'X',	6 },		/* 16th rest */
		{ 247,		SYNCtype,	1,				'R',	7 },		/* 32nd rest */
		{ 248,		SYNCtype,	1,				'T',	8 },		/* 64th rest */
		{ 249,		SYNCtype,	1,				'Y',	9 },		/* 128th rest */
		{ 256,		SYNCtype,	2,				'6',	4 },		/* Chord slash: inserting acts like rest */
		{ 260,		SLURtype,	0,				'`',	0 },		/* Slur tool */
		{ 271,		MODNRtype,	MOD_FERMATA,	'U',	0 },		/* Note modifier: fermata */
		{ 271,		MODNRtype,	MOD_TRILL,		'u',	0 },		/* Note modifier: trill */
		{ 271,		MODNRtype,	MOD_ACCENT,		'\'',	0 },		/* Note modifier: common accent */
		{ 271,		MODNRtype,	MOD_HEAVYACCENT,'^',	0 },		/* Note modifier: heavy accent */
		{ 271,		MODNRtype,	MOD_STACCATO,	';',	0 },		/* Note modifier: staccato dot */
		{ 271,		MODNRtype,	MOD_WEDGE,		0xB7,	0 },		/* Note modifier: wedge accent */
		{ 271,		MODNRtype,	MOD_TENUTO,		'-',	0 },		/* Note modifier: tenuto */
		{ 271,		MODNRtype,	MOD_MORDENT,	0xB5,	0 },		/* Note modifier: mordent */
		{ 271,		MODNRtype,	MOD_INV_MORDENT,'m',	0 },		/* Note modifier: inverted mordent */
		{ 271,		MODNRtype,	MOD_TURN,		0xA0,	0 },		/* Note modifier: turn */
		{ 271,		MODNRtype,	MOD_PLUS,		'+',	0 },		/* Note modifier: plus */
		{ 271,		MODNRtype,	MOD_CIRCLE,		'o',	0 },		/* Note modifier: circle (harmonics,etc.) */
		{ 271,		MODNRtype,	MOD_UPBOW,		'V',	0 },		/* Note modifier: upbow */
		{ 271,		MODNRtype,	MOD_DOWNBOW,	'D',	0 },		/* Note modifier: downbow */
		{ 271,		MODNRtype,	MOD_TREMOLO1,	'/',	0 },		/* Note modifier: tremolo */
		{ 271,		MODNRtype,	MOD_HEAVYACC_STACC,0xDF,0 },		/* Note modifier: heavy acc+staccato */
		{ 271,		MODNRtype,	MOD_LONG_INVMORDENT,0xA3,0 },		/* Note modifier: long inv.mordent */
		{ 271,		MODNRtype,	0,				'0',	0 },		/* Note modifier: fingering 0 */
		{ 271,		MODNRtype,	1,				'1',	0 },		/* Note modifier: fingering 1 */
		{ 271,		MODNRtype,	2,				'2',	0 },		/* Note modifier: fingering 2 */
		{ 271,		MODNRtype,	3,				'3',	0 },		/* Note modifier: fingering 3 */
		{ 271,		MODNRtype,	4,				'4',	0 },		/* Note modifier: fingering 4 */
		{ 271,		MODNRtype,	5,				'5',	0 },		/* Note modifier: fingering 5 */
		{ 280,		GRAPHICtype,GRString,		'A',	0 },		/* Text */
		{ 281,		GRAPHICtype,GRRehearsal,	'c',	0 },		/* Rehearsal mark */
		{ 282,		GRAPHICtype,GRMIDIPatch,	'z',	0 },		/* MIDI patch change */
		{ 280,		GRAPHICtype,GRChordSym,		'7',	0 },		/* Chord symbol */
		{ 271,		GRAPHICtype,GRArpeggio,		'g',	ARP },		/* Arpeggio sign */
		{ 271,		GRAPHICtype,GRArpeggio,		'G',	NONARP },	/* Non-arpeggio sign */
		{ 275,		GRAPHICtype,GRDraw,			'_',	0 },		/* Line */
		{ 341,		GRAPHICtype,GRSusPedalDown,	0xB6,	0 },		/* Music char. "Ped." (pedal down = sustain) */
		{ 342,		GRAPHICtype,GRSusPedalUp,	0xFA,	1 },		/* Music char. pedal up */
		{ 340,		GRAPHICtype,GRMIDIPan,		0x7C,	1 },		/* MIDI Pan Controller */
		{ 271,		GRAPHICtype,GRChordFrame,	0xB1,	0 },		/* Chord frame */
		{ 300,		MEASUREtype, BAR_RPT_L,		'[',	0 },		/* left repeat */
		{ 301,		MEASUREtype, BAR_RPT_R,		']',	0 },		/* right repeat */
		{ 302,		MEASUREtype, BAR_RPT_LR,	'{',	0 },		/* two-headed repeat monster */
		{ 310,		TEMPOtype,	0,				'M',	0 },		/* Tempo/metronome mark */
		{ 320,		SPACERtype,	0,				'O',	0 },		/* Insert space tool: symcode is capital 'o', not zero */
		{ 330,		ENDINGtype,	0,				'!',	0 },		/* Ending */
		{ THREAD_CURS, 0,		0,				'd',	0 },		/* Threader tool */
		{ GENLDRAG_CURS, 0,		0,				'j',	0 }			/* General object dragging tool */
};

short nsyms=(sizeof(symtable)/sizeof(SYMDATA));			/* Length of symtable */ 

/*	objTable gives information for each object type */
 
OBJDATA objTable[] = {
	/*	objtype  	justType	minEnt	maxEnt		objRectOrd */
	{ HEADERtype,	0,			2,		MAXSTAVES+1,False },
	{ TAILtype,		J_IT,		0,		0,			False },
	{ SYNCtype,		J_IT,		1,		255,		True },
	{ RPTENDtype,	J_IT,		1,		MAXSTAVES,	True },
	{ PAGEtype,		J_STRUC,	0,		0,			False },

	{ SYSTEMtype,	J_STRUC,	0,		0,			False },
	{ STAFFtype,	J_STRUC,	1,		MAXSTAVES,	False },
	{ MEASUREtype,	J_IT,		1,		MAXSTAVES,	True },
	{ CLEFtype,		J_IP,		1,		MAXSTAVES,	True },
	{ KEYSIGtype,	J_IP,		1,		MAXSTAVES,	True },

	{ TIMESIGtype,	J_IP,		1,		MAXSTAVES,	True },
	{ BEAMSETtype,	J_D,		2,		127,		False },
	{ CONNECTtype,	J_D,		1,		MAXSTAVES,	True },
	{ DYNAMtype,	J_D,		1,		MAXSTAVES,	False },
	{ MODNRtype,	0,			0,		0,			False },

	{ GRAPHICtype,	J_D,		1,		255,		False },
	{ OTTAVAtype,	J_D,		1,		MAXINOTTAVA,True },
	{ SLURtype,		J_D,		1,		MAXCHORD,	False },
	{ TUPLETtype,	J_D,		2,		127,		False },
	{ GRSYNCtype,	J_IP,		1,		255,		True },
	{ TEMPOtype,	J_D,		0,		0,			False },
	{ SPACERtype,	J_IT,		0,		0,			True },	
	{ ENDINGtype,	J_D,		0,		0,			False },
	{ PSMEAStype, 	J_IT,		1,		MAXSTAVES,	True },	
	{ OBJtype,		J_STRUC,	0,		0,			False }
};

/* The point sizes we really want for staff rastral sizes, as given, e.g., in Ted
Ross' book, are:

FASTFLOAT rSize[MAXRASTRAL+1] =							Sizes for staff rastral nos.
	{26, 21.6, 20, 18.8, 18, 16.4, 14.2, 12.4, 10;		in points

But we use the following approximations, which are more practical with the Mac's
nominal screen resolution of 72 dpi, or 1 point. NB: pdrSize[0] is normally replaced
by a value from the CNFG resource. */

short pdrSize[MAXRASTRAL+1] =							/* Sizes for staff rastral nos., */
	{ 28, 24, 20, 19, 18, 16, 14, 12, 10};				/*  in points */ 

/* Sets of related characters in Adobe's Sonata and compatibles (individual characters
are declared in Defs.h) */

unsigned char SonataAcc[6] = { ' ', 0xBA,'b','n','#',0xDC };	/* Accidentals */
unsigned char MCH_idigits[10] =									/* Small italic digits for tuplets, etc. */
	{ 0xBC, 0xC1, 0xAA, 0xA3, 0xA2, 0xB0, 0xA4, 0xA6, 0xA5, 0xBB };

unsigned char MCH_notes[MAX_L_DUR] =
	{ MCH_breveNoteHead, MCH_wholeNoteHead, MCH_halfNoteHead, MCH_quarterNoteHead,
	  MCH_quarterNoteHead, MCH_quarterNoteHead, MCH_quarterNoteHead,
	  MCH_quarterNoteHead, MCH_quarterNoteHead };
unsigned char MCH_rests[MAX_L_DUR] =
	{ 0xE3, 0xB7, 0xEE, 0xCE, 0xE4, 0xC5, 0xA8, 0xF4, 0xE5 };

/* Coarse correction to font for rest Y-positions (interline half-spaces) */

short restYOffset[MAX_L_DUR+1] = 
					{ 0, 0, 0, 0, 0, -1, 1, 1, 3, 3 };

short noteOffset[] = { 7, 14, 21, -7, -14, -21 };			/* Vert. offset for ottavas, in half-spaces */

/*	Text sizes, in interline spaces:
                             Tiny VSmall Small Medium Large VLarge Jumbo ------- StaffHt */
							 
FASTFLOAT relFSizeTab[] =  { 1.0, 1.5,   1.7,  2.0,   2.2,	2.5,   3.0,  3.6, 0,   4.0  };

/* Subobject sizes in memory and in current-format files. */

short subObjLength[] = {
		sizeof(PARTINFO),		/* HEADER subobject */
		0,						/* No TAIL subobjects */
		sizeof(ANOTE),			/* SYNC subobject */
		sizeof(ARPTEND),		/* RPTEND subobject */
		0,						/* etc. */
		0,
		sizeof(ASTAFF),
		sizeof(AMEASURE),
		sizeof(ACLEF),
		sizeof(AKEYSIG),
		sizeof(ATIMESIG),
		sizeof(ANOTEBEAM),
		sizeof(ACONNECT),
		sizeof(ADYNAMIC),
		sizeof(AMODNR),
		sizeof(AGRAPHIC),
		sizeof(ANOTEOTTAVA),
		sizeof(ASLUR),
		sizeof(ANOTETUPLE),
		sizeof(AGRNOTE),
		0,						/* No TEMPO subobjects */
		0,						/* etc. */
		0,
		sizeof(APSMEAS),
		sizeof(SUPEROBJECT)
	};
		
/* Object sizes in memory (a fixed size, that of SUPEROBJECT) and in current-format files
(size dependent on object type). */

short objLength[] = {
		sizeof(HEADER),
		sizeof(TAIL),
		sizeof(SYNC),
		sizeof(RPTEND),
		sizeof(PAGE),
		sizeof(SYSTEM),
		sizeof(STAFF),
		sizeof(MEASURE),
		sizeof(CLEF),
		sizeof(KEYSIG),
		sizeof(TIMESIG),
		sizeof(BEAMSET),
		sizeof(CONNECT),
		sizeof(DYNAMIC),
		0, 						/* No MODNR objects */
		sizeof(GRAPHIC),
		sizeof(OTTAVA),
		sizeof(SLUR),
		sizeof(TUPLET),
		sizeof(GRSYNC),
		sizeof(TEMPO),
		sizeof(SPACER),
		sizeof(ENDING),
		sizeof(PSMEAS),
		sizeof(SUPEROBJECT)
	};


/* Subobject sizes in 'N105' format files. */

short subObjLength_5[] = {
		sizeof(PARTINFO),		/* HEADER subobject */
		0,						/* TAIL objects can't have subobjects */
		sizeof(ANOTE_5),		/* SYNC subobject */
		sizeof(ARPTEND_5),		/* etc. */
		0,
		0,
		sizeof(ASTAFF_5),
		sizeof(AMEASURE_5),
		sizeof(ACLEF_5),
		sizeof(AKEYSIG_5),
		sizeof(ATIMESIG_5),
		sizeof(ANOTEBEAM_5),
		sizeof(ACONNECT_5),
		sizeof(ADYNAMIC_5),
		sizeof(AMODNR_5),
		sizeof(AGRAPHIC_5),
		sizeof(ANOTEOTTAVA_5),
		sizeof(ASLUR_5),
		sizeof(ANOTETUPLE),
		sizeof(AGRNOTE_5),
		0,
		0,
		0,
		sizeof(APSMEAS_5),
		sizeof(SUPEROBJECT_N105)
	};
		
/* Object sizes in memory (a fixed size, that of SUPEROBJECT_N105) and in 'N105' format
files (size dependent on object type). */

short objLength_5[] = {
		sizeof(HEADER_5),
		sizeof(TAIL_5),
		sizeof(SYNC_5),
		sizeof(RPTEND_5),
		sizeof(PAGE_5),
		sizeof(SYSTEM_5),
		sizeof(STAFF_5),
		sizeof(MEASURE_5),
		sizeof(CLEF_5),
		sizeof(KEYSIG_5),
		sizeof(TIMESIG_5),
		sizeof(BEAMSET_5),
		sizeof(CONNECT_5),
		sizeof(DYNAMIC_5),
		0, 							/* No MODNR objects */
		sizeof(GRAPHIC_5),
		sizeof(OTTAVA_5),
		sizeof(SLUR_5),
		sizeof(TUPLET_5),
		sizeof(GRSYNC_5),
		sizeof(TEMPO_5),
		sizeof(SPACER_5),
		sizeof(ENDING_5),
		sizeof(PSMEAS_5),
		sizeof(SUPEROBJECT_N105)
	};


GLOBAL SignedByte threadableType=SYNCtype;

GLOBAL short durPalCode[] = { 
	ONE28TH_L_DUR, SIXTY4TH_L_DUR, THIRTY2ND_L_DUR, SIXTEENTH_L_DUR, EIGHTH_L_DUR, QTR_L_DUR, HALF_L_DUR, WHOLE_L_DUR, BREVE_L_DUR,
	NO_L_DUR, SIXTY4TH_L_DUR, THIRTY2ND_L_DUR, SIXTEENTH_L_DUR, EIGHTH_L_DUR, QTR_L_DUR, HALF_L_DUR, WHOLE_L_DUR, BREVE_L_DUR,
	UNKNOWN_L_DUR, NO_L_DUR, THIRTY2ND_L_DUR, SIXTEENTH_L_DUR, EIGHTH_L_DUR, QTR_L_DUR, HALF_L_DUR, WHOLE_L_DUR, BREVE_L_DUR
	};

GLOBAL short durPalNDots[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 2, 2, 2, 2, 2, 2, 2, 2
	};

#else

GLOBAL SYMDATA symtable[];
GLOBAL short nsyms;
GLOBAL OBJDATA objTable[];
GLOBAL short pdrSize[];
GLOBAL unsigned char SonataAcc[];
GLOBAL unsigned char MCH_idigits[];
GLOBAL unsigned char MCH_notes[];
GLOBAL unsigned char MCH_rests[];
GLOBAL short restYOffset[];

GLOBAL short noteOffset[];
GLOBAL FASTFLOAT relFSizeTab[];

GLOBAL short subObjLength[];
GLOBAL short objLength[];
GLOBAL short subObjLength_5[];
GLOBAL short objLength_5[];

GLOBAL SignedByte threadableType;

GLOBAL short durPalCode[];
GLOBAL short durPalNDots[];

#endif


/* ============================================================================= enums == */

enum { ClickErase, ClickSelect, ClickInsert, ClickFrame };
