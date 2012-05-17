/* Prototypes for ShowFormat.c for Nightingale */

void OffsetSystem(LINK sysL, INT16 dh, INT16 dv);
void UpdateSysMeasYs(Document *doc, LINK sysL, Boolean useLedg, Boolean masterPg);
void VisifyAllObjs(Document *,LINK,LINK,INT16);
void InvisFirstMeas(LINK);
Boolean	XStfObjOnStaff(LINK, INT16);
DDIST SetStfInvis(Document *doc, LINK pL, LINK aStaffL);

void DoShowFormat(Document *doc);

void MoveSysOnPage(Document *doc, LINK sysL, long newPos);

void UpdateFormatSystem(Document *doc, LINK sysL, long newPos);
void UpdateFormatStaff(Document *doc, LINK staffL, LINK aStaffL, long newPos);
void UpdateFormatConnect(Document *doc, LINK staffL, INT16 stfAbove, INT16 stfBelow,
							long newPos);

void DoFormatSelect(Document *doc, Point pt);
Boolean DoEditFormat(Document *doc,Point pt,INT16 modifiers,INT16 doubleClick);

INT16 FirstStaffn(LINK);
INT16 LastStaffn(LINK);
INT16 NextStaffn(Document *, LINK, INT16, INT16);
INT16 NextLimStaffn(Document *, LINK, INT16, INT16);
INT16 NumVisStaves(LINK);

void InvisifySelStaves(Document *);
void SFInvisify(Document *doc);
DDIST VisifyStf(LINK pL, LINK aStaffL, INT16 staffn);
LINK VisifyAllStaves(Document *doc, LINK staffL);
void SFVisify(Document *doc);

void GrayFormatPage(Document *doc, LINK fromL, LINK toL);
Boolean EditSysRect(Document *doc,Point pt,LINK sysL);
