/*	Set.h	for Nightingale: prototypes for SetCommand.c and SetUtils.c */

Boolean IsSetEnabled(Document *);
Boolean SetDialog(Document *, INT16 *, INT16 *, INT16 *);
void DoSet(Document *doc);

Boolean SetSelVoice(Document *, INT16);
Boolean SetSelStaff(Document *, INT16);
Boolean SetSelStemlen(Document *, unsigned STDIST);
Boolean SetSelNRAppear(Document *, INT16);
Boolean SetSelNRSmall(Document *doc, INT16 small);
Boolean SetSelNRParens(Document *doc, INT16 small);
Boolean SetVelFromContext(Document *, Boolean);
Boolean SetSelVelocity(Document *, INT16);

Boolean SetSelGraphicX(Document *, STDIST, INT16);
Boolean SetSelGraphicY(Document *, STDIST, INT16, Boolean);
Boolean SetSelTempoX(Document *, STDIST);
Boolean SetSelTempoY(Document *, STDIST, Boolean);
Boolean SetSelGraphicStyle(Document *, INT16, INT16);

Boolean SetSelMeasVisible(Document *, Boolean);
Boolean SetSelMeasType(Document *, INT16);

Boolean SetSelDynamVisible(Document *, Boolean);
Boolean SetSelDynamSmall(Document *, Boolean);

Boolean SetSelBeamsetThin(Document *, Boolean);

Boolean SetSelClefVisible(Document *, Boolean);

Boolean SetSelKeySigVisible(Document *, Boolean);

Boolean SetSelTimeSigVisible(Document *, Boolean);

Boolean SetSelTupBracket(Document *, Boolean);
Boolean SetSelTupNums(Document *, INT16);

void SetSlurShape(Document *doc, LINK pL, LINK aSlurL, CONTEXT context);
void SetAllSlursShape(Document *doc, LINK pL, Boolean recalc);
Boolean SetSelSlurShape(Document *);
Boolean SetSelSlurAppear(Document *, INT16);

Boolean SetSelLineThickness(Document *, INT16);
Boolean SetSelPatchChangeVisible(Document *doc, Boolean visible);
Boolean SetSelPanVisible(Document *doc, Boolean visible);