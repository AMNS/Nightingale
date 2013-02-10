/* Nightingale header file for Octava.c */

void DoOctava(Document *);
void DoRemoveOctava(Document *);
void SetOctavaYPos(Document *, LINK);
LINK CreateOCTAVA(Document *, LINK, LINK, short, short, Byte, Boolean, Boolean);
long GetOctTypeNum(LINK, Boolean *);
void DrawOCTAVA(Document *, LINK, CONTEXT []);
void RemoveOctOnStf(Document *,LINK,LINK,LINK,short);
void UnOctavaRange(Document *, LINK, LINK, short);

LINK FirstInOctava(LINK);
LINK LastInOctava(LINK);
LINK HasOctAcross(LINK node,short staff,Boolean skipNode);
LINK HasOctavaAcross(LINK, short);
LINK OctOnStaff(LINK, short);
LINK GROctOnStaff(LINK, short);
LINK HasOctavaAcrossIncl(LINK, short);
LINK HasOctavaAcrossPt(Document *, Point, short);

Boolean SelRangeChkOctava(short, LINK, LINK);
short OctCountNotesInRange(short stf,LINK startL,LINK endL, Boolean selectedOnly);
void DashedLine(short x1, short y1, short x2, short y2);
void DrawOctBracket(DPoint, DPoint, short, DDIST, Boolean, PCONTEXT);
void XLoadOctavaSeg(void);




