/* Prototypes for Reconstruct.c for Nightingale */

Boolean VoiceInSelRange(Document *doc, short v);
Boolean CheckContinVoice(Document *doc);

void InitDurArray(PTIME *durArray,short nInMeas);

void SetPlayDurs(Document *doc,PTIME *durArray,short nInMeas,LINK startMeas,LINK endMeas);
short SetPTimes(Document *, PTIME *, short, SPACETIMEINFO *, LINK, LINK);
void SetLinkOwners(PTIME *durArray,short nInMeas,LINK startMeas,LINK endMeas);

short CompactDurArray(PTIME *pDurArray,PTIME *qDurArray, short numNotes);
LINK CopySubObjs(Document *doc,LINK newObjL,PTIME *pTime);
LINK CopySync(Document *doc,short subCount,PTIME *pTime);
Boolean ReconstructArray(PTIME *durArray,short arrBound,LINK prevL,LINK insertL);

LINK CopyNotes(Document *doc, LINK subL, LINK newSubL, long pDur, long pTime);
LINK CopyClipNotes(Document *doc, LINK subL, LINK newSubL, long pTime);

void InsertJDBefore(LINK, LINK);
void InsertJITBefore(LINK, LINK);
void InsertJIPBefore(LINK, LINK);

void DebugDurArray(short narrBound, PTIME *durArray);

void RelocateObjs(Document *,LINK,LINK,LINK,LINK,PTIME *);
void RelocateGenlJDObjs(Document *,LINK,LINK,PTIME *);

void InitCopyMap(LINK startL,LINK endL,COPYMAP *mergeMap);
void SetCopyMap(LINK startL,LINK endL,short numObjs,COPYMAP *mergeMap);
LINK GetCopyMap(LINK link,short numObjs,COPYMAP *mergeMap);
short GetNumClObjs(Document *);

void RelocateClObjs(Document *,LINK,LINK,LINK,LINK,PTIME *,short,COPYMAP *,short *, Boolean);
void RelocateClGenlJDObjs(Document *,LINK,LINK,LINK,LINK,PTIME *,short,COPYMAP *,short *, Boolean);

LINK GetFirstBeam(LINK syncL);
LINK GetFirstTuplet(LINK syncL);
LINK GetFirstOttava(LINK syncL);
LINK GetFirstSlur(PTIME *);
LINK GetBaseLink(Document *doc, short type, LINK startMeasL);

void FixNBJDLinks(LINK startL, LINK endL, PTIME *durArray);
void FixCrossPtrs(Document *, LINK, LINK, PTIME *, PTIME *);

void FixStfAndVoice(LINK pL,short stfDiff,short *vMap);
