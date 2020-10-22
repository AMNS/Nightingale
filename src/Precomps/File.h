/* File.h for Nightingale */

/* A version code is 'N' followed by three digits, e.g., 'N105': N-one-zero-five.
Be careful: It's neither a valid C string nor a valid Pascal string! */

#define THIS_FILE_VERSION 'N106'		/* Current file format version */
#define FIRST_FILE_VERSION 'N105'		/* We can open all versions from this one to the current one */

/* Error codes and error info codes */

#define HDR_TYPE_ERR 217			/* Heap header in file contains incorrect type */
#define HDR_SIZE_ERR 251			/* Heap header in file contains incorrect objSize */
#define FIX_LINKS_ERR 990			/* Problem fixing cross links */
#define MEM_FULL_ERR 997			/* ExpandFreeList failed */
#define MISC_HEAPIO_ERR 998

#define MEM_ERRINFO 99				/* ExpandFreeList failed */

void DisplayDocumentHdr(short id, Document *doc);
Boolean CheckDocumentHdr(Document *doc);
void DisplayScoreHdr(short id, Document *doc);
Boolean CheckScoreHdr(Document *doc);

short OpenFile(Document *doc, unsigned char *filename, short vRefNum, FSSpec *pfsSpec, long *fileVersion);
void OpenError(Boolean, short, short, short);
short SaveFile(Document *, Boolean);
void SaveError(Boolean, short, short, short);
