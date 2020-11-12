/*
 *								NOTICE
 *
 *	THIS FILE IS CONFIDENTIAL PROPERTY OF MATHEMAESTHETICS INC.
 *	IT IS CONSIDERED A TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY
 *	PARTIES WHO HAVE NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 *
 *	String Pool Manager Source Code, Nightingale version.
 *	Copyright 1987-1997	Mathemaesthetics, Inc.
 *	All Rights Reserved.
 *
 *	Version 3.0
 *	Douglas McKenna, MATHEMAESTHETICS, INC.
 *	Originally written Nov 13, 1988
 *	Recent modifications Mar 13, 1996
 *
 *	This is the public header file with interfaces to String Pool routines.
 *
 *	The String Pool Manager is a set of routines that lets the programmer worry less about
 *	C, Pascal, and (in the full, but not the Nightingale, version) Unicode strings and
 *	inefficient allocation of Str255's, which are usually 95% empty.  It takes care of
 *	automatic conversion between Pascal and C strings, and uses a stack-based memory
 *	management model a la PostScript. Individual pools of strings can be saved as a single
 *	block in a file or as a resource in a resource file, since references to them are in
 *	byte offsets from the start of the pool, not actual memory addresses.
 *
 *	The String Manager is designed similarly to other Macintosh Toolbox
 *	Managers.  The library lets you create, using NewStringPool(), and destroy, using
 *	DisposeStringPool(), objects of type StringPool, which are used to hold arbritrary
 *	collections of related and intermixed strings.
 *
 *	Most of the String Manager routines operate on a current StringPool, which is
 *	installed using SetStringPool().  The current StringPool can be found out by
 *	calling GetStringPool().  There are also routines that work on explicitly given
 *	pools without changing the current one.
 *	
 *	The current StringPool is a block that floats in memory, from which
 *	and into which we store our strings.  A C string is stored into the current
 *	StringPool using CStore(); a Pascal string using PStore().  A string already
 *	stored can be replaced with a new string using CReplace() or PReplace(), both
 *	of which will overwrite the previous string value if the new string is not
 *	longer than the previous value, or will allocate new storage if it's longer.
 *	When a string is stored into the current StringPool, a StringRef is returned
 *	to the caller.  All strings are then accessed via their StringRef in the
 *	current StringPool.  StringRefs are 32 bit signed offsets in this implementation.
 *
 *	Once a string has been stored into the current StringPool, we can retrieve it
 *	using any of various routines.  PCopy() delivers the address of a copy
 *	of a given Pascal string.  CAddr() and PAddr() deliver the actual floating
 *	address within the current StringPool of the string whose StringOffset is
 *	provided.
 *
 *	Since an empty string is the same regardless of C or Pascal (it's just a
 *	single 0 byte), we always keep an empty string at StringRef 0.  This
 *	way, the caller can always be assured that a StringRef of 0 means the legal
 *	empty string.  Other strings are allocated sequentially from the current StringPool,
 *	with the StringPool automatically enlarged if necessary.
 *
 *	To allocate temporary strings, you can create a new StringPool, keep them
 *	there, and then later destroy the StringPool.  Or you can use the stack-based
 *	save and restore mechanism on the current StringPool using SaveStrings() and
 *	RestoreStrings().  There is no other form of garbage collection implemented.
 */

#pragma once

typedef long StringRef;							// The pool "address" of any given string
#define StringOffset StringRef					/* For compatibility with older code */

// The client should not be able to access anything within the StringPool handle

typedef struct OpaqueStringPool **StringPoolRef;


typedef void (*NoMemoryFunc)(OSErr err);

#define nilpool					((StringPoolRef)nil)
#define defaultPool				nilpool

#define emptyStringRef			0L
#define noMemoryStringRef		(-1L)
#define badParamStringRef		(-2L)
#define noResourceStringRef		(-3L)
#define IsErrorStringRef(ref)	((ref) < 0L)

#define kDefaultPoolSize		(-1L)


/*
	Prototypes for public routines, with explanations

	All of the routines that operate on an explicit pool are matched by macros that
	declare pseudo-routines of similar (and shorter) names that do the same thing,
	except they operate on the current (default) pool.  This is signaled by passing
	defaultPool to the original routine.
*/


/*--------------------------------------
	InitStringPools() initializes or re-initializes the String Manager to use a
	PushStringPool/PopStringPool stack of size maxNest, and to create a default
	current string pool of size firstSize.  Delivers True if okay, or False if not.
	Typically you need only call InitStringPools() once near the start of the
	application. If maxNest or firstSize is 0, the routine uses an internal default size.
																							*/
	int InitStringPools(short maxNest, long firstSize);


/*--------------------------------------
	Declare an out-of-memory handler and the error code to pass to it.  If the
	ErrorFunc argument is non-NULL, it will be called by every routine that delivers
	a StringRef value of -1.  Delivers the old value.
																							*/
	NoMemoryFunc SetNoStringMemoryHandler(NoMemoryFunc func, OSErr err);


/*--------------------------------------
	Allocate and initialize a new string pool, and deliver it.  This sets
	the pool to contain only the empty string.  If successful, it delivers the
	reference to the pool; otherwise, nil (not enough memory).
																							*/
	StringPoolRef NewStringPool(void);


/*--------------------------------------
	Re-initialize a given string pool.  This empties all strings, and resizes the pool
	to the given size.  Pools are grown automatically as you add strings to them; the
	size parameter lets you preset the free space size to something larger if you know
	you are about to store a lot of strings into the empty pool.  Delivers True if
	okay to proceed; False if memory problem.
																							*/
	int	InitStringPool(StringPoolRef pool, long size);


/*--------------------------------------
	Given a previously allocated string pool, ditch it and all its strings.
	This routine always delivers nil.
																							*/
	StringPoolRef DisposeStringPool(StringPoolRef pool);


/*--------------------------------------
	Install a new string pool as the current pool the library works on, and deliver
	the old pool to the calling routine.  This makes it easy to temporarily install
	a different (nested) pool for special operations in a subroutine.
																							*/
	StringPoolRef SetStringPool(StringPoolRef pool);


/*--------------------------------------
	Deliver the current string pool without changing it.
	This routine lets you get at the entire internal data for writing out to disk, etc.
																							*/
	StringPoolRef GetStringPool(void);


/*--------------------------------------
	Save the current pool on an internal stack, and install the given pool.
	This routine should be matched by a call to PopPool().  Delivers True if
	success; False if stack ran out.  pool can be nilPool to leave the current
	pool unchanged (although it will still be stacked).
																							*/
	int PushStringPool(StringPoolRef pool);


/*--------------------------------------
	Re-instate the string pool in effect before the most recent PushStringPool.
	This does nothing if internal stack is empty.  Delivers the new current pool.
																							*/
	StringPoolRef PopStringPool(void);


/*--------------------------------------
 	After a call to SaveStrings(), all strings allocated prior to the call will
	be considered permanently allocated until a matching call to RestoreStrings().
	Thus a call to ClearStrings() only cuts back the allocation pointer to the
	level of the latest call to SaveStrings().  SaveStrings() returns False if
	it runs out of stack space; True if all went OK.
																							*/
	int SaveStringsInPool(StringPoolRef pool);
	#define SaveStrings() SaveStringsInPool(defaultPool)


/*--------------------------------------
	RestoreStrings() undoes the effect of a previous matching call to SaveStrings().
	If there was no previous call, it does nothing.  Strings in the string
	memory block that were protected from clearing due to the previous
	SaveStrings() call will now be considered available for clearing.
																							*/
	void RestoreStringsInPool(StringPoolRef pool);
	#define RestoreStrings() RestoreStringsInPool(defaultPool)


/*--------------------------------------
	ClearStrings() resets the next free byte available to be allocated from the
	string memory block back down to the current save level.  All strings allocated
	since the last SaveStrings() (or InitStringPools()) will be invalid and their storage
	considered reclaimed.  The empty string at Offset 0 will always remain valid and
	allocated.  The current size of the string memory block remains the same.
																							*/
	void ClearStringsInPool(StringPoolRef pool);
	#define ClearStrings() ClearStringsInPool(defaultPool)


/*--------------------------------------
	ClearFromString() resets the next free byte available to be allocated from the
	string memory block back down to (and including) the given string.  All strings allocated
	since the the given string will be invalid and their storage
	considered reclaimed.  The empty string at Offset 0 will always remain valid and
	allocated.  The current size of the string memory block remains the same.  This is
	primarily useful for removing storage for the last stored string.
																							*/
	void ClearFromStringInPool(StringRef offset, StringPoolRef pool);
	#define ClearFromString(offset) ClearFromStringInPool(offset,defaultPool)


/*--------------------------------------
	LockStrings() and UnlockStrings() are used in pairs to temporarily lock
	the string pool memory.  The two routines maintain a lock level counter, so
	that they can be nested.  Every call to LockStrings() should have a matching
	call to UnlockStrings().  While the string memory is locked, any addresses
	returned by CAddrInPool() or PAddrInPool() will remain valid.  Pools should
	be unlocked if you plan to store new strings into them.
																							*/
	void LockStringsInPool(StringPoolRef pool);
	void UnlockStringsInPool(StringPoolRef pool);
	#define LockStrings() LockStringsInPool(defaultPool)
	#define UnlockStrings() UnlockStringsInPool(defaultPool)


/*--------------------------------------
 *	GetStringPoolSize() delivers the size, in bytes, of the given string pool
																							*/
	long GetStringPoolSize(StringPoolRef pool);
	#define GetTheStringPoolSize() GetStringPoolSize(defaultPool)


/*--------------------------------------
 *	ShrinkStringPool() removes all free space from given pool.
																							*/
	void ShrinkStringPool(StringPoolRef pool);
	#define ShrinkTheStringPool() ShrinkStringPool(defaultPool)



							/*****  C  Strings  *****/

/*--------------------------------------
	CStoreInPool() takes a given pointer to a C string, and stores it in the next
	available spot in the given string pool.  If there is not enough room in
	the pool, the library attempts to enlarge it so that there will be.  The
	data is then copied into string memory, and typed internally as a C string.
	The unique reference to the string in the pool is returned for use as an argument
	to other library routines to identify the string just stored.
	If the string being stored is the empty string, the StringRef of 0 is always
	returned.  This means that no string memory allocation occurs when storing
	empty strings.  If CStoreInPool() couldn't allocate enough memory, it delivers -1.
	
	NOTE: CStoreInPool() cannot tell if a given argument string pointer is indeed a
	pointer to a C string or not.  It stores all data up to and including the
	first null (EOS) byte at or after the address provided.
																							*/
	StringRef CStoreInPool(Byte *str, StringPoolRef pool);
	#define CStore(str) CStoreInPool(str, defaultPool)


/*--------------------------------------
	CAddrInPool() delivers the address within the memory block of the 0'th byte of
	the C string for the given StringRef.  Since C strings can be arbitrarily
	long, the caller is in a better position to know how long to expect the
	particular string.  This address is only good as long as the library's
	internal memory block remains in the same location in the heap, since the
	block is usually unlocked.  Thus the pointer that CAddrInPool() returns should be
	treated gingerly, and should be used only temporarily to copy the C string
	out to a fixed spot in memory, or to do other simple non-ToolBox things with.
	Any Toolbox call should be assumed to invalidate the address CAddrInPool() returns.
	Be especially careful about Mac 68K inter-segment calling, since this can cause
	an unloaded segment to be loaded into the heap, without explicitly calling
	any toolbox routine.
																							*/
	Byte *CAddrInPool(StringRef offset, StringPoolRef pool);
	#define CAddr(offset) CAddrInPool(offset, defaultPool)


/*--------------------------------------
	CConcatStringInPool() takes a given stored C string, and concatenates
	the given C string to it, delivering the StringRef of the result.
	If the stored string is the last in the pool, then concatenation can
	be done by appending, and the returned reference will be the same as the
	argument; otherwise, the storage at the given offset is discarded, and
	new storage for the concatenation is returned.  The caller must assure
	that both strings are C strings.
																							*/
	StringRef CConcatStringInPool(StringRef offset, Byte *str, StringPoolRef pool);
	#define CConcatString(offset,str) CConcatStringInPool(offset, str, defaultPool)

/*--------------------------------------
	/*  Fix Little/Big Endian problems in the string pool by converting from either form to
		the other.																			*/

	void EndianFixStringPool(StringPoolRef pool);

/*--------------------------------------
	Display parameters of the string pool in the log file.									*/
	
	void DisplayStringPool(StringPoolRef pool);

/*--------------------------------------

	Check the pool for obvious problems; if any are found, return a nonzero code.			*/

	short StringPoolProblem(StringPoolRef pool);

/*--------------------------------------
	IsCStringInPool() delivers True or False, depending on whether the given
	reference is for a string originally stored as a C string.
																							*/
	int IsCStringInPool(StringRef offset, StringPoolRef pool);
	#define IsCString(offset) IsCStringInPool(offset, defaultPool)


/*--------------------------------------
	CDrawStringInPool() draws the C string using DrawText in the current port.
																							*/
	void CDrawStringInPool(StringRef offset, StringPoolRef pool);
	#define CDrawString(offset) CDrawStringInPool(offset, defaultPool)
																				

/*--------------------------------------
	CStringWidthInPool() delivers the C string's width using TextWidth.
	To avoid a name conflict, I renamed CStringWidth to CStringWidthSP. --DAB
																							*/
	long CStringWidthInPool(StringRef offset, StringPoolRef pool);
	#define CStringWidthSP(offset) CStringWidthInPool(offset, defaultPool)

							/*****  Pascal  Strings  *****/
								
/*--------------------------------------
	PStoreInPool() takes a given pointer to a Pascal string, and stores it in the
	next available spot in the string pool.  If there is not enough room
	in the pool, the library attempts to enlarge it so that there will be.  The
	data is then copied into string memory, and typed internally as Pascal.
	The reference to the string in the pool is returned for use as an argument
	to other library routines to identify the string just stored.
	If the string being stored is the empty string, the offset of 0 is always
	returned.  This means that no string memory allocation occurs when storing
	empty strings.  If PStoreInPool() couldn't allocate enough memory, it delivers -1.
	
	NOTE: PStoreInPool() cannot tell if a given argument string pointer is indeed a
	pointer to a Pascal string or not.  The byte pointed at is taken to be the
	length byte of the string, and up to 255 subsequent bytes are stored.
																							*/
	StringRef PStoreInPool(Byte *str, StringPoolRef pool);
	#define PStore(str) PStoreInPool(str, defaultPool)


/*--------------------------------------
	PReplaceInPool() replaces a given string in the pool at offset with a given Pascal
	string, str.  If the new string is not longer than the one it is to replace,
	then it is stored at the same StringRef as the old one.  If it is longer,
	then it is allocated its own memory, and stored there as if you had made a
	call to PStore().  In either case, the StringRef of the newly stored
	string is returned, or -1 in case of memory error.
																							*/
	StringRef PReplaceInPool(StringRef offset, Byte *str, StringPoolRef pool);
	#define PReplace(offset,str) PReplaceInPool(offset, str, defaultPool)


/*--------------------------------------
	PAddrInPool() delivers the address within the pool of the 0'th (length)
	byte of the Pascal string at the given StringRef.  This address is only good
	as long as the library's internal memory block remains in the same location
	in the heap, since the block is always unlocked.  Thus the pointer PAddr()
	returns should be treated gingerly, and should be used only temporarily to
	copy the Pascal string out to a fixed spot in memory, or to do other simple
	non-ToolBox things with.  Any Toolbox call should be assumed to invalidate
	the address PAddr() returns.  This routine should only be used in cases
	where efficiency is important; the preferred method of accessing your Pascal
	strings should be using PCopy().  Again, be careful about Mac 68K inter-segment
	calling, since this can cause an unloaded segment to be loaded into the
	heap, without explicitly calling any toolbox routine.
																							*/
	Byte *PAddrInPool(StringRef offset, StringPoolRef pool);
	#define PAddr(offset) PAddrInPool(offset, defaultPool)


/*--------------------------------------
	PCopyInPool() delivers the address of a static copy of the string to be found
	at the given StringRef.  The string to be found there should be a Pascal
	string stored previously by PStore().  PCopy() is the preferred method of
	accessing your Pascal strings in the memory block.  If the given StringRef
	is out of bounds, or if the string stored there is not Pascal, then the
	library calls SysBeep() and returns a pointer to an empty string.
	PCopy() maintains two separate static buffers, so that it can be used at
	most twice in any function argument list (alternate calls use alternate buffers).
																							*/
	Byte *PCopyInPool(StringRef offset, StringPoolRef pool);
	#define PCopy(offset) PCopyInPool(offset, defaultPool)


/*--------------------------------------
	PReadStringInPool() reads a string ('STR ') resource having the given ID from
	the current resource file using the Mac Toolbox Get1Resource() function;
	installs the string into string memory at the next available free spot;
	and delivers the appropriate StringRef to identify it.  If the string
	resource wasn't found, it delivers -1 for error.
																							*/
	StringRef PReadStringInPool(short ID, StringPoolRef pool);
	#define PReadString(ID) PReadStringInPool(ID, defaultPool)


/*--------------------------------------
	PReadStringList() reads the i'th string from a string list resource having the
	given ID, stores it in string memory, and delivers the appropriate
	StringRef to it.  It uses the GetIndString() toolbox routine to retrieve
	the string from the resource.  Delivers -1 if the index is out of bounds or
	the given resource couldn't be found.
																							*/
	StringRef PReadStringListInPool(short ID, short i, StringPoolRef pool);
	#define PReadStringList(ID, i) PReadStringListInPool(ID, i, defaultPool)


/*--------------------------------------
	PDrawStringInPool() draws the Pascal string using DrawString in the current port.
																							*/
	void PDrawStringInPool(StringRef offset, StringPoolRef pool);
	#define PDrawString(offset) PDrawStringInPool(offset, defaultPool)
																				

/*--------------------------------------
	PStringWidthInPool() delivers the C string's width using TextWidth
																							*/
	long PStringWidthInPool(StringRef offset, StringPoolRef pool);
	#define PStringWidth(offset) PStringWidthInPool(offset, defaultPool)

