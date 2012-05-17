/* Nodes.h - header file for Nodes.c for Nightingale */

Boolean		BuildEmptyList(Document *doc, LINK *headL, LINK *tailL);
void		DisposeNodeList(Document *doc, LINK *headL, LINK *tailL);

LINK		InsertNode(Document *, LINK, INT16, INT16);
void		InsNodeInto(LINK pL, LINK insLink);
void		InsNodeIntoSlot(LINK node, LINK beforeL);

void		MoveNode(LINK pL, LINK insLink);
void		MoveRange(LINK startL, LINK endL, LINK insLink);

LINK		DeleteNode(Document *, LINK);
LINK		DeleteRange(Document *, LINK, LINK);
void		CutRange(LINK, LINK);
void		CutNode(LINK);

Boolean 	ExpandNode(LINK, LINK *, INT16);
void		ShrinkNode(LINK, INT16);
void		InitNode(INT16, LINK);

LINK		NewNode(Document *,INT16, INT16);

LINK		MergeObject(Document *, LINK, LINK);
