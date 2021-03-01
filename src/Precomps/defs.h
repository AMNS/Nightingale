/* Defs.h for Nightingale - general global #defines and enums. Most limits are in NLimits.h.
Some other #defines, dependent on _VERSION, are in versionStrings. */

/* -------------------------------------------------------- Miscellaneous global enums -- */

enum {									/* In the order of TEXTSTYLEs in NIGHTSCOREHDR */
	FONT_THISITEMONLY=0,
	FONT_MN,
	FONT_PN,
	FONT_RM,
	FONT_R1,
	FONT_R2,
	FONT_R3,
	FONT_R4,
	FONT_TM,
	FONT_CS,
	FONT_PG,
	FONT_R5,
	FONT_R6,
	FONT_R7,
	FONT_R8,
	FONT_R9
};

enum {									/* Selection types */
	MARCHING_ANTS = 1,
	SWEEPING_RECTS,
	SLURSOR
};

enum {									/* Goto types */
	goDirectlyToJAIL = 0,
	gotoPAGE,
	gotoBAR,
	gotoREHEARSALMARK
};

enum {
	WHOLEMR_L_DUR=-1,					/* l_dur code (subType) for whole measure rest */
	UNKNOWN_L_DUR=0,					/*							unknown CMN value */
	BREVE_L_DUR=1,						/*							breve */
	WHOLE_L_DUR,						/* 							whole note/rest */
	HALF_L_DUR,							/* 							half note/rest */
	QTR_L_DUR,							/* 							quarter note/rest */
	EIGHTH_L_DUR,						/*							eighth note/rest */
	SIXTEENTH_L_DUR,					/*							16th note/rest */
	THIRTY2ND_L_DUR,					/*							32nd note/rest */
	SIXTY4TH_L_DUR,						/*							64th note/rest */
	ONE28TH_L_DUR,						/*							128th note/rest */
	NO_L_DUR	
};

enum {									/* Accidental codes */
	AC_DBLFLAT=1,
	AC_FLAT,
	AC_NATURAL,
	AC_SHARP,
	AC_DBLSHARP
};

enum {									/* Output device types */
	toScreen=0,
	toBitmapPrint,						/* I.e., QuickDraw printer */
	toPostScript,
	toPICT,
	toVoid								/* (for debugging: do everything but actual drawing) */
};

enum {									/* Location for adding Systems and Measures: */
	FirstSystem,						/* Adding the very first System of the score, when it is created */
	BeforeFirstSys,						/* Adding before the first System of the score */
	FirstOnPage,						/* Adding a new first System to some page other than the first */
	SuccSystem,							/* Adding a System following the first to any page */
	SuccMeas							/* Adding a Measure following the first to any System */
};

enum {									/* DrawSTAFF foreground/background codes */
	BACKGROUND_STAFF=0,
	OTHERSYS_STAFF,
	SECONDSYS_STAFF,
	TOPSYS_STAFF
};

enum {
	LEFT_SIDE=1,						/* Page number position codes */
	CENTER,
	RIGHT_SIDE
};

enum {									/* Note/rest/grace note codes: */
	NOTES_ONLY,							/* Notes and rests */
	GRNOTES_ONLY,						/* Grace notes */
	NOTES_BOTH							/* All */
};

enum {									/* MIDI Driver: */
	MIDIDR_CM,							/* Core MIDI */
	MIDIDR_NONE							/* No MIDI driver */
};

enum {									/* MIDI Driver config settings: */
	MIDISYS_CM=0,						/* Core MIDI */
	MIDISYS_NONE
};

enum {									/*  Dialog buttons FIXME: lousy old names; should change!  */
	OK = 1,
	Cancel
};


/* ------------------------------------------------------------------ Music Characters -- */
/* Characters in our music font, Adobe's "Sonata" and compatible fonts: */

#define MCH_trebleclef '&'
#define MCH_cclef 'B'
#define MCH_bassclef '?'
#define MCH_percclef '/'

#define MCH_sharp '#'
#define MCH_flat 'b'
#define MCH_natural 'n'

#define MCH_common 'c'						/* Time signatures */
#define MCH_cut 'C'

#define MCH_breveNoteHead 0xDD				/* =shift-option-4 */
#define MCH_wholeNoteHead 'w'
#define MCH_halfNoteHead 0xFA				/* =option-h */
#define MCH_quarterNoteHead  0xCF			/* =option-q */
#define MCH_xShapeHead  0xC0
#define MCH_harmonicHead 'O'
#define MCH_squareHHead 0xAD
#define MCH_squareFHead 0xD0
#define MCH_diamondHHead 0xE1
#define MCH_diamondFHead 0xE2

#define MCH_eighthFlagUp 'j'
#define MCH_eighthFlagDown 'J'
#define MCH_16thFlagUp 'k'
#define MCH_16thFlagDown 'K'
#define MCH_16thNoteStemDown 'X'			/* complete stem-down 16th note */
#define MCH_extendFlagUp 0xFB				/* =option-K */
#define MCH_extendFlagDown 0xF0				/* =shift-option-K */
#define MCH_dot '.'							/* augmentation dot */
#define MCH_stemspace 0xCA					/* notehead minus stem width; =option-space */
#define MCH_graceSlash 'G'

#define MCH_fermata 'U'						/* Note modifiers: */
#define MCH_fermataBelow 'u'
#define MCH_fancyTrill '`'
#define MCH_accent '>'
#define MCH_heavyAccent '^'
#define MCH_heavyAccentBelow 'v'
#define MCH_staccato MCH_dot
#define MCH_wedge 0xAE						/* =shift-option-' */
#define MCH_wedgeBelow '\''
#define MCH_tenuto '-'
#define MCH_mordent 'M'
#define MCH_invMordent 'm'
#define MCH_turn 'T'
#define MCH_plus '+'
#define MCH_circle 'o'
#define MCH_upbow '²'
#define MCH_downbow '³'
#define MCH_heavyAccAndStaccato 0xAC		/* =option-u option-u */
#define MCH_heavyAccAndStaccatoBelow 0xE8	/* =shift-option-u */
#define MCH_longInvMordent 0xB5				/* =option-m */

#define MCH_ppp 0xB8						/* Dynamic marks: */
#define MCH_pp 0xB9
#define MCH_p 'p'
#define MCH_mp 'P'
#define MCH_mf 'F'
#define MCH_f 'f'
#define MCH_ff 0xC4
#define MCH_fff 0xEC
#define MCH_sf 'S'							/* "sf" */

#define MCH_topbracket 0xC2
#define MCH_bottombracket 'L'
#define MCH_braceup 167
#define MCH_bracedown 234
#define MCH_rptDots 0x7B
#define MCH_arpeggioSign 'g'

#define MCH_lParen '('						/* Parentheses */
#define MCH_rParen ')'

#define BREVEYOFFSET 2						/* Correction for Sonata error in breve origin (half-spaces) */

/* ----------------------------------------------------- Other Character and Key Codes -- */

#define CH_ENTER 0x03					/* ASCII character code for enter key */
#define CH_BS 0x08						/* ASCII character code for backspace (delete) */
#define CH_CR '\r'						/* ASCII (=C's) character code for carriage return  */
#define CH_NLDELIM '#'					/* ASCII code for "start a new line" where '\r' can't be used */
#define CH_ESC 0x1B						/* ASCII character code for ESC (escape) */
#define CH_HARDSPACE 0xCA				/* 8-bit (non-ASCII) character code for hard space */

#define CLEARKEY 27						/* Macintosh character code for clear key */
#define CMD_KEY 17						/* Macintosh character code for command key */
#define LEFTARROWKEY 28					/* Macintosh character codes for arrow keys... */
#define RIGHTARROWKEY 29
#define UPARROWKEY 30
#define DOWNARROWKEY 31
#define FWDDEL_KEY 127					/* Macintosh character code for forward delete key (on ext. kybds) */

#define CS_DELIMITER FWDDEL_KEY			/* Chord symbol field delimiter */

/* ----------------------------------------------------- Numeric Function Return Codes -- */
/* Return codes for numeric-value functions. FIXME: Add NRV_ prefix to all; move OP_
constants here from MiscUtils.h; use enum's for all or #define's for all. */

enum {
	FAILURE=-1,
	NOTHING_TO_DO=0,
	OP_COMPLETE=1
};

#define NRV_CANCEL -31999				/* Operation cancelled */
#define NRV_ERROR -32765				/* Error */
#define NRV_SUCCESS 999					/* All is well */

enum {
	NRV_C1_THEN_2=-899,
	NRV_CEQUAL,
	NRV_C2_THEN_1
};

/* ----------------------------------------------------------- Miscellaneous Constants -- */

#define FIDEAL_RESOLVE 10				/* Fractional STDIST resolution in parts of an STDIST */

#define POINTSPERIN 72					/* Points per inch (really about 72.27) */

#define PDURUNIT 15						/* p_dur code for shortest note,i.e., w/dur.code=MAX_L_DUR */

#define ACCTABLE_OFF 30					/* Staff half-space offset in accTable */

#define CTL_ACTIVE 0					/* Standard HiliteControl codes */
#define CTL_INACTIVE 255

#define ENDTEXT 999						/* "End of text" to SelIText */

#define TOO_MANY_MEASNODES 31999		/* Exceeded MAX_MEASNODES limit */

#define BIGNUM 1999999999L				/* A very large signed long, < LONG_MAX but not << */

#define ANYTYPE -1
#define ANYSUBTYPE -1

#define ANYONE -1						/* passed to search routines if we just don't care */
#define NOONE -2						/* returned by FindStaffSetSys, GetStaffFromSel, etc. */

#define NOMATCH -1

#define GO_LEFT True					/* used in calls to search routines */
#define GO_RIGHT False

#define EXACT_TIME True					/* used in calls to TimeSearch routines */
#define MIN_TIME False

#define DFLT_CLEF TREBLE_CLEF			/* Default clefType for new staves */
#define DFLT_NKSITEMS 0					/* Dflt. key sig. NB: !=0 has side effects: CAREFUL! */
#define DFLT_TSTYPE N_OVER_D			/* Default time sig. fields for new staves */
#define DFLT_DYNAMIC MF_DYNAM			/* Default dynamic marking for new staves */
#define DFLT_SPACETABLE 0				/* Default space table number */
#define DFLT_XMOVEACC 5					/* Default note <xMoveAcc> */

#define STFLINES 5						/* Number of lines in standard staff */
#define STFHALFLNS (STFLINES+STFLINES-2)	/* Height of standard staff, in half-spaces */

#define ENLARGE_NR_SELH 1				/* Enlarge note selection rect. (horiz. pixels) */
#define ENLARGE_NR_SELV 0				/* Enlarge note selection rect. (vert. pixels) */

/* NB: If (MAXSPACE*RESFACTOR) exceeds SHRT_MAX, the justification routines may have
have serious problems.  See especially RespaceBars and JustAddSpace. */
 
#define RESFACTOR 50L					/* Resolution factor for justification routines */

#define MESSAGEBOX_WIDTH 252			/* Width of msg area (for page no., measure no., voice, etc.) */

#define MARGWIDTH(doc)	pt2d(doc->marginRect.right-doc->marginRect.left)
#define MARGLEFT(doc)	pt2d(doc->marginRect.left)

#define HILITE_TICKS	12				/* Minimum hiliting time to show user structural relationships */ 

/* Document info for the Mac OS Finder. These are mostly ignored by OS X, so obsolescent. */

#define DOCUMENT_TYPE_NORMAL 'SCOR'
#define CREATOR_TYPE_NORMAL 'BYRD'		/* "signature" */
#define DOCUMENT_TYPE_VIEWER 'SCOV'
#define CREATOR_TYPE_VIEWER 'BYRV'		/* "signature" */

/* ---------------------------------------------------------------- MIDI and real-time -- */

/* Some MIDI parameters. Others are in the CNFG resource (and most of these should be
moved there eventually). */

/* When Importing MIDI files... */
#define MIDI_BASS_TOP (MIDI_MIDDLE_C+4)			/* If avg. noteNum>this, change from bass to treble */
#define MIDI_TREBLE_BOTTOM (MIDI_MIDDLE_C-3)	/* If avg. noteNum<this, change from treble to bass */

#define DFLT_BEATDUR (32L*PDURUNIT)				/* Quarter-note duration in PDUR ticks */

/* Convert PDUR ticks to milliseconds at constant tempo */

#define PDUR2MS(t, mbeats)	(long)((FASTFLOAT)(mbeats)*(FASTFLOAT)(t)/1000L) 

/* Convert PDUR ticks per minute to microseconds per PDUR tick */

#define TSCALE2MICROBEATS(ts) (60*1000000L/(ts))


/* -------------------------------------------------------------- Miscellaneous Macros -- */
/* FIXME: These should probably be in a new file, <GeneralMacros.h> or some such. */

/* Arithmetic, etc. */

#define ABS(a) ( (a)<0 ? -(a) : (a) )							/* absolute value function */
#define odd(a) ((a) & 1)										/* True if a is odd */

// CER: 02.19.2003 changed min, max to n_min, n_max
#define n_min(a,b)		( (a)<(b) ? (a) : (b) )					/* minimum functions */
#define min3(a, b, c)	( n_min(n_min((a), (b)), (c)) )
#define n_max(a,b)		( (a)>(b) ? (a) : (b) )					/* maximum functions */
#define max3(a, b, c)	( n_max(n_max((a), (b)), (c)) )
#define clamp(low, val, high)	( (val)<(low)?(low):((val)>(high)?(high):(val)) )

#define divby2(num) 	( (num)>=0? (num)>>1 : (-(-(num)>>1)) )	/* num must be an integer */
#define divby4(num) 	( (num)>=0? (num)>>2 : (-(-(num)>>2)) )	/* num must be an integer */

#define LXOR(a, b)  (((a)!=0) ^ ((b)!=0))						/* Logical exclusive or */

/* Round <n> up to an even number, i.e., if <n> is odd, return <n+1>, else <n>. */

#define ROUND_UP_EVEN(n) ((n) + ((n) & 1))

/* Get the nth character from the right (0=rightmost) of <var> */

#define ACHAR(var, pos)	(char)( ((var)>>(8*pos)) & 0xFF )

#define IsRelDynam(link)	(DynamType(link)>=FIRSTREL_DYNAM && DynamType(link)<FIRSTSF_DYNAM)
#define IsSFDynam(link)		(DynamType(link)>=FIRSTSF_DYNAM && DynamType(link)<FIRSTHAIRPIN_DYNAM)
#define IsHairpin(link)		(DynamType(link)>=FIRSTHAIRPIN_DYNAM)

/* To skip subordinate notes in chords, return False if note in chord & has no stem */

#define MainNote(link) (!NoteINCHORD(link) || NoteYD(link)!=NoteYSTEM(link))
#define GRMainNote(link) (!GRNoteINCHORD(link) || GRNoteYD(link)!=GRNoteYSTEM(link))

/* Is link the main note (or grace note) in voice v? */

#define MainNoteInVOICE(link, v) 	( (NoteVOICE(link) == (v)) && MainNote(link) )
#define GRMainNoteInVOICE(link, v) 	( (GRNoteVOICE(link) == (v)) && GRMainNote(link) )

/* Get the default systemTop for the top system of a page */

#define SYS_TOP(doc) n_max(pt2d((doc)->marginRect.top), pt2d(1))

/* Get the default Measure top for the top staff of a System */

#define MEAS_TOP(stHt) (doc->ledgerYSp)*(stHt)/STFHALFLNS
/* Get the default Measure bottom for the bottom staff of a System */

#define MEAS_BOTTOM(bsTop, stHt) ((bsTop)+(stHt)+(doc->ledgerYSp)*(stHt)/STFHALFLNS)

/* Get width (in pixels) of a keysig. with given nKSItems */

#define KS_WIDTH(sf)	((sf)>0 ? (sf)*(CharWidth(S_sharp)+2) : 	\
								  (-(sf))*(CharWidth(S_flat)+2) )

/* Get staff length in points, ignoring indent */

#define STAFF_LEN(doc)	( (doc)->marginRect.right - (doc)->marginRect.left )

/* Return no. of flags for note with given CMN duration code (subType) */

#define NFLAGS(l_dur)		( ((l_dur)>QTR_L_DUR)? (l_dur)-QTR_L_DUR : 0 )

/* Does note with given y-offset "normally" have stem down? */ 

#define DOWNSTEM(yd, staffHeight)	( ((yd) > (staffHeight)/2)? False : True )

/* Quarter-line stem length for single or multiple voices, shorter stem or normal */

#define QSTEMLEN(singleV, shrt)	((singleV)? config.stemLenNormal :				\
												((shrt)? config.stemLenOutside : config.stemLen2v) )

#define SizePercentSCALE(value)	((int)(((long)sizePercent*(value))/100L))

/* For note with given CMN duration code (subType), wider-than-normal head value */

#define WIDEHEAD(lDur)			( (lDur)==BREVE_L_DUR? 2 : ((lDur)==WHOLE_L_DUR? 1 : 0 ) )

/* Is rest with given dur. code wider than normal (at level of aug. dots)? */

#define IS_WIDEREST(l_dur)		((l_dur)>EIGHTH_L_DUR)

/* If we're "looking at" a voice, return that voice, else the staff's default voice */

#define USEVOICE(doc, staff)	(doc->lookVoice<0 ? (staff) : doc->lookVoice)

/* Are we "Looking at" the given voice? */

#define LOOKING_AT(doc, v)		(doc->lookVoice<0 || (v)==doc->lookVoice)

/* Is the given voice in use? N.B. ALWAYS considers default voices to be in use. */

#define VOICE_MAYBE_USED(doc, v)	((doc)->voiceTab[v].partn!=0)

/* Return True if object is visible or we are showing invisibles */

#define VISIBLE(pL) (LinkVIS(pL) || doc->showInvis)

#define ENABLE_REDUCE(doc)		((doc)!=NULL && (doc)->magnify>MIN_MAGNIFY)
#define ENABLE_ENLARGE(doc)		((doc)!=NULL && (doc)->magnify<MAX_MAGNIFY)
#define ENABLE_GOTO(doc)		((doc)!=NULL && (!(doc)->masterView))

/* Get distance between staff lines for given PCONTEXT (DDIST) */

#define LNSPACE(pCont) ((pCont)->staffHeight/((pCont)->staffLines-1))

/* Copy a WHOLE_KSINFO. Unfortunately, a WHOLE_KSINFO has odd length, while the struct
equivalent KSINFO is necessarily even, so BlockMove(srcKS, dstKS, sizeof(KSINFO)) doesn't
work. */

#define KEYSIG_COPY(srcKS, dstKS)						\
{	int i;												\
														\
	(dstKS)->nKSItems = (srcKS)->nKSItems;				\
	for (i = 0; i<(srcKS)->nKSItems; i++)				\
		(dstKS)->KSItem[i] = (srcKS)->KSItem[i];		\
}

/* Simple compound-meter identifying heuristic. Better for fast tempi than slow ones! */

#define COMPOUND(numer) (numer%3==0 && numer>3)

/* -------------------------------------------------------------------- Error Checking -- */ 
		
/* Is the link garbage (greater than the length of the heap for its type)? */

#define GARBAGEL(type, pL)		((pL)>=(Heap+(type))->nObjs)

/* Is the staff no./voice no./object type illegal? */

#define STAFFN_BAD(doc, staff)	((staff)<1 || (staff)>doc->nstaves)
#define VOICE_BAD(voice)		((voice)<0 || (voice)>MAXVOICES)
#define TYPE_BAD(pL)			(ObjLType((pL))<LOW_TYPE || ObjLType((pL))>HIGH_TYPE)

/* Check timeSigType, numerator, denominator for legality. TSDENOM_BAD must not allow
values forbidden by MAX_TSDENOM (defined elsewhere)! */

#define TSTYPE_BAD(t)			(	(t)<LOW_TSTYPE || (t)>HIGH_TSTYPE)
#define TSNUMER_BAD(t)			(	(t)<1 || (t)>MAX_TSNUM )
#define TSDENOM_BAD(t)			(	(t)!=1 && (t)!=2 && (t)!=4 && (t)!=8		\
									&&	(t)!=16 && (t)!=32 && (t)!=64 )
#define TSDUR_BAD(tn, td)		(TimeSigDur(N_OVER_D, (tn), (td))>MAX_SAFE_MEASDUR )

/* Crude checks for object and subobject vertical and horizontal positions */

#define ABOVE_VLIM(doc) 	(-pt2d((doc)->marginRect.bottom))
#define BELOW_VLIM(doc) 	(pt2d((doc)->marginRect.bottom))
#define RIGHT_HLIM(doc) 	(pt2d((doc)->marginRect.right-(doc)->marginRect.left))
#define LEFT_HLIM(doc, pL)	(J_DTYPE(pL)? -RIGHT_HLIM(doc) : 0)

#define DETAIL_SHOW			(ShiftKeyDown() && ControlKeyDown())
#define MORE_DETAIL_SHOW	(DETAIL_SHOW && OptionKeyDown())

/* ----------------------------------------------------------------- Conversion Macros -- */

/* Convert STDIST to DDIST and vice-versa */

#define std2d(std, stfHt, lines)	( ((long)(std)*(stfHt)) / (8*((lines)-1)) )
#define d2std(d, stfHt, lines)	(8*(long)(d)*((lines)-1) / (stfHt))

/* Convert staff quarter-line position to DDIST and vice-versa */

#define qd2d(qd, stfHt, lines)	( ((long)(qd)*(stfHt)) / (4*((lines)-1)) )
#define d2qd(d, stfHt, lines)		(4*(long)(d)*((lines)-1) / (stfHt))

/* Convert staff halfLn position to DDIST and vice-versa */

#define halfLn2d(halfLn, stfHt, lines) ((halfLn)*(stfHt)/((lines)+(lines)-2))
#define d2halfLn(dd, stfHt, lines)	((dd)*((lines)+(lines)-2)/(stfHt))

/* Convert staff halfLn position to STDIST and vice-versa */

#define halfLn2std(halfLn)	((STD_LINEHT/2)*(halfLn))
#define std2halfLn(std)	((std)/(STD_LINEHT/2))

/* Convert staff halfLn position to quarter-line and vice-versa */
#define halfLn2qd(halfLn)	((STD_LINEHT/4)*(halfLn))
#define qd2halfLn(qd)			((qd)/(STD_LINEHT/4))

/* Convert staff quarter-line position to STDIST and vice-versa */

#define qd2std(qtrLn)	(STD_LINEHT*(qtrLn)/4)
#define std2qd(std)	((std)*4/STD_LINEHT)

/* Convert points to inches and vice-versa */

#define pt2in(p) ((FASTFLOAT)p/POINTSPERIN)
#define in2pt(i) ((i) * POINTSPERIN)

/* Convert DDIST to pixels/points and back */

#define d2p(d)	D2PFunc(d)								/* DDIST to pixels */
#define d2px(d)	D2PXFunc(d)								/* DDIST to pixels, truncating */
#define p2d(p)	P2DFunc(p)								/* pixels to DDIST */

#define d2pt(d)	(((d)+8)>>4)							/* DDIST to points */
#define pt2d(p)	((p)<<4)								/* points to DDIST */
#define p2pt(p)	d2pt(p2d(p))							/* pixels to points */
#define pt2p(p)	d2p(pt2d(p))							/* points to pixels */


/* ---------------------------------------------------- Miscellaneous System-Dependent -- */

/* Convert OS ticks (in Carbon, 1/60 sec.) to milliseconds. */
#define TICKS2MS(ticks) (1000*(ticks)/60)

/* Standard font names */

#define SYSFONTID_SANSSERIF		kFontIDGeneva			/* Universal Interfaces 3.0.1 */
#define SYSFONTID_MONOSPACED	kFontIDMonaco			/* Universal Interfaces 3.0.1 */

/* Rect corners */

#define TOP_LEFT(r)			(((Point *) &(r))[0])
#define BOT_RIGHT(r)		(((Point *) &(r))[1])

/* Getting/setting window type */

#define GetWindowType(wind)		  ( ((WindowPeek)(wind))->windowKind-userKind-1 )
#define SetWindowType(wind, type)	((WindowPeek)(wind))->windowKind = (type)+userKind+1

#define PORT_IS_FREE(portStatus) ((portStatus & 0x80)!=0)

