/* MasterPage.h: prototypes for MasterPage.c for Nightingale */

void FixLedgerYSp(Document *);

void ExportMasterPage(Document *);

void DoMasterStfLen(Document *);
void DoMasterStfSize(Document *);
void DoMasterStfLines(Document *);
void MPEditMargins(Document *);

void SetupMasterMenu(Document *, Boolean);
Boolean SetupMasterView(Document *);
Boolean ExitMasterView(Document *);
void EnterMasterView(Document *);
void ImportMasterPage(Document *);

void MPUpdateStaffTops(Document *, LINK, LINK);
void UpdateMPSysRectHeight(Document *, DDIST);
void UpdateDraggedSystem(Document *, long);
void UpdateDraggedStaff(Document *, LINK, LINK, long);
void UpdateMasterMargins(Document *);

void Score2MasterPage(Document *doc);
void ReplaceMasterPage(Document *doc);
void VisifyMasterStaves(Document *doc);
Boolean NewMasterPage(Document *, DDIST sysTop, Boolean fromDoc);
Boolean CopyMasterRange(Document *, LINK, LINK, LINK);

void MPDeletePart(Document *doc);
short CountConnParts(Document *,LINK,LINK,LINK,LINK);
void UpdateConnFields(LINK connectL,short stfDiff,short lastStf);

void StoreAllConnectParts(LINK headL);
void StoreConnectPart(LINK headL,LINK aConnectL);
void MPAddPart(Document *doc);

short GetPartSelRange(Document *doc, LINK *firstPart, LINK *lastPart);
short PartRangeIsSel(Document *doc);
short GroupIsSel(Document *doc);
short PartIsSel(Document *doc);

void MPSetInstr(Document *doc, PPARTINFO pPart);
void GetSelStaves(LINK staffL,short *minStf,short *maxStf);
void DoGroupParts(Document *doc);
void DoUngroupParts(Document *doc);
Boolean DoMake1StaffParts(Document *doc);

void MPDistributeStaves(Document *);
