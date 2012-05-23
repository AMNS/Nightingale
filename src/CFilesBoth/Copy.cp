/* Copy.c for Nightingale, revised for v.2.1 */

/*										NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROPERTY OF
 *	ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE SECRET AND IS
 *	NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED WRITTEN
 *	AUTHORIZATION FROM THE OWNER.
 *
 *	Copyright © 1989-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


#define EXTRAOBJS	4

LINK GetSrcLink(LINK dstL, COPYMAP *copyMap, INT16 numObjs)
{
	INT16 i=0;

	for ( ; i<numObjs; i++)
		if (copyMap[i].srcL)
			if (copyMap[i].dstL==dstL)
				return copyMap[i].srcL;
				
	return NILINK;
}

LINK GetDstLink(LINK srcL, COPYMAP *copyMap, INT16 numObjs)
{
	INT16 i=0;

	for ( ; i<numObjs; i++)
		if (copyMap[i].srcL==srcL)
			return copyMap[i].dstL;
				
	return NILINK;
}

/* --------------------------------------------------------------- SetupCopyMap -- */
/* Set up the copyMap array for use in updating links to objects (slurs, Graphics,
dynamics, etc.) that refer to objects of other types. Returns TRUE if it succeeds,
FALSE if it fails (due to lack of memory). */

Boolean SetupCopyMap(LINK startL, LINK endL, COPYMAP **copyMap, INT16 *objCount)
{
	LINK pL;
	INT16 i, numObjs=0;
	Boolean okay=TRUE;

	for (pL=startL; pL!=endL; pL = RightLINK(pL))
		numObjs++;						
	*copyMap = (COPYMAP *)NewPtr((Size)(numObjs+EXTRAOBJS)*sizeof(COPYMAP));

	if (GoodNewPtr((Ptr)*copyMap))
		for (i=0; i<numObjs; i++)
			(*copyMap)[i].srcL = (*copyMap)[i].dstL = NILINK;
	else
		{ NoMoreMemory(); okay = FALSE; }

	*objCount = numObjs;					/* Return number of objects in range. */
	return okay;
}


/* --------------------------------------------------------------- CopyFixLinks -- */
/* Use the copy map to fix up cross links in the object list to objects of
 * different types.
 */

void CopyFixLinks(Document *doc, Document *fixDoc, LINK startL, LINK endL,
						COPYMAP *copyMap, INT16 numObjs)
{
	register INT16 i; INT16 type;
	register LINK pL;
	
	if (fixDoc!=doc)
		InstallDoc(fixDoc);

	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		switch ((type=ObjLType(pL))) {
			case SLURtype:
				for (i = 0; i<numObjs; i++)
					if (copyMap[i].srcL)
						if (copyMap[i].srcL==SlurFIRSTSYNC(pL)) {
							SlurFIRSTSYNC(pL) = copyMap[i].dstL;
							break;
						}
				for (i = 0; i<numObjs; i++)
					if (copyMap[i].srcL)
						if (copyMap[i].srcL == SlurLASTSYNC(pL)) {
							SlurLASTSYNC(pL) = copyMap[i].dstL;
							break;
						}
				break;
			case DYNAMtype:
				for (i = 0; i<numObjs; i++)
					if (copyMap[i].srcL)
						if (copyMap[i].srcL==DynamFIRSTSYNC(pL)) {
							DynamFIRSTSYNC(pL) = copyMap[i].dstL;
							break;
						}
				if (IsHairpin(pL))
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL==DynamLASTSYNC(pL)) {
								DynamLASTSYNC(pL) = copyMap[i].dstL;
								break;
							}
				break;
			case GRAPHICtype:
				for (i = 0; i<numObjs; i++)
					if (copyMap[i].srcL)
						if (copyMap[i].srcL==GraphicFIRSTOBJ(pL)) {
							GraphicFIRSTOBJ(pL) = copyMap[i].dstL;
							break;
						}
				if (GraphicSubType(pL)==GRDraw)
					for (i = 0; i<numObjs; i++)
						if (copyMap[i].srcL)
							if (copyMap[i].srcL==GraphicLASTOBJ(pL)) {
								GraphicLASTOBJ(pL) = copyMap[i].dstL;
								break;
							}
				break;
			case TEMPOtype:
				for (i = 0; i<numObjs; i++)
					if (copyMap[i].srcL)
						if (copyMap[i].srcL==TempoFIRSTOBJ(pL)) {
							TempoFIRSTOBJ(pL) = copyMap[i].dstL;
							break;
						}
				break;
			case ENDINGtype:
				for (i = 0; i<numObjs; i++)
					if (copyMap[i].srcL)
						if (copyMap[i].srcL==EndingFIRSTOBJ(pL)) {
							EndingFIRSTOBJ(pL) = copyMap[i].dstL;
							break;
						}				
				for (i = 0; i<numObjs; i++)
					if (copyMap[i].srcL)
						if (copyMap[i].srcL==EndingLASTOBJ(pL)) {
							EndingLASTOBJ(pL) = copyMap[i].dstL;
							break;
						}	
			default:
				;
		}
	}

	if (fixDoc!=doc)
		InstallDoc(doc);
}


/*
 * Copy range in srcDoc from srcStartL to srcEndL into data structure of 
 * dstDoc at insertL. Copying is done from heaps in srcDoc to heaps in dstDoc.
 * Leaves the heaps of srcDoc installed.
 */

Boolean CopyRange(Document *srcDoc, Document *dstDoc, LINK srcStartL, LINK srcEndL,
							LINK insertL,
							INT16 /*toRange*/		/* unused */
							)
{
	LINK pL,prevL,copyL,initL; INT16 i,numObjs; COPYMAP *copyMap;

	InstallDoc(srcDoc);

	SetupCopyMap(srcStartL, srcEndL, &copyMap, &numObjs);

	InstallDoc(dstDoc);

	initL = prevL = LeftLINK(insertL);

	for (i=0, pL=srcStartL; pL!=srcEndL; i++, pL=DRightLINK(srcDoc, pL)) {
		copyL = DuplicateObject(DObjLType(srcDoc, pL), pL, FALSE, srcDoc, dstDoc, FALSE);
		if (!copyL)
			return FALSE;

		RightLINK(copyL) = insertL;
		LeftLINK(insertL) = copyL;
		RightLINK(prevL) = copyL;
		LeftLINK(copyL) = prevL;

		copyMap[i].srcL = pL;	copyMap[i].dstL = copyL;
		prevL = copyL;
  	}

 	InstallDoc(dstDoc);
	FixCrossLinks(srcDoc, dstDoc, RightLINK(initL), insertL);

	InstallDoc(dstDoc);
	CopyFixLinks(srcDoc, dstDoc, RightLINK(initL), insertL, copyMap, numObjs);

	DisposePtr((Ptr)copyMap);
	
	InstallDoc(srcDoc);

	return TRUE;
}
