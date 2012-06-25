/*	RhythmDur.h for Nightingale */

typedef struct {
	long		time;
	char		lDur;		/* of note ENDING at <time> */
	char		nDots;	/* of note ENDING at <time> */
} NOTEPIECE;

Boolean SetSyncPDur(Document *, LINK, short, Boolean);
void SetPDur(Document *, short, Boolean);
Boolean SetSelNoteDur(Document *, short, short, Boolean);
Boolean SetDoubleHalveSelNoteDur(Document *, Boolean, Boolean);
Boolean SetSelMBRest(Document *, short);
short SetAndClarifyDur(Document *, LINK, LINK, short);
void SetNRCDur(Document *, LINK, short, char, char, PCONTEXT, Boolean);
void ClearAccAndMods(Document *, LINK, short);
Boolean MakeClarifyList(long, short, short, short, Boolean, NOTEPIECE [], short, short *);
Boolean ClarifyFromList(Document *, LINK, short, short, NOTEPIECE [], short, PCONTEXT);
Boolean ClarifyRhythm(Document *);
