/* Prototypes for Copy.c for Nightingale */

LINK GetSrcLink(LINK,COPYMAP *,INT16);
LINK GetDstLink(LINK,COPYMAP *,INT16);

Boolean SetupCopyMap(LINK, LINK, COPYMAP **, INT16 *);
void CopyFixLinks(Document *, Document *, LINK, LINK, COPYMAP *, INT16);
Boolean CopyRange(Document *, Document *, LINK, LINK, LINK, INT16);
