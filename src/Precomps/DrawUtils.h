/*	DrawUtils.h	for Nightingale */

enum {					/* Drawing modes. */
	MEDraw,				/* Normal */
	SDDraw,				/* Dragging */
	SWDraw				/* Unused--I think intended for "swimming" selection feedback */
};

short GetSmallerRSize(short);
Rect ContextedObjRect(Document *doc,LINK pL,short staff,PCONTEXT pContext);

void DrawPaddedChar(unsigned char);
void DrawMChar(Document *, short, short, Boolean);
void DrawMString(Document *, unsigned char *, short, Boolean);
void DrawMColon(Document *, Boolean, Boolean, DDIST);
void GetClefDrawInfo(Document *, LINK, LINK, CONTEXT [], short, unsigned char *, DDIST *,
								DDIST *, DDIST *, DDIST *);
void GetHairpinBBox(DDIST, DDIST, DDIST, DDIST, DDIST, DDIST, short, Rect *);
void GetDynamicDrawInfo(Document *, LINK, LINK, CONTEXT [], unsigned char *, DDIST *, DDIST *);
void DrawRptBar(Document *, LINK, short, short, CONTEXT [], DDIST, char, short, Boolean);
void GetRptEndDrawInfo(Document *, LINK, LINK, CONTEXT [], DDIST *, DDIST *, Rect *);
short GetKSYOffset(PCONTEXT, KSITEM);
void DrawKSItems(Document *, LINK, LINK, PCONTEXT, DDIST, DDIST, DDIST, short, 
								short *, short);
void GetKeySigDrawInfo(Document *, LINK, LINK, CONTEXT [], DDIST *, DDIST *, DDIST *,
								DDIST *, short *);
short  FillTimeSig(Document *,LINK, PCONTEXT, unsigned char *, unsigned char *, DDIST,
								DDIST *, DDIST *, DDIST, DDIST *, DDIST *);
void GetTimeSigDrawInfo(Document *, LINK, LINK, PCONTEXT, DDIST *, DDIST *);
short GetRestDrawInfo(Document *, LINK , LINK, PCONTEXT, DDIST *, DDIST *, DDIST *, DDIST *,
								char *);
Boolean GetModNRInfo(short, short, Boolean, Boolean, unsigned char *, short *, short *,
								short *);
DDIST NoteXLoc(LINK, LINK, DDIST, DDIST, DDIST *);
DDIST GRNoteXLoc(LINK, LINK, DDIST, DDIST, DDIST *);
void NCHasLedgers(LINK syncL, LINK aNoteL, PCONTEXT	pContext, Boolean *hasLedgersAbove,
					Boolean *hasLedgersBelow);
void GetNCLedgerInfo(LINK, LINK, PCONTEXT, QDIST *, QDIST *, QDIST *, QDIST *);
void DrawNoteLedgers(DDIST, QDIST, QDIST, Boolean, DDIST, PCONTEXT, short);
void InsertLedgers(DDIST, short, PCONTEXT);
void GetMBRestBar(short, PCONTEXT, DRect *);

void VisStavesForPart(Document *, LINK, LINK, short *, short *);

Boolean ShouldDrawConnect(Document *, LINK, LINK, LINK);
Boolean ShouldREDrawBarline(Document *, LINK, LINK, short *);
Boolean ShouldDrawBarline(Document *, LINK, LINK, short *);
Boolean ShouldPSMDrawBarline(Document *, LINK, LINK, short *);
Boolean ShouldDrawMeasNum(Document *, LINK, LINK);

void DrawPSMSubType(short, short, short, short);
void DrawNonArp(short, short, DDIST, DDIST);
void DrawArp(Document *, short, short, DDIST, DDIST, Byte, PCONTEXT);

void DrawBMP(Byte bmpBits[], short bWidth, short bWidthWithPad, short height,
														short drawHeight, Rect bmpRect);
void DrawBMPChar(Byte bmpBits[], short offset, short width, short height, Rect bmpRect);

char TempoGlyph(LINK);
short GetGraphicOrTempoDrawInfo(Document *, LINK, LINK, short, DDIST *, DDIST *, PCONTEXT);
short GetGRDrawLastDrawInfo(Document *, LINK, LINK, short, DDIST *, DDIST *);
void GetGraphicFontInfo(Document *doc, LINK pL, PCONTEXT pContext, short *pFontID,
												short *pFontSize, short *pFontStyle);

short Voice2Color(Document *doc, short iVoice);

Boolean CheckZoom(Document *);
Boolean DrawCheckInterrupt(Document *);

void MaySetPSMusSize(Document *, PCONTEXT);
