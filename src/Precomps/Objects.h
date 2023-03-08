/*	Objects.h for Nightingale */

#pragma options align=mac68k

/*  This is used to access fields that are found only in subobjects whose data begins
with a SUBOBJHEADER. Not all do: Extend objects, Pages and Systems, etc., don't. */

typedef struct {
SUBOBJHEADER		
} GenSubObj;

#pragma options align=reset

LINK CopyModNRList(Document *, Document *, LINK);
LINK DuplicateObject(short, LINK, Boolean, Document *src, Document *dst, Boolean keepGraphics);
LINK DuplicNC(Document *, LINK, short);

void InitObject(LINK, LINK, LINK, DDIST, DDIST, Boolean, Boolean, Boolean);
void SetObject(LINK, DDIST, DDIST, Boolean, Boolean, Boolean);
LINK GrowObject(Document *, LINK, short);

long SimplePlayDur(LINK);
short CalcPlayDur(LINK, LINK, char, Boolean, PCONTEXT);

LINK SetupNote(Document *, LINK, LINK, char, short, char, short, char, Boolean, short, short);
LINK SetupGRNote(Document *, LINK, LINK, char, short, char, short, char, short, short);
void InitPart(LINK, short, short);
void InitStaff(LINK, short, short, short, short, DDIST, short, short);
void InitClef(LINK, short, DDIST, short);
void InitKeySig(LINK, short, DDIST, short);
void SetupKeySig(LINK, short);
void InitTimeSig(LINK, short, DDIST, short, short, short);
void InitMeasure(LINK, short, short, short, short, short, Boolean, Boolean, short, short);
void InitPSMeasure(LINK, short, Boolean, Boolean, short, char);
void InitRptEnd(LINK, short, char, LINK);
void InitDynamic(Document *, LINK, short, short, DDIST, short, PCONTEXT);
void SetupHairpin(LINK, short, LINK, DDIST, short, Boolean);
void InitGraphic(LINK, short, short, short, short, Boolean, short, short, short);

void SetMeasVisible(LINK, Boolean);

short ChordHasUnison(LINK, short);
Boolean ChordNoteToRight(LINK, short);
Boolean ChordNoteToLeft(LINK, short);

DDIST GetNCYStem(Document *, LINK, short, short, Boolean, PCONTEXT);
void FixChordForYStem(LINK, short, short, short);
Boolean FixSyncForChord(Document *, LINK, short, Boolean, short, short, PCONTEXT);
void FixSyncNote(Document *, LINK, short, PCONTEXT);
void FixSyncForNoChord(Document *, LINK, short, PCONTEXT);
void FixNoteForClef(Document *, LINK, LINK, short);

void FixGRChordForYStem(LINK, short, short, short);
Boolean FixGRSyncForChord(Document *, LINK, short, Boolean, short, short, PCONTEXT);
void FixGRSyncNote(Document *, LINK, short, PCONTEXT);
void FixGRSyncForNoChord(Document *, LINK, short, PCONTEXT);

void FixAugDotPos(Document *doc, LINK syncL, short voice, Boolean lineOnly);
void ToggleAugDotPos(Document *, LINK, Boolean);
