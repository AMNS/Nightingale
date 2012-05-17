/* Tuplet.h	for Nightingale */

Boolean CheckMaxTupleNum(INT16 v, LINK vStartL, LINK vEndL, TupleParam *tParam);
INT16 GetTupleNum(LINK, LINK, INT16, Boolean);
void DoTuple(Document *, TupleParam *);
Boolean GetBracketVis(Document *, LINK startL, LINK endL, INT16 staff);
void InitTuplet(LINK, INT16, INT16, INT16, INT16, Boolean, Boolean, Boolean);
void SetTupletYPos(Document *, LINK);
LINK CreateTUPLET(Document *,LINK,LINK,INT16,INT16,INT16,Boolean,Boolean,TupleParam *);

LINK RemoveTuplet(Document *,LINK);
void UnTuple(Document *);
void DoUntuple(Document *);
void UntupleObj(Document *, LINK, LINK, LINK, INT16);

Boolean IsTupletCrossStf(LINK);
void GetTupletInfo(Document *, LINK, PCONTEXT, DDIST *, DDIST *, INT16 *, DPoint *,
							DPoint *, unsigned char [], DDIST *, Rect *);
void DrawTupletBracket(DPoint firstPt, DPoint lastPt, DDIST brackDelta, 
			DDIST yCutoffLen, DDIST xd, INT16 tuplWidth, Boolean firstBelow,
			Boolean lastBelow, PCONTEXT pContext, Boolean dim);
void DrawPSTupletBracket(DPoint firstPt, DPoint lastPt, DDIST brackDelta, 
			DDIST yCutoffLen, DDIST xd, INT16 tuplWidth, Boolean firstBelow,
			Boolean lastBelow, PCONTEXT pContext);
void DrawTUPLET(Document *, LINK, CONTEXT []);

LINK HasTupleAcross(LINK, INT16);
LINK VHasTupleAcross(LINK, INT16, Boolean);
LINK FirstInTuplet(LINK);
LINK LastInTuplet(LINK);
