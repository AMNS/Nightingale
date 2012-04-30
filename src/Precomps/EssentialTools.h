/* EssentialTools.h for Nightingale */

#ifndef _ESSENTIALTOOLS_
#define _ESSENTIALTOOLS_
#endif

#ifndef MAXINPUTTYPE
#define MAXINPUTTYPE	4
#endif

/*
 *	Codes for various open commands as delivered by GetInputName
 */

enum {
		OP_Cancel,
		OP_OpenFile,
		OP_NewFile,
		OP_QuitInstead
	};

/*
 *	Prototypes for routines in EssentialTools.c
 */

char		*ftoa(char *str, double val);

pascal long GrowMemory(Size memoryNeeded);
Boolean		PreflightMem(short nKBytes);
void		ZeroMem(void *m, long nBytes);
Boolean		GoodNewPtr(Ptr p);
Boolean		GoodNewHandle(Handle hndl);
Boolean		GoodResource(Handle hndl);
void		*NewZPtr(Size size);
void		WaitCursor(void);
void		ArrowCursor(void);
Boolean		CheckAbort(void);
Boolean		IsDoubleClick(Point clickPt, short tol, long now);

Boolean		KeyIsDown(short);
Boolean		CmdKeyDown(void);
Boolean		OptionKeyDown(void);
Boolean		ShiftKeyDown(void);
Boolean		CapsLockKeyDown(void);
Boolean		ControlKeyDown(void);

void		FrameDefault(DialogPtr dlog, INT16 item, INT16 draw);
void		BlinkCaret(DialogPtr dlog, EventRecord *evt);
void		TextEditState(DialogPtr dlog, Boolean save);

Handle		PutDlgWord(DialogPtr dlog, INT16 item, INT16 val, Boolean sel);
Handle		PutDlgLong(DialogPtr dlog, INT16 item, long val, Boolean sel);
Handle		PutDlgDouble(DialogPtr dlog, INT16 item, double val, Boolean sel);
Handle		PutDlgType(DialogPtr dlog, INT16 item, ResType type, Boolean sel);
Handle		PutDlgString(DialogPtr dlog, INT16 item, const unsigned char *str, Boolean sel);
Handle		PutDlgChkRadio(DialogPtr dlog, INT16 item, INT16 val);
INT16		GetDlgWord(DialogPtr dlog, INT16 item, INT16 *num);
INT16		GetDlgLong(DialogPtr dlog, INT16 item, long *num);
INT16		GetDlgDouble(DialogPtr dlog, INT16 item, double *val);
INT16		GetDlgString(DialogPtr dlog, INT16 item, unsigned char *str);
INT16		GetDlgChkRadio(DialogPtr dlog, INT16 item);
INT16		GetDlgType(DialogPtr dlog, INT16 item, ResType *type);

short		AnyEmptyDlgItems(DialogPtr dlog, short fromItem, short toItem);
void		ShowHideItem(DialogPtr dlog, short item, Boolean show);

short		TextSelected(DialogPtr dlog);
OSType		CanPaste(INT16 n, ...);
void		DrawGrowBox(WindowPtr w, Point pt, Boolean drawit);
void		GetGlobalPort(WindowPtr w, Rect *r);
Boolean		ContainedRect(Rect *r, Rect *bounds);
void		GetIndPosition(WindowPtr w, short i, short n, Point *ans);
void		AdjustWinPosition(WindowPtr w);
void		CenterRect(Rect *r, Rect *inside, Rect *ans);
void		PullInsideRect(Rect *r, Rect *inside, short margin);
short		GetMyScreen(Rect *r, Rect *bounds);
void		PlaceAlert(short id, WindowPtr w, short left, short top);
Boolean		PlaceWindow(WindowPtr w, WindowPtr inside, short left, short top);
void 		CenterWindow(WindowPtr w, short top);
void		ZoomRect(Rect *smallRect, Rect *bigRect, Boolean zoomUp);
void		XableItem(MenuHandle menu, INT16 item, INT16 enable);
void		UpdateMenu(MenuHandle menu, Boolean enable);
void		UpdateMenuBar(void);
void		EraseAndInval(Rect *r);
void		UseStandardType(OSType type);
void		ClearStandardTypes(void);

#if TARGET_API_MAC_CARBON
INT16		GetInputName(char *prompt, Boolean newName, unsigned char *name, short *vrefnum, NSClientDataPtr pnsData);
Boolean		GetOutputName(short promptsID, short promptInd, unsigned char *name, short *vrefnum, NSClientDataPtr pnsData);
#else
INT16		GetInputName(char *prompt, Boolean new, unsigned char *name, short *vrefnum);
Boolean		GetOutputName(short promptsID, short promptInd, unsigned char *name, short *vrefnum);
#endif

void		GetSelection(WindowPtr w, Rect *sel, Rect *pin, void (*CallBack)());
void		DrawTheSelection(void);
unsigned char	*VersionString(void);
OSErr SysEnvirons(short versionRequested,SysEnvRec *theWorld);
//OSErr SysEnvirons(short versionRequested,void *theWorld);
