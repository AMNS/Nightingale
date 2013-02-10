/* File.h for Nightingale */

#define THIS_VERSION 'N105'
#define FIRST_VERSION 'N100'			/* Can read all versions from this one on */
#define FILE_ADDHEAP					/* Converting to 'N097' or higher adds one heap */

short OpenFile(Document *doc, unsigned char *filename, short vRefNum, FSSpec *pfsSpec, long *fileVersion);
void OpenError(Boolean, short, short, short);
short SaveFile(Document *, Boolean);
void SaveError(Boolean, short, short, short);

