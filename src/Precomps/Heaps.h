/* Heaps.h for Nightingale - Header file for Heaps.c */

#ifdef LinkToPtrFUNCTION
char *LinkToPtr(HEAP *, LINK);
#endif

Boolean		InitAllHeaps(Document *doc);
void		DestroyAllHeaps(Document *doc);
Boolean		ExpandFreeList(HEAP *heap, long nObjs);
LINK		HeapAlloc(HEAP *heap, unsigned short nObjs);
LINK		HeapFree(HEAP *heap, LINK list);
LINK		InsertLink(HEAP *heap, LINK head, LINK before, LINK objlist);
LINK		InsAfterLink(HEAP *heap, LINK head, LINK after, LINK objlist);
LINK		RemoveLink(LINK objL, HEAP *heap, LINK head, LINK obj);
