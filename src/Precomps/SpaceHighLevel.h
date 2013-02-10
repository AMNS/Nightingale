/* SpaceHighLevel.h for Nightingale */

void CenterNR(Document *, LINK, LINK, DDIST);
void CenterWholeMeasRests(Document *, LINK, LINK, DDIST);

Boolean RemoveObjSpace(Document *doc, LINK pL);

void RespaceAll(Document *, short);
DDIST Respace1Bar(Document *, LINK, short, SPACETIMEINFO [], long);
long GetJustProp(Document *, RMEASDATA [], short, short, CONTEXT);
void PositionSysByTable(Document *, RMEASDATA *, short, short, long, CONTEXT);
Boolean RespaceBars(Document *, LINK, LINK, long, Boolean, Boolean);

Boolean StretchToSysEnd(Document *, LINK, LINK, long, DDIST, DDIST);
FASTFLOAT SysJustFact(Document *, LINK, LINK, DDIST *, DDIST *);
void JustifySystem(Document *, LINK, LINK);
void JustifySystems(Document *, LINK, LINK);
