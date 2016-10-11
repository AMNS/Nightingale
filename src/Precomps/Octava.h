/* Nightingale header file for Ottava.c */

void DoOttava(Document *);
void DoRemoveOttava(Document *);
void SetOttavaYPos(Document *, LINK);
LINK CreateOTTAVA(Document *, LINK, LINK, short, short, Byte, Boolean, Boolean);
long GetOctTypeNum(LINK, Boolean *);
void DrawOTTAVA(Document *, LINK, CONTEXT []);
void RemoveOctOnStf(Document *,LINK,LINK,LINK,short);
void UnOttavaRange(Document *, LINK, LINK, short);

LINK FirstInOttava(LINK);
LINK LastInOttava(LINK);
LINK HasOctAcross(LINK node,short staff,Boolean skipNode);
LINK HasOttavaAcross(LINK, short);
LINK OctOnStaff(LINK, short);
LINK GROctOnStaff(LINK, short);
LINK HasOttavaAcrossIncl(LINK, short);
LINK HasOttavaAcrossPt(Document *, Point, short);

Boolean SelRangeChkOttava(short, LINK, LINK);
short OctCountNotesInRange(short stf,LINK startL,LINK endL, Boolean selectedOnly);
void QD_DrawDashedLine(short x1, short y1, short x2, short y2);
void DrawOctBracket(DPoint, DPoint, short, DDIST, Boolean, PCONTEXT);
void XLoadOttavaSeg(void);




