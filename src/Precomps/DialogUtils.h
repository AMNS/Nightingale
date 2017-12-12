/* DialogUtils.h for Nightingale */

void		FrameDefault(DialogPtr dlog, short item, short draw);
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
void		BlinkCaret(DialogPtr dlog, EventRecord *evt);

void OutlineOKButton(DialogPtr, Boolean);
void FlashButton(DialogPtr, short);
pascal Boolean OKButFilter(DialogPtr, EventRecord *, short *);
pascal Boolean OKButDragFilter(DialogPtr, EventRecord *, short *);
Boolean DlgCmdKey(DialogPtr, EventRecord *, short *, Boolean);
void SwitchRadio(DialogPtr, short *, short);

typedef void (*TrackNumberFunc)(short limit, DialogPtr dlog);
void TrackNumberArrow(Rect *, TrackNumberFunc, short, DialogPtr);

Boolean HandleKeyDown(EventRecord *, short, short, DialogPtr);
Boolean HandleMouseDown(EventRecord *, short, short, DialogPtr);
void UseNumberFilter(DialogPtr, short, short, short);
pascal Boolean NumberFilter(DialogPtr, EventRecord *, short *);

void InitDurStrings(void);
void UseDurNumFilter(DialogPtr, short, short, short);
pascal Boolean DurNumberFilter(DialogPtr, EventRecord *, short *);

void XableControl(DialogPtr, short, Boolean);
void MoveDialogControl(DialogPtr, short, short, short);
void GetHiddenDItemBox(DialogPtr, short, Rect *);
void SetDlgFont(DialogPtr, short, short, short);
