/*	UIFUtils.h for Nightingale */

// MAS
#pragma once
// MAS

void HiliteInsertNode(Document *, LINK, INT16, Boolean);
void HiliteTwoNodesOn(Document *, LINK, LINK, INT16);
void HiliteAttPoints(Document *, LINK, LINK, INT16);
void FixCursor(void);
void FlashRect(Rect *);
Boolean SamePoint(Point, Point);

INT16 Advise(INT16);
INT16 NoteAdvise(INT16);
INT16 CautionAdvise(INT16);
INT16 StopAdvise(INT16);
void Inform(INT16);
void NoteInform(INT16);
void CautionInform(INT16);
void StopInform(INT16);

Boolean ProgressMsg(INT16, char *);

Boolean UserInterrupt(void);
Boolean UserInterruptAndSel(void);

const char *NameHeapType(INT16, Boolean);
const char *NameNodeType(LINK);
const char *NameGraphicType(LINK);

INT16	ConvertQuote(TEHandle textH, INT16 ch);

void DrawBox(Point pt,INT16 size);
void HiliteRect(Rect *);

void Voice2UserStr(Document *doc, INT16 voice, char str[]);
void Staff2UserStr(Document *doc, INT16 staffn, char str[]);

void    DrawPopUp(UserPopUp *popup);
void    TruncPopUpString(UserPopUp *popup);
INT16    InitPopUp(DialogPtr dlog, UserPopUp *p, INT16 item, INT16 pItem,
					INT16 menuID, INT16 choice);
INT16	DoUserPopUp(UserPopUp *p);
void	ChangePopUpChoice(UserPopUp *p, INT16 choice);
void	DisposePopUp(UserPopUp *p);
void	HilitePopUp (UserPopUp *p,INT16 activ);
INT16	ResizePopUp(UserPopUp *p);
void	ShowPopUp(UserPopUp *p, INT16 vis);

void	HiliteArrowKey(DialogPtr, INT16, UserPopUp *, Boolean *);

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
