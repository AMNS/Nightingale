/* File.h for Nightingale */

/* A version code is 'N' followed by three digits, e.g., 'N105': N-one-zero-five.
Be careful: It's neither a valid C string nor a valid Pascal string! */

#define THIS_FILE_VERSION 'N106'		/* Current file format version code */
#define FIRST_FILE_VERSION 'N105'		/* We can open all versions from this to the current one */

/* Error codes and error info codes (all positive) */

#define HDR_TYPE_ERR 217			/* Heap header in file contains incorrect type */
#define HDR_SIZE_ERR 251			/* Heap header in file contains incorrect objSize */
#define FIX_LINKS_ERR 990			/* Problem fixing cross links */
#define MEM_FULL_ERR 997			/* ExpandFreeList failed */
#define MISC_HEAPIO_ERR 998

#define MEM_ERRINFO 99				/* ExpandFreeList failed */

/* Codes for object types being read/written or for non-read/write call when an I/O
error occurs (all negative). */

#define	HEADERobj -999
#define VERSIONobj -998
#define SIZEobj -997
#define CREATEcall -996
#define OPENcall -995
#define CLOSEcall -994
#define DELETEcall -993
#define RENAMEcall -992
#define WRITEcall -991
#define STRINGobj -990
#define INFOcall -989
#define SETVOLcall -988
#define BACKUPcall -987
#define MAKEFSSPECcall -986
#define NENTRIESerr -899
#define READHEAPScall -898


void DisplayDocumentHdr(short id, Document *doc);
short CheckDocumentHdr(Document *doc, short *pFirstErr);
void DisplayScoreHdr(short id, Document *doc);
short CheckScoreHdr(Document *doc, short *pFirstErr);

short OpenFile(Document *doc, unsigned char *filename, short vRefNum, FSSpec *pfsSpec, long *fileVersion);
void OpenError(Boolean, short, short, short);
short SaveFile(Document *, Boolean);
void SaveError(Boolean, short, short, short);
