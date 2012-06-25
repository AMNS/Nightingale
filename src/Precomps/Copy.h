/* Prototypes for Copy.c for Nightingale */

LINK GetSrcLink(LINK,COPYMAP *,short);
LINK GetDstLink(LINK,COPYMAP *,short);

Boolean SetupCopyMap(LINK, LINK, COPYMAP **, short *);
void CopyFixLinks(Document *, Document *, LINK, LINK, COPYMAP *, short);
Boolean CopyRange(Document *, Document *, LINK, LINK, LINK, short);
