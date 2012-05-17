/* Prototypes for Merge.c for Nightingale */

void DoMerge(Document *);

INT16 CheckMerge1(Document *doc,ClipVInfo *clipVInfo);

/* Prototype for CheckMerge.c */

INT16 DoChkMergeDialog(Document *doc);

/* Prototypes for MergeUtils.c */

void MergeFixVOverlaps(Document *doc,LINK initL,LINK succL,INT16 *vMap,VInfo *vInfo);
void MergeFixContext(Document *doc,LINK initL,LINK succL,INT16 minStf,INT16 maxStf,INT16 staffDiff);
void MFixCrossPtrs(Document *doc,LINK startMeas,LINK endMeas,PTIME *durArray);

