/* Context.h for Nightingale */

void ContextClef(LINK, CONTEXT []);
void ContextKeySig(LINK, CONTEXT []);
void ContextMeasure(LINK, CONTEXT []);
void Context1Staff(LINK, PCONTEXT);
void ContextStaff(LINK, CONTEXT []);
void ContextSystem(LINK, CONTEXT []);
void ContextTimeSig(LINK, CONTEXT []);
void ContextPage(Document *, LINK, CONTEXT []);
void GetContext(Document *, LINK, INT16, PCONTEXT);
void GetAllContexts(Document *, CONTEXT [], LINK);

void ClefFixBeamContext(Document *doc, LINK startL, LINK doneL, INT16 staffn);
void EFixContForClef(Document *, LINK, LINK, INT16, SignedByte, SignedByte, CONTEXT);
LINK FixContextForClef(Document *, LINK, INT16, SignedByte, SignedByte);
void EFixContForKeySig(LINK, LINK, INT16, KSINFO, KSINFO);
LINK FixContextForKeySig(Document *, LINK, INT16, KSINFO, KSINFO);
void EFixContForTimeSig(LINK, LINK, INT16, TSINFO);
LINK FixContextForTimeSig(Document *, LINK, INT16, TSINFO);
void EFixContForDynamic(LINK, LINK, INT16, SignedByte, SignedByte);
LINK FixContextForDynamic(Document *, LINK, INT16, SignedByte, SignedByte);

void FixMeasureContext(LINK, PCONTEXT);
void FixStaffContext(LINK, PCONTEXT);
LINK UpdateKSContext(Document *, LINK, INT16, CONTEXT, CONTEXT);
void UpdateTSContext(Document *, LINK, INT16, CONTEXT);

void CombineTables(SignedByte [], SignedByte []);
void InitPitchModTable(SignedByte [], KSINFO *);
void CopyTables(SignedByte [], SignedByte []);
LINK EndMeasRangeSearch(Document *, LINK, LINK);
LINK EFixAccsForKeySig(Document *, LINK, LINK, INT16, KSINFO, KSINFO);
LINK FixAccsForKeySig(Document *, LINK, INT16, KSINFO, KSINFO);
