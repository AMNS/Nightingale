/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */


#define DDB

/* Prototype for internal routines */
Boolean DCheckMeasSubobjs(Document *, LINK, Boolean);
Boolean DCheckMBBox(Document *, LINK, Boolean);
Boolean DCheck1SubobjLinks(Document *, LINK);
Boolean DCheck1NEntries(Document *, LINK);

/* These functions implement three levels of checking:
	Most important: messages about problems are prefixed with '•'
	More important: messages about problems are prefixed with '*'
	Less & Least important: messages about problems have no prefix
*/

extern short nerr, errLim;
extern Boolean minDebugCheck;			/* TRUE=don't print Less & Least important checks */

#ifdef DDB

Boolean QDP(char *fmtStr);

#define COMPLAIN(f, v) { if (QDP(f)) { LogPrintf(LOG_WARNING, (f), (v)); bad = TRUE; } }
#define COMPLAIN2(f, v1, v2) { if (QDP(f)) { LogPrintf(LOG_WARNING, (f), (v1), (v2)); bad = TRUE; } }
#define COMPLAIN3(f, v1, v2, v3) { if (QDP(f)) { LogPrintf(LOG_WARNING, (f), (v1), (v2), (v3)); bad = TRUE; } }
#define COMPLAIN4(f, v1, v2, v3, v4) { if (QDP(f)) { LogPrintf(LOG_WARNING, (f), (v1), (v2), (v3), (v4)); bad = TRUE; } }

#else

#define COMPLAIN(f, v) { MayErrMsg((f), (v)); nerr++; bad = TRUE; }
#define COMPLAIN2(f, v1, v2) { MayErrMsg((f), (v1), (v2)); nerr++; bad = TRUE; }
#define COMPLAIN3(f, v1, v2, v3) { MayErrMsg((f), (v1), (v2), (v3)); nerr++; bad = TRUE; }
#define COMPLAIN4(f, v1, v2, v3, v4) { MayErrMsg((f), (v1), (v2), (v3), (v4)); nerr++; bad = TRUE; }

#endif


