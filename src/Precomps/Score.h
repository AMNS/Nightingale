/* Score.h for Nightingale */

void InitN103FontRecs(Document *);
void InitFontRecs(Document *);
void FixGraphicFont(Document *, LINK);
Boolean NewDocScore(Document *);
DDIST GetSysHeight(Document *, LINK sysL, short where);
Boolean FillStaffTopArray(Document *, LINK, DDIST []);
void UpdateStaffTops(Document *doc, LINK startL, LINK endL, DDIST staffTop[]);
void FixMeasRectYs(Document *, LINK, Boolean, Boolean, Boolean);
void FixTopSystemYs(Document *, LINK, LINK);
void PageFixSysRects(Document *, LINK, Boolean);
void FixSystemRectYs(Document *, Boolean);
LINK FillStaffArray(Document *, LINK, LINK []);
Boolean FixMeasRectXs(LINK, LINK);
Boolean FixSysMeasRectXs(LINK sysL);
void FixForLedgers(Document *);
void SetStaffLength(Document *, short);
void SetStaffSize(Document *, LINK, LINK, short, short);
Boolean ChangeSysIndent(Document *, LINK, DDIST);
Boolean IndentSystems(Document *, DDIST, Boolean);

LINK AddSysInsertPt(Document *, LINK, short *);
LINK AddSystem(Document *, LINK, short);
void InitParts(Document *, Boolean);
DDIST ExtraSysRSp(Document *,LINK,short);
LINK MakeSystem(Document *, LINK , LINK, LINK, DDIST, short);
LINK MakeStaff(Document *, LINK, LINK, LINK, DDIST, short, DDIST []);
LINK MakeConnect(Document *, LINK, LINK, short);
LINK MakeMeasure(Document *, LINK, LINK, LINK, LINK, short, DDIST [], short);
LINK CreateSystem(Document *, LINK, DDIST, short);
LINK AddPageInsertPt(Document *, LINK);
LINK AddPage(Document *, LINK);
LINK CreatePage(Document *, LINK);

void ScrollToPage(Document *, short);
void GoTo(Document *);
void GoToSel(Document *);
LINK FindPartInfo(Document *, short);

