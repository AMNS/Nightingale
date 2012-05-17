/* Nightingale header file for Octava.c */

void DoOctava(Document *);
void DoRemoveOctava(Document *);
void SetOctavaYPos(Document *, LINK);
LINK CreateOCTAVA(Document *, LINK, LINK, INT16, INT16, Byte, Boolean, Boolean);
long GetOctTypeNum(LINK, Boolean *);
void DrawOCTAVA(Document *, LINK, CONTEXT []);
void RemoveOctOnStf(Document *,LINK,LINK,LINK,INT16);
void UnOctavaRange(Document *, LINK, LINK, INT16);

LINK FirstInOctava(LINK);
LINK LastInOctava(LINK);
LINK HasOctAcross(LINK node,INT16 staff,Boolean skipNode);
LINK HasOctavaAcross(LINK, INT16);
LINK OctOnStaff(LINK, INT16);
LINK GROctOnStaff(LINK, INT16);
LINK HasOctavaAcrossIncl(LINK, INT16);
LINK HasOctavaAcrossPt(Document *, Point, INT16);

Boolean SelRangeChkOctava(INT16, LINK, LINK);
INT16 OctCountNotesInRange(INT16 stf,LINK startL,LINK endL, Boolean selectedOnly);
void DashedLine(INT16 x1, INT16 y1, INT16 x2, INT16 y2);
void DrawOctBracket(DPoint, DPoint, INT16, DDIST, Boolean, PCONTEXT);
void XLoadOctavaSeg(void);




