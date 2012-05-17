/*	Select.h	for Nightingale */

void DeselectSTAFF(LINK);
void SetDefaultSelection(Document *doc);
void DeselAll(Document *doc);
void DeselAllNoHilite(Document *doc);
void DeselRangeNoHilite(Document *, LINK, LINK);
void DeselRange(Document *, LINK, LINK);
Boolean DeselVoice(INT16 voiceNum, Boolean deselThis);

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

LINK ObjTypeSel(Document *doc, INT16, INT16);
void BoundSelRange(Document *doc);
void LimitSelRange(Document *doc);
void GetOptSelEnds(Document *doc, LINK *startL, LINK *endL);
void CountSelection(Document *, INT16 *, INT16 *);
Boolean ChordSetSel(LINK, INT16);
Boolean ExtendSelChords(Document *);
Boolean ChordHomoSel(LINK, INT16, Boolean);
Boolean ContinSelection(Document *, Boolean);
void OptimizeSelection(Document *);
void UpdateSelection(Document *);
void GetStfSelRange(Document *, INT16, LINK *, LINK *);
void GetVSelRange(Document *, INT16, LINK *, LINK *);
void GetNoteSelRange(Document *, INT16, LINK *, LINK *, Boolean);
Boolean BFSelClearable(Document *, Boolean);
void XLoadSelectSeg(void);
