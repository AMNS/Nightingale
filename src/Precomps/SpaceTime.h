/* SpaceTime.h for Nightingale */

Boolean FillRelStaffSizes(Document *);

STDIST SymWidthLeft(Document *, LINK, short);
STDIST SymWidthRight(Document *, LINK, short, Boolean);
STDIST SymLikelyWidthRight(Document *, LINK, long);
DDIST SymDWidthLeft(Document *, LINK, short, CONTEXT);
DDIST SymDWidthRight(Document *, LINK, short, Boolean, CONTEXT);
DDIST ConnectDWidth(short, char);

DDIST GetClefSpace(LINK clefL);
DDIST GetTSWidth(LINK timeSigL);
short GetKSWidth(LINK, DDIST, short);
DDIST KSDWidth(LINK,CONTEXT);
DDIST GetKeySigWidth(Document *doc,LINK keySigL,short staffn);

long SyncNeighborTime(Document *, LINK, Boolean);
long GetLTime(Document *, LINK);
short GetSpTimeInfo(Document *, LINK, LINK, SPACETIMEINFO [], Boolean);
long GetLDur(Document *, LINK, short);
long GetVLDur(Document *, LINK, short);

void FillSpaceMap(Document *, short);
STDIST IdealSpace(Document *, long, long);
short FIdealSpace(Document *, long, long);
DDIST CalcSpaceNeeded(Document *doc, LINK pL);

short MeasSpaceProp(LINK);
void SetMeasSpacePercent(LINK, long);
Boolean GetMSpaceRange(Document *, LINK, LINK, short *, short *);

Boolean LDur2Code(short, short, short, char *, char *);
long Code2LDur(char, char);
long SimpleLDur(LINK);
short GetMinDur(short, LINK, LINK);
short TupletTotDir(LINK);
short GetDurUnit(short, short, short);
short GetMaxDurUnit(LINK);
long SimpleGRLDur(LINK);
long TimeSigDur(short, short, short);

Boolean RhythmUnderstood(Document *, LINK, Boolean);
Boolean FixTimeStamps(Document *, LINK, LINK);
long CalcNoteLDur(Document *, LINK, LINK);
long SyncMaxDur(Document *, LINK);
long GetMeasDur(Document *, LINK);
long NotatedMeasDur(Document *, LINK);

Boolean WholeMeasRestIsBreve(char numerator, char denominator);
