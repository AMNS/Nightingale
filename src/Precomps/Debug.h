/* Debug.h: Nightingale	public header file for Debug files */

enum { MAIN_DSTR, CLIP_DSTR, UNDO_DSTR, MP_DSTR };	/* ??Should probably be in NTypes.h */

Boolean DCheckEverything(Document *, Boolean, Boolean);
INT16 DCheckNode(Document *, LINK, INT16, Boolean);

Boolean DBadLink(Document *, INT16, LINK, Boolean);
Boolean DCheckHeaps(Document *);
Boolean DCheckVoiceTable(Document *, Boolean, INT16 *);
Boolean DCheckHeadTail(Document *, LINK, Boolean);
Boolean DCheckSyncSlurs(LINK, LINK);
Boolean DCheckNodeSel(Document *, LINK);
Boolean DCheckSel(Document *, INT16 *, INT16 *);
Boolean DCheckHeirarchy(Document *);
Boolean DCheckJDOrder(Document *);
Boolean DCheckBeams(Document *);
Boolean DCheckOctaves(Document *);
Boolean DCheckSlurs(Document *);
Boolean DCheckTuplets(Document *, Boolean);
Boolean DCheckPlayDurs(Document *, Boolean);
Boolean DCheckHairpins(Document *);
Boolean DCheckContext(Document *);
Boolean DCheckRedundKS(Document *);
Boolean DCheckRedundTS(Document *);
Boolean DCheckMeasDur(Document *);
Boolean DCheckUnisons(Document *);
Boolean DCheckNEntries(Document *);
INT16 DBadNoteNum(Document *doc, INT16 clefType, INT16 octType, SignedByte accTable[],
						LINK syncL, LINK theNoteL);
Boolean DCheckNoteNums(Document *doc);

Boolean DoDebug(char *);

void ResetDErrLimit(void);
Boolean DErrLimit(void);

void DKSPrintf(PKSINFO);
void DisplayNode(Document *, LINK, INT16, Boolean, Boolean, Boolean);
void MemUsageStats(Document *);
void DisplayIndexNode(Document *, LINK, INT16, INT16 *);
void DHexDump(unsigned char *, long, INT16, INT16);

#define DebugPrintf printf
