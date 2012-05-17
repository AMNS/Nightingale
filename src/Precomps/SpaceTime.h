/* SpaceTime.h for Nightingale */

Boolean FillRelStaffSizes(Document *);

STDIST SymWidthLeft(Document *, LINK, INT16);
STDIST SymWidthRight(Document *, LINK, INT16, Boolean);
STDIST SymLikelyWidthRight(Document *, LINK, long);
DDIST SymDWidthLeft(Document *, LINK, INT16, CONTEXT);
DDIST SymDWidthRight(Document *, LINK, INT16, Boolean, CONTEXT);
DDIST ConnectDWidth(INT16, char);

DDIST GetClefSpace(LINK clefL);
DDIST GetTSWidth(LINK timeSigL);
INT16 GetKSWidth(LINK, DDIST, INT16);
DDIST KSDWidth(LINK,CONTEXT);
DDIST GetKeySigWidth(Document *doc,LINK keySigL,INT16 staffn);

long SyncNeighborTime(Document *, LINK, Boolean);
long GetLTime(Document *, LINK);
INT16 GetSpTimeInfo(Document *, LINK, LINK, SPACETIMEINFO [], Boolean);
long GetLDur(Document *, LINK, INT16);
long GetVLDur(Document *, LINK, INT16);

void FillSpaceMap(Document *, INT16);
STDIST IdealSpace(Document *, long, long);
INT16 FIdealSpace(Document *, long, long);
DDIST CalcSpaceNeeded(Document *doc, LINK pL);

INT16 MeasSpaceProp(LINK);
void SetMeasSpacePercent(LINK, long);
Boolean GetMSpaceRange(Document *, LINK, LINK, INT16 *, INT16 *);

Boolean LDur2Code(INT16, INT16, INT16, char *, char *);
long Code2LDur(char, char);
long SimpleLDur(LINK);
INT16 GetMinDur(INT16, LINK, LINK);
INT16 TupletTotDir(LINK);
INT16 GetDurUnit(INT16, INT16, INT16);
INT16 GetMaxDurUnit(LINK);
long SimpleGRLDur(LINK);
long TimeSigDur(INT16, INT16, INT16);

Boolean RhythmUnderstood(Document *, LINK, Boolean);
Boolean FixTimeStamps(Document *, LINK, LINK);
long CalcNoteLDur(Document *, LINK, LINK);
long SyncMaxDur(Document *, LINK);
long GetMeasDur(Document *, LINK);
long NotatedMeasDur(Document *, LINK);

Boolean WholeMeasRestIsBreve(char numerator, char denominator);
