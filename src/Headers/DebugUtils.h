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
Boolean DCheckMBBox(LINK, Boolean);
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

#else

#define COMPLAIN(f, v) { MayErrMsg((f), (v)); nerr++; bad = TRUE; }
#define COMPLAIN2(f, v1, v2) { MayErrMsg((f), (v1), (v2)); nerr++; bad = TRUE; }
#define COMPLAIN3(f, v1, v2, v3) { MayErrMsg((f), (v1), (v2), (v3)); nerr++; bad = TRUE; }

#endif

/* If we're looking at the Clipboard, Undo or Mstr Page, "flag" arg by adding a huge offset */
#define FP(pL)	( abnormal ? 1000000L+(pL) : (pL) )

/* Check whether a Rect is a valid 1st-quadrant rectangle with positive height
(Nightingale uses rectangles of zero width for empty key signatures). */
#define GARBAGE_Q1RECT(r)	(  (r).left>(r).right || (r).top>=(r).bottom		\
									|| (r).left<0 || (r).right<0							\
									|| (r).top<0 || (r).bottom<0)
#define ZERODIM_RECT(r)		(  (r).left==(r).right || (r).top==(r).bottom	)


