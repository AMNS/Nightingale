/* SelUtils.h for Nightingale */

short 	GetSelStaff(Document *);
short	GetStaffFromSel(Document *, LINK *);
void	GetStfRangeOfSel(Document *, STFRANGE *);
void	GetSelPartList(Document *, LINK []);
Boolean	IsSelPart(LINK, LINK []);
short	CountSelParts(LINK []);
LINK	GetSelPart(Document *);
short	GetVoiceFromSel(Document *);
void	Sel2MeasPage(Document *, short *, short *);

void	GetSelMIDIRange(Document *, short *, short *);
LINK	FindSelAcc(Document *, short);
Boolean HomogenizeSel(Document *, short);

void	UnemptyRect(Rect *);
void	FixEmptySelection(Document *, Point);
Boolean SelectStaffRect(Document *, Point);
void	DoThreadSelect(Document *, Point pt);
void	DrawTheSweepRects(void);

Point	InsertSpaceTrackStf(Document *, Point, short *, short *);
void	GetUserRect(Document *, Point, Point, short, short, Rect *);

void	NotesSel2TempFlags(Document *);
void	TempFlags2NotesSel(Document *);
