/*	Dragging.h forNightingale: prototypes for general bitmap dragging functions. */

/* Prototypes for DragUtils.c */

void GetHDragLims(Document *,LINK,LINK,INT16,CONTEXT,DDIST *,DDIST *);

void SDInsertLedgers(LINK pL, LINK aNoteL, INT16 halfLine, PCONTEXT pContext);
INT16 SDSetAccidental(Document *doc, GrafPtr accPort, Rect accBox, Rect saveBox, INT16 accy);
void SDMIDIFeedback(Document *doc, INT16 *noteNum, INT16 useChan, INT16 acc, INT16 transp, 
						  INT16 midCHalfLn, INT16 halfLn, INT16 octTransp, short ioRefNum);

DDIST Getystem(INT16 voice, LINK sync);
DDIST GetGRystem(INT16 voice, LINK GRsync);
void SDFixStemLengths(Document *doc, LINK beamL);
void SDFixGRStemLengths(LINK beamL);
DDIST SDGetClosestClef(Document *doc, INT16 halfLnDiff, LINK pL, LINK subObjL, CONTEXT context);

Rect GetSysRect(Document *doc, LINK pL);
Rect GetMeasRect(Document *, LINK);
Rect Get2MeasRect(Document *, LINK, LINK);
Rect SDGetMeasRect(Document *, LINK, LINK);

GrafPtr NewMeasGrafPort(Document *doc, LINK);
GrafPtr New2MeasGrafPort(Document *, LINK);
GrafPtr NewNMeasGrafPort(Document *, LINK, LINK);

GrafPtr NewBeamGrafPort(Document *, LINK, Rect *);

GrafPtr SetupMeasPorts(Document *doc, LINK);
GrafPtr Setup2MeasPorts(Document *doc, LINK);
GrafPtr SetupNMeasPorts(Document *doc, LINK, LINK);
GrafPtr SetupSysPorts(Document *doc, LINK);

void DragBeforeFirst(Document *doc, LINK, Point);
void PageRelDrag(Document *doc, LINK, Point);
void SDSpaceMeasures(Document *doc, LINK pL, long spaceFactor);

void SDCleanUp(Document *doc, GrafPtr oldPort, long spaceFactor, LINK pL, Boolean dirty, DDIST xdDiff, DDIST ydDiff);
void DisposMeasPorts(void);
void ErrDisposPorts(void);

Boolean DelSlurTie(Document *, LINK);

/* Prototypes for DragSet.c */

void SetClefFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp, Boolean vert);
void SetKeySigFields(LINK pL, LINK subObjL, DDIST xdDiff, INT16 xp);
void SetTimeSigFields(LINK pL, LINK subObjL, DDIST xdDiff, INT16 xp);
void SetNoteFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp, Boolean vert, Boolean beam, INT16 newAcc);
void SetGRNoteFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp, Boolean vert, Boolean beam, INT16 newAcc);
void SetDynamicFields(LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);
void SetRptEndFields(LINK pL, LINK subObjL, DDIST xdDiff, INT16 xp);
void SetEndingFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);
long SetMeasureFields(Document *doc, LINK pL, DDIST xdDiff);
void SetPSMeasFields(Document *doc, LINK pL, DDIST xdDiff);
void SetTupletFields(LINK pL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);
void SetOctavaFields(LINK pL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);
void SetGraphicFields(LINK pL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);
void SetTempoFields(LINK pL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);
void SetSpaceFields(LINK pL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);
void SetBeamFields(LINK pL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);
void SetSlurFields(LINK pL, DDIST xdDiff, DDIST ydDiff, INT16 xp, INT16 yp);

void SetForNewPitch(Document *doc, LINK pL, LINK subObjL, CONTEXT context, INT16 pitchLev, INT16 acc);

/* Prototypes for Dragging.c */

Boolean HandleSymDrag(Document *doc, LINK pL, LINK subObjL, Point pt, unsigned char glyph);
Boolean DoSymbolDrag(Document *doc, Point);
