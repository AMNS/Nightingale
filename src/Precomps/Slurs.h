/* Slurs.h for Nightingale */

void 		GetSlurPoints(LINK aSlurL, DPoint *knot, DPoint *c0, DPoint *c1, DPoint *endpoint);
void		DrawSlurSubObject(Rect *paper, LINK aSlurL);
void		SelectSlur(Rect *paper, LINK aSlurL);
Boolean		FindSLUR(Rect *paper, Point pt,LINK aSlurL);
SplineSeg	*IsSlurPoint(Point p, LINK aSlurL);
void		StartSlurBounds(void);
void		EndSlurBounds(Rect *paper, Rect *box);
void		DrawSlursor(Rect *paper, DPoint *start, DPoint *end, DPoint *c0, DPoint *c1,
						short how, Boolean dashed);
void		DrawTheSlursor(void);
void		RotateSlurCtrlPts(LINK, DDIST, DDIST, short);
void 		CreateAllTies(Document *doc);

/* Non-Doug Utilities */

Boolean		TrackSlur(Rect *,DPoint,DPoint *,DPoint *,DPoint *,DPoint *,Boolean);
void 		SetSlurPoints(LINK pL, LINK aSlurL, short index);

short		InfoCheckSlur(LINK);
short		InvalSlurAcross(LINK);
void		GetSlurContext(Document *, LINK, Point [], Point []);
short		InvalSlurs(LINK, LINK);
void		DoSlurEdit(Document *, LINK, LINK, short);
void		GetSlurBBox(Document *doc, LINK pL, LINK aSlurL, Rect *bbox, short margin);
void		HiliteSlurNodes(Document *, LINK pL);

Boolean		SetSlurCtlPoints(Document *, LINK, LINK, LINK, LINK, short, short, CONTEXT,
								Boolean);

Boolean		DeleteSlurTie(Document *, LINK);
Boolean		FlipSelSlur(LINK);

Boolean		InitAntikink(Document *, LINK, LINK);
void		Antikink(void);

/* For debugging */
void 		PrintSlurPoints(LINK, char *);
