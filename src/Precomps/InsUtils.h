/* InsUtils.h for Nightingale */

#define ACCSIZE 18								/* Point size for feedback accs. */
#define ACCBOXWIDE (ACCSIZE-4)

Boolean GetPaperMouse(Point *pt, Rect *paper);
INT16 ForceInMeasure(Document *, INT16);
void GetInsRect(Document *, LINK, Rect *);
Boolean TrackAndAddHairpin(Document *, LINK, Point, INT16, INT16, INT16, PCONTEXT); 
Boolean TrackAndAddEnding(Document *, Point, INT16);
Boolean TrackAndAddLine(Document *, Point, INT16, INT16);

INT16 GetPitchLim(Document *, LINK, INT16, Boolean);
INT16 CalcPitchLevel(INT16, PCONTEXT, LINK, INT16);

LINK FindSync(Document *, Point pt, Boolean isGraphic, INT16 clickStaff);
Boolean AddCheckVoiceTable(Document *, INT16);
Boolean AddNoteCheck(Document *, LINK addToSyncL, INT16 staff, INT16 symIndex);
LINK FindGRSync(Document *, Point pt, Boolean isGraphic, INT16 clickStaff);
Boolean AddGRNoteCheck(Document *, LINK addToSyncL, INT16 staff);
LINK FindSyncRight(Document *, Point, Boolean);
LINK FindSyncLeft(Document *, Point, Boolean);
LINK FindGRSyncRight(Document *, Point, Boolean);
LINK FindGRSyncLeft(Document *, Point, Boolean);
LINK FindSymRight(Document *, Point, Boolean, Boolean);
LINK FindLPI(Document *, Point, INT16, INT16, Boolean);	
LINK ObjAtEndTime(Document *, LINK, INT16, INT16, long *, Boolean, Boolean);
LINK LocateInsertPt(LINK);

INT16 FindStaff(Document *, Point);
LINK SetCurrentSystem(Document *, Point);
LINK GetCurrentSystem(Document *);
void CheckSuccSystem(LINK);
INT16 InsError(void);
void AddNoteFixBeams(Document *, LINK, LINK);
void AddGRNoteFixBeams(Document *, LINK, LINK);
void UntupleObj(Document *, LINK, LINK, LINK, INT16);
void AddNoteFixTuplets(Document *, LINK, LINK, INT16);
Boolean AddNoteFixOctavas(LINK, LINK);
void AddNoteFixSlurs(Document *, LINK, INT16);
void AddNoteFixGraphics(LINK, LINK);

void FixWholeRests(Document *, LINK);
void FixNewMeasAccs(Document *, LINK);
Boolean InsFixMeasNums(Document *, LINK);
LINK FindGraphicObject(Document *, Point, INT16 *, INT16 *);
Boolean ChkGraphicRelObj(LINK, char);
LINK FindTempoObject(Document *, Point, INT16 *, INT16 *);
LINK FindEndingObject(Document *, Point, INT16 *);
void UpdateTailxd(Document *doc);

void CenterTextGraphic(Document *doc,LINK newL);

void FixInitialxds(Document *, LINK, LINK, INT16);
void FixInitialKSxds(Document *, LINK, LINK, INT16);

Boolean NewObjInit(Document *, INT16, INT16 *, char, INT16, CONTEXT *);
void NewObjSetup(Document *, LINK, INT16, DDIST);
LINK NewObjPrepare(Document *, INT16, INT16 *, char, INT16, INT16, CONTEXT *);
void NewObjCleanup(Document *, LINK, INT16);
void XLoadInsUtilsSeg(void);
