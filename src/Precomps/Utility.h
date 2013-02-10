/*	Utility.h for Nightingale */

DDIST CalcYStem(Document *, DDIST, short, Boolean, DDIST, short, short, Boolean);
DDIST GetNoteYStem(Document *, LINK, LINK, CONTEXT);
DDIST GetGRNoteYStem(LINK, CONTEXT);
Boolean ShortenStem(LINK, CONTEXT, Boolean);
Boolean GetCStemInfo(Document *, LINK, LINK, CONTEXT, short *);
Boolean GetStemInfo(Document *, LINK, LINK, short *);
Boolean GetCGRStemInfo(Document *, LINK, LINK, CONTEXT, short *);
Boolean GetGRStemInfo(Document *, LINK, LINK, short *);
short GetLineAugDotPos(short, Boolean);
DDIST ExtraSysWidth(Document *);

void ApplHeapCheck(void);

short Char2Dur(char);
short Dur2Char(short);
short GetSymTableIndex(char);
short SymType(char);
char Objtype2Char(SignedByte);

Rect StrToObjRect(unsigned char *string);
void GetNFontInfo(short, short, short, FontInfo *);
short NStringWidth(Document *, const unsigned char *, short, short, short);
short NPtStringWidth(Document *, const unsigned char *, short, short, short);
void GetNPtStringBBox(Document *, unsigned char *, short, short, short, Boolean, Rect *);
short NPtGraphicWidth(Document *, LINK, PCONTEXT);

short MaxNameWidth(Document *, short);
double PartNameMargin(Document *, short);

void SetFont(short);
Rect StringRect(Str255 *);

PCONTEXT AllocContext(void);
SPACETIMEINFO *AllocSpTimeInfo(void);

GWorldPtr MakeGWorld(short, short, Boolean);
void DestroyGWorld(GWorldPtr);
Boolean SaveGWorld(void);
Boolean RestoreGWorld(void);
Boolean LockGWorld(GWorldPtr);
void UnlockGWorld(GWorldPtr);

GrafPtr NewGrafPort(short, short);
void DisposGrafPort(GrafPtr);

void D2Rect(DRect *, Rect *);
void Rect2D(Rect *, DRect *);
void PtRect2D(Rect *, DRect *);
void AddDPt(DPoint, DPoint *);
void SetDPt(DPoint *, DDIST, DDIST);
void SetDRect(DRect *, DDIST, DDIST, DDIST, DDIST);
void OffsetDRect(DRect *, DDIST, DDIST);
void InsetDRect(DRect *, DDIST, DDIST);
void DMoveTo(DDIST, DDIST);

short GCD(short m, short n);
double RoundDouble(double, double);
short RoundSignedInt(short value, short quantum);
short InterpY(short, short, short, short, short);

long FindIntInString(unsigned char *);
short BlockCompare(void *src, void *dst, short len);
short RelIndexToSize(short, DDIST);
short GetTextSize(Boolean, short, DDIST);

short GetFontIndex(Document *, unsigned char *);
short User2HeaderFontNum(Document *, short);
short Header2UserFontNum(short);

void Rect2Window(Document *doc, Rect *r);
void Pt2Window(Document *doc, Point *pt);
void Pt2Paper(Document *doc, Point *pt);
void GlobalToPaper(Document *doc, Point *pt);
void RefreshScreen(void);

void InitSleepMS(void);
void SleepMS(long millisec);
void SleepTicks(unsigned long ticks);
void SleepTicksWaitButton(unsigned long ticks);

long NMIDIVersion(void);
char *StdVerNumToStr(long verNum, char *verStr);

short PlayResource(Handle, Boolean);
Boolean TrapAvailable(short);

