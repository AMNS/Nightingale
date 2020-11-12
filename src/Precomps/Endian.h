/* Endian.h for Nightingale. Created by Michel Alexandre Salim on 4/13/08. */

/* PowerPCs are Big Endian; Intel processors are Little Endian. */

#if TARGET_RT_LITTLE_ENDIAN
#define FIX_END(v) FIX_ENDIAN(sizeof(v), v)
#define FIX_ENDIAN(n, v) if (n==2) { v = CFSwapInt16BigToHost(v); } \
else if (n==4) { v = CFSwapInt32BigToHost(v); }						\
else { LogPrintf(LOG_ERR, "PROGRAM ERROR: FIX_ENDIAN called with size %d\n", n); exit(147); }
#else
#define FIX_END(v) v = v
#define FIX_ENDIAN(n, v) v = v
#endif

void		EndianFixRect(Rect *pRect);
void		EndianFixPoint(Point *pPoint);
void		EndianFixConfig(void);
void		EndianFixMIDIModNRTable(void);
void		EndianFixPaletteGlobals(short idx);
void		EndianFixSpaceMap(Document *doc);
void		EndianFixDocumentHdr(Document *doc);
void		EndianFixScoreHdr(Document *doc);
void		EndianFixHeapHdr(Document *doc, HEAP *heap);
void		FixEndian2(unsigned short *arg);
void		FixEndian4(unsigned long *arg);
