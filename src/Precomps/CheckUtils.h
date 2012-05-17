/* Prototypes for CheckUtils.c */

Rect SetStaffRect(LINK, PCONTEXT);
INT16 AboveStfDist(LINK);
INT16 BelowStfDist(Document *, LINK, LINK);
Rect ComputeSysRect(LINK, Rect, PCONTEXT);
Boolean DragGraySysRect(Document *, LINK, Ptr, Rect, Rect, Rect, PCONTEXT);

LINK SelectStaff(Document *doc,LINK pL,INT16 staffn);
void SetLimitRect(Document *, LINK, INT16, Point, Rect, Rect *, CONTEXT []);
Boolean DragGrayStaffRect(Document *, LINK, LINK, INT16, Ptr, Rect, Rect, CONTEXT []);
void HiliteStaves(Document *, LINK, CONTEXT [], INT16);
void HiliteAllStaves(Document *, INT16);

INT16 GraphicWidth(Document *, LINK, PCONTEXT);
Rect SetSubRect(DDIST, DDIST, INT16, PCONTEXT);

void ContextObject(Document *, LINK, CONTEXT []);
LINK CheckObject (Document *, LINK, Boolean *, Ptr, CONTEXT [], INT16, INT16 *, STFRANGE);
Boolean ObjectTest(Rect *, Point, LINK);
LINK FindObject(Document *, Point, INT16 *, INT16);

LINK FindRelObject(Document *, Point, INT16 *, INT16);

LINK CheckMasterObject (Document *, LINK, Boolean *, Ptr, CONTEXT [], INT16, INT16 *, STFRANGE);
Boolean MasterObjectTest(Rect *, Point, LINK);
LINK FindMasterObject(Document *, Point, INT16 *, INT16);

Boolean FormatObjectTest(Rect *, Point, LINK);
LINK FindFormatObject(Document *, Point, INT16 *, INT16);
