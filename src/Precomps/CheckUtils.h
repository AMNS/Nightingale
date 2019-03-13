/* Prototypes for CheckUtils.c */

Rect SetStaffRect(LINK, PCONTEXT);
short AboveStfDist(LINK);
short BelowStfDist(Document *, LINK, LINK);
Rect ComputeSysRect(LINK, Rect, PCONTEXT);
Boolean DragGraySysRect(Document *, LINK, Ptr, Rect, Rect, Rect, PCONTEXT);

LINK SelectStaff(Document *doc,LINK pL,short staffn);
void SetLimitRect(Document *, LINK, short, Point, Rect, Rect *, CONTEXT []);
Boolean DragGrayStaffRect(Document *, LINK, LINK, short, Ptr, Rect, Rect, CONTEXT []);
void HiliteStaves(Document *, LINK, CONTEXT [], short);
void HiliteAllStaves(Document *, short);

short GraphicWidth(Document *, LINK, PCONTEXT);
Rect SetSubRect(DDIST, DDIST, short, PCONTEXT);

void ContextObject(Document *, LINK, CONTEXT []);
LINK CheckObject (Document *, LINK, Boolean *, Ptr, CONTEXT [], short, short *, STFRANGE);
Boolean ObjectTest(Rect *, Point, LINK);
LINK FindAndActOnObject(Document *, Point, short *, short);

LINK FindRelObject(Document *, Point, short *, short);

LINK CheckMasterObject (Document *, LINK, Boolean *, Ptr, CONTEXT [], short, short *, STFRANGE);
Boolean MasterObjectTest(Rect *, Point, LINK);
LINK FindAndActOnMasterObj(Document *, Point, short *, short);

Boolean FormatObjectTest(Rect *, Point, LINK);
LINK FindAndActOnFormatObj(Document *, Point, short *, short);

