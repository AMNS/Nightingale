/* Debug.h: Nightingale	public header file for Debug files */

enum { MAIN_DSTR, CLIP_DSTR, UNDO_DSTR, MP_DSTR };

Boolean DCheckEverything(Document *, Boolean, Boolean);
short DCheckNode(Document *, LINK, short, Boolean);

Boolean DBadLink(Document *, short, LINK, Boolean);
Boolean DCheckHeaps(Document *);
Boolean DCheckHeadTail(Document *, LINK, Boolean);
Boolean DCheckSyncSlurs(Document *, LINK, LINK);
Boolean DCheckNodeSel(Document *, LINK);
Boolean DCheckSel(Document *, short *, short *);
Boolean DCheckVoiceTable(Document *, Boolean, short *);
Boolean DCheckHeirarchy(Document *);
Boolean DCheckJDOrder(Document *);
Boolean DCheckBeams(Document *, Boolean);
Boolean DCheckOttavas(Document *);
Boolean DCheckSlurs(Document *);
Boolean DCheckTuplets(Document *, Boolean);
Boolean DCheckHairpins(Document *);
Boolean DCheckContext(Document *);

Boolean DCheckPlayDurs(Document *, Boolean);
Boolean DCheckTempi(Document *);
Boolean DCheckRedundantKS(Document *);
Boolean DCheckExtraTS(Document *);
Boolean DCheckCautionaryTS(Document *doc);
Boolean DCheckMeasDur(Document *);
Boolean DCheckUnisons(Document *);
Boolean DCheckNEntries(Document *);
short DBadNoteNum(Document *doc, short clefType, short octType, SignedByte accTable[],
						LINK syncL, LINK theNoteL);
Boolean DCheckNoteNums(Document *doc);

Boolean DoDebug(char *);

void ResetDErrLimit(void);
Boolean DErrLimit(void);

void KeySigSprintf(PKSINFO, char []);
void DKeySigPrintf(PKSINFO);
void DisplayNode(Document *, LINK, short, Boolean, Boolean, Boolean);
void MemUsageStats(Document *);
void DisplayIndexNode(Document *, LINK, short, short *);
void NHexDump(short logLevel, char *label, unsigned char *pBuffer, long nBytes,
				short nPerGroup, short nPerLine);

/* If we're running inside Xcode, #define'ing _DebugPrintf_ as simply _printf_ is OK:
	then DebugPrintf output will appear in the Run Log window. But if we're not in
	Xcode, its output seems to disappear without a trace. A better alternative is
	the BSD function syslog(3), but using it will require some rethinking, including
	in all probability modifying every call. --DAB, 2017(?)
*/
#define DebugPrintf printf
