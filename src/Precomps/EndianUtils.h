/*
 *  EndianUtils.h
 *  Nightingale
 *
 *  Created by Michel Alexandre Salim on 4/13/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#if TARGET_RT_LITTLE_ENDIAN
#define fix_end(v) fix_endian(sizeof(v), v)
#define fix_endian(n, v) if (n==2) { v = CFSwapInt16BigToHost(v); } \
else if (n==4) { v = CFSwapInt32BigToHost(v); } else exit(1)
#else
#define fix_end(v) v
#define fix_endian(n, v) v
#endif
