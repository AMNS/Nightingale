/* ToolPalette.c for Nightingale - routines for operating the tools palette */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include <ctype.h>					/* for isdigit() */
#include "Nightingale.appl.h"

static short oldItem;

/*
 *	Prototypes for local functions
 */	

static Boolean	IsOnKeyPad(unsigned char);
static Boolean	IsDurKey(unsigned char, Boolean *);

static short	GetPalItem(short ch);
static void		SwapTools(short firstItem, short lastItem);
static void		PalCopy(GrafPtr dstPort, GrafPtr srcPort, short dstItem, short srcItem);
static void		UpdateGridCoords(void);

/* ------------------------------------------------------ TranslatePalChar and friends -- */

typedef struct {
	unsigned char origChar;
	unsigned char newChar;
} CHARMAP, *PCHARMAP;

static unsigned char shiftKeyPad[] = {
	')',		/* 0 */
	'!',		/* 1 */
	'@',		/* 2 */
	'#',		/* 3 */
	'$',		/* 4 */
	'%',		/* 5 */
	'^',		/* 6 */
	'&',		/* 7 */
	'*',		/* 8 */
	'('		/* 9 */
};

/* TranslatePalChar lets us modify Nightingale's standard mapping of character
 * codes to palette tools without having to mess around with the symtable or grid
 * globals. Only DoToolKeyDown need call this function.
 *
 * TranslatePalChar looks at a 'PLMP' resource for a mapping of the standard tool
 * chars to new ones. If it finds an entry that matches theChar, it replaces theChar
 * with that entry. First it translates chars produced on the numeric keypad with
 * the shift key down to the chars that shifted numbers on the main keyboard produce.
 * We do this to let numeric keypad users generate rests with shift-number combinations.
 * (NB: there are likely to be some international compatibility problems here.)
 *
 * Further, TranslatePalChar lets us set up a key (on the main keyboard and/or on
 * the numeric keypad) that toggles the duration keys to mean either rests or notes.
 * (This is the way HB Engraver behaved.) This would be useful for someone who
 * does most of their note entry by Step Recording. They could map numbers on the
 * keypad to durations and then use the zero on the keypad to toggle between
 * notes and rests.
 * NB: When this feature is active, clicking in the palette should affect the toggle
 * state. (i.e., toggle set for rests; user clicks on note; toggle should be set to
 * notes.) Also, if the currently selected palette item is a note, and the user hits
 * a toggle key, the corresponding rest should be selected in palette.
 *
 * Returns False if error getting resource; else returns True, even if no match
 * found. Maybe we shouldn't freak out if we can't find the resource, though probably
 * we should just ship a standard identity mapping. This 'PLMP' should probably go
 * into the setup file, and NightCustomizer should let the user edit it safely.
 */

Boolean TranslatePalChar(
				short *theChar,					/* character code */
				short /*theKey*/,				/* key code */
				Boolean doNoteRestToggle 		/* allow a key to toggle between notes and rests */
				)
{
	Handle		resH;
	unsigned char ch;
	char		*resP;
	short		numPairs;
	PCHARMAP	theCharMap;
	short		i,curResFile;
	unsigned char mainTogChar, kpadTogChar;
	static Boolean note = True;			/* True if note, False if rest; changed only when doNoteRestToggle==True */
	Boolean		isNote;					/* whether the current remapped char represents a note (e.g., w,h,q,e rather than W,H,Q,E) */
	Boolean		shiftIsDown;
	
	shiftIsDown = ShiftKeyDown();
	
	ch = (unsigned char)*theChar;
	
	/* If the shift key is down and theChar is still a digit, it means theChar
	 * was produced on the keypad. Remap it so that it appears to have resulted
	 * from shifting a number on the main keyboard.
	 * ??If there's a problem with this logic, consider using the new IsOnKeyPad().
	 */
	if (shiftIsDown && isdigit(ch))
		ch = shiftKeyPad[ch-48];
	
	/* 
	 * Get the PLMP resource from the Nightingale Setup file, and then restore
	 * the previous current resource file.
	 */
	curResFile = CurResFile();
	UseResFile(setupFileRefNum);

	resH = Get1Resource('PLMP', THE_PLMP);
	if (resH == NULL) {
		UseResFile(curResFile);
		return False;
	}
	
	UseResFile(curResFile);
		
	HLock(resH);
	resP = *resH;
	mainTogChar = *(unsigned char *)resP++;
	kpadTogChar = *(unsigned char *)resP++;
	numPairs = *(short *)resP;
	resP += 2;
	theCharMap = (PCHARMAP)resP;

	/* See if we're handling a toggle event. First make sure the toggle state
	 * is in sync with the currently selected tool palette item. If that item
	 * is a duration, the toggle state should be the same type (note or rest)
	 * as the item. If the user hits the rest toggle key to enter rests and
	 * then clicks on a note in the palette, the toggle state should switch
	 * to notes without the user having to hit the key again. This seems the
	 * most natural way to handle it. Once that's taken care of, see if this is
	 * a toggle key event, and if so, flip the toggle state.
	 */
	if (doNoteRestToggle) {
		if (IsDurKey((unsigned char)palChar, &isNote)) {			/* If user has clicked on a duration in palette, reset toggle state */
			if (note && !isNote)
				note = False;
			else if (!note && isNote)
				note = True;		
		}
		if ((ch == kpadTogChar && IsOnKeyPad(ch)) ||
			 (ch == mainTogChar && !IsOnKeyPad(ch))) {
			note = !note;			
			/* If current palette tool is a note or rest, must toggle the tool.
			 * Assigning palChar to theChar forces code below to change notes
			 * into rests or vice versa. We want to skip the 'PLMP' mapping below,
			 * since we're already working with Ngale's internal duration codes.
			 */
			if (IsDurKey((unsigned char)palChar, &isNote)) {
				ch = (unsigned char)palChar;
				goto skipMapping;
			}
			else {
				*theChar = ch = 0;						/* so GetPalItem won't do anything */
				HUnlock(resH);
				return True;
			}
		}
	}
	
	/* Remap the character according to 'PLMP' resource */
	for (i=0; i<numPairs; i++) {
		if (ch == theCharMap[i].origChar) {
			ch = theCharMap[i].newChar;
			break;
		}
	}
	
	/* Do note/rest toggle on the remapped character. */
skipMapping:
	if (doNoteRestToggle) {
		if (IsDurKey(ch, &isNote)) {
			if (isNote && !note) {
				if (ch == 0xDD)							/* take care of unruly breve */
					ch = '@';
				else
					ch -= 32;							/* turn the note into a rest */
			}
			/* NB: Code below used only when hitting a toggle key. See goto skipMapping above. */
			else if (!isNote && note && !shiftIsDown) {	/* don't defeat shiftkey=>rest action */
				if (ch == '@')							/* breve */
					ch = 0xDD;
				else
					ch += 32;							/* turn the rest into a note */
			}
		}
	}
	
	*theChar = ch;										/* convert back to short */

	HUnlock(resH);	
	return True;
}


/* The third long in the keymap consists only of key codes on the numeric
 * keypad, none of which appear elsewhere in the keymap. The only exception
 * is that the arrow keys on the Mac Plus fall in this long as well, so we
 * screen those out based on their character codes.
 * Assumes no other keys have been pressed since ch.
 */

Boolean IsOnKeyPad(unsigned char ch)
{
	KeyMap theKeys;
	
	GetKeys(theKeys);
	// MAS TODO -- figure this out
//	if (theKeys[2]) {
//		if (ch < 28 || ch > 31)
//			return True;							/* not an arrow key */
//	}
	return False;
}


static unsigned char durKeys[18] = {
	0xDD,'@',		/* breve (0xDD == opt-$) */
	'w','W',		/* whole */
	'h','H',		/* half */
	'q','Q',		/* qtr */
	'e','E',		/* 8th */
	'x','X',		/* 16th */
	'r','R',		/* 32nd */
	't','T',		/* 64th */
	'y','Y'			/* 128th */
};

Boolean IsDurKey(register unsigned char ch, register Boolean *isNote)
{
	register short i;
	
	*isNote = False;
	
	for (i=0; i<18; i++) {
		if (ch == durKeys[i]) {
			if (ch > 96)
				*isNote = True;
			return True;
		}
	}
	
	return False;
}


/* ----------------------------------------------------- regular Tool Palette routines -- */

/* Draw the tool menu palette.  The argument rect will be in global screen coordinates
if the palette is being drawn from the MDEF (i.e., it's still in the menu bar), or in
local palette window coordinates if it's in a standalone window or the menu has been
torn off (i.e., this is part of an update event). */

pascal void DrawToolMenu(Rect *r)
	{
		Rect frame, port, tmp, src;
		PaletteGlobals *whichPalette;
		
		whichPalette = *paletteGlobals[TOOL_PALETTE];
		
		/* Get the area within palette to draw picture */
		
		frame = *r;
		InsetRect(&frame, TOOLS_MARGIN, TOOLS_MARGIN);
		SetRect(&port, 0, 0,	whichPalette->maxAcross*toolCellWidth-1,
								whichPalette->maxDown*toolCellHeight-1);
		OffsetRect(&port, r->left+TOOLS_MARGIN, r->top+TOOLS_MARGIN);
		
		src = port;
		OffsetRect(&src, -src.left, -src.top);
		const BitMap *palPortBits = GetPortBitMapForCopyBits(palPort);
		const BitMap *thePortBits = GetPortBitMapForCopyBits(GetQDGlobalsThePort());
		CopyBits(palPortBits, thePortBits, &src, &port, srcCopy, NULL);
{ Rect portRect;
LogPrintf(LOG_NOTICE, "DrawToolMenu: TEST\n");
long len = 600; GetPortBounds(palPort, &portRect);
LogPrintf(LOG_NOTICE, "DrawToolMenu: MemBitCount(palPortBits, %ld)=%ld portRect tlbr=%d,%d,%d,%d\n",
len, MemBitCount((unsigned char *)palPortBits, len),
portRect.top, portRect.left, portRect.bottom, portRect.right);
}

		/* Add frame surrounding picture */
		
		InsetRect(&frame, -1, -1);
		FrameRect(&frame);
		
		/* Erase area to right and below frame, in case window was just shrunk */
		
		tmp = *r; tmp.left = tmp.right - (TOOLS_MARGIN-1);
		EraseRect(&tmp);
		tmp = *r; tmp.top = tmp.bottom - (TOOLS_MARGIN-1);
		EraseRect(&tmp);
		
		/* Place a poor person's grow box in lower right only when not in the menu bar */
		
		if (notMenu) {
			tmp = *r;
			tmp.left = tmp.right - (TOOLS_MARGIN+1);
			tmp.top = tmp.bottom - (TOOLS_MARGIN+1);
			EraseRect(&tmp);
			
			MoveTo(frame.right-2,frame.bottom-2); Line(TOOLS_MARGIN, 0);
			MoveTo(frame.right-2, frame.bottom-2); Line(0, TOOLS_MARGIN);
			
			MoveTo(frame.right+1,frame.bottom+1); Line(TOOLS_MARGIN, 0);
			MoveTo(frame.right+1, frame.bottom+1); Line(0, TOOLS_MARGIN);
			}
	}

/* 
 *  Toggle hiliting the given item in the tool palette.
 */

pascal void HiliteToolItem(Rect *r, short item)
	{
		Rect hiliteRect; short row,col;
		
		if (item) {
		
			/* Don't hilite items outside of current palette if it's been shrunk */
			row = (item-1) / (*paletteGlobals[TOOL_PALETTE])->maxAcross;
			col = (item-1) % (*paletteGlobals[TOOL_PALETTE])->maxAcross;
			
			if (row >= (*paletteGlobals[TOOL_PALETTE])->down) return;
			if (col >= (*paletteGlobals[TOOL_PALETTE])->across) return;
			
			/* Otherwise, it's a visible item */
	
			hiliteRect = toolRects[item];
			hiliteRect.bottom -= 1;
			hiliteRect.right -= 1;
			OffsetRect(&hiliteRect, r->left, r->top);
			InvertRect(&hiliteRect);
			}
	}

/*
 *	Locate which item in tools palette a point is within.  Returns an item
 *	number in 1-based menu item coordinates, or 0 if none found.
 */

pascal short FindToolItem(Point mousePt)
	{
		short index,count;
		PaletteGlobals *pg = *paletteGlobals[TOOL_PALETTE];
		
		if (PtInRect(mousePt,&toolsFrame)) {
			count = pg->maxAcross * pg->maxDown;
			for (index=1; index<=count; index++)
				if (PtInRect(mousePt,&toolRects[index]))
					return(index);
			}
		return(0);
	}

/*
 *	Handle a mouse down in the Tool Palette.  This tracks the mouse as
 *	as it passes over various items in the palette.
 */

void DoToolContent(Point pt, short modifiers)
	{
		short item = 0,firstItem=0, ch; WindowPtr w = palettes[TOOL_PALETTE];
		GrafPtr oldPort; Rect frame,portRect,itemCell; Boolean doSwap;
		
		GetPort(&oldPort);
		SetPort(GetWindowPort(w));
		
		GetWindowPortBounds(w, &frame);
		InsetRect(&frame,TOOLS_MARGIN,TOOLS_MARGIN);
		SetRect(&itemCell,0,0,0,0);
		doSwap = (modifiers & optionKey)!=0;
		if (doSwap) {
			PenMode(patXor);
			PenPat(NGetQDGlobalsGray());
			PenSize(2,2);
			}
		if (StillDown()) while (WaitMouseUp()) {
			GetMouse(&pt);
			if (PtInRect(pt,&frame)) {
				item = FindToolItem(pt);
				if (item != (*paletteGlobals[TOOL_PALETTE])->currentItem) {
					if (doSwap) {
						FrameRect(&itemCell);					/* Unhilite last item cell */
						itemCell = toolRects[item];
						itemCell.right--; itemCell.bottom--;
						}
					GetWindowPortBounds(w,&portRect);
					HiliteToolItem(&portRect, (*paletteGlobals[TOOL_PALETTE])->currentItem);
					HiliteToolItem(&portRect, item);
					(*paletteGlobals[TOOL_PALETTE])->currentItem = item;
					if (doSwap) {
						FrameRect(&itemCell);					/* Extra hilite for new item */
						}
					}
				if (item>0 && firstItem==0) firstItem = item;
				}
			}
		if (doSwap) {
			FrameRect(&itemCell);
			PenNormal();
			}
		
		if (item) {
			if (doSwap) {
				GetWindowPortBounds(w,&portRect);
				HiliteToolItem(&portRect, item);			/* Unhilite while swapping */
				SwapTools(firstItem,item);
				HiliteToolItem(&portRect, item);			/* Restore hilight */
				
				// CER 10.18.2004
				// OS 10.3.3 problem?
				// DoUpdate draws the tool in the global window, not in the palette window
				// DoUpdate(w);
				}
			ch = GetPalChar(item);
			if (ch != ' ') {
				HandleToolCursors(item);
				/*
				 *	Keep FixCursor() from changing back to an arrow until user
				 *	moves mouse outside of palette the first time from now.
				 */
				holdCursor = True;
				}
			 else {
				/* Palette item is empty: revert to arrow */
				HandleToolCursors(GetPalItem(CH_ENTER));
				PalKey(CH_ENTER);
				}
			}
		SetPort(oldPort);
	}

/*
 *	Given two items from the Tool Palette, swap them in their respective positions.
 */

void SwapTools(short firstItem, short lastItem)
	{
		char tmp; GrafPtr scratchPort,port;
		
		if (firstItem == lastItem) return;

		/* Data has been swapped, now swap bitmaps in picture */
		
		port = (OpaqueGrafPtr *)palettes[TOOL_PALETTE];
		scratchPort = NewGrafPort(toolCellWidth+1, toolCellHeight+1);
		if (scratchPort) {
			/* Swap source and dest cells in visible palette */
			PalCopy(scratchPort, port, 0, firstItem);
			PalCopy(port, port, firstItem,lastItem);
			PalCopy(port, scratchPort, lastItem, 0);
			/* Swap source and dest cells in underlying offscreen picture */
			PalCopy(scratchPort, palPort, 0, firstItem);
			PalCopy(palPort, palPort, firstItem,lastItem);
			PalCopy(palPort, scratchPort, lastItem, 0);
			DisposGrafPort(scratchPort);					/* toss offscreen bitmap */
			/* Swap source and dest info for items */
			tmp = grid[firstItem-1].ch;
			grid[firstItem-1].ch = grid[lastItem-1].ch;
			grid[lastItem-1].ch = tmp;
			GetToolZoomBounds();
			toolPalChanged = True;
			}
	}

/*
 *	Copy the tool item sized bitmap in scrPort to dstPort, in the position
 *	indicated by the source and destination items, srcItem and dstItem.  If
 *	a port is our temporary offscreen port, the corresponding item is 0.
 */

static void PalCopy(GrafPtr dstPort, GrafPtr srcPort, short dstItem, short srcItem)
	{
		Rect srcRect,dstRect;
		
		if (srcItem) srcRect = toolRects[srcItem];
		if (dstItem) dstRect = toolRects[dstItem];
		
		if (!srcItem)
			SetRect(&srcRect,0,0,dstRect.right-dstRect.left,dstRect.bottom-dstRect.top);
		if (!dstItem)
			SetRect(&dstRect,0,0,srcRect.right-srcRect.left,srcRect.bottom-srcRect.top);
		
		if (srcPort == palPort) OffsetRect(&srcRect,-TOOLS_MARGIN,-TOOLS_MARGIN);
		if (dstPort == palPort) OffsetRect(&dstRect,-TOOLS_MARGIN,-TOOLS_MARGIN);
		
		srcRect.right--; srcRect.bottom--;
		dstRect.right--; dstRect.bottom--;
		
		const BitMap *srcPortBits = GetPortBitMapForCopyBits(srcPort);
		const BitMap *dstPortBits = GetPortBitMapForCopyBits(dstPort);		
		CopyBits(srcPortBits, dstPortBits, &srcRect, &dstRect, srcCopy, NULL);
	}

/*
 *	Get cursor corresponding to choice from the tool palette; set globals 
 * currentItem, currentCursor, and palChar.  item should be from 1 to number of
 *	choices in palette (menu item code).
 */

void HandleToolCursors(short item)
	{
		short symIndex;
		
		palChar = GetPalChar(item);
		symIndex = GetSymTableIndex(palChar);
		if (symIndex == NOMATCH) {
			currentCursor = arrowCursor;
			PalKey(CH_ENTER);
			}
		 else
			currentCursor = palChar==CH_ENTER ? arrowCursor : cursorList[symIndex];
		
		SetCursor(*currentCursor);					/* So no delay before it changes */
	}

/*
 * Set the current item in the tool palette to correspond to <ch> and hilite it.
 */

void PalKey(char ch)
	{
		short	item, numItems;
		
		PaletteGlobals *toolGlobal; GrafPtr oldPort;
		WindowPtr w = palettes[TOOL_PALETTE];
		Rect portRect;

		toolGlobal = *paletteGlobals[TOOL_PALETTE];
		numItems = toolGlobal->maxAcross * toolGlobal->maxDown;

		GetPort(&oldPort); SetPort(GetWindowPort(w));
		for (item=0; item<numItems; item++)
			if (grid[item].ch == (unsigned char)ch) {
				item += 1;			/* Convert back to menu item coordinates */
				GetWindowPortBounds(w,&portRect);
				HiliteToolItem(&portRect, toolGlobal->currentItem);
				HiliteToolItem(&portRect, item);
				toolGlobal->currentItem = item;
				break;
				}
		SetPort(oldPort);
	}

/*
 * Return the palette char corresponding to the item in the tool palette.
 */
 
char GetPalChar(short item)
	{
		return grid[item-1].ch;
	}

/*
 *	Search the tools palette for the arrow item, which can be anywhere since
 *	the user has the ability to rearrange the palette.  Delivers the item
 *	number as a menu item code (lowest item is 1).
 */

short GetPalItem(short ch)
	{
		register short item,n; short maxRow,maxCol;
		
		maxRow = (*paletteGlobals[TOOL_PALETTE])->maxDown;
		maxCol = (*paletteGlobals[TOOL_PALETTE])->maxAcross;
		n = maxRow * maxCol;
		
		if (ch!=' ')
			for (item=0; item<n; item++)
				if (grid[item].ch == ch) return(item+1);
		
		return(0);
	}

/*
 *	Handle a key down as a choice from the tool palette, regardless of whether
 *	the tool palette is currently visible or not.
 */

void DoToolKeyDown(
			short ch,
			short key,				/* <0 means simulated key down so key is undef. and NO REMAPPING */
			short /*modifiers*/	/* <0 means simulated key down so modifiers are undefined */
			)
	{
		GrafPtr oldPort;
		PaletteGlobals *toolGlobal; short item;
		WindowPtr w = palettes[TOOL_PALETTE];
		Rect portRect;
		
		GetPort(&oldPort); SetPort(GetWindowPort(w));
		toolGlobal = *paletteGlobals[TOOL_PALETTE];
		
		if (ch==CH_ENTER) {
			if (currentCursor == arrowCursor)		/* Go back to old item saved */
				item = oldItem;
			 else {
				/* Save current item, and install arrow, whereever it is */
				oldItem = toolGlobal->currentItem;
			 	item = GetPalItem(CH_ENTER);
				}
			shook = currentCursor;
			HandleToolCursors(item);
			GetWindowPortBounds(w,&portRect);
			HiliteToolItem(&portRect, toolGlobal->currentItem);
			HiliteToolItem(&portRect, item);
			toolGlobal->currentItem = item;
			}
		 else {
			if (key>=0)
				if (!TranslatePalChar(&ch, (unsigned char)key, False)) ;
				/* If not, GetResource failed. Maybe no need to warn. */

			if (GetPalChar(toolGlobal->currentItem) != ch) {
				item = GetPalItem(ch);
				if (item) {
					HandleToolCursors(item);
					PalKey(ch);
					holdCursor = True;
					}
				}
			}
		 
		 SetPort(oldPort);
	}

/*
 *	Entertain a mouse down in Tool Palette's grow box, and grow the palette.
 *	pt is the initial mouse down point to pass to GrowWindow().
 */

void DoToolGrow(Point pt)
	{
		Rect oldRect,r; WindowPtr w = palettes[TOOL_PALETTE];
		short margin,x,y,across,down; long newSize;
		PaletteGlobals *whichPalette = *paletteGlobals[TOOL_PALETTE];
		
		GetWindowPortBounds(w,&oldRect);
		margin = 2*TOOLS_MARGIN;
		SetRect(&r,toolCellWidth+margin,
				   toolCellHeight+margin,
				   whichPalette->maxAcross*toolCellWidth+margin,
				   whichPalette->maxDown*toolCellHeight+margin);
				   
		newSize = GrowWindow(w,pt,&r);
		if (newSize) {
			x = LoWord(newSize) - margin; y = HiWord(newSize) - margin;
			across = (x + toolCellWidth/2) / toolCellWidth;
			down   = (y + toolCellHeight/2) / toolCellHeight;
			ChangeToolSize(across,down,False);
			}
	}

/*
 *	Set the tools palette window to show the first across columns by the
 *	first down rows of the palette.  Force everything to be redrawn.  If
 *	we're not zooming, record the previous palette size so that we can
 *	zoom in later.
 */

void ChangeToolSize(short across, short down, Boolean doingZoom)
	{
		GrafPtr oldPort;
		
		short margin = 2*TOOLS_MARGIN;
		WindowPtr w = palettes[TOOL_PALETTE];
		Rect portRect;
		
		SizeWindow(w,across*toolCellWidth +margin-1,
					   down*toolCellHeight+margin-1,True);
		(*paletteGlobals[TOOL_PALETTE])->across = across;
		(*paletteGlobals[TOOL_PALETTE])->down = down;
		GetWindowPortBounds(w,&toolsFrame);
		InsetRect(&toolsFrame,TOOLS_MARGIN,TOOLS_MARGIN);
		
		if (!doingZoom) {
			(*paletteGlobals[TOOL_PALETTE])->oldAcross = across;
			(*paletteGlobals[TOOL_PALETTE])->oldDown = down;
			}
		if (IsWindowVisible(w)) {
			GetPort(&oldPort); SetPort(GetWindowPort(w));
			GetWindowPortBounds(w,&portRect);
			InvalWindowRect(w,&portRect);
			SetPort(oldPort);
			}
	}

/*
 *	Compute the smallest grid rectangle that encloses all defined tools.  Undefined
 *	tools are marked in the grid[] array with a space.
 */

void GetToolZoomBounds()
	{
		register short r,c; short rMax,cMax; register GridRec *g;
		
		PaletteGlobals *pg = *paletteGlobals[TOOL_PALETTE];
		
		rMax = cMax = 0;
		r = c = 0;
		g = grid;
		
		for (r=0; r<pg->maxDown; r++)
			for (c=0; c<pg->maxAcross; c++,g++)
				if (g->ch != ' ') {
					if (r > rMax) rMax = r;
					if (c > cMax) cMax = c;
					}
		
		pg->zoomAcross = cMax + 1;
		pg->zoomDown =   rMax + 1;
	}

/*
 *	If the user has rearranged the tools palette at all, then we inquire
 *	if they want to save the changes, and save them if yes.  We return True
 *	if the user chooses either Save or Discard, False if Cancel.  If inquire
 *	is False, then we save without bringing the user into it.
 */

#define DiscardButton 3

Boolean SaveToolPalette(Boolean inquire)
	{
		short itemHit, curResFile; PicHandle thePic,toolPicture;
		Rect picRect; long size; GrafPtr port,oldPort;
		
		if (!toolPalChanged) return(True);
		
		if (inquire) {
			PlaceAlert(SAVEPALETTE_ALRT,NULL,0,80);
			ArrowCursor();
			itemHit = CautionAlert(SAVEPALETTE_ALRT,NULL);
			if (itemHit == Cancel) return(False);
			if (itemHit == DiscardButton) return(True);
			}
		
		/* Return offscreen picture back to resource whence it came and write out */

		curResFile = CurResFile();
		UseResFile(setupFileRefNum);

		toolPicture = (PicHandle)GetPicture(ToolPaletteID);
		if(!GoodResource((Handle)toolPicture)) {
			MayErrMsg("SaveToolPalette: Can't load resource");
			UseResFile(curResFile);
			return(True);
			}
		LoadResource((Handle)toolPicture);
		HNoPurge((Handle)toolPicture);
		picRect = (*toolPicture)->picFrame;
		OffsetRect(&picRect, -picRect.left, -picRect.top);
		
		GetPort(&oldPort);
		
		port = NewGrafPort(picRect.right,picRect.bottom);
		if (port) {
			SetPort(port);
			thePic = OpenPicture(&picRect);
			if (thePic) {
				const BitMap *palPortBits = GetPortBitMapForCopyBits(palPort);
				const BitMap *portBits = GetPortBitMapForCopyBits(port);		
				CopyBits(palPortBits,portBits,&picRect,&picRect,srcCopy,NULL);
				ClosePicture();
				DisposGrafPort(port);
				size = GetHandleSize((Handle)thePic);
				SetHandleSize((Handle)toolPicture,size);
				if (MemError() == noErr) {
					BlockMove(*thePic,*toolPicture,size);
					ChangedResource((Handle)toolPicture);
					
					UpdateGridCoords();
					
					UpdateResFile(HomeResFile((Handle)toolPicture));
					/* FlushVol(NULL,??); ?? */
					}
				}
			}
		
		SetPort(oldPort);
		UseResFile(curResFile);

		return(True);
	}

/*
 *	Write the PLCH resource back out, since we're saving the tool palette.
 */

static void UpdateGridCoords()
	{
		Handle hdl; register unsigned char *p;
		GridRec *pGrid; short row,col,curResFile;
		PaletteGlobals *pg = *paletteGlobals[TOOL_PALETTE];
				
		/* Get char map of palette from Nightingale Setup file */

		curResFile = CurResFile();
		UseResFile(setupFileRefNum);

		hdl = GetResource('PLCH', ToolPaletteID);
		if (!GoodNewHandle((Handle)hdl)) {
			UseResFile(curResFile);
			return;
		}

		LoadResource(hdl);

		/* Pull in the maximum and suggested sizes for palette */
		
		p = (unsigned char *) *hdl;
		
		*p++ = pg->maxAcross;
		*p++ = pg->maxDown;
		*p++ = pg->across;
		*p++ = pg->down;
		
		/* Set filler to 0 */
		
		*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
		*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
		*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
		
		pGrid = grid;
		for (row=0; row<pg->maxDown; row++)	
			for (col=0; col<pg->maxAcross; col++,pGrid++)
				*p++ = pGrid->ch;

		/* Write out the resource so we can restore the current resource file. */
		
		ChangedResource(hdl);
		UpdateResFile(HomeResFile(hdl));

		UseResFile(curResFile);
	}
