/*
	Style.h for Nightingale
	Some notation style parameters, for engravers and other fanatics. Numerous others
	are kept in the CNFG resource; cf. Initialize.c. (Those below that are constants
	should eventually be moved there, or put somewhere that lets users override them.)
	
	Created by donbyrd on 25 May 2016.
 */

#define CLEF8_ITAL TRUE						/* Use italic digits (normal) instead of roman for clef '8's? */

#define EXPAND_WIDER true					/* Stretch text in style Expand with two blanks btwn chars., else one */

#define STD_ACCWIDTH (9*STD_LINEHT/8)		/* Width of common accidentals (STDIST). A bit less than Ross, p.131, says. */
#define STD_KS_ACCSPACE STD_ACCWIDTH		/* Space between accidentals in key sigs. */

/* Relative sizes for various types of symbols. Cf. Gould, _Behind Bars_, pp. 125, 569. */

#define GRACESIZE(size)		(7*(size)/10)	/* For grace notes */
#define SMALLSIZE(size)		(3*(size)/4)	/* For "small" versions of symbols: notes/rests, clefs, dynamics, etc. */
#define METROSIZE(size)		(8*(size)/10)	/* For metronome marks */

/* <lnSp> in the following #defines is DDIST staff interline space, e.g., LNSPACE(pContext). */

/* -------------- Notes/rests/grace notes -------------- */

#define STEMSHORTEN_NOHEAD 6					/* Shorten stem by this if note has no head (eighth-spaces) */
#define STEMSHORTEN_XSHAPEHEAD 3				/* Shorten stem by this if note has "X" head (eighth-spaces)  */

/* Adjust notehead <width> for given WIDEHEAD code. NB: watch out for integer overflow! */
#define WIDENOTEHEAD_PCT(whCode, width) (whCode==2? 160*(width)/100 : \
											(whCode==1? 135*(width)/100 : (width)) )
/* If notehead is wider or narrow than normal and note is upstemmed, move so right edge touches the stem. */
#define NHEAD_XD_GLYPH_ADJ(stemDn, headRSize)		\
	(!stemDn? ((headRSize-100L)*HeadWidth(lnSpace))/100L : 0L)
#define GRNOTE_SLASH_YDELTA(len) (3*(len)/4)	/* "Height" of slash of length len */

/* STRICT_SHORTSTEM is an offset, in half-spaces toward the center of staff, on where
stem shortening kicks in. With STRICT_SHORTSTEM=0, any notes entirely outside the staff
with stems away from the staff are shortened. */
#define STRICT_SHORTSTEM 0

#define REST_STEMLET_LEN 4					/* For "stemlets" on beamed rests (quarter-spaces) */

#define NOTEHEAD_GRAPH_WIDTH 5				/* Width of tiny graphs drawn for noteheads
												if <doNoteheadGraphs> (staff spaces) */

#define ACC_IN_CONTEXT TRUE					/* Do accidentals last to end of measure, as usual in CMN?
												NB: Value of FALSE is not fully implemented! */

#define FlagLeading(lnSp)	(3*(lnSp)*4/16)	/* Vertical distance between flags */

/* Note modifier sizes. Caveat: see the comment on GetModNRInfo before adjusting these. */
#define FINGERING_SIZEPCT 65				/* Percent of normal font size for fingerings */
#define CIRCLE_SIZEPCT 150					/* Percent of normal font size for circle */
#define OCTNUM_SIZEPCT 120					/* PostScript relative size for 8ve sign number (unused) */

/* ---------------- Chord symbols ---------------- */

#define CS_GAPBTWFIELDS		8				/* distance between fields (e.g., btw. root & qual strings); % of csSmallSize */
#define CS_PAREN_YOFFSET	18				/* offset of parentheses from baseline (above); % of csSmallSize */
#define CS_BASELINE_FROMBOT 16				/* pixels */

/* ---------------- Octave signs ---------------- */

#define OTTAVA_XDPOS_FIRST -pt2d(2)			/* Default x-position of left end rel. to first Sync */
#define OTTAVA_XDPOS_LAST (pt2d(1)/2)		/* Default x-position of right end rel. to first Sync */

#define OTTAVA_STANDOFF_ALTA 1				/* Minimum half-line distance from octave sign to staff */
#define OTTAVA_STANDOFF_BASSA 2

#define OTTAVA_MARGIN_ALTA	3				/* Default half-line distance from octave sign to note */
#define OTTAVA_MARGIN_BASSA	5
#define OTTAVA_THICK(lnSp)	((6*(lnSp))/50)		/* PostScript thickness of dotted lines in ottavas (DDIST) */
#define OTTAVA_CUTOFFLEN(lnSp) (lnSp)			/* Length of vertical cutoff line (DDIST) */

/* ---------------- Tuplets ---------------- */

#define TUPLE_BRACKTHICK(lnSp) ((6*(lnSp))/50)	/* PostScript thickness of lines in tuplet bracket (DDIST) */
#define TUPLE_CUTOFFLEN (STD_LINEHT/2)			/* Length of vertical cutoff line (STDIST, not DDIST) */

/* ---------------- Endings ---------------- */

#define ENDING_THICK(lnSp)	((6*(lnSp))/50)		/* PostScript thickness of lines in Endings (DDIST) */
#define ENDING_CUTOFFLEN(lnSp) (2*(lnSp))		/* Length of Ending's vertical cutoff line (DDIST) */

/* ---------------- Nonarpeggio sign ---------------- */

#define NONARP_CUTOFF_LEN(lnSp) ((3*(lnSp))/4)	/* Nonarpeggio sign horiz. cutoff length (DDIST) */

/* Miscellaneous notation style macros. */

#define HeadWidth(lnSp)	(9*(lnSp)*4/32)			/* Width of common (beamable) note heads */
#define FracBeamWidth(lnSp) ((lnSp)*4/4)		/* Fractional beam length */

/* LedgerLen and LedgerOtherLen are used only for insertion feedback and for ledger
lines on rests. Lengths of note ledger lines are determined by <config> fields. */
#define LedgerLen(lnSp) 	(12*(lnSp)*4/32)	/* Length on notehead's side of stem */
#define LedgerOtherLen(lnSp) (3*(lnSp)*4/32)	/* Length on side of stem away from notehead */

#define InsLedgerLen(lnSp) ((lnSp)*4/4)			/* Note insert pseudo-ledger line length */

/* The following #defines are just abbreviations for convenience. */

#define SP_AFTERBAR (2*config.spAfterBar)		/* "Normal" space to leave after barlines (STDIST) */
#define STHEIGHT drSize[doc->srastral]			/* initial staff height (DDIST) */

