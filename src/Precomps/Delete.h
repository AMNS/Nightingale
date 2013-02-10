/*	Delete.h for Nightingale */

void DeleteMeasure(Document *, LINK);
void FixSyncForSlur(LINK, short, Boolean, Boolean);
void DelModsForSync(LINK, LINK);
void ChangeSpaceBefFirst(Document *, LINK, DDIST, Boolean);
void PPageFixDelSlurs(Document *);
void DeleteXSysSlur(Document *, LINK);
void FixDelCrossSysObjs(Document *);
void FixDelCrossSysBeams(Document *);
void FixDelCrossSysSlurs(Document *);
void FixDelCrossSysHairpins(Document *);
void FixAccsForNoTie(Document *, LINK);
void DeleteSelection(Document *, Boolean, Boolean *);
