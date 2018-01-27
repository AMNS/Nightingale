/* EndianUtils.h for Nightingale
 *
 *  Created by Michel Alexandre Salim on 4/13/08.
 */

/* PowerPCs are big endian; Intel processors are little endian. */

#if TARGET_RT_LITTLE_ENDIAN
#define FIX_END(v) FIX_ENDIAN(sizeof(v), v)
#define FIX_ENDIAN(n, v) if (n==2) { v = CFSwapInt16BigToHost(v); } \
else if (n==4) { v = CFSwapInt32BigToHost(v); } else exit(1)
#else
#define FIX_END(v) v
#define FIX_ENDIAN(n, v) v
#endif
