/* Tuplet.h	for Nightingale */

Boolean CheckMaxTupleNum(short v, LINK vStartL, LINK vEndL, TupleParam *tParam);
short GetTupleNum(LINK, LINK, short, Boolean);
void DoTuple(Document *, TupleParam *);
Boolean GetBracketVis(Document *, LINK startL, LINK endL, short staff);
void InitTuplet(LINK, short, short, short, short, Boolean, Boolean, Boolean);
void SetTupletYPos(Document *, LINK);
LINK CreateTUPLET(Document *,LINK,LINK,short,short,short,Boolean,Boolean,TupleParam *);

LINK RemoveTuplet(Document *,LINK);
void UnTuple(Document *);
void DoUntuple(Document *);
void UntupleObj(Document *, LINK, LINK, LINK, short);

Boolean IsTupletCrossStf(LINK);
void GetTupletInfo(Document *, LINK, PCONTEXT, DDIST *, DDIST *, short *, DPoint *,
							DPoint *, unsigned char [], DDIST *, Rect *);
void DrawTupletBracket(DPoint firstPt, DPoint lastPt, DDIST brackDelta, 
			DDIST yCutoffLen, DDIST xd, short tuplWidth, Boolean firstBelow,
			Boolean lastBelow, PCONTEXT pContext, Boolean dim);
void DrawPSTupletBracket(DPoint firstPt, DPoint lastPt, DDIST brackDelta, 
			DDIST yCutoffLen, DDIST xd, short tuplWidth, Boolean firstBelow,
			Boolean lastBelow, PCONTEXT pContext);
void DrawTUPLET(Document *, LINK, CONTEXT []);

LINK HasTupleAcross(LINK, short);
LINK VHasTupleAcross(LINK, short, Boolean);
LINK FirstInTuplet(LINK);
LINK LastInTuplet(LINK);
