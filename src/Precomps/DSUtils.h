/* DSUtils.h for Nightingale */

void InvalMeasure(LINK, short);
void InvalMeasures(LINK, LINK, short);
void InvalSystem(LINK);
void InvalSystems(LINK, LINK);
void InvalSysRange(LINK, LINK);
void InvalSelRange(Document *);
void InvalRange(LINK, LINK);
void EraseAndInvalRange(Document *, LINK, LINK);
void InvalContent(LINK, LINK);
void InvalObject(Document *doc, LINK pL, short doErase);

void FixMeasNums(LINK, short);
Boolean IsFakeMeasure(Document *, LINK);
void UpdatePageNums(Document *);
void UpdateSysNums(Document *, LINK);
Boolean UpdateMeasNums(Document *, LINK);
short GetMeasNum(Document *, LINK);

Boolean IsPtInMeasure(Document *doc, Point pt, LINK sL);

DDIST PageRelxd(LINK pL, PCONTEXT pContext);
DDIST PageRelyd(LINK pL, PCONTEXT pContext);
DDIST GraphicPageRelxd(Document *, LINK, LINK, PCONTEXT);
Point LinkToPt(Document *doc, LINK pL, Boolean toWindow);
DDIST ObjSpaceUsed(Document *doc, LINK pL);
DDIST SysRelxd(LINK);
DDIST Sysxd(Point, DDIST);
DDIST PMDist(LINK, LINK);
Boolean HasValidxd(LINK);
LINK FirstValidxd(LINK, Boolean);
LINK DFirstValidxd(Document *, Document *, LINK, Boolean);
LINK ObjWithValidxd(LINK, Boolean);
DDIST GetSubXD(LINK, LINK);

void ZeroXD(LINK, LINK);
Boolean RealignObj(LINK, Boolean);

DDIST GetSysWidth(Document *);
DDIST GetSysLeft(Document *);
DDIST StaffHeight(Document *doc, LINK, short);
DDIST StaffLength(LINK);
LINK GetLastMeasInSys(LINK sysL);
void GetMeasRange(Document *doc, LINK pL, LINK *startMeas, LINK *endMeas);
DDIST MeasWidth(LINK);
DDIST MeasOccupiedWidth(Document *, LINK, long);
DDIST MeasJustWidth(Document *, LINK, CONTEXT);
Boolean SetMeasWidth(LINK, DDIST);
Boolean MeasFillSystem(LINK);

Boolean IsAfter(LINK, LINK);
Boolean IsAfterIncl(LINK, LINK);
Boolean BetweenIncl(LINK, LINK, LINK);
Boolean WithinRange(LINK, LINK, LINK);
Boolean IsOutside(LINK, LINK, LINK);
Boolean	SamePage(LINK, LINK);
Boolean SameSystem(LINK, LINK);
Boolean SameMeasure(LINK, LINK);
Boolean SyncsAreConsec(LINK, LINK, short, short);
Boolean LinkBefFirstMeas(LINK);
short BeforeFirstPageMeas(LINK);
Boolean BeforeFirstMeas(LINK);

LINK GetCurrentPage(Document *doc);
LINK GetMasterPage(Document *doc);
void GetSysRange(Document *, LINK, LINK, LINK *, LINK *);
LINK FirstSysMeas(LINK pL);
LINK FirstDocMeas(Document *doc);
Boolean FirstMeasInSys(LINK measL);
Boolean LastMeasInSys(LINK);
Boolean IsLastUsedMeasInSys(Document *doc, LINK measL);
LINK LastOnPrevSys(LINK);
Boolean IsLastInSystem(LINK pL);
LINK LastObjInSys(Document *, LINK);
LINK GetNextSystem(Document *, LINK);
Boolean IsLastSysInPage(LINK);
LINK GetLastSysInPage(LINK);
Boolean FirstSysInPage(LINK);
Boolean FirstSysInScore(LINK sysL);
short NSysOnPage(LINK);
LINK LastObjOnPage(Document *, LINK);
Boolean RoomForSystem(Document *, LINK);
Boolean MasterRoomForSystem(Document *, LINK);
LINK NextSystem(Document *, LINK);
LINK GetNext(LINK, Boolean);
LINK RSysOnPage(Document *, LINK);

LINK MoveInMeasure(LINK, LINK, DDIST);
void MoveMeasures(LINK, LINK, DDIST);
void MoveAllMeasures(LINK, LINK, DDIST);
LINK DMoveInMeasure(Document *, Document *, LINK, LINK, DDIST);
void DMoveMeasures(Document *, Document *, LINK, LINK, DDIST);
Boolean CanMoveMeasures(Document *doc, LINK, DDIST);
Boolean CanMoveRestOfSystem(Document *, LINK, DDIST);

long SyncAbsTime(LINK);
LINK MoveTimeInMeasure(LINK, LINK, long);
void MoveTimeMeasures(LINK, LINK, long);

unsigned short CountNotesInRange(short staff, LINK startL, LINK endL, Boolean selectedOnly);
unsigned short CountGRNotesInRange(short staff, LINK startL, LINK endL, Boolean selectedOnly);
unsigned short CountNotes(short, LINK, LINK, Boolean);
unsigned short VCountNotes(short, LINK, LINK, Boolean);
unsigned short CountGRNotes(short, LINK, LINK, Boolean);
unsigned short SVCountNotes(short, short, LINK, LINK, Boolean);
unsigned short SVCountGRNotes(short, short, LINK, LINK, Boolean);
unsigned short CountNoteAttacks(Document *doc);
unsigned short CountObjects(LINK, LINK, short);

void CountInHeaps(Document *, unsigned short [], Boolean);

Boolean HasOtherStemSide(LINK, short);
Boolean IsNoteLeftOfStem(LINK, LINK, Boolean);

short GetStemUpDown(LINK, short);
short GetGRStemUpDown(LINK, short);
void GetExtremeNotes(LINK syncL, short voice, LINK *pLowNoteL, LINK *pHiNoteL);
void GetExtremeGRNotes(LINK syncL, short voice, LINK *pLowNoteL, LINK *pHiNoteL);
LINK FindMainNote(LINK, short);
LINK FindGRMainNote(LINK, short);

void GetObjectLimits(short, short *, short *, Boolean *);
Boolean InDataStruct(Document *doc, LINK, short);
short GetSubObjStaff(LINK, short);
short GetSubObjVoice(LINK, short);
Boolean ObjOnStaff(LINK, short, Boolean);
short CommonStaff(Document *, LINK, LINK);
Boolean ObjHasVoice(LINK pL);
LINK ObjSelInVoice(LINK pL, short v);
LINK StaffOnStaff(LINK staffL, short s);
LINK ClefOnStaff(LINK pL, short s);
LINK KeySigOnStaff(LINK pL, short s);
LINK TimeSigOnStaff(LINK pL, short s);
LINK TempoOnStaff(LINK pL, short s);
LINK MeasOnStaff(LINK pL, short s);
LINK NoteOnStaff(LINK pL, short s);
LINK GRNoteOnStaff(LINK pL, short s);
LINK NoteInVoice(LINK pL, short v, Boolean needSel);
LINK GRNoteInVoice(LINK pL, short v, Boolean needSel);
Boolean SyncInVoice(LINK pL, short voice);
Boolean GRSyncInVoice(LINK pL, short voice);
short SyncVoiceOnStaff(LINK, short);
Boolean SyncInBEAMSET(LINK, LINK);
Boolean SyncInOTTAVA(LINK, LINK);

Boolean PrevTiedNote(LINK, LINK, LINK *, LINK *);
Boolean FirstTiedNote(LINK, LINK, LINK *, LINK *);

LINK ChordNextNR(LINK syncL, LINK theNoteL);

Boolean GetCrossStaff(short, LINK[], STFRANGE *);

Boolean InitialClefExists(LINK clefL);
Boolean BFKeySigExists(LINK keySigL); 
Boolean BFTimeSigExists(LINK timeSigL);

char *StaffPartName(Document *doc, short staff);
void SetTempFlags(Document *, Document *, LINK, LINK, Boolean);
void SetSpareFlags(LINK, LINK, Boolean);
Boolean IsSyncMultiVoice(LINK pL, short staff);
short GetSelectionStaff(Document *doc);
void TweakSubRects(Rect *r, LINK aNoteL, CONTEXT *pContext);
Boolean CompareScoreFormat(Document *doc1, Document *doc2, short pasteType);
LINK GetaMeasL(LINK measL, short stf);
void DisposeMODNRs(LINK, LINK);

LINK Staff2PartL(Document *doc, LINK headL, short stf);
short PartL2Partn(Document *doc, LINK partL);
LINK Partn2PartL(Document *doc, short partn);

LINK VHasTieAcross(LINK node, short voice);
Boolean HasSmthgAcross(Document *, LINK, char *);

short LineSpace2Rastral(DDIST);
DDIST Rastral2LineSpace(short);
short StaffRastral(LINK);
