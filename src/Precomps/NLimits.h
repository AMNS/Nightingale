/* NLimits.h for Nightingale */

/* MacOS 9's limit on filename length was 31 characters; OS X's limit is 255. In the
early days of OS X, problems were often reported with anything over 31 chars., but not
in recent years. However, the Carbon toolkit seems to retain the OS 9 limit, so that's
still all we can handle. --DAB, Oct. 2017.  FIXME: FILENAME_MAXLEN should be used in
several more places, e.g., DoPostScript, NameMFScore. */

#define FILENAME_MAXLEN 31	/* Max. number of chars. in a filename */

#define MAXSTAVES 64		/* Maximum number of staves attached to a system */
#define MAXSTPART 16		/* Maximum no. of staves in one part */

#define MAX_STAFFPOS 76		/* Max. no. of staff lines/spaces, at 7/octave (MIDI covers <11 octaves) */
#define MAXVOICES 100		/* Maximum voice no. in score */
#define MAXCHORD 88			/* Maximum no. of notes in a chord */
#define MAXINBEAM 127		/* Maximum no. of notes/chords in a beamset */
#define MAXINOTTAVA	127		/* Maximum no. of notes/chords in an ottava */
#define MAXINTUPLET	50		/* Maximum no. of notes/chords in an tuplet */
#define MAX_TUPLENUM 255	/* Maximum possible numerator/denominator in a tuplet */
#define MAX_MEASNODES 250	/* Maximum no. of nodes in a measure */
#define MAX_L_DUR 9			/* Max. legal SYNC l_dur code; the shortest legal */
							/*   duration = 1/2^(MAX_L_DUR-WHOLE_L_DUR) */
#define MAX_KSITEMS 7		/* Maximum no. of items in key signature */

#define MAX_FONTSTYLENUM FONT_R9	/* Max. value of index into font style table */
#define MAX_SCOREFONTS 20	/* Maximum no. of font families in one score */
#define MAX_CURSORS 100		/* Maximum no. of cursors used by symtable */

#define MAX_TSNUM 99		/* Maximum time signature numerator */
#define MAX_TSDENOM 64		/* Maximum time signature denominator */

#define MAX_FIRSTMEASNUM 4000	/* Largest number for the first measure */
#define MAX_RSP_MEASURES 5000	/* Max. no. of Measures we can respace per call */

#define MAX_CHANGES 50		/* Max. no. of changes in one call to Master Page */

#define MF_MAXPIECES 300	/* Max. pieces one note/rest can be "clarified" into */

/* Cf. MAX_CARET_HEIGHT in MCaret.c before changing MAX_MAGNIFY. */
#define MIN_MAGNIFY -4		/* Max. reduction = 2^(this no./2) */
#define MAX_MAGNIFY 5		/* Max. magnification = 2^(this no./2) */

#define MAXRASTRAL 8		/* Maximum legal staff rastral size no. */

#define MAX_LEDGERS 22		/* Maximum no. of ledger lines under any circumstances. */
							/* 22 is enough to reach MIDI note no. 127 in bass clef. */
												
/* N.B. If (MAXSPACE*RESFACTOR) exceeds SHRT_MAX, the justification routines may
 *	have serious problems.  See especially RespaceBars and JustAddSpace. */
 
#define MINSPACE 10				/* Minimum legal <spacePercent> for respacing */
#define MAXSPACE 500			/* Maximum legal <spacePercent> for respacing */

#define MIN_TEXT_SIZE 4			/* Minimum text size, in points */
#define MAX_TEXT_SIZE 127		/* Maximum text size, in points */

#define MAX_ENDING_STRLEN 16	/* Maximum length of any ending label */
#define MAX_ENDING_STRINGS 31	/* Maximum no. of ending labels */

#define MAX_COMMENT_LEN 255		/* One less than length of <comment> header field */

#define MIN_BPM 10				/* Minimum legal tempo, in beats per min.  */
#define MAX_BPM 1200			/* Maximum legal tempo, in beats per min. */

#define MAXEVENTLIST 128		/* Maximum no. of simultaneous notes for Play cmds */
#define MAXMFEVENTLIST 128		/* Maximum no. of simultaneous notes in MIDI files */
#define MAX_SAFE_MEASDUR 65500L	/* in PDURticks: cf. ANOTE timeStamp field */
