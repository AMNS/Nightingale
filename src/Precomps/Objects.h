/*	Objects.h for Nightingale */

//#if TARGET_CPU_PPC
	#pragma options align=mac68k
//#endif

/*
 *	This is used to access fields that are found only in subobjects whose data
 *	begins with a SUBOBJHEADER (not all do: extend objects, pages and systems,
 * etc. don't).
 */

typedef struct {
		LINK			next;					/* To next subobject in list */
		char			staffn;				/* staff number */
		char			subType;				/* subobject subtype. N.B. Signed--see ANOTE. */
		Boolean		selected:1;			/* TRUE if subobject is selected */
		Boolean		visible:1;			/* TRUE if subobject is visible */
		Boolean		soft:1;				/* TRUE if subobject is program-generated */
		
} GenSubObj;

//#if TARGET_CPU_PPC
	#pragma options align=reset
//#endif


LINK CopyModNRList(Document *, Document *, LINK);
LINK DuplicateObject(INT16,LINK,Boolean,Document *src,Document *dst,Boolean keepGraphics);
LINK DuplicNC(Document *, LINK, INT16);

void InitObject(LINK, LINK, LINK, DDIST, DDIST, Boolean, Boolean, Boolean);
void SetObject(LINK, DDIST, DDIST, Boolean, Boolean, Boolean);
LINK GrowObject(Document *, LINK, INT16);

long SimplePlayDur(LINK);
INT16 CalcPlayDur(LINK, LINK, char, Boolean, PCONTEXT);

LINK SetupNote(Document *, LINK, LINK, char, INT16, char, INT16, char, Boolean, INT16, INT16);
LINK SetupGRNote(Document *, LINK, LINK, char, INT16, char, INT16, char, INT16, INT16);
void InitPart(LINK, INT16, INT16);
void InitStaff(LINK, INT16, INT16, INT16, INT16, DDIST, INT16, INT16);
void InitClef(LINK, INT16, DDIST, INT16);
void InitKeySig(LINK, INT16, DDIST, INT16);
void SetupKeySig(LINK, INT16);
void InitTimeSig(LINK, INT16, DDIST, INT16, INT16, INT16);
void InitMeasure(LINK, INT16, INT16, INT16, INT16, INT16, Boolean, Boolean, INT16, INT16);
void InitPSMeasure(LINK, INT16, Boolean, Boolean, INT16, char);
void InitRptEnd(LINK, INT16, char, LINK);
void InitDynamic(Document *, LINK, INT16, INT16, DDIST, INT16, PCONTEXT);
void SetupHairpin(LINK, INT16, LINK, DDIST, INT16, Boolean);
void InitGraphic(LINK, INT16, INT16, INT16, INT16, Boolean, INT16, INT16, INT16);

void SetMeasVisible(LINK, Boolean);

Boolean ChordHasUnison(LINK, INT16);
Boolean ChordNoteToRight(LINK, INT16);
Boolean ChordNoteToLeft(LINK, INT16);

void FixTieIndices(LINK);

DDIST GetNCYStem(Document *, LINK, INT16, INT16, Boolean, PCONTEXT);
void FixChordForYStem(LINK, INT16, INT16, INT16);
Boolean FixSyncForChord(Document *, LINK, INT16, Boolean, INT16, INT16, PCONTEXT);
void FixSyncNote(Document *, LINK, INT16, PCONTEXT);
void FixSyncForNoChord(Document *, LINK, INT16, PCONTEXT);
void FixNoteForClef(Document *, LINK, LINK, INT16);

void FixGRChordForYStem(LINK, INT16, INT16, INT16);
Boolean FixGRSyncForChord(Document *, LINK, INT16, Boolean, INT16, INT16, PCONTEXT);
void FixGRSyncNote(Document *, LINK, INT16, PCONTEXT);
void FixGRSyncForNoChord(Document *, LINK, INT16, PCONTEXT);

void FixAugDotPos(Document *doc, LINK syncL, INT16 voice, Boolean lineOnly);
void ToggleAugDotPos(Document *, LINK, Boolean);
