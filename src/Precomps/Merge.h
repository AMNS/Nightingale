/* Prototypes for Merge.c for Nightingale */

void DoMerge(Document *);

short CheckMerge1(Document *doc,ClipVInfo *clipVInfo);

/* Prototype for CheckMerge.c */

short DoChkMergeDialog(Document *doc);

/* Prototypes for MergeUtils.c */

void MergeFixVOverlaps(Document *doc,LINK initL,LINK succL,short *vMap,VInfo *vInfo);
void MergeFixContext(Document *doc,LINK initL,LINK succL,short minStf,short maxStf,short staffDiff);
void MFixCrossPtrs(Document *doc,LINK startMeas,LINK endMeas,PTIME *durArray);

