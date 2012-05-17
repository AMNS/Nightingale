/*	DrawUtils.h	for Nightingale */

enum {					/* Drawing modes. */
	MEDraw,				/* Normal */
	SDDraw,				/* Dragging */
	SWDraw				/* Unused--I think intended for "swimming" selection feedback */
};

INT16 GetSmallerRSize(INT16);
Rect ContextedObjRect(Document *doc,LINK pL,INT16 staff,PCONTEXT pContext);

void DrawPaddedChar(unsigned char);
void DrawMChar(Document *, INT16, INT16, Boolean);
void DrawMString(Document *, unsigned char *, INT16, Boolean);
void DrawMColon(Document *, Boolean, Boolean, DDIST);
void GetClefDrawInfo(Document *, LINK, LINK, CONTEXT [], INT16, unsigned char *, DDIST *,
								DDIST *, DDIST *, DDIST *);
void GetHairpinBBox(DDIST, DDIST, DDIST, DDIST, DDIST, DDIST, INT16, Rect *);
void GetDynamicDrawInfo(Document *, LINK, LINK, CONTEXT [], unsigned char *, DDIST *, DDIST *);
void DrawRptBar(Document *, LINK, INT16, INT16, CONTEXT [], DDIST, char, INT16, Boolean);
void GetRptEndDrawInfo(Document *, LINK, LINK, CONTEXT [], DDIST *, DDIST *, Rect *);
INT16 GetKSYOffset(PCONTEXT, KSITEM);
void DrawKSItems(Document *, LINK, LINK, PCONTEXT, DDIST, DDIST, DDIST, INT16, 
								INT16 *, INT16);
void GetKeySigDrawInfo(Document *, LINK, LINK, CONTEXT [], DDIST *, DDIST *, DDIST *,
								DDIST *, INT16 *);
INT16  FillTimeSig(Document *,LINK, PCONTEXT, unsigned char *, unsigned char *, DDIST,
								DDIST *, DDIST *, DDIST, DDIST *, DDIST *);
void GetTimeSigDrawInfo(Document *, LINK, LINK, PCONTEXT, DDIST *, DDIST *);
INT16 GetRestDrawInfo(Document *, LINK , LINK, PCONTEXT, DDIST *, DDIST *, DDIST *, DDIST *,
								char *);
Boolean GetModNRInfo(INT16, INT16, Boolean, Boolean, unsigned char *, INT16 *, INT16 *,
								INT16 *);
DDIST NoteXLoc(LINK, LINK, DDIST, DDIST, DDIST *);
DDIST GRNoteXLoc(LINK, LINK, DDIST, DDIST, DDIST *);
void NoteLedgers(DDIST, QDIST, QDIST, Boolean, DDIST, PCONTEXT, INT16);
void InsertLedgers(DDIST, INT16, PCONTEXT);
void GetMBRestBar(INT16, PCONTEXT, DRect *);

void VisStavesForPart(Document *, LINK, LINK, INT16 *, INT16 *);

Boolean ShouldDrawConnect(Document *, LINK, LINK, LINK);
Boolean ShouldREDrawBarline(Document *, LINK, LINK, INT16 *);
Boolean ShouldDrawBarline(Document *, LINK, LINK, INT16 *);
Boolean ShouldPSMDrawBarline(Document *, LINK, LINK, INT16 *);
Boolean ShouldDrawMeasNum(Document *, LINK, LINK);

void DrawPSMSubType(INT16, INT16, INT16, INT16);
void DrawNonArp(INT16, INT16, DDIST, DDIST);
void DrawArp(Document *, INT16, INT16, DDIST, DDIST, Byte, PCONTEXT);
char TempoGlyph(LINK);
INT16 GetGraphicDrawInfo(Document *, LINK, LINK, INT16, DDIST *, DDIST *, PCONTEXT);
INT16 GetGRDrawLastDrawInfo(Document *, LINK, LINK, INT16, DDIST *, DDIST *);
void GetGraphicFontInfo(Document *doc, LINK pL, PCONTEXT pContext, INT16 *pFontID,
									INT16 *pFontSize, INT16 *pFontStyle);

short Voice2Color(Document *doc, INT16 iVoice);

Boolean CheckZoom(Document *);
Boolean DrawCheckInterrupt(Document *);

void MaySetPSMusSize(Document *, PCONTEXT);
