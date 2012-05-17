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
INT16 CheckMainClip(Document *, LINK, LINK);
void PasteFixStfSize(Document *doc, LINK startL, LINK endL);
void PasteFixMeasureRect(LINK, DDIST);
INT16 GetClipMinStf(Document *);
INT16 GetClipMaxStf(Document *);
INT16 GetStfDiff(Document *, INT16 *, INT16 *);
void MapStaves(Document *doc,LINK startL,LINK endL,INT16 staffDiff);

INT16 NewVoice(Document *doc,INT16 stf,INT16 voice,INT16 stfDiff);
void InitVMapTable(Document *doc, INT16 stfDiff);
void SetupVMap(Document *doc, INT16 *vMap, INT16 stfDiff);

void PasteFixMeasStruct(Document *, LINK, LINK, Boolean);
void PasteFixOctavas(Document *doc,LINK startL,INT16 s);
void PasteFixBeams(Document *doc,LINK startL,INT16 v);
void PasteUpdate(Document *, LINK, LINK, DDIST);
void PasteFixContext(Document *, LINK, LINK, INT16);
