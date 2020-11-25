/*
 *								NOTICE
 *
 *	THIS FILE IS CONFIDENTIAL PROPERTY OF MATHEMAESTHETICS INC.  IT IS CONSIDERED
 *	A TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 *	WRITTEN AUTHORIZATION FROM THE OWNER.
 *
 *	String Pool Manager Source Code, Nightingale version.
 *	Copyright 1987-96	Mathemaesthetics, Inc.
 *	All Rights Reserved.
 *
 *	This file contains routines to implement the String Pool Manager. A StringPool
 *	can contain C or Pascal strings, which are referenced by the outside world
 *	using StringRefs.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


#ifndef UnusedArg
	#define UnusedArg(x)	x = x;
#endif

/* Constants and macros */

#ifndef nil
	#define nil			((void *)0)
#endif

#ifndef True
	#define True		1
	#define False		0
#endif

#define MAXPOOLNEST		32

enum {									/* Types of strings in separate bits */
	C_String = 1,
	P_String = 2
	};

/* Be careful: in Nightingale, MAXSAVE must be 8 for compatibility with existing files! */

#define MAXSAVE		8

/* Since we're storing entire string pools as blocks in outside files, and have been
doing so since the days of Motorola 68000 CPUs, we have to ensure that the header struct
alignment is stable. (This was no problem in THINK C 7, since it's 68K-only.) */

/* MAS: force alignment to mac68k on all platforms */

#pragma options align=mac68k

typedef struct OpaqueStringPool {
		Byte nulltype;						/* Type of null string: both C and Pascal */
		Byte nullstr;						/* Always 0 */
		StringRef firstFreeByte;			/* Next available byte in pool for allocating */
		StringRef topFreeByte;				/* 1 more than last free byte in pool */
		StringRef bottomByte[MAXSAVE];		/* Clear context for save/restore */
		short saveLevel;					/* Save/restore stack level */
		short lockLevel;					/* How many times pool has been locked */
		long sizeJump;						/* How many bytes to increase pool size by */
		
		/* Subsequent bytes in pool are allocated for strings */
		
		} StringPool;


/* Private globals */

static NoMemoryFunc NoStringMemoryFunc;		/* What to call if no memory */
static OSErr noMemoryErrCode;				/* What error to pass to error callback */

static long			 firstPoolSize;			/* Default pool size to create */
static StringPoolRef thePool;				/* Always the current string pool */
static StringPoolRef theFirstPool;			/* The original default pool */
static StringPoolRef *poolStack,			/* Stack for pushing/popping pool contexts */
					 *poolStackTop,			/* One more than last legal entry */
					 *poolStackPtr;			/* Where to push to and pop from */


/* Private routine prototypes */

static OSErr	MoreStringMemory(long n, StringPoolRef pool);
static void	DefaultNoMemory(OSErr err);


/* Re-initialize the String Manager to use a push/pop context stack of given nesting size,
maxNest, and to allocate a default string pool with given initial size, firstSize. Deliver
True if done, False if couldn't do it. All hell will break loose if this routine's failure
is ignored! If either of the arguments is 0, then we use internal default sizes. */

int InitStringPools(short maxNest, long firstSize)
	{
		int okay = False;  long size;
		
		/* Allocate/reallocate the pushme/poolyou stack */
		
		if (poolStack) DisposePtr((Ptr)poolStack);
			
		if (maxNest <= 0) maxNest = MAXPOOLNEST;			/* Use default if 0 */
		size = maxNest * sizeof(StringPoolRef);
		poolStackPtr = poolStack = (StringPoolRef *)NewPtr(size);
		
		if (poolStack) {
			poolStackTop = poolStack + MAXPOOLNEST;
			
			/* Allocate/reallocate a current pool, in case user forgets or only wants to
			   use a single default pool.  Any current pool that is not theFirstPool
			   has to have been allocated by the library's client;  we're not tracking
			   all allocations, so they are responsible for disposing of it. */
			
			DisposeStringPool(theFirstPool);						/* Can be nil */
			firstPoolSize = (firstSize==0 ? 256 : firstSize);
			
			theFirstPool = NewStringPool();
			if (theFirstPool) {										/* Initializes it also */
				SetStringPool(theFirstPool);
				NoStringMemoryFunc = DefaultNoMemory;
				okay = True;
				}
			 else {
				DisposePtr((Ptr)poolStack);
				poolStack = poolStackPtr = nil;
				}
			}
		
		return(okay);
	}

/* Attempt to enlarge the current pool by a given number of bytes. Deliver noErr if
successful, or error code if not.  This routine is called automatically by the storage
routines below. */

static OSErr MoreStringMemory(long n, StringPoolRef pool)
	{
		long len;  OSErr err = 0;
		
		if (pool) {
			n += (*pool)->sizeJump;
			
			len = GetHandleSize((Handle)pool);
			SetHandleSize((Handle)pool, len += n);
			err = MemError();
			if (err == noErr) (*pool)->topFreeByte += n;
			}
		
		return(err);
	}

/* If the client is using a jmp_buf error return scheme, then it can call this to declare
the address of the routine it would normally call after a StringRef is returned from the
library here to check for a memory error (-1). In this case, the moMemory error code,
err, is passed in here for storage until the ErrorFunc is called with it as its only
argument. Once ErrorFunc is declared, the caller no longer needs to check allocations if
it doesn't want to. Note that NoStringMemoryFunc is always defined, so that we don't
have to check for nil before calling it (assuming InitStringPools has been called). */

NoMemoryFunc SetNoStringMemoryHandler(NoMemoryFunc ErrorFunc, OSErr err)
	{
		NoMemoryFunc oldFunc = NoStringMemoryFunc;
		
		NoStringMemoryFunc = ErrorFunc==nil ? DefaultNoMemory : ErrorFunc;
		noMemoryErrCode = err;
		
		return(oldFunc);
	}

/*	The default "no memory" handler does nothing, so the library routine will just
deliver -1.  */

static void DefaultNoMemory(OSErr err)
	{
		UnusedArg(err)
	}

/*	Create a new string pool, and initialize it to be ready to allocate from it. */

StringPoolRef NewStringPool()
	{
		StringPoolRef pool;
		
		pool = (StringPoolRef)NewHandle(sizeof(struct OpaqueStringPool));
		if (pool)
			if (!InitStringPool(pool,firstPoolSize))
				pool = DisposeStringPool(pool);				/* Sets it to nil */
		
		return(pool);
	}

/* Ditch a string pool and return nil so caller can set its reference. */

StringPoolRef DisposeStringPool(StringPoolRef pool)
	{
		if (pool) DisposeHandle((Handle)pool);

		return(nil);
	}

/* Ditch all strings in a given StringPool, and reset it to allocate from the beginning.
Set the pool to initially have space for <size> bytes of storage. Returns True if okay
to continue, False if problem.  Attempts to initialize the nil pool are ignored. */

int InitStringPool(StringPoolRef pool, long size)
	{
		int okay = True;
		
		if (pool) {
			StringPool *p = *pool;								/* Caution: floating ptr */
			
			p->firstFreeByte = sizeof(StringPool);
			p->lockLevel = 0;
			p->bottomByte[p->saveLevel = 0] = p->firstFreeByte;
			p->nullstr = 0;										/* Load empty string at offset 0 */
			p->nulltype = (C_String | P_String);				/* The only string of both types */
			p->sizeJump = 256;
			
			if (size==kDefaultPoolSize || size<0) size = 0;
			
			SetHandleSize((Handle)pool, sizeof(StringPool) + size);
			
			/* Memory has moved; p is invalid. */
			
			if (MemError())	okay = False;
			 else			(*pool)->topFreeByte = (*pool)->firstFreeByte + size;
			}
		
		return(okay);
	}

/*	Deliver the handle to the current string pool */

StringPoolRef GetStringPool()
	{
		return(thePool);
	}

/* Given a handle, assume it to be a handle to a previously created StringPool, and 
install it as the current string pool.  Delivers the old value of the current pool, and
does nothing if pool is nil. */

StringPoolRef SetStringPool(StringPoolRef pool)
	{
		StringPoolRef oldPool = thePool;
		
		if (pool) thePool = pool;
		
		return(oldPool);
	}

/* Deliver the size, in bytes, of the given string pool, or if nil, the current string
pool, or if none, 0.  */

long GetStringPoolSize(StringPoolRef pool)
	{
		if (pool == nil) pool = thePool;
		
		return( pool? GetHandleSize((Handle)pool) : 0L );
	}

/* Remove all free space from the given pool, or the current pool. The pool should
not be locked, although this will probably still work, since it always shrinks the
handle in place. */

void ShrinkStringPool(StringPoolRef pool)
	{
		if (pool == nil) pool = thePool;
		
		if (pool) SetHandleSize((Handle)pool,(*pool)->firstFreeByte);
	}

/* Clear all strings in given pool back down to the current save level's bottom
byte, so that subsequent allocation starts from there. */

void ClearStringsInPool(StringPoolRef pool)
	{
		StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		
		p = *pool;
		p->firstFreeByte = p->bottomByte[p->saveLevel];
	}

/* If the pool is currently unlocked, lock it and bump the lock level. If already
locked, then just bump the level.  While the pool is locked, the addresses returned
by PAddr and CAddr are valid. */

void LockStringsInPool(StringPoolRef pool)
	{
		if (pool == nilpool) pool = thePool;
		
		if ((*pool)->lockLevel++ == 0) HLock((Handle)pool);
	}

/* If the lock level is greater than 1, decrement it and return. If the level gets
back to 0, then unlock the given pool. */

void UnlockStringsInPool(StringPoolRef pool)
	{
		if (pool == nilpool) pool = thePool;
		
		if (--(*pool)->lockLevel == 0) HUnlock((Handle)pool);
	}

/*	Bump the current save level of the given pool, returning False for stack overflow
or other problem, and True if all okay to continue. */

int SaveStringsInPool(StringPoolRef pool)
	{
		StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		
		p = *pool;
		if (++p->saveLevel >= MAXSAVE) {
			p->saveLevel--;
			return(False);
			}
		
		p->bottomByte[p->saveLevel] = p->firstFreeByte;
		return(True);
	}

/*	Pop the current save level off the save stack, unless it's already empty. */

void RestoreStringsInPool(StringPoolRef pool)
	{
		StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		
		p = *pool;
		if (p->saveLevel > 0)
			p->firstFreeByte = p->bottomByte[p->saveLevel--];
	}

/*	Reclaim all space in pool from the given string on. */

void ClearFromStringInPool(StringRef offset, StringPoolRef pool)
	{
		StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (offset <= 0)
			p->firstFreeByte = sizeof(StringPool);		/* Deletes everything */
			
		 else if (offset < p->firstFreeByte) {
			p->firstFreeByte = offset;					/* Deletes this string and beyond */
			}
	}

/*  Fix Little/Big Endian problems in the string pool by swapping bytes to convert from
either form to the other. */

void EndianFixStringPool(StringPoolRef pool) {
	StringPool *p = *pool;
	
	FIX_END(p->firstFreeByte);
	FIX_END(p->topFreeByte);
	for (short k = 0; k<MAXSAVE; k++)
		FIX_END(p->bottomByte[k]);
	FIX_END(p->saveLevel);
	FIX_END(p->lockLevel);
	FIX_END(p->sizeJump);
}

/* Display parameters of the string pool in the log file. */

void DisplayStringPool(StringPoolRef pool) {
	StringPool *p = *pool;
	
	LogPrintf(LOG_INFO, "String pool nullstr=%d lockLevel=%d saveLevel=%d sizeJump=%d  (DisplayStringPool)\n",
		p->nullstr, p->lockLevel, p->saveLevel, p->sizeJump);
}

/* Check the pool for obvious problems; if any are found, return a nonzero code. */

short StringPoolProblem(StringPoolRef pool)
	{
		StringPool *p;
		short errCode;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;

		errCode = 0;
		if (p->nullstr!=0) errCode += 1;
		if (p->lockLevel>1) errCode += 2;
		if (p->saveLevel>1) errCode += 4;
		if (p->sizeJump<4 || p->sizeJump>4096) errCode += 8;
		
		return(errCode);
	}

						/*************************/
						/*** C String Routines ***/
						/*************************/

/*
 *	Tell caller whether given string was stored as C string or not. Always True
 *	for the empty string, and garbage if given offset is garbage.
 */

int IsCStringInPool(StringRef offset, StringPoolRef pool)
	{
		StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (offset>=0 && offset<p->firstFreeByte) {
			Byte *start = ((Byte *)p) + offset;
			if (*start & C_String) return(True);
			}
		
		return(False);
	}

/* Store a string into given pool, and deliver offset so it can be retrieved. We have a
routine for the two kinds of string we might use.  Strings are stored as either C or
Pascal, with a type byte in front.  The offset to this type byte is returned, or -1.  If
the string to be stored is the empty string, deliver offset of preallocated one at 0. */

StringRef CStoreInPool(Byte *str, StringPoolRef pool)
	{
		StringRef itsOffset;  long len;  Byte *dst;
		StringPool *p;
		
		/* If the string is nil or empty, always maps to the canonical empty string
		   stored at offset 0 in every pool. */
		
		if (str==nil || *str=='\0') return(emptyStringRef);
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		len = 2+strlen((char *)str);					/* Type byte + str + EOS */
		if ((p->firstFreeByte+len) >= p->topFreeByte) {
			if (MoreStringMemory(len,pool) != noErr) {
				NoStringMemoryFunc(noMemoryErrCode);
				return(noMemoryStringRef);
				}
			p = *pool;								/* Memory moved, deref handle again */
			}
		
		/* Copy str to its resting place */
		
		itsOffset = p->firstFreeByte;
		p->firstFreeByte += len;					/* Includes EOS */
		dst = ((Byte *)p) + itsOffset;
		*dst++ = C_String;
		while ((*dst++ = *str++) != '\0') ;			/* Better be a C string! */
		
		return(itsOffset);
	}

/* Append the given C string, str, to the already stored string at offset in the
given pool.  Deliver the new offset of the concatenated string, in case we have
to move the whole thing.  The caller should ensure that the stored string is C. */

StringRef CConcatStringInPool(StringRef offset, Byte *str, StringPoolRef pool)
	{
		StringRef newOff;  int isLastString;
		Byte *start,*dst;  long len,newLen,bytesNeeded;
		StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (offset<=emptyStringRef || offset>=p->firstFreeByte) {
			/* Concatenating to empty or non-existent string is same as installing
			   the given string in a new spot. */
			   
			return(CStoreInPool(str,pool));
			}
		
		/*  Look at the type first */
		
		start = ((Byte *)p) + offset;
		if ((*start++ & C_String) == 0) {
		
			/* Bad offset or not a C string, so skip it */
			
			return(CStoreInPool(str,pool));
			}
		
		/* Determine if we can expand pool and directly append to existing string, which
		   can only happen if it's the last installed string in pool.  Remember that
		   offset points at the character tag that precedes the first character in the
		   EOS-terminated string. */
		
		newLen = strlen((char *)str);
		len = 2 + strlen((char *)start);
		isLastString = (offset + len) == p->firstFreeByte;
		bytesNeeded = isLastString ? newLen : (len+newLen);
		
		if ((p->firstFreeByte+bytesNeeded) >= p->topFreeByte) {
			if (MoreStringMemory(bytesNeeded,pool) != noErr) {
				NoStringMemoryFunc(noMemoryErrCode);
				return(noMemoryStringRef);
				}
			p = *pool;			/* Memory moved, so deref again */
			}
		
		if (isLastString) {
			/* And we're now guaranteed that we can append directly so the offset we
			   deliver remains the same. */
			   
			newOff = offset;
			
			/* Get to tag character of stored string and move pointer to EOS at end */
			
			dst = ((Byte *)p) + offset;
			dst += len-1;
			}
		 else {
			/* Not last string in pool, so make a copy that is last. Remember where it
			   starts so we can return offset to caller. */
			   
			newOff = p->firstFreeByte;
			start = ((Byte *)p) + offset;
			dst = ((Byte *)p) + newOff;
			
			/* Copy initial tag field (which isn't a character) and copy string
			   including EOS, leaving dst pointing at EOS. */
			   
			*dst = *start;
			while ((*++dst = *++start) != '\0') ;
			}
		
		/* Overwrite EOS, thereby appending str, with new final EOS, and update where
		   subsequent strings are to be stored. */
		   
		while ((*dst++ = *str++) != '\0') ;
		p->firstFreeByte += bytesNeeded;
		
		return(newOff);
	}

/* Deliver pointer for a given C string offset in given pool, with bounds and type
check.  This pointer can only be assumed good up until the next ToolBox or inter-
segment call, if the pool is unlocked. */

Byte *CAddrInPool(StringRef offset, StringPoolRef pool)
	{
		Byte *start;  StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (offset<0 || offset>=p->firstFreeByte)
			start = nil;
		 else {
			start = ((Byte *)p) + offset;
			if (*start==0 || *start>(C_String|P_String))
				start = nil;						// Check for legal string types
			 else
				if ((*start++ & C_String) == 0)
					SysBeep(1);						// But deliver it anyway
			}
		
		return(start);
	}

/*
 *	Deliver the width, in pixels, off the given stored C string.
 */

long CStringWidthInPool(StringRef offset, StringPoolRef pool)
	{
		StringPool *p;  long width;  Byte *str;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (p->lockLevel == 0) HLock((Handle)pool);
		
		str = CAddrInPool(offset,pool);
		width = TextWidth(str,0,strlen((char *)str));
		
		if (p->lockLevel == 0) HUnlock((Handle)pool);
		
		return(width);
	}

/*
 *	Draw the given stored C string in the current port
 */

void CDrawStringInPool(StringRef offset, StringPoolRef pool)
	{
		StringPool *p; Byte *str;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (p->lockLevel == 0) HLock((Handle)pool);
		
		str = CAddrInPool(offset,pool);
		DrawText(str,0,strlen((char *)str));
		
		if (p->lockLevel == 0) HUnlock((Handle)pool);
	}


							/******************************/
							/*** Pascal String Routines ***/
							/******************************/

/*
 *	Similar to above, except for Pascal strings instead of C strings
 */

StringRef PStoreInPool(Byte *str, StringPoolRef pool)
	{
		StringRef start;  long len;  Byte *dst;
		StringPool *p;
		
		// If the string is nil or empty, always maps to the canonical
		// empty string stored at offset 0 in every pool.
		
		if (str==nil || *str==0) return(emptyStringRef);
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		len = 2 + *str;
		if ((p->firstFreeByte+len) >= p->topFreeByte) {
			if (MoreStringMemory(len,pool) != noErr) {
				NoStringMemoryFunc(noMemoryErrCode);
				return(noMemoryStringRef);
				}
			p = *pool;			// Memory moved so deref again
			}
		
		/* Copy str to its resting place */
		
		start = p->firstFreeByte;
		p->firstFreeByte += len--;
		dst = ((Byte *)p) + start;
		*dst++ = P_String;
		while (len-- > 0)
			*dst++ = *str++;
		
		return(start);
	}

/*
 *	Replace the string at a given offset with another string and deliver new offset.
 *	If the replacement is longer than the one already there, add new one, and
 *	deliver new offset, which may be -1 if error.  Old storage will be lost, except
 *	if allocation is done at some save level that gets restored.
 */

StringRef PReplaceInPool(StringRef offset, Byte *str, StringPoolRef pool)
	{
		Byte *dst;  long len, newLen;
		StringPool *p;
		
		// If the string is nil or empty, always maps to the canonical
		// empty string stored at offset 0 in every pool.
		
		if (str==nil || *str==0) return(emptyStringRef);
		
		// Install current pool if necessary
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		// Reality check on offset
		if (offset<0 || offset>=p->firstFreeByte) return(badParamStringRef);
		
		// Get tag byte of Pascal string, and reality check it
		dst = ((Byte *)p) + offset;
		if (*dst==0 || *dst>(C_String|P_String)) return(badParamStringRef);
		
		if ((*dst++ & P_String) == 0) {		// Moves dst up to length byte
			SysBeep(1);
			return(badParamStringRef);
			}
		
		// Check to see if string being replaced is last string in pool
		len = 2 + *dst;
		if ((offset + len) == p->firstFreeByte) {
			// Yup, it's last string in pool
			p->firstFreeByte = offset;				// Delete it
			return(PStoreInPool(str,pool));			// and append
			}
		
		newLen = 2 + *str;
		if (newLen > len)
			return(PStoreInPool(str,pool));		// If won't fit, append it to pool
		
		while (--newLen > 0)
			*dst++ = *str++;					// Else, overwrite old value
		
		return(offset);
	}

/*
 *	Deliver pointer for a given Pascal string offset, with bounds and type check.
 *	This pointer can only be assumed good up until the next ToolBox or intersegment
 *	call, if the pool is unlocked.
 */

Byte *PAddrInPool(StringRef offset, StringPoolRef pool)
	{
		Byte *start;
		StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (offset<0 || offset>=p->firstFreeByte)
			start = nil;
		 else {
			start = ((Byte *)p) + offset;
			if (*start==0 || *start>(C_String|P_String))
				start = nil;						// Check for legal types
			 else
				if ((*start++ & P_String) == 0)
					SysBeep(1);						// But deliver it anyway
			}
		
		return(start);
	}

/*
 *	Deliver pointer to static copy of a given Pascal string, which is easy to do
 *	since they can't get longer than 255 chars.  C strings must be copied by the
 *	caller, which is in a better position of knowing how long it might be, using
 *	one of the above routines.  We use two separate static buffers, so that we
 *	can have at most 2 calls in any one argument list.  Anything goes wrong, it
 *	returns the empty Pascal string.
 */

Byte *PCopyInPool(long offset, StringPoolRef pool)
	{
		static Byte theCopy[256],anotherCopy[256];
		static short flip;
		Byte *dst,*src;  Byte *buf;  short len;
		StringPool *p;
		
		if (pool == nilpool)
			pool = thePool;
		p = *pool;
		
		// Allow two consecutive calls to avoid interference
		dst = buf = (flip ? theCopy : anotherCopy);
		
		if (offset<0 || offset>=p->firstFreeByte) {
			// Returns the empty string
			buf[0] = 0;
			return(buf);
			}
		
		src = ((Byte *)p) + offset;
		if (*src==0 || *src>(C_String|P_String)) {			// Check for legal types
			buf[0] = 0;
			return(buf);
			}
		
		if ((*src++ & P_String) == 0) {
			SysBeep(1);
			buf[0] = 0;
			return(buf);
			}
		
		// Use alternate buffers on alternate calls
		
		flip = !flip;
		len = 1 + *src;
		while (len-- > 0) *dst++ = *src++;
		
		return(buf);
	}

/*
 *	Deliver the width, in pixels, off the given stored Pascal string.
 */

long PStringWidthInPool(StringRef offset, StringPoolRef pool)
	{
		StringPool *p;  long width;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (p->lockLevel == 0) HLock((Handle)pool);
		
		width = StringWidth(PAddrInPool(offset,pool));
		
		if (p->lockLevel == 0) HUnlock((Handle)pool);
		
		return(width);
	}

/*
 *	Draw the given stored Pascal string in the current port
 */

void PDrawStringInPool(StringRef offset, StringPoolRef pool)
	{
		StringPool *p;
		
		if (pool == nilpool) pool = thePool;
		p = *pool;
		
		if (p->lockLevel == 0) HLock((Handle)pool);
		
		DrawString(PAddrInPool(offset,pool));
		
		if (p->lockLevel == 0) HUnlock((Handle)pool);
	}


/*
 *	Read and store the i'th string from a given 'STR#' resource. Delivers -1
 *	for memory error, -2 for resource error.
 */

StringRef PReadStringListInPool(short id, short i, StringPoolRef pool)
	{
		Handle rsrc;  Byte str[256];
		short len;  OSErr err;
		
		rsrc = Get1Resource('STR#',id);
		err = ResError();
		if (err == memFullErr) {
			NoStringMemoryFunc(noMemoryErrCode);
			return(noMemoryStringRef);
			}
		if (rsrc==nil || *rsrc==nil || err!=noErr)
			return(noResourceStringRef);
		
		len = *(short *)(*rsrc);
		if (i<1 || i>len)
			return(badParamStringRef);
		
		GetIndString(str,id,i);
		
		return(PStoreInPool(str,pool));
	}

