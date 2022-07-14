/* SearchScore.h for Nightingale - header file for SearchScore.c and related file(s) */

/* Codes for search types */

#define TYPE_PITCHMIDI_ABS 0		/* Pitch search on absolute MIDI note nos. */
#define TYPE_PITCHMIDI_ABS_TROCT 1	/* Pitch search on absolute w/octave transp. MIDI note nos. */
#define TYPE_PITCHMIDI_REL 2		/* Pitch search on interval pattern via MIDI note nos. */
#define TYPE_DURATION_ABS 3			/* Duration search on absolute dur. pattern */
#define TYPE_DURATION_REL 4			/* Duration search on relative dur. pattern */
#define TYPE_DURATION_CONTOUR 5		/* Duration search on "contour" (longer/equal/shorter) */

/* Codes for chord-note matching types */

#define TYPE_CHORD_ALL 0			/* Match all notes of chords */
#define TYPE_CHORD_OUTER 1			/* Match only outer notes of chords */
#define TYPE_CHORD_TOP 2			/* Match only top notes of chords */

/* Information on I/O warnings opening files searched */

typedef struct {
	INT16	nDocsMissingFonts;
	INT16	nDocsNonstandardSize;
	INT16	nDocsTooManyPages;
	INT16	nDocsPageSetupPblm;
	INT16	nDocsOther;
} INFOIOWARNING;


/* Prototypes for public routines */

Boolean SearchDialog(Boolean findInFiles,	Boolean *findAll, Boolean *pUsePitch,
							Boolean *pUseDuration, INT16 *pPitchSearchType, INT16 *pDurSearchType,
							Boolean *pIncludeRests, INT16 *pMaxTranspose, INT16 *pPitchTolerance,
							Boolean *pKeepContour, INT16 *pChordNotes, Boolean *pmatchTiedDur);
Boolean DoSearchScore(Document *doc, Boolean again, Boolean usePitch, Boolean useDuration,
							INT16 pitchSearchType, INT16 durSearchType, Boolean includeRests,
							INT16 maxTranspose, INT16 pitchTolerance, FASTFLOAT pitchWeight,
							Boolean pitchKeepContour, INT16 chordNotes, Boolean matchTiedDur);
Boolean DoIRSearchScore(Document *doc, Boolean usePitch, Boolean useDuration,
							INT16 pitchSearchType, INT16 durSearchType, Boolean includeRests,
							INT16 maxTranspose, INT16 pitchTolerance, FASTFLOAT pitchWeight,
							Boolean pitchKeepContour, INT16 chordNotes, Boolean matchTiedDur);
#ifdef SEARCH_DBFORMAT_MEF
Boolean DoIRSearchFiles(Str255 filename, short vRefNum, Boolean usePitch, Boolean useDuration,
							INT16 pitchSearchType, INT16 durSearchType, Boolean includeRests,
							INT16 maxTranspose, INT16 pitchTolerance, FASTFLOAT pitchWeight,
							Boolean pitchKeepContour, INT16 chordNotes, Boolean matchTiedDur);
#else
Boolean DoIRSearchFiles(Document *doc, Boolean usePitch, Boolean useDuration,
							INT16 pitchSearchType, INT16 durSearchType, Boolean includeRests,
							INT16 maxTranspose, INT16 pitchTolerance, FASTFLOAT pitchWeight,
							Boolean pitchKeepContour, INT16 chordNotes, Boolean matchTiedDur);
#endif

void ShowSearchPatDocument();
void DoDialogEvent(EventRecord *theEvent);
