/* FileUtils.h for Nightingale */

#include <Carbon/Carbon.h>

FILE *FSpOpenInputFile(Str255 macfName, FSSpec *fsSpec);
void FillFontTable(Document *);
void GetPrintHandle(Document *, unsigned long version, short vRefNum, FSSpec *pfsSpec);
Boolean WritePrintHandle(Document *);

#define BITMAP_NCOLORS 2			/* BMP images for palettes must be black and white */

FILE *NOpenBMPFile(char *filename, long *pixOffset, short *pWidth, short *pByteWidth,
		short *pByteWidthWithPad, short *pHeight);
