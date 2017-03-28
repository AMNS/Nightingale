/* InsUtils.h for Nightingale */

#define ACCSIZE 18								/* Point size for feedback accs. */
#define ACCBOXWIDE (ACCSIZE-4)

Boolean GetPaperMouse(Point *pt, Rect *paper);
short ForceInMeasure(Document *, short);
void GetInsRect(Document *, LINK, Rect *);
Boolean TrackAndAddHairpin(Document *, LINK, Point, short, short, short, PCONTEXT); 
Boolean TrackAndAddEnding(Document *, Point, short);
Boolean TrackAndAddLine(Document *, Point, short, short);

short GetPitchLim(Document *, LINK, short, Boolean);
short CalcPitchLevel(short, PCONTEXT, LINK, short);

LINK FindSync(Document *, Point pt, Boolean isGraphic, short clickStaff);
Boolean AddCheckVoiceTable(Document *, short);
Boolean AddNoteCheck(Document *, LINK addToSyncL, short staff, short symIndex);
LINK FindGRSync(Document *, Point pt, Boolean isGraphic, short clickStaff);
Boolean AddGRNoteCheck(Document *, LINK addToSyncL, short staff);
LINK FindSyncRight(Document *, Point, Boolean);
LINK FindSyncLeft(Document *, Point, Boolean);
LINK FindGRSyncRight(Document *, Point, Boolean);
LINK FindGRSyncLeft(Document *, Point, Boolean);
LINK FindSymRight(Document *, Point, Boolean, Boolean);
LINK FindLPI(Document *, Point, short, short, Boolean);	
LINK ObjAtEndTime(Document *, LINK, short, short, long *, Boolean, Boolean);
LINK FindInsertPt(LINK);

short FindStaffSetSys(Document *, Point);
LINK SetCurrentSystem(Document *, Point);
LINK GetCurrentSystem(Document *);
void CheckSuccSystem(LINK);
short InsError(void);
void AddNoteFixBeams(Document *, LINK, LINK);
void AddGRNoteFixBeams(Document *, LINK, LINK);
void UntupleObj(Document *, LINK, LINK, LINK, short);
void AddNoteFixTuplets(Document *, LINK, LINK, short);
Boolean AddNoteFixOttavas(LINK, LINK);
void AddNoteFixSlurs(Document *, LINK, short);
void AddNoteFixGraphics(LINK, LINK);

void FixWholeRests(Document *, LINK);
void FixNewMeasAccs(Document *, LINK);
Boolean InsFixMeasNums(Document *, LINK);
LINK FindGraphicObject(Document *, Point, short *, short *);
Boolean ChkGraphicRelObj(LINK, char);
LINK FindTempoObject(Document *, Point, short *, short *);
LINK FindEndingObject(Document *, Point, short *);
void UpdateTailxd(Document *doc);

void CenterTextGraphic(Document *doc,LINK newL);

void FixInitialxds(Document *, LINK, LINK, short);
void FixInitialKSxds(Document *, LINK, LINK, short);

Boolean NewObjInit(Document *, short, short *, char, short, CONTEXT *);
void NewObjSetup(Document *, LINK, short, DDIST);
LINK NewObjPrepare(Document *, short, short *, char, short, short, CONTEXT *);
void NewObjCleanup(Document *, LINK, short);
void XLoadInsUtilsSeg(void);
