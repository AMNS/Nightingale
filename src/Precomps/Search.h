/* Search.h: Nightingale header file for Search.c */

void InitSearchParam(SearchParam *);

LINK RPageSearch(LINK, SearchParam *);
LINK LPageSearch(LINK, SearchParam *);
LINK LSystemSearch(LINK, SearchParam *);
LINK RSystemSearch(LINK, SearchParam *);
LINK LStaffSearch(LINK, SearchParam *);
LINK RStaffSearch(LINK, SearchParam *);
LINK LMeasureSearch(LINK, SearchParam *);
LINK RMeasureSearch(LINK, SearchParam *);

LINK L_Search(LINK, short, Boolean, SearchParam *);
LINK LSSearch(LINK, short, short, Boolean, Boolean);
LINK LSISearch(LINK, short, short, Boolean, Boolean);
LINK LVSearch(LINK, short, short, Boolean, Boolean);
LINK LSUSearch(LINK, short, short, Boolean, Boolean);
LINK LVUSearch(LINK, short, short, Boolean, Boolean);
LINK GSearch(Document *, Point, short, Boolean, Boolean, Boolean, SearchParam *);
LINK GSSearch(Document *, Point, short, short, Boolean, Boolean, Boolean, Boolean);

LINK StaffFindSymRight(Document *, Point, Boolean, short);
LINK StaffFindSyncRight(Document *, Point, Boolean, short);
LINK FindValidSymRight(Document *, LINK, short);
LINK StaffFindSymLeft(Document *, Point, Boolean, short);
LINK StaffFindSyncLeft(Document *, Point, Boolean, short);
LINK FindValidSymLeft(Document *, LINK, short);

LINK SyncInVoiceMeas(Document *, LINK, short, Boolean);
LINK EndMeasSearch(Document *doc, LINK startL);
LINK EndSystemSearch(Document *doc, LINK startL);
LINK EndPageSearch(Document *doc, LINK startL);

LINK TimeSearchRight(Document *doc, LINK, short, long, Boolean);
LINK AbsTimeSearchRight(Document *doc, LINK startL, long lTime);
LINK MNSearch(Document *, LINK, short, Boolean, Boolean);
LINK RMSearch(Document *, LINK, const unsigned char *, Boolean);
LINK SSearch(LINK, short, Boolean);
LINK EitherSearch(LINK, short, short, Boolean, Boolean);
LINK FindUnknownDur(LINK, Boolean);

LINK XSysSlurMatch(LINK);
LINK LeftSlurSearch(LINK pL,short v,Boolean isTie);

LINK KSSearch(Document *, LINK, short, Boolean, Boolean);
LINK ClefSearch(Document *, LINK, short, Boolean, Boolean);
LINK FindNoteNum(LINK, short, short);
