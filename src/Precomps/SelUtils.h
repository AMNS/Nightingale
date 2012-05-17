/* SelUtils.h for Nightingale */

INT16 	GetSelStaff(Document *);
INT16		GetStaffFromSel(Document *, LINK *);
void	GetStfRangeOfSel(Document *, STFRANGE *);
void	GetSelPartList(Document *, LINK []);
Boolean	IsSelPart(LINK, LINK []);
INT16	CountSelParts(LINK []);
LINK	GetSelPart(Document *);
INT16		GetVoiceFromSel(Document *);
void	Sel2MeasPage(Document *, INT16 *, INT16 *);

void	GetSelMIDIRange(Document *, INT16 *, INT16 *);
LINK	FindSelAcc(Document *, INT16);

void	UnemptyRect(Rect *);
void	FixEmptySelection(Document *, Point);
Boolean SelectStaffRect(Document *, Point);
void	DoThreadSelect(Document *, Point pt);
void	DrawTheSweepRects(void);

Point	InsertSpaceTrackStf(Document *, Point, INT16 *, INT16 *);
void	GetUserRect(Document *, Point, Point, INT16, INT16, Rect *);

void	NotesSel2TempFlags(Document *);
void	TempFlags2NotesSel(Document *);
