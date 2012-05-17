/* SpaceHighLevel.h for Nightingale */

void CenterNR(Document *, LINK, LINK, DDIST);
void CenterWholeMeasRests(Document *, LINK, LINK, DDIST);

Boolean RemoveObjSpace(Document *doc, LINK pL);

void RespaceAll(Document *, INT16);
DDIST Respace1Bar(Document *, LINK, INT16, SPACETIMEINFO [], long);
long GetJustProp(Document *, RMEASDATA [], INT16, INT16, CONTEXT);
void PositionSysByTable(Document *, RMEASDATA *, INT16, INT16, long, CONTEXT);
Boolean RespaceBars(Document *, LINK, LINK, long, Boolean, Boolean);

Boolean StretchToSysEnd(Document *, LINK, LINK, long, DDIST, DDIST);
FASTFLOAT SysJustFact(Document *, LINK, LINK, DDIST *, DDIST *);
void JustifySystem(Document *, LINK, LINK);
void JustifySystems(Document *, LINK, LINK);
