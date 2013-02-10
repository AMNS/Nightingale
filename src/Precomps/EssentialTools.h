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

void		FrameDefault(DialogPtr dlog, short item, short draw);
void		BlinkCaret(DialogPtr dlog, EventRecord *evt);
void		TextEditState(DialogPtr dlog, Boolean save);

Handle		PutDlgWord(DialogPtr dlog, short item, short val, Boolean sel);
Handle		PutDlgLong(DialogPtr dlog, short item, long val, Boolean sel);
Handle		PutDlgDouble(DialogPtr dlog, short item, double val, Boolean sel);
Handle		PutDlgType(DialogPtr dlog, short item, ResType type, Boolean sel);
Handle		PutDlgString(DialogPtr dlog, short item, const unsigned char *str, Boolean sel);
Handle		PutDlgChkRadio(DialogPtr dlog, short item, short val);
short		GetDlgWord(DialogPtr dlog, short item, short *num);
short		GetDlgLong(DialogPtr dlog, short item, long *num);
short		GetDlgDouble(DialogPtr dlog, short item, double *val);
short		GetDlgString(DialogPtr dlog, short item, unsigned char *str);
short		GetDlgChkRadio(DialogPtr dlog, short item);
short		GetDlgType(DialogPtr dlog, short item, ResType *type);

short		AnyEmptyDlgItems(DialogPtr dlog, short fromItem, short toItem);
void		ShowHideItem(DialogPtr dlog, short item, Boolean show);

short		TextSelected(DialogPtr dlog);
OSType		CanPaste(short n, ...);
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
void		XableItem(MenuHandle menu, short item, short enable);
void		UpdateMenu(MenuHandle menu, Boolean enable);
void		UpdateMenuBar(void);
void		EraseAndInval(Rect *r);
void		UseStandardType(OSType type);
void		ClearStandardTypes(void);

#if TARGET_API_MAC_CARBON
short		GetInputName(char *prompt, Boolean newName, unsigned char *name, short *vrefnum, NSClientDataPtr pnsData);
Boolean		GetOutputName(short promptsID, short promptInd, unsigned char *name, short *vrefnum, NSClientDataPtr pnsData);
#else
short		GetInputName(char *prompt, Boolean new, unsigned char *name, short *vrefnum);
Boolean		GetOutputName(short promptsID, short promptInd, unsigned char *name, short *vrefnum);
#endif

void		GetSelection(WindowPtr w, Rect *sel, Rect *pin, void (*CallBack)());
void		DrawTheSelection(void);
unsigned char	*VersionString(void);
OSErr SysEnvirons(short versionRequested,SysEnvRec *theWorld);
//OSErr SysEnvirons(short versionRequested,void *theWorld);
