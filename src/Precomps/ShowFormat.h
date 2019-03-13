/* Prototypes for ShowFormat.c for Nightingale */

void OffsetSystem(LINK sysL, short dh, short dv);
void UpdateSysMeasYs(Document *doc, LINK sysL, Boolean useLedg, Boolean masterPg);
void VisifyAllObjs(Document *,LINK,LINK,short);
void InvisFirstMeas(LINK);
Boolean	XStfObjOnStaff(LINK, short);
DDIST SetStfInvis(Document *doc, LINK pL, LINK aStaffL);

void DoShowFormat(Document *doc);

void MoveSysOnPage(Document *doc, LINK sysL, long newPos);

void UpdateFormatSystem(Document *doc, LINK sysL, long newPos);
void UpdateFormatStaff(Document *doc, LINK staffL, LINK aStaffL, long newPos);
void UpdateFormatConnect(Document *doc, LINK staffL, short stfAbove, short stfBelow,
							long newPos);

void DoFormatSelect(Document *doc, Point pt);
Boolean DoEditFormat(Document *doc,Point pt,short modifiers,short doubleClick);

short FirstStaffn(LINK);
short LastStaffn(LINK);
short NextVisStaffn(Document *, LINK, short, short);
short NextLimVisStaffn(Document *, LINK, short, short);
short NumVisStaves(LINK);

void InvisifySelStaves(Document *);
void SFInvisify(Document *doc);
DDIST VisifyStf(LINK pL, LINK aStaffL, short staffn);
LINK VisifyAllStaves(Document *doc, LINK staffL);
void SFVisify(Document *doc);

void GrayFormatPage(Document *doc, LINK fromL, LINK toL);
Boolean EditSysRect(Document *doc,Point pt,LINK sysL);
