/* MCaret.h for Nightingale */

Boolean	MEInitCaretSystem(void);
void	MEInitCaret(Document *doc);
void	MEUpdateCaret(Document *doc);
void	MEActivateCaret(Document *doc, Boolean active);
void	MESaveBackBits(Document *doc);
void	MEHideCaret(Document *doc);
void	MEIdleCaret(Document *doc);
void	MEAdjustCaret(Document *doc, Boolean doMove);
LINK	Point2InsPt(Document *, Point);
LINK	MESetCaret(Document *, Point, Boolean);

