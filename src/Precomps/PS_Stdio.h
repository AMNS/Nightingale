/* PS_Stdio.h - prototypes for public functions in PS_Stdio library - for Nightingale */

#define USING_PRINTER	0
#define USING_FILE		1
#define USING_HANDLE	2

void	PS_PreparePrintDict(Document *doc, Rect *imageRect);
void	PS_FinishPrintDict(void);
Handle	PS_GetTextHandle(void);

OSErr	PS_Open(Document *doc, unsigned char *fileName, short vRefNum, short usingWhat, ResType fType, FSSpec *fsSpec);
OSErr	PS_Close(void);

void	PS_PageSize(DDIST x, DDIST y);
void	PS_SetWidths(DDIST wStaff, DDIST wLedger, DDIST wBar, DDIST wStem);

OSErr	PS_Header(Document *doc, const unsigned char *docName, short nPages, FASTFLOAT scaleFactor,
				Boolean landscape, Boolean EPSF, Rect *bBox, Rect *paperRect);
OSErr PS_HeaderHdl(Document *doc, const unsigned char *docName, short nPages, FASTFLOAT scaleFactor,
						Boolean landscape, Boolean doEncoding, Rect *bBox, Rect *paper);
OSErr	PS_NewPage(Document *doc, char *name, short nSeq);

OSErr	PS_EndPage(void);
OSErr	PS_Trailer(Document *, short nfontsUsed, FONTUSEDITEM fontUsedTbl[], unsigned char *measNumFont, Rect *bBox);
OSErr	PS_IntoQD(Document *doc, short first);
OSErr	PS_OutOfQD(Document *doc, short last, Rect *imageRect);

void	PS_Print(char *msg, ...);
OSErr	PS_String(char *str);
OSErr	PS_PString(unsigned char *str);
OSErr	PS_Resource(short resFile, ResType type, short resID);
void PS_Handle(void);
void	PS_Flush(void);

OSErr	PS_SetLineWidth(DDIST width);
OSErr	PS_Line(DDIST x0, DDIST y0, DDIST x1, DDIST y1, DDIST width);
OSErr	PS_LineVT(DDIST x0, DDIST y0, DDIST x1, DDIST y1, DDIST width);
OSErr	PS_HDashedLine(DDIST x0, DDIST y, DDIST x1, DDIST width, DDIST dashLen);
OSErr	PS_VDashedLine(DDIST x, DDIST y0, DDIST y1, DDIST width, DDIST dashLen);
OSErr	PS_FrameRect(DRect *dr, DDIST width);

OSErr	PS_MusSize(Document *doc, short ptSize);
OSErr	PS_FontString(Document *doc, DDIST x, DDIST y, const unsigned char *str, const unsigned char *font, short size, short style);
OSErr	PS_MusChar(Document *doc, DDIST x, DDIST y, char sym, Boolean visible, short sizePercent);
OSErr	PS_MusString(Document *doc, DDIST x, DDIST y, unsigned char *str, short sizePercent);
OSErr	PS_MusColon(Document *doc, DDIST x, DDIST y, short sizePercent, DDIST lnSpace, Boolean italic);
OSErr	PS_NoteStem(Document *doc, DDIST x, DDIST y, DDIST xNorm, char sym, DDIST stemLen, DDIST stemShorten, 
					Boolean beamed, Boolean headVisible, short sizePercent);
OSErr	PS_ArpSign(Document *doc, DDIST x, DDIST y, DDIST height, short sizePercent, Byte glyph, DDIST lnSpace);
OSErr	PS_StaffLine(DDIST height, DDIST x0, DDIST x1);
OSErr	PS_BarLine(DDIST top, DDIST bot, DDIST x, char type);
OSErr	PS_ConLine(DDIST top, DDIST bot, DDIST x);
OSErr 	PS_Repeat(Document *doc, DDIST top, DDIST bot, DDIST botNorm, DDIST x, char type, short sizePercent, Boolean dotsOnly);
OSErr	PS_Staff(DDIST height, DDIST x0, DDIST x1, short nLines, DDIST *dy);
OSErr	PS_LedgerLine(DDIST height, DDIST x0, DDIST dx);
OSErr	PS_Beam(DDIST x0, DDIST y0, DDIST x1, DDIST y1, DDIST thick, short upOrDown0, short upOrDown1);
OSErr	PS_Bracket(Document *doc, DDIST x, DDIST top, DDIST bot);
OSErr	PS_Brace(Document *doc, DDIST x, DDIST yTop, DDIST yBot);

OSErr	PS_Slur(DDIST p0x,DDIST p0y,DDIST c1x,DDIST c1y,DDIST c2x,DDIST c2y,
				DDIST p3x,DDIST p3y, Boolean dashed);


