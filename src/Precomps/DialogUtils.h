/* DialogUtils.h for Nightingale */

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
