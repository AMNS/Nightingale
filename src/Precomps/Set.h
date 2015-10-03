/*	Set.h	for Nightingale: prototypes for SetCommand.c and SetUtils.c */

Boolean IsSetEnabled(Document *);
Boolean SetDialog(Document *, short *, short *, short *);
void DoSet(Document *doc);

Boolean SetSelVoice(Document *, short);
Boolean SetSelStaff(Document *, short);
Boolean SetSelStemlen(Document *, unsigned STDIST);
Boolean SetSelNRAppear(Document *, short);
Boolean SetSelNRSmall(Document *doc, short small);
Boolean SetSelNRParens(Document *doc, short small);
Boolean SetVelFromContext(Document *, Boolean);
Boolean SetSelVelocity(Document *, short);

Boolean SetSelGraphicX(Document *, STDIST, short);
Boolean SetSelGraphicY(Document *, STDIST, short, Boolean);
Boolean SetSelTempoX(Document *, STDIST);
Boolean SetSelTempoY(Document *, STDIST, Boolean);
Boolean SetSelTempoVisible(Document *, Boolean);

Boolean SetSelGraphicStyle(Document *, short, short);

Boolean SetSelMeasVisible(Document *, Boolean);
Boolean SetSelMeasType(Document *, short);

Boolean SetSelDynamVisible(Document *, Boolean);
Boolean SetSelDynamSmall(Document *, Boolean);

Boolean SetSelBeamsetThin(Document *, Boolean);

Boolean SetSelClefVisible(Document *, Boolean);

Boolean SetSelKeySigVisible(Document *, Boolean);

Boolean SetSelTimeSigVisible(Document *, Boolean);

Boolean SetSelTupBracket(Document *, Boolean);
Boolean SetSelTupNums(Document *, short);

void SetSlurShape(Document *doc, LINK pL, LINK aSlurL, CONTEXT context);
void SetAllSlursShape(Document *doc, LINK pL, Boolean recalc);
Boolean SetSelSlurShape(Document *);
Boolean SetSelSlurAppear(Document *, short);

Boolean SetSelLineThickness(Document *, short);
Boolean SetSelPatchChangeVisible(Document *doc, Boolean visible);
Boolean SetSelPanVisible(Document *doc, Boolean visible);