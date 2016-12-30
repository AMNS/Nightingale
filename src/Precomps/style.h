/*
	style.h for Nightingale
	Some notation style parameters, for engravers and other fanatics. Numerous others
	are kept in the CNFG resource. (Those below that are constants should eventually be
	moved there, or somewhere that lets users override them.)
	
	Created by donbyrd on 25 May 2016.
 */


#define CLEF8_ITAL TRUE						/* Use italic digits (normal) instead of roman for clef '8's? */

#define ACC_IN_CONTEXT TRUE					/* Do accs. last to end of measure, as usual in CMN? */
											/* NB: Value of FALSE isn't fully implemented! */

#define EXPAND_WIDER TRUE					/* Stretch text in style Expand with two blanks btwn chars., else one */

#define STD_ACCWIDTH (9*STD_LINEHT/8)		/* Width of common accidentals (STDIST). A bit less than Ross, p.131, says. */
#define STD_KS_ACCSPACE STD_ACCWIDTH		/* Space between accs. in key sig. is the same as their width */

#define REST_STEMLET_LEN 4					/* For "stemlets" on beamed rests (quarter-spaces) */

/* Relative sizes for various types of symbols. Cf. Gould, _Behind Bars_, pp. 125, 569. */

#define GRACESIZE(size)		(7*(size)/10)	/* For grace notes */
#define SMALLSIZE(size)		(3*(size)/4)	/* For "small" versions of symbols: notes/rests, clefs, dynamics, etc. */
#define MEDIUMSIZE(size)	(9*(size)/10)	/* For tempo marks */

/* Caveat: see the comment on GetModNRInfo before adjusting note modifier sizes. */

#define FINGERING_SIZEPCT 65				/* Percent of normal font size for fingerings */
#define CIRCLE_SIZEPCT 150					/* Percent of normal font size for circle */
#define OCTNUM_SIZEPCT 120					/* PostScript relative size for 8ve sign number (unused) */

/* The following assume <lnSp> is DDIST staff interline space, e.g., LNSPACE(pContext). */

#define TUPLE_BRACKTHICK(lnSp) ((6*(lnSp))/50)	/* PostScript thickness of lines in tuplet bracket (DDIST) */
#define OCT_THICK(lnSp)	((6*(lnSp))/50)			/* PostScript thickness of dotted lines in ottavas (DDIST) */
#define ENDING_THICK(lnSp)	((6*(lnSp))/50)		/* PostScript thickness of Ending's lines in ottavas (DDIST) */

#define TUPLE_CUTOFFLEN (STD_LINEHT/2)			/* Length of vertical cutoff line (STDIST, not DDIST) */
#define OCT_CUTOFFLEN(lnSp) (lnSp)				/* Length of vertical cutoff line (DDIST) */
#define NONARP_CUTOFF_LEN(lnSp) ((3*(lnSp))/4)	/* Nonarpeggio sign horiz. cutoff length (DDIST) */
#define ENDING_CUTOFFLEN(lnSp) (2*(lnSp))		/* Length of Ending's vertical cutoff line (DDIST) */

#define STEMSHORTEN_NOHEAD 6					/* Shorten stem by this if note has no head (eighth-spaces) */
#define STEMSHORTEN_XSHAPEHEAD 3				/* Shorten stem by this if note has "X" head (eighth-spaces)  */

/* STRICT_SHORTSTEM is an offset, in half-spaces toward the center of staff, on where
shortening kicks in. With STRICT_SHORTSTEM=0, any notes entirely outside the staff
with stems away from the staff are shortened. */

#define STRICT_SHORTSTEM 0

/* Notation style macros. */

#define FlagLeading(lnSp)	(3*(lnSp)*4/16)		/* Vert. dist. between flags */
#define HeadWidth(lnSp)	(9*(lnSp)*4/32)			/* Width of common (beamable) note heads */
#define FracBeamWidth(lnSp) ((lnSp)*4/4)		/* Fractional beam length */

/* LedgerLen and LedgerOtherLen are used only for insertion feedback and for ledger
lines on rests. Lengths of note ledger lines are determined by <config> fields. */
#define LedgerLen(lnSp) 	(12*(lnSp)*4/32)	/* Length on notehead's side of stem */
#define LedgerOtherLen(lnSp) (3*(lnSp)*4/32)	/* Length on side of stem away from notehead */

#define InsLedgerLen(lnSp) ((lnSp)*4/4)			/* Note insert pseudo-ledger line length */

#define NOTEHEAD_GRAPH_WIDTH 5					/* Width of tiny graphs drawn for noteheads if <doNoteheadGraphs> */

/* The following #defines are just abbreviations for convenience. */

#define SP_AFTERBAR (2*config.spAfterBar)		/* "Normal" space to leave after barlines (STDIST) */
#define STHEIGHT drSize[doc->srastral]			/* initial staff height (DDIST) */

