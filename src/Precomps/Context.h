/* Context.h for Nightingale */

void ContextClef(LINK, CONTEXT []);
void ContextKeySig(LINK, CONTEXT []);
void ContextMeasure(LINK, CONTEXT []);
void Context1Staff(LINK, PCONTEXT);
void ContextStaff(LINK, CONTEXT []);
void ContextSystem(LINK, CONTEXT []);
void ContextTimeSig(LINK, CONTEXT []);
void ContextPage(Document *, LINK, CONTEXT []);
void GetContext(Document *, LINK, short, PCONTEXT);
void GetAllContexts(Document *, CONTEXT [], LINK);

void ClefFixBeamContext(Document *doc, LINK startL, LINK doneL, short staffn);
void EFixContForClef(Document *, LINK, LINK, short, SignedByte, SignedByte, CONTEXT);
LINK FixContextForClef(Document *, LINK, short, SignedByte, SignedByte);
void EFixContForKeySig(LINK, LINK, short, KSINFO, KSINFO);
LINK FixContextForKeySig(Document *, LINK, short, KSINFO, KSINFO);
void EFixContForTimeSig(LINK, LINK, short, TSINFO);
LINK FixContextForTimeSig(Document *, LINK, short, TSINFO);
void EFixContForDynamic(LINK, LINK, short, SignedByte, SignedByte);
LINK FixContextForDynamic(Document *, LINK, short, SignedByte, SignedByte);

void FixMeasureContext(LINK, PCONTEXT);
void FixStaffContext(LINK, PCONTEXT);
LINK UpdateKSContext(Document *, LINK, short, CONTEXT, CONTEXT);
void UpdateTSContext(Document *, LINK, short, CONTEXT);

void CombineTables(SignedByte [], SignedByte []);
void InitPitchModTable(SignedByte [], KSINFO *);
void CopyTables(SignedByte [], SignedByte []);
LINK EndMeasRangeSearch(Document *, LINK, LINK);
LINK EFixAccsForKeySig(Document *, LINK, LINK, short, KSINFO, KSINFO);
LINK FixAccsForKeySig(Document *, LINK, short, KSINFO, KSINFO);
