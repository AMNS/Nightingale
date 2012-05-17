/* DialogUtils.h for Nightingale */

void OutlineOKButton(DialogPtr, Boolean);
void FlashButton(DialogPtr, INT16);
pascal Boolean OKButFilter(DialogPtr, EventRecord *, INT16 *);
pascal Boolean OKButDragFilter(DialogPtr, EventRecord *, INT16 *);
Boolean DlgCmdKey(DialogPtr, EventRecord *, INT16 *, Boolean);
void SwitchRadio(DialogPtr, INT16 *, short);

typedef void (*TrackNumberFunc)(INT16 limit, DialogPtr dlog);
void TrackNumberArrow(Rect *, TrackNumberFunc, INT16, DialogPtr);

Boolean HandleKeyDown(EventRecord *, INT16, INT16, DialogPtr);
Boolean HandleMouseDown(EventRecord *, INT16, INT16, DialogPtr);
void UseNumberFilter(DialogPtr, INT16, INT16, INT16);
pascal Boolean NumberFilter(DialogPtr, EventRecord *, INT16 *);

void InitDurStrings(void);
void UseDurNumFilter(DialogPtr, INT16, INT16, INT16);
pascal Boolean DurNumberFilter(DialogPtr, EventRecord *, INT16 *);

void XableControl(DialogPtr, INT16, Boolean);
void MoveDialogControl(DialogPtr, INT16, INT16, INT16);
void GetHiddenDItemBox(DialogPtr, short, Rect *);
void SetDlgFont(DialogPtr, short, short, short);