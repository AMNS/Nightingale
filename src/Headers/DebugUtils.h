/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 *
 * Copyright © 1986-95 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 * These are private interfaces for the two "DebugUtil" files, which had
 * to be broken into two files because of a code overflow.
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

extern INT16 nerr, errLim;
extern Boolean minDebugCheck;			/* TRUE=don't print Less & Least important checks */

#ifdef DDB

Boolean QDP(char *fmtStr);

#define COMPLAIN(f, v) { if (QDP(f)) { DebugPrintf((f), (v)); bad = TRUE; } }
#define COMPLAIN2(f, v1, v2) { if (QDP(f)) { DebugPrintf((f), (v1), (v2)); bad = TRUE; } }
#define COMPLAIN3(f, v1, v2, v3) { if (QDP(f)) { DebugPrintf((f), (v1), (v2), (v3)); bad = TRUE; } }

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


