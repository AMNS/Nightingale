//#include <ListMgr.h>
//#include <ColorToolbox.h>
//#include <stdio.h>

//#define TRUE 1
//#define FALSE 0
//#define NIL (void *)0

#define CODE_RESOURCE 0

#if CODE_RESOURCE
pascal void	main(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell,
				short lDataOffset, short lDataLen, ListHandle lHandle);
#else
pascal void	MyLDEFproc(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell,
				short lDataOffset, short lDataLen, ListHandle lHandle);
#endif


