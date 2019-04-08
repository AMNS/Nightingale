/* FileUtils.h for Nightingale */

#include <Carbon/Carbon.h>

FILE *FSpOpenInputFile(Str255 macfName, FSSpec *fsSpec);
void FillFontTable(Document *);
void GetPrintHandle(Document *, unsigned long version, short vRefNum, FSSpec *pfsSpec);
Boolean WritePrintHandle(Document *);
