/* PitchUtils.h for Nightingale */

void MoveModNRs(LINK, STDIST);

short GetSharpsOrFlats(PCONTEXT);
Boolean KeySigEqual(PKSINFO, PKSINFO);
void KeySigCopy(PKSINFO, PKSINFO);
short ClefMiddleCHalfLn(SignedByte clefType);
short Char2Acc(unsigned char);
unsigned char Acc2Char(short);
short EffectiveAcc(Document *, LINK, LINK);
short EffectiveGRAcc(Document *, LINK, LINK);
short Pitch2MIDI(short, short);
short Pitch2PC(short, short);
short PCDiff(short, short, Boolean);
short GetRespell(short, short, short *, short *);

void FixVoiceForPitchChange(Document *, LINK, short);
short Char2LetName(char);
char LetName2Char(short);

Boolean RespellChordSym(Document *, LINK);
Boolean RespellNote(Document *, LINK, LINK, PCONTEXT);
Boolean RespellGRNote(Document *, LINK, LINK, PCONTEXT);
Boolean TranspChordSym(Document *, LINK, short, short);
Boolean TranspNote(Document *, LINK, LINK, CONTEXT, short, short, short);
Boolean TranspGRNote(Document *, LINK, LINK, CONTEXT, short, short, short);
void DTranspChordSym(Document *, LINK, short);
void DTranspNote(Document *, LINK, LINK, CONTEXT, Boolean, short, short);
void DTranspGRNote(Document *, LINK, LINK, CONTEXT, Boolean, short, short);
short XGetSharpsOrFlats(PKSINFO);
Boolean TranspKeySig(Document *, LINK, LINK, CONTEXT, short, short);

Boolean CompareNCNotes(LINK, LINK, short, short *, short *, CHORDNOTE [], CHORDNOTE [],
								short *);
short CompareNCNoteNums(LINK, LINK, short);

short PSortChordNotes(LINK, short, Boolean, CHORDNOTE []);
short GSortChordNotes(LINK, short, Boolean, CHORDNOTE []);
short GSortChordGRNotes(LINK, short, Boolean, CHORDNOTE []);
void ArrangeNCAccs(CHORDNOTE [], short, Boolean);
Boolean ArrangeChordNotes(LINK, short, Boolean);
void ArrangeGRNCAccs(CHORDNOTE [], short, Boolean);
Boolean ArrangeChordGRNotes(LINK, short, Boolean);

void FixAccidental(Document *, LINK, short, short, short);
void FixAllAccidentals(LINK, LINK, short, Boolean);
void KeySig2AccTable(SignedByte[], PKSINFO);
void GetAccTable(Document *, SignedByte[], LINK, short);
void GetPitchTable(Document *, SignedByte[], LINK, short);
void DelNoteFixAccs(Document *, LINK, SignedByte, short, short);
short InsNoteFixAccs(Document *, LINK, SignedByte, short, short);

Boolean AvoidUnisons(Document *, LINK, short, PCONTEXT);
void AvoidUnisonsInRange(Document *, LINK, LINK, short);
