/*	Dragging.h forNightingale: prototypes for general bitmap dragging functions. */

/* Prototypes for DragUtils.c */

void GetHDragLims(Document *,LINK,LINK,short,CONTEXT,DDIST *,DDIST *);

void SDInsertLedgers(LINK pL, LINK aNoteL, short halfLine, PCONTEXT pContext);
short SDSetAccidental(Document *doc, GrafPtr accPort, Rect accBox, Rect saveBox, short accy);
void SDMIDIFeedback(Document *doc, short *noteNum, short useChan, short acc, short transp, 
						  short midCHalfLn, short halfLn, short octTransp, short ioRefNum);

DDIST Getystem(short voice, LINK sync);
DDIST GetGRystem(short voice, LINK GRsync);
void SDFixStemLengths(Document *doc, LINK beamL);
void SDFixGRStemLengths(LINK beamL);
DDIST SDGetClosestClef(Document *doc, short halfLnDiff, LINK pL, LINK subObjL, CONTEXT context);

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

/* Prototypes for DragSet.c */

void SetClefFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, short xp, short yp, Boolean vert);
void SetKeySigFields(LINK pL, LINK subObjL, DDIST xdDiff, short xp);
void SetTimeSigFields(LINK pL, LINK subObjL, DDIST xdDiff, short xp);
void SetNoteFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, short xp, short yp, Boolean vert, Boolean beam, short newAcc);
void SetGRNoteFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, short xp, short yp, Boolean vert, Boolean beam, short newAcc);
void SetDynamicFields(LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);
void SetRptEndFields(LINK pL, LINK subObjL, DDIST xdDiff, short xp);
void SetEndingFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);
long SetMeasureFields(Document *doc, LINK pL, DDIST xdDiff);
void SetPSMeasFields(Document *doc, LINK pL, DDIST xdDiff);
void SetTupletFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);
void SetOttavaFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);
void SetGraphicFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);
void SetTempoFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);
void SetSpaceFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);
void SetBeamFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);
void SetSlurFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp);

void SetForNewPitch(Document *doc, LINK pL, LINK subObjL, CONTEXT context, short pitchLev, short acc);

/* Prototypes for Dragging.c */

Boolean HandleSymDrag(Document *doc, LINK pL, LINK subObjL, Point pt, unsigned char glyph);
Boolean DoSymbolDrag(Document *doc, Point);
