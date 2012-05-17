/* Slurs.h for Nightingale */

void 		GetSlurPoints(LINK aSlurL, DPoint *knot, DPoint *c0, DPoint *c1, DPoint *endpoint);
void		DrawSlurSubObject(Rect *paper, LINK aSlurL);
void		SelectSlur(Rect *paper, LINK aSlurL);
Boolean		FindSLUR(Rect *paper, Point pt,LINK aSlurL);
SplineSeg	*IsSlurPoint(Point p, LINK aSlurL);
void		StartSlurBounds(void);
void		EndSlurBounds(Rect *paper, Rect *box);
void		DrawSlursor(Rect *paper, DPoint *start, DPoint *end, DPoint *c0, DPoint *c1,
						INT16 how, Boolean dashed);
void		DrawTheSlursor(void);
void 		CreateAllTies(Document *doc);
void		RotateSlurCtrlPts(LINK, DDIST, DDIST, INT16);

/* Non-Doug Utilities */

Boolean		TrackSlur(Rect *,DPoint,DPoint *,DPoint *,DPoint *,DPoint *,Boolean);
void 		SetSlurPoints(LINK pL, LINK aSlurL, INT16 index);

INT16		InfoCheckSlur(LINK);
INT16		InvalSlurAcross(LINK);
void		GetSlurContext(Document *, LINK, Point [], Point []);
INT16		InvalSlurs(LINK, LINK);
void		DoSlurEdit(Document *, LINK, LINK, INT16);
void		GetSlurBBox(Document *doc, LINK pL, LINK aSlurL, Rect *bbox, INT16 margin);
void		HiliteSlurNodes(Document *, LINK pL);

Boolean		SetSlurCtlPoints(Document *, LINK, LINK, LINK, LINK, INT16, INT16, CONTEXT,
								Boolean);

Boolean		FlipSelSlur(LINK);

Boolean		InitAntikink(Document *, LINK, LINK);
void		Antikink(void);

/* For debugging */
void 		PrintSlurPoints(LINK, char *);
