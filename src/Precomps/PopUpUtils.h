/* PopUpUtils.h for Nightingale */

Boolean InitGPopUp(PGRAPHIC_POPUP, Point, short, short);
void SetGPopUpChoice(PGRAPHIC_POPUP, short);
void DrawGPopUp(PGRAPHIC_POPUP);
void HiliteGPopUp(PGRAPHIC_POPUP, short);
Boolean DoGPopUp(PGRAPHIC_POPUP);
void DisposeGPopUp(PGRAPHIC_POPUP);

void DrawPopUp(UserPopUp *popup);
void TruncPopUpString(UserPopUp *popup);
short InitPopUp(DialogPtr dlog, UserPopUp *p, short item, short pItem,
					short menuID, short choice);
short	DoUserPopUp(UserPopUp *p);
void	ChangePopUpChoice(UserPopUp *p, short choice);
void	DisposePopUp(UserPopUp *p);
void	HilitePopUp (UserPopUp *p,short activ);
short	ResizePopUp(UserPopUp *p);
void	ShowPopUp(UserPopUp *p, short vis);

void	HiliteArrowKey(DialogPtr, short, UserPopUp *, Boolean *);
