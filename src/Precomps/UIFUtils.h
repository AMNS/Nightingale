/*	UIFUtils.h for Nightingale */

// MAS
#pragma once
// MAS

void		WaitCursor(void);
void		ArrowCursor(void);
void		FixCursor(void);
void		XableItem(MenuHandle menu, short item, short enable);
void		UpdateMenu(MenuHandle menu, Boolean enable);
void		UpdateMenuBar(void);

void		CenterRect(Rect *r, Rect *inside, Rect *ans);
void		PullInsideRect(Rect *r, Rect *inside, short margin);
Boolean		ContainedRect(Rect *r, Rect *bounds);
void		ZoomRect(Rect *smallRect, Rect *bigRect, Boolean zoomUp);

void		GetGlobalPort(WindowPtr w, Rect *r);
void		GetIndWinPosition(WindowPtr w, short i, short n, Point *ans);
void		AdjustWinPosition(WindowPtr w);
short		GetMyScreen(Rect *r, Rect *bounds);
void		PlaceAlert(short id, WindowPtr w, short left, short top);
Boolean		PlaceWindow(WindowPtr w, WindowPtr inside, short left, short top);
void 		CenterWindow(WindowPtr w, short top);
void		EraseAndInval(Rect *r);

Boolean		KeyIsDown(short);
Boolean		CmdKeyDown(void);
Boolean		OptionKeyDown(void);
Boolean		ShiftKeyDown(void);
Boolean		CapsLockKeyDown(void);
Boolean		ControlKeyDown(void);
Boolean		CommandKeyDown(void);

Boolean		CheckAbort(void);
Boolean		IsDoubleClick(Point clickPt, short tol, long now);

void InvertSymbolHilite(Document *, LINK, short, Boolean);
void InvertTwoSymbolHilite(Document *, LINK, LINK, short);
void HiliteAttPoints(Document *, LINK, LINK, short);
void FlashRect(Rect *);
Boolean SamePoint(Point, Point);

short Advise(short);
short NoteAdvise(short);
short CautionAdvise(short);
short StopAdvise(short);
void Inform(short);
void NoteInform(short);
void CautionInform(short);
void StopInform(short);

Boolean ProgressMsg(short, char *);

Boolean UserInterrupt(void);
Boolean UserInterruptAndSel(void);

const char *NameHeapType(short, Boolean);
const char *NameObjType(LINK);
const char *NameGraphicType(LINK, Boolean);
Boolean DynamicToString(short dynamicType, char dynStr[]);
Boolean ClefToString(short clefType, char clefStr[]);

short SmartenQuote(TEHandle textH, short ch);

void DrawBox(Point pt,short size);
void DrawGrowBox(WindowPtr w, Point pt, Boolean drawit);
void DrawTheSelection(void);
void HiliteRect(Rect *);

void Voice2UserStr(Document *doc, short voice, char str[]);
void Staff2UserStr(Document *doc, short staffn, char str[]);

Rect GetQDPortBounds(void);
Rect GetQDScreenBitsBounds(void);
Rect *GetQDScreenBitsBounds(Rect *bounds);

Pattern *NGetQDGlobalsDarkGray();
Pattern *NGetQDGlobalsLightGray();
Pattern *NGetQDGlobalsGray();
Pattern *NGetQDGlobalsBlack();
Pattern *NGetQDGlobalsWhite();

GrafPtr GetDocWindowPort(Document *doc);

GrafPtr GetDialogWindowPort(DialogPtr dlog);

void GetDialogPortBounds(DialogPtr d, Rect *boundsR);
void GetWindowRgnBounds(WindowPtr w, WindowRegionCode rgnCode, Rect *bounds);

void UpdateDialogVisRgn(DialogPtr d);
void LUpdateDVisRgn(DialogPtr d, ListHandle lHdl);
void LUpdateWVisRgn(WindowPtr w, ListHandle lHdl);

void FlushPortRect(Rect *r);
void FlushWindowPortRect(WindowPtr w);
void FlushDialogPortRect(DialogPtr d);

short GetPortTxFont(void);
short GetPortTxFace(void);
short GetPortTxSize(void);
short GetPortTxMode(void);

short GetWindowTxFont(WindowPtr w);
short GetWindowTxFace(WindowPtr w);
short GetWindowTxSize(WindowPtr w);
void SetWindowTxFont(WindowPtr w, short font);
void SetWindowTxFace(WindowPtr w, short face);
void SetWindowTxSize(WindowPtr w, short size);

short GetDialogTxFont(DialogPtr d);
short GetDialogTxFace(DialogPtr d);
short GetDialogTxSize(DialogPtr d);
void SetDialogTxFont(DialogPtr d, short font);
void SetDialogTxFace(DialogPtr d, short face);
void SetDialogTxSize(DialogPtr d, short size);

short IsPaletteKind(WindowPtr w);
short IsDocumentKind(WindowPtr w);
void SetPaletteKind(WindowPtr w);
void SetDocumentKind(WindowPtr w);

void OffsetContrlRect(ControlRef ctrl, short dx, short dy);

Boolean VLogPrintf(const char *fmt, va_list argp);
Boolean LogPrintf(short priLevel, const char *fmt, ...);
short InitLogPrintf();
