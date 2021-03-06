/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */


#define DDB

/* These functions implement three levels of checking:
	Most important: messages about problems are prefixed with '•'
	More important: messages about problems are prefixed with '*'
	Less & Least important: messages about problems have no prefix
*/

extern short nerr, errLim;
extern Boolean minDebugCheck;			/* True=don't print Less & Least important checks */

#ifdef DDB

Boolean QDP(char *fmtStr);

#define COMPLAIN(f, v) { if (QDP(f)) { LogPrintf(LOG_WARNING, (f), (v)); bad = True; } }
#define COMPLAIN2(f, v1, v2) { if (QDP(f)) { LogPrintf(LOG_WARNING, (f), (v1), (v2)); bad = True; } }
#define COMPLAIN3(f, v1, v2, v3) { if (QDP(f)) { LogPrintf(LOG_WARNING, (f), (v1), (v2), (v3)); bad = True; } }
#define COMPLAIN4(f, v1, v2, v3, v4) { if (QDP(f)) { LogPrintf(LOG_WARNING, (f), (v1), (v2), (v3), (v4)); bad = True; } }

#else

#define COMPLAIN(f, v) { MayErrMsg((f), (v)); nerr++; bad = True; }
#define COMPLAIN2(f, v1, v2) { MayErrMsg((f), (v1), (v2)); nerr++; bad = True; }
#define COMPLAIN3(f, v1, v2, v3) { MayErrMsg((f), (v1), (v2), (v3)); nerr++; bad = True; }
#define COMPLAIN4(f, v1, v2, v3, v4) { MayErrMsg((f), (v1), (v2), (v3), (v4)); nerr++; bad = True; }

#endif


