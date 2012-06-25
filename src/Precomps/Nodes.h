/* Nodes.h - header file for Nodes.c for Nightingale */

Boolean		BuildEmptyList(Document *doc, LINK *headL, LINK *tailL);
void		DisposeNodeList(Document *doc, LINK *headL, LINK *tailL);

LINK		InsertNode(Document *, LINK, short, short);
void		InsNodeInto(LINK pL, LINK insLink);
void		InsNodeIntoSlot(LINK node, LINK beforeL);

void		MoveNode(LINK pL, LINK insLink);
void		MoveRange(LINK startL, LINK endL, LINK insLink);

LINK		DeleteNode(Document *, LINK);
LINK		DeleteRange(Document *, LINK, LINK);
void		CutRange(LINK, LINK);
void		CutNode(LINK);

Boolean 	ExpandNode(LINK, LINK *, short);
void		ShrinkNode(LINK, short);
void		InitNode(short, LINK);

LINK		NewNode(Document *,short, short);

LINK		MergeObject(Document *, LINK, LINK);
