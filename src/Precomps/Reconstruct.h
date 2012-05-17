/* Prototypes for Reconstruct.c for Nightingale */

Boolean VoiceInSelRange(Document *doc, INT16 v);
Boolean CheckContinVoice(Document *doc);

void InitDurArray(PTIME *durArray,INT16 nInMeas);

void SetPlayDurs(Document *doc,PTIME *durArray,INT16 nInMeas,LINK startMeas,LINK endMeas);
INT16 SetPTimes(Document *, PTIME *, INT16, SPACETIMEINFO *, LINK, LINK);
void SetLinkOwners(PTIME *durArray,INT16 nInMeas,LINK startMeas,LINK endMeas);

INT16 CompactDurArray(PTIME *pDurArray,PTIME *qDurArray, INT16 numNotes);
LINK CopySubObjs(Document *doc,LINK newObjL,PTIME *pTime);
LINK CopySync(Document *doc,INT16 subCount,PTIME *pTime);
Boolean ReconstructArray(PTIME *durArray,INT16 arrBound,LINK prevL,LINK insertL);

LINK CopyNotes(Document *doc, LINK subL, LINK newSubL, long pDur, long pTime);
LINK CopyClipNotes(Document *doc, LINK subL, LINK newSubL, long pTime);

void InsertJDBefore(LINK, LINK);
void InsertJITBefore(LINK, LINK);
void InsertJIPBefore(LINK, LINK);

void LocateJITObj(Document *, LINK, LINK, LINK, PTIME *);
void LocateJIPObj(Document *, LINK, LINK, LINK, PTIME *);
void LocateGenlJDObj(Document *, LINK, PTIME *);
void LocateJDObj(Document *, LINK, LINK, PTIME *);

void DebugDurArray(INT16 narrBound, PTIME *durArray);

void RelocateObjs(Document *,LINK,LINK,LINK,LINK,PTIME *);
void RelocateGenlJDObjs(Document *,LINK,LINK,PTIME *);

void InitCopyMap(LINK startL,LINK endL,COPYMAP *mergeMap);
void SetCopyMap(LINK startL,LINK endL,INT16 numObjs,COPYMAP *mergeMap);
LINK GetCopyMap(LINK link,INT16 numObjs,COPYMAP *mergeMap);
INT16 GetNumClObjs(Document *);

void RelocateClObjs(Document *,LINK,LINK,LINK,LINK,PTIME *,INT16,COPYMAP *,INT16 *);
void RelocateClGenlJDObjs(Document *,LINK,LINK,LINK,LINK,PTIME *,INT16,COPYMAP *,INT16 *);

LINK GetFirstBeam(LINK syncL);
LINK GetFirstTuplet(LINK syncL);
LINK GetFirstOctava(LINK syncL);
LINK GetFirstSlur(PTIME *);
LINK GetBaseLink(Document *doc, INT16 type, LINK startMeasL);

void FixNBJDPtrs(LINK startL, LINK endL, PTIME *durArray);
void FixCrossPtrs(Document *, LINK, LINK, PTIME *, PTIME *);

void FixStfAndVoice(LINK pL,INT16 stfDiff,INT16 *vMap);
