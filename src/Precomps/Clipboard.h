/* Clipboard.h for Nightingale */

void SetupClipDoc(Document *doc,Boolean);

void SelRangeCases(Document *doc,Boolean *hasClef, Boolean *hasKS,Boolean *hasTS,
					Boolean *befFirst, Boolean *hasPgRelGraphic);

Boolean MeasHomoSel(LINK, Boolean);
Boolean AllMeasHomoSel(Document *);
void SelPartlySelMeasSubobjs(Document *);
Boolean DeselPartlySelMeasSubobjs(Document *);

Boolean Clear(Document *);

void DoCut(Document *);
void DoCopy(Document *);
void DoClear(Document *);
Boolean DoPaste(Document *);

void CopyFontTable(Document *);
short CheckMainClip(Document *, LINK, LINK);
void PasteFixStfSize(Document *doc, LINK startL, LINK endL);
void PasteFixMeasureRect(LINK, DDIST);
short GetClipMinStf(Document *);
short GetClipMaxStf(Document *);
short GetStfDiff(Document *, short *, short *);
void MapStaves(Document *doc,LINK startL,LINK endL,short staffDiff);

short NewVoice(Document *doc,short stf,short voice,short stfDiff);
void InitVMapTable(Document *doc, short stfDiff);
void SetupVMap(Document *doc, short *vMap, short stfDiff);

void PasteFixMeasStruct(Document *, LINK, LINK, Boolean);
void PasteFixOttavas(Document *doc,LINK startL,short s);
void PasteFixBeams(Document *doc,LINK startL,short v);
void PasteUpdate(Document *, LINK, LINK, DDIST);
void PasteFixContext(Document *, LINK, LINK, short);
