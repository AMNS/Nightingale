/* This LDEF draws just the first character in a cell and draws a
 * dotted line border. It tries to center the character in the cell.
 * It uses all the font characteristics of the current port.
 * It ignores the indent values stored in the list.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "ChooseCharLDEF.h"

static void CenterInRect(Rect *r,Rect *cInR);

void GetCharBBox(GrafPtr, Rect *);
GrafPtr NewGrafPort(int, int);
void DisposGrafPort(GrafPtr);

#define SYS_GRAY	16

static GrafPtr charPort;
//static PatHandle grayPat;


/* main LDEF entry point */

#if CODE_RESOURCE
pascal void	main(short lMessage, Boolean lSelect, Rect *lRect, Cell /*lCell*/,
				short lDataOffset, short lDataLen, ListHandle lHandle)
#else
pascal void	MyLDEFproc(short lMessage, Boolean lSelect, Rect *lRect, Cell /*lCell*/,
				short lDataOffset, short lDataLen, ListHandle lHandle)
#endif
{
	FontInfo fontInfo;							/* font information (ascent/descent/etc) */
	ListPtr listPtr;								/* pointer to store dereferenced list */
	SignedByte hStateList, hStateCells;		/* state variables for HGetState/SetState */
	Ptr cellData;									/* points to start of cell data for list */
	//short leftDraw, topDraw;					/* left/top offsets from topleft of cell */
	short charWid, charHt, cellWid, cellHt;
	register char *p;
	PenState pnState;
	Rect cellRect;
	//Rect charBBox;
	//GrafPtr oldPort;
	//Rect bounds,cpBounds;
	//short cpHt,cpWd;
	short lMarg,tMarg;
		
	
#if CODE_RESOURCE
	RememberA0();
	SetUpA4();
#endif	

	/* lock and dereference list mgr handles */
	hStateList = HGetState((Handle)lHandle);
	HLock((Handle)lHandle);
	listPtr = *lHandle;
	hStateCells = HGetState(listPtr->cells);
	HLock(listPtr->cells);
	cellData = (Ptr)*(listPtr->cells);
	
	cellWid = lRect->right - lRect->left;
	cellHt = lRect->bottom - lRect->top;

	GetFontInfo(&fontInfo);
	
	switch (lMessage) {
		case lInitMsg:
	  		break;

		case lDrawMsg:
			EraseRect(lRect);
	
			/* Draw cell border */
			GetPenState(&pnState);
			PenPat(NGetQDGlobalsGray());
			cellRect = *lRect;
			cellRect.right++;			
			cellRect.bottom++;
			FrameRect(&cellRect);
			SetPenState(&pnState);

	  		if (lDataLen > 0) { // && charPort != NIL) {  			  		
				p = cellData + lDataOffset;
				
				cellRect.top++; cellRect.left++;
				cellRect.bottom--; cellRect.right--; 
				cellWid = cellRect.right - cellRect.left;
				cellHt = cellRect.bottom - cellRect.top;
				
				charWid = CharWidth(*p);
				if (charWid==0) charWid = cellWid>>2;				/* helps with zero-width chars */
				
				charHt = fontInfo.ascent + fontInfo.descent + fontInfo.leading;
				
				lMarg = (cellWid - charWid)/2;
				//tMarg = (cellHt - charHt)/2;
				tMarg = cellHt - (fontInfo.descent + fontInfo.leading);
				if (lMarg < 0) lMarg = 0;
				if (tMarg < 0) tMarg = 0;
				
				MoveTo(cellRect.left+lMarg, cellRect.top+tMarg);
				DrawChar(*p);
				
	  		}
		
			if (!lSelect)							/* otherwise, fall through to hilite */
	  			break;
		
		case lHiliteMsg:
//			BitClr(&HiliteMode, pHiliteBit);							/* do hilite color */
			LMSetHiliteMode(pHiliteBit);
			InvertRect(lRect);
			break;

		case lCloseMsg:
			break;
	}
	
	HSetState(listPtr->cells, hStateCells);
	HSetState((Handle)lHandle, hStateList);

#if CODE_RESOURCE
	RestoreA4();
#endif
}

static void CenterInRect(Rect *r,Rect *cInR)
{
	short rWid = r->right-r->left;
	short rHt = r->bottom-r->top;
	
	short cWid = cInR->right-cInR->left;
	short cHt = cInR->bottom-cInR->top;
	
	short lMarg = (rWid - cWid)/2;
	short tMarg = (rHt - cHt)/2;
	
	OffsetRect(cInR,lMarg,tMarg);	
}


/* •••so far, bbox.left & right undefined!! */
void GetCharBBox(GrafPtr port, Rect *bbox)
{
	register short	i;
	register char	*p;
	short				portHt, rowBytes, numBytes;
	Boolean			gotIt;
	Rect				bounds;

	GetPortBounds(port, &bounds);	

	portHt = bounds.bottom - bounds.top;
	
	const BitMap *portBits = GetPortBitMapForCopyBits(port);
	rowBytes = portBits->rowBytes;
	numBytes = portHt * rowBytes;
	
	p = portBits->baseAddr;
	for (i=0, gotIt=FALSE; i<numBytes; i++, p++) {
		if (*p) {
			gotIt = TRUE;
			break;
		}
	}
	if (gotIt)
		bbox->top = i / rowBytes;								/* num rows (pixels) from top */
	else
		bbox->top = 0;

/* •••Will this next line cause that odd address crash on a macPlus?
i.e., am I trying to access a WORD (or larger) value aligned on an odd address?
I think the answer is "No". If I were doing "if (*(short *)p)" below, then I would be.
*/
	p = portBits->baseAddr + numBytes-1;				/* point at last byte in bitmap */
	for (i=numBytes-1, gotIt=FALSE; i>=0; i--, p--) {
		if (*p) {
			gotIt = TRUE;
			break;
		}
	}
	if (gotIt)
		bbox->bottom = i / rowBytes;							/* num rows (pixels) from top */
	else
		bbox->bottom = 1;
	
}


