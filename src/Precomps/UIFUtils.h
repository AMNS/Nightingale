/*	UIFUtils.h for Nightingale */

// MAS
#pragma once
// MAS

void HiliteInsertNode(Document *, LINK, short, Boolean);
void HiliteTwoNodesOn(Document *, LINK, LINK, short);
void HiliteAttPoints(Document *, LINK, LINK, short);
void FixCursor(void);
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
const char *NameNodeType(LINK);
const char *NameGraphicType(LINK);

short	ConvertQuote(TEHandle textH, short ch);

void DrawBox(Point pt,short size);
void HiliteRect(Rect *);

void Voice2UserStr(Document *doc, short voice, char str[]);
void Staff2UserStr(Document *doc, short staffn, char str[]);

void    DrawPopUp(UserPopUp *popup);
void    TruncPopUpString(UserPopUp *popup);
short    InitPopUp(DialogPtr dlog, UserPopUp *p, short item, short pItem,
					short menuID, short choice);
short	DoUserPopUp(UserPopUp *p);
void	ChangePopUpChoice(UserPopUp *p, short choice);
void	DisposePopUp(UserPopUp *p);
void	HilitePopUp (UserPopUp *p,short activ);
short	ResizePopUp(UserPopUp *p);
void	ShowPopUp(UserPopUp *p, short vis);

void	HiliteArrowKey(DialogPtr, short, UserPopUp *, Boolean *);

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

StringPtr CtoPstr(StringPtr str);
StringPtr PtoCstr(StringPtr str);
