/* Score.h for Nightingale */

void InitN103FontRecs(Document *);
void InitFontRecs(Document *);
void FixGraphicFont(Document *, LINK);
Boolean NewDocScore(Document *);
DDIST GetSysHeight(Document *, LINK sysL, INT16 where);
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
void SetStaffLength(Document *, INT16);
void SetStaffSize(Document *, LINK, LINK, INT16, INT16);
Boolean ChangeSysIndent(Document *, LINK, DDIST);
Boolean IndentSystems(Document *, DDIST, Boolean);

LINK AddSysInsertPt(Document *, LINK, INT16 *);
LINK AddSystem(Document *, LINK, INT16);
void InitParts(Document *, Boolean);
DDIST ExtraSysRSp(Document *,LINK,INT16);
LINK MakeSystem(Document *, LINK , LINK, LINK, DDIST, INT16);
LINK MakeStaff(Document *, LINK, LINK, LINK, DDIST, INT16, DDIST []);
LINK MakeConnect(Document *, LINK, LINK, INT16);
LINK MakeMeasure(Document *, LINK, LINK, LINK, LINK, INT16, DDIST [], INT16);
LINK CreateSystem(Document *, LINK, DDIST, INT16);
LINK AddPageInsertPt(Document *, LINK);
LINK AddPage(Document *, LINK);
LINK CreatePage(Document *, LINK);

void ScrollToPage(Document *, INT16);
void GoTo(Document *);
void GoToSel(Document *);
LINK FindPartInfo(Document *, INT16);
