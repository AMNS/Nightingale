/*	Utility.h for Nightingale */

DDIST CalcYStem(Document *, DDIST, INT16, Boolean, DDIST, INT16, INT16, Boolean);
DDIST GetNoteYStem(Document *, LINK, LINK, CONTEXT);
DDIST GetGRNoteYStem(LINK, CONTEXT);
Boolean ShortenStem(LINK, CONTEXT, Boolean);
Boolean GetCStemInfo(Document *, LINK, LINK, CONTEXT, INT16 *);
Boolean GetStemInfo(Document *, LINK, LINK, INT16 *);
Boolean GetCGRStemInfo(Document *, LINK, LINK, CONTEXT, INT16 *);
Boolean GetGRStemInfo(Document *, LINK, LINK, INT16 *);
INT16 GetLineAugDotPos(INT16, Boolean);
DDIST ExtraSysWidth(Document *);

void ApplHeapCheck(void);

INT16 Char2Dur(char);
INT16 Dur2Char(INT16);
INT16 GetSymTableIndex(char);
INT16 SymType(char);
char Objtype2Char(SignedByte);

Rect StrToObjRect(unsigned char *string);
void GetNFontInfo(short, short, short, FontInfo *);
INT16 NStringWidth(Document *, const unsigned char *, INT16, INT16, INT16);
INT16 NPtStringWidth(Document *, const unsigned char *, INT16, INT16, INT16);
void GetNPtStringBBox(Document *, unsigned char *, INT16, INT16, INT16, Boolean, Rect *);
INT16 NPtGraphicWidth(Document *, LINK, PCONTEXT);

INT16 MaxNameWidth(Document *, INT16);
double PartNameMargin(Document *, INT16);

void SetFont(INT16);
Rect StringRect(Str255 *);

PCONTEXT AllocContext(void);
SPACETIMEINFO *AllocSpTimeInfo(void);

GWorldPtr MakeGWorld(INT16, INT16, Boolean);
void DestroyGWorld(GWorldPtr);
Boolean SaveGWorld(void);
Boolean RestoreGWorld(void);
Boolean LockGWorld(GWorldPtr);
void UnlockGWorld(GWorldPtr);

GrafPtr NewGrafPort(INT16, INT16);
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

INT16 GCD(INT16 m, INT16 n);
double RoundDouble(double, double);
INT16 RoundSignedInt(INT16 value, INT16 quantum);
INT16 InterpY(INT16, INT16, INT16, INT16, INT16);

long FindIntInString(unsigned char *);
INT16 BlockCompare(void *src, void *dst, INT16 len);
INT16 RelIndexToSize(INT16, DDIST);
INT16 GetTextSize(Boolean, INT16, DDIST);

INT16 GetFontIndex(Document *, unsigned char *);
INT16 User2HeaderFontNum(Document *, INT16);
INT16 Header2UserFontNum(INT16);

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

INT16 PlayResource(Handle, Boolean);
Boolean TrapAvailable(short);

Boolean GetSN(char *);

Boolean EnforcePageLimit(Document *doc);
