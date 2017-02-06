/*	Select.h	for Nightingale */

void DeselectSTAFF(LINK);
void SetDefaultSelection(Document *doc);
void DeselAll(Document *doc);
void DeselAllNoHilite(Document *doc);
void DeselRangeNoHilite(Document *, LINK, LINK);
void DeselRange(Document *, LINK, LINK);
Boolean DeselVoice(short voiceNum, Boolean deselThis);

LINK DoOpenSymbol(Document *, Point);
void DoSelect(Document *, Point, Boolean, Boolean, Boolean, short);
void SelectAll(Document *);
void SelRangeNoHilite(Document *doc, LINK startL, LINK endL);
void SelAllNoHilite(Document *);
void DeselectNode(LINK);
void SelAllSubObjs(LINK);
void SelectObject(LINK);
void SelectRange(Document *, LINK, LINK, short, short);

void ExtendSelection(Document *);

LINK ObjTypeSel(Document *doc, short, short);
void BoundSelRange(Document *doc);
void LimitSelRange(Document *doc);
void GetOptSelEnds(Document *doc, LINK *startL, LINK *endL);
void CountSelection(Document *, short *, short *);
Boolean ChordSetSel(LINK, short);
Boolean ExtendSelChords(Document *);
Boolean ChordHomoSel(LINK, short, Boolean);
Boolean SelAttachedToPage(Document *doc);
Boolean ContinSelection(Document *, Boolean);
void OptimizeSelection(Document *);
void UpdateSelection(Document *);
void GetStfSelRange(Document *, short, LINK *, LINK *);
void GetVSelRange(Document *, short, LINK *, LINK *);
void GetNoteSelRange(Document *, short, LINK *, LINK *, Boolean);
Boolean BFSelClearable(Document *, Boolean);
void XLoadSelectSeg(void);
