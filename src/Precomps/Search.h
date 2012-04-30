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

LINK L_Search(LINK, INT16, Boolean, SearchParam *);
LINK LSSearch(LINK, INT16, INT16, Boolean, Boolean);
LINK LSISearch(LINK, INT16, INT16, Boolean, Boolean);
LINK LVSearch(LINK, INT16, INT16, Boolean, Boolean);
LINK LSUSearch(LINK, INT16, INT16, Boolean, Boolean);
LINK LVUSearch(LINK, INT16, INT16, Boolean, Boolean);
LINK GSearch(Document *, Point, INT16, Boolean, Boolean, Boolean, SearchParam *);
LINK GSSearch(Document *, Point, INT16, INT16, Boolean, Boolean, Boolean, Boolean);

LINK StaffFindSymRight(Document *, Point, Boolean, INT16);
LINK StaffFindSyncRight(Document *, Point, Boolean, INT16);
LINK FindValidSymRight(Document *, LINK, INT16);
LINK StaffFindSymLeft(Document *, Point, Boolean, INT16);
LINK StaffFindSyncLeft(Document *, Point, Boolean, INT16);
LINK FindValidSymLeft(Document *, LINK, INT16);

LINK SyncInVoiceMeas(Document *, LINK, INT16, Boolean);
LINK EndMeasSearch(Document *doc, LINK startL);
LINK EndSystemSearch(Document *doc, LINK startL);
LINK EndPageSearch(Document *doc, LINK startL);

LINK TimeSearchRight(Document *doc, LINK, INT16, long, Boolean);
LINK AbsTimeSearchRight(Document *doc, LINK startL, long lTime);
LINK MNSearch(Document *, LINK, INT16, Boolean, Boolean);
LINK RMSearch(Document *, LINK, const unsigned char *, Boolean);
LINK SSearch(LINK, INT16, Boolean);
LINK EitherSearch(LINK, INT16, INT16, Boolean, Boolean);
LINK FindUnknownDur(LINK, Boolean);

LINK XSysSlurMatch(LINK);
LINK LeftSlurSearch(LINK pL,INT16 v,Boolean isTie);

LINK KSSearch(Document *, LINK, INT16, Boolean, Boolean);
LINK ClefSearch(Document *, LINK, INT16, Boolean, Boolean);
LINK FindNoteNum(LINK, INT16, INT16);
