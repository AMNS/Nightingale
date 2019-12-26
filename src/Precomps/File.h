/* File.h for Nightingale */

#define THIS_FILE_VERSION 'N106'		/* Current file format version */
#define FIRST_FILE_VERSION 'N104'		/* We can open all versions from this one to the current one */

short OpenFile(Document *doc, unsigned char *filename, short vRefNum, FSSpec *pfsSpec, long *fileVersion);
void OpenError(Boolean, short, short, short);
short SaveFile(Document *, Boolean);
void SaveError(Boolean, short, short, short);
