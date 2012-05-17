/*	RhythmDur.h for Nightingale */

typedef struct {
	long		time;
	char		lDur;		/* of note ENDING at <time> */
	char		nDots;	/* of note ENDING at <time> */
} NOTEPIECE;

Boolean SetSyncPDur(Document *, LINK, INT16, Boolean);
void SetPDur(Document *, INT16, Boolean);
Boolean SetSelNoteDur(Document *, INT16, INT16, Boolean);
Boolean SetDoubleHalveSelNoteDur(Document *, Boolean, Boolean);
Boolean SetSelMBRest(Document *, INT16);
INT16 SetAndClarifyDur(Document *, LINK, LINK, INT16);
void SetNRCDur(Document *, LINK, INT16, char, char, PCONTEXT, Boolean);
void ClearAccAndMods(Document *, LINK, INT16);
Boolean MakeClarifyList(long, INT16, INT16, INT16, Boolean, NOTEPIECE [], INT16, INT16 *);
Boolean ClarifyFromList(Document *, LINK, INT16, INT16, NOTEPIECE [], INT16, PCONTEXT);
Boolean ClarifyRhythm(Document *);
