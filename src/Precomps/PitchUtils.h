/* PitchUtils.h for Nightingale */

void MoveModNRs(LINK, STDIST);

INT16 GetSharpsOrFlats(PCONTEXT);
Boolean KeySigEqual(PKSINFO, PKSINFO);
void KeySigCopy(PKSINFO, PKSINFO);
INT16 ClefMiddleCHalfLn(SignedByte clefType);
INT16 Char2Acc(unsigned char);
unsigned char Acc2Char(INT16);
INT16 EffectiveAcc(Document *, LINK, LINK);
INT16 EffectiveGRAcc(Document *, LINK, LINK);
INT16 Pitch2MIDI(INT16, INT16);
INT16 Pitch2PC(INT16, INT16);
INT16 PCDiff(INT16, INT16, Boolean);
INT16 GetRespell(INT16, INT16, INT16 *, INT16 *);

void FixRespelledVoice(Document *, LINK, INT16);
INT16 Char2LetName(char);
char LetName2Char(INT16);

Boolean RespellChordSym(Document *, LINK);
Boolean RespellNote(Document *, LINK, LINK, PCONTEXT);
Boolean RespellGRNote(Document *, LINK, LINK, PCONTEXT);
Boolean TranspChordSym(Document *, LINK, INT16, INT16);
Boolean TranspNote(Document *, LINK, LINK, CONTEXT, INT16, INT16, INT16);
Boolean TranspGRNote(Document *, LINK, LINK, CONTEXT, INT16, INT16, INT16);
void DTranspChordSym(Document *, LINK, INT16);
void DTranspNote(Document *, LINK, LINK, CONTEXT, Boolean, INT16, INT16);
void DTranspGRNote(Document *, LINK, LINK, CONTEXT, Boolean, INT16, INT16);
INT16 XGetSharpsOrFlats(PKSINFO);
Boolean TranspKeySig(Document *, LINK, LINK, CONTEXT, INT16, INT16);

Boolean CompareNCNotes(LINK, LINK, INT16, INT16 *, INT16 *, CHORDNOTE [], CHORDNOTE [],
								INT16 *);
INT16 CompareNCNoteNums(LINK, LINK, INT16);

INT16 PSortChordNotes(LINK, INT16, Boolean, CHORDNOTE []);
INT16 GSortChordNotes(LINK, INT16, Boolean, CHORDNOTE []);
INT16 GSortChordGRNotes(LINK, INT16, Boolean, CHORDNOTE []);
void ArrangeNCAccs(CHORDNOTE [], INT16, Boolean);
Boolean ArrangeChordNotes(LINK, INT16, Boolean);
void ArrangeGRNCAccs(CHORDNOTE [], INT16, Boolean);
Boolean ArrangeChordGRNotes(LINK, INT16, Boolean);

void FixAccidental(Document *, LINK, INT16, INT16, INT16);
void FixAllAccidentals(LINK, LINK, INT16, Boolean);
void KeySig2AccTable(SignedByte[], PKSINFO);
void GetAccTable(Document *, SignedByte[], LINK, INT16);
void GetPitchTable(Document *, SignedByte[], LINK, INT16);
void DelNoteFixAccs(Document *, LINK, SignedByte, INT16, INT16);
INT16 InsNoteFixAccs(Document *, LINK, SignedByte, INT16, INT16);
