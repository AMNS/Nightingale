/* NLimits.h for Nightingale - rev. for v. 99 */

#include "compilerFlags.h"	/* Must do this to grab version, since MAXSTAVES depends on it. */

/* A few additional limits are defined in defs.h. */

/* The Light version has a much smaller MAXSTAVES limit. Note that MAXSTPART must not be
greater than MAXSTAVES, or else we hit trouble in AddChangePart (and probably elsewhere). */
#if defined(LIGHT_VERSION)
#define MAXSTAVES 9			/* Maximum number of staves attached to a system */
#define MAXSTPART	9			/* Maximum no. of staves in one part */
#define MAXPAGES	4			/* Maximum number of pages allowed in score. */
#else
#define MAXSTAVES 64			/* Maximum number of staves attached to a system */
#define MAXSTPART 16			/* Maximum no. of staves in one part */
/* NOTE: no MAXPAGES limit outside of the light version */
#endif

#define MAX_STAFFPOS 76		/* Max. no. of staff lines/spaces, at 7/octave (MIDI covers <11 octaves) */
#define MAXVOICES 100		/* Maximum voice no. in score */
#define MAXCHORD 88			/* Maximum no. of notes in a chord */
#define MAXINBEAM 127		/* Maximum no. of notes/chords in a beamset */
#define MAXINOCTAVA	127	/* Maximum no. of notes/chords in an octava */
#define MAXINTUPLET	50		/* Maximum no. of notes/chords in an tuplet */
#define MAX_TUPLENUM 255	/* Maximum possible numerator/denominator in a tuplet */
#define MAX_MEASNODES 250	/* Maximum no. of nodes in a measure */
#define MAX_L_DUR 9			/* Max. legal SYNC l_dur code; the shortest legal */
									/*   duration = 1/2^(MAX_L_DUR-WHOLE_L_DUR) */
#define MAX_KSITEMS 7		/* Maximum no. of items in key signature */

#define MAX_SCOREFONTS 20	/* Maximum no. of font families in one score */
#define MAX_CURSORS 100		/* Maximum no. of cursors used by symtable */

#define MAX_TSNUM 99			/* Maximum time signature numerator */
#define MAX_TSDENOM 64		/* Maximum time signature denominator */

#define MAX_MEASURES 5000	/* Max. no. of Measures we can respace per call */

#define MAX_CHANGES 	50		/* Max. no. of changes in one call to Master Page */

#define MF_MAXPIECES 300		/* Max. pieces one note/rest can be "clarified" into */

/* Cf. MAX_CARET_HEIGHT in MCaret.c before changing MAX_MAGNIFY. */
#define MIN_MAGNIFY -4			/* Max. reduction = 2^(this no./2) */
#define MAX_MAGNIFY 5			/* Max. magnification = 2^(this no./2) */

#define MAXRASTRAL 8				/* Maximum legal staff rastral size no. */

#define MAX_LEDGERS 22			/* Maximum no. of ledger lines under any circumstances. */
										/* 22 is enough to reach MIDI note no. 127 in bass clef. */
												
/* N.B. If (MAXSPACE*RESFACTOR) exceeds SHRT_MAX, the justification routines may
 *	have serious problems.  See especially RespaceBars and JustAddSpace. */
 
#define MINSPACE 10				/* Minimum legal <spacePercent> */
#define MAXSPACE 500				/* Maximum legal <spacePercent> */

#define MIN_TEXT_SIZE 4			/* Minimum text size, in points */
#define MAX_TEXT_SIZE 127		/* Maximum text size, in points */

#define MAX_ENDING_STRLEN 16		/* Maximum length of any ending label */
#define MAX_ENDING_STRINGS 31		/* Maximum no. of ending labels */

#define MAX_COMMENT_LEN 255	/* One less than length of <comment> header field */

