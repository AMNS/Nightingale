/* Debug.h: Nightingale	public header file for Debug files */

enum { MAIN_DSTR, CLIP_DSTR, UNDO_DSTR, MP_DSTR };	/* ??Should probably be in NTypes.h */

Boolean DCheckEverything(Document *, Boolean, Boolean);
short DCheckNode(Document *, LINK, short, Boolean);

Boolean DBadLink(Document *, short, LINK, Boolean);
Boolean DCheckHeaps(Document *);
Boolean DCheckVoiceTable(Document *, Boolean, short *);
Boolean DCheckHeadTail(Document *, LINK, Boolean);
Boolean DCheckSyncSlurs(LINK, LINK);
Boolean DCheckNodeSel(Document *, LINK);
Boolean DCheckSel(Document *, short *, short *);
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
short DBadNoteNum(Document *doc, short clefType, short octType, SignedByte accTable[],
						LINK syncL, LINK theNoteL);
Boolean DCheckNoteNums(Document *doc);

Boolean DoDebug(char *);

void ResetDErrLimit(void);
Boolean DErrLimit(void);

void DKSPrintf(PKSINFO);
void DisplayNode(Document *, LINK, short, Boolean, Boolean, Boolean);
void MemUsageStats(Document *);
void DisplayIndexNode(Document *, LINK, short, short *);
void DHexDump(unsigned char *, long, short, short);

#define DebugPrintf printf
