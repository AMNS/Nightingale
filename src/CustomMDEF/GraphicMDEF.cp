/* GraphicMDEF.c for Nightingale

The app's resource fork must contain a 'chgd' resource having the same ID as any 'MENU'
that uses this defproc. NB: If you want a special background color for a popup menu,
specify  it as a menu bar color rather than a menu title color. Because of DeleteMenu in
DoGPopUp, the second time the popup is invoked, it loses the title part of its 'mctb'.
It's not clear to me why this happens. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "GraphicMDEF.h"

#define USE_COLOR		1				/* compile (lengthy) code that handles color menus */

#define POP_FROM_EDGE	4
#define ARROWRECT_WID	14
#define SICN_ID			128
#define SICN_WID		16				/* number of horiz. and vert. bits in any sicn */

	/* Returns 1 if entire menu is enabled */
#define MenuEnabled(menuH) IsMenuItemEnabled(menuH, 0)
#define odd(a) ((a) & 1)							/* True if a is odd */

static void InitGlobals(MenuHandle);
static void DrawMenu(MenuHandle, Rect *, MenuTrackingData*, CGContextRef);
static void DrawItem(short, Rect *, MenuHandle, Boolean);
static void DrawRow(MenuHandle, Rect *, short);
static void DrawColumn(MenuHandle, Rect *, short);
static void InvertItem(short, Boolean, Rect *, MenuHandle);
static void FindMenuItem(MenuHandle, Rect *, Point, MenuTrackingData* trackingData, CGContextRef context );
static void	HiliteMenuItem(MenuRef menu, const Rect* bounds, HiliteMenuItemData* hiliteData, CGContextRef context );
static Boolean ItemIsVisible(short);
static Boolean InScrollRect(Point, Rect *);
static void ScrollMenu(MenuHandle, Rect *, short);
static void EraseMenu(Rect *);
static void DrawEraseScrollArrow(Rect *, short, Boolean);
static void SizeMenu(MenuHandle, Point);
static void GetItemRect(short, Rect *, Rect *);
static void PopUpMenu(MenuHandle, Rect *, short, short, short *);
#if USE_COLOR
void GetColors(MenuHandle, short);
void SetColors(RGBColor *, RGBColor *, Boolean);
void SaveCurColors(void);
void RestoreColors(void);
#endif
static void SetGrayPat(void);

static HCHARGRID	gCharGridH;				/* handle to 'chgd' resource */
static short		gItemCharsOffset;		/* offset from a PCHARGRID to start of its variable length array */
											/* of item chars. Offset is dependent on length of fontName. */
static short		gArrowRectWid;
static long			gScrollTicks;
static short		gScrollStatus;
static short		gTopVisRow, gBotVisRow;		/* Number of row containing top or bottom vis menu item. */
														/* i.e., not a scroll arrow slot. Initialized in DrawMenu; */
														/* changed only in ScrollMenu. */ 
static short		gLeftVisColumn, gRightVisColumn;	/* analogous to above */
static short		gNumItems;
static short		gNumCols, gNumRows;
static short		gPopFontNum;
static short		gItemHt, gItemWid;
static Boolean		gIsAPopUp;				/* If this MDEF is used for a popup AND a normal menu, you */
											/* must call CalcMenuSize for the normal menu EVERY time */
											/* the user clicks in menu bar. This generates a mSizeMsg */
											/* for the MDEF, giving it a chance to reset gIsAPopUp to False. */
#if USE_COLOR
static short		gPixelDepth;
static RGBColor		gSavedForeColor;
static RGBColor		gSavedBackColor;
static RGBColor 	gMenuTitleColor;
static RGBColor 	gMenuBgColor;
static RGBColor 	gItemNameColor;
static RGBColor 	gCmdKeyColor;
static RGBColor		gMyGrayColor = {32768, 32768, 32768};	
#endif


#define FST_SCROLL_PIXELS	ARROWRECT_WID/2
#define FST_SCROLL_TICKS	1L
#define SLO_SCROLL_TICKS	12L

enum {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

enum {
	NOSCROLL = 0,			/* no need to scroll */
	SCROLLUP = 2,			/* scroll up arrow present */
	SCROLLDOWN = 4,			/* scroll down arrow present */
	SCROLLLEFT = 8,			/* scroll left arrow present */
	SCROLLRIGHT = 16		/* scroll right arrow present */
};


#define kSysEnvironsVersion 1

#if USE_COLOR
static Boolean gHasColorQD=False;
#endif



/* For a normal menu, the MDEF main gets called with msgs in this order:
 *		1) mSizeMsg, when CalcMenuSize is called (e.g., when installing in menu bar;
 *				NB: CalcMenuSize normally not called again)
 *		2) mDrawMsg, after mouse is clicked in menu title
 *		3) mChooseMsg, while mouse is down
 *   In this case, DrawMenu should call InitGlobals.
 * For a popup menu, this is the order:
 *		1) mSizeMsg, when CalcMenuSize is called (e.g., in my DoGPopUp) after click
 *				in popup menu box in window
 *		2) mPopUpMsg, when PopUpMenuSelect called (e.g., by DoGPopUp)
 *		3) mDrawMsg, while mouse still down
 *		4) mChooseMsg, while mouse still down
 *   In this case, PopUpMenu should call InitGlobals.
 * In both cases, only SizeMenu and any functions it calls can NOT rely on
 * my globals having been initialized.
 *
 * Msg			theMenu		menuRect			hitPt			whichItem
 * ------------------------------------------------------------------------------------------
 * mDrawMsg		handle		entry: ptr to		not used		not used
 * 							return: no chg		
 * mChooseMsg	handle		entry: ptr to		mouse pt		entry: ptr to currently selected item
 * 							return: no chg						return: newly selected item
 * mSizeMsg		handle		not used			not used		not used
 * mPopUpMsg	handle		entry: none			top/left		entry: CrsrItem to be placed at HitPt
 * 							return: menu rect					return: TopMenuItem
 */

pascal void MyMDefProc(short message, MenuHandle theMenu, Rect *menuRect, Point hitPt,
						short *whichItem)
{
	switch (message) {	
		case kMenuInitMsg:
			*whichItem = noErr;
			gHasColorQD = thisMac.hasColorQD;
			break;

		case kMenuDisposeMsg:
			break;

		case kMenuDrawMsg: 
			DrawMenu(theMenu, menuRect, (MenuTrackingData*)whichItem, (CGContextRef)((MDEFDrawData*)whichItem)->context);
			break;
		
		case kMenuSizeMsg: 
			SizeMenu(theMenu, hitPt );
			break;
			
		case kMenuPopUpMsg:
			// NB: h and v are reversed in hitPt for this message! See Apple Tech Note 172.
			PopUpMenu(theMenu, menuRect, hitPt.h, hitPt.v, whichItem);
			break;
			
		case kMenuFindItemMsg:
			FindMenuItem(theMenu, menuRect, hitPt, (MenuTrackingData*)whichItem, (CGContextRef)((MDEFFindItemData*)whichItem)->context);
			break;
			
		case kMenuHiliteItemMsg:
			HiliteMenuItem(theMenu, menuRect, (HiliteMenuItemData*)whichItem, (CGContextRef)((MDEFHiliteItemData*)whichItem)->context);
			break;
			
		case kMenuCalcItemMsg:
			//CalcMenuItemBounds( menu, bounds, *whichItem );
			break;

		case kMenuThemeSavvyMsg:
			//*whichItem = kThemeSavvyMenuResponse;
			break;

		default:
			break;
	}
}


static void InitGlobals(MenuHandle theMenu)
{
	register short fontNameLen;
	short menuID = GetMenuID(theMenu);
	
	gCharGridH = (HCHARGRID)Get1Resource('chgd', menuID);
	if (gCharGridH==NULL) {
		SysBeep(10);										/* character grid rsrc missing */
		LogPrintf(LOG_WARNING, "Character grid resource for graphic menu is missing.  (InitGlobals)\n");
		return;
	}
	
	/* determine number of rows and columns */
	gNumItems = (*gCharGridH)->numChars;	FIX_END(gNumItems);
	gNumCols = (*gCharGridH)->numColumns;
	if (gNumItems < gNumCols) gNumItems = gNumCols;
	gNumRows = gNumItems / gNumCols;
	if (gNumItems % gNumCols) gNumRows++;

	gItemHt = GetMenuHeight(theMenu) / gNumRows;			/* menu ht, wid already set by SizeMenu */
	gItemWid = GetMenuWidth(theMenu) / gNumCols;
LogPrintf(LOG_DEBUG, "InitGlobals: gNumItems=%d gNumCols=%d gNumRows=%d gItemHt=%d gItemWid=%d\n",
gNumItems, gNumCols, gNumRows, gItemHt, gItemWid);

	HLock((Handle)gCharGridH);
	GetFNum((*gCharGridH)->fontName, &gPopFontNum);			/* this moves memory */
	if (gPopFontNum==0) {
		SysBeep(10);										/* font missing */
		LogPrintf(LOG_WARNING, "Font for graphic menu is missing.  (InitGlobals)\n");
		HUnlock((Handle)gCharGridH);
		return;
	}
	HUnlock((Handle)gCharGridH);
	
	fontNameLen = (*gCharGridH)->fontName[0] + 1;
	if (odd(fontNameLen)) fontNameLen++;

	gItemCharsOffset = (short)((char *)(*gCharGridH)->fontName - (char *)*gCharGridH) + fontNameLen;
	
	gArrowRectWid = ARROWRECT_WID;
}


/* Bits behind have been saved, menu structure has been drawn, and menu rectangle is
cleared to proper color (all by mbarproc). Since this is called only when mouse has just
entered menu title, and not while mouse moves among the menu items--which may scroll,
we can initialize globals here. */

static void DrawMenu(MenuHandle theMenu, Rect *menuRect, MenuTrackingData* trackingData,
						CGContextRef context )
{
#pragma unused(trackingData)
#pragma unused(context)

	register short	i;
	Rect			theRect;
	
	InitGlobals(theMenu);

#if USE_COLOR
	if (gHasColorQD) {
		GetColors(theMenu, 1);		/* just to init colors for 1st scroll arrow */
		SaveCurColors();			/* just to init gPixelDepth */
	}
#endif
	
	if (gScrollStatus == NOSCROLL) {
		/* draw menu chars into menuRect */
		for (i=1; i<=gNumItems; i++) {
			GetItemRect(i, menuRect, &theRect);
LogPrintf(LOG_DEBUG, "DrawMenu: NOSCROLL i=%d theRect tlbr=%d,%d,%d,%d\n", i, theRect.top,
theRect.left, theRect.bottom, theRect.right);
			DrawItem(i, &theRect, theMenu, False);
		}
	}
	else {
		if (gScrollStatus & SCROLLUP) {
			DrawEraseScrollArrow(menuRect, UP, True);
			for (i=gTopVisRow; i<=gNumRows; i++)
				DrawRow(theMenu, menuRect, i);
		}
		else if (gScrollStatus & SCROLLDOWN) {
			DrawEraseScrollArrow(menuRect, DOWN, True);
			for (i=gBotVisRow; i>=1; i--)
				DrawRow(theMenu, menuRect, i);
		}
		if (gScrollStatus & SCROLLLEFT) {
			DrawEraseScrollArrow(menuRect, LEFT, True);
			for (i=gLeftVisColumn; i<=gNumCols; i++)
				DrawColumn(theMenu, menuRect, i);
		}
		else if (gScrollStatus & SCROLLRIGHT) {
			DrawEraseScrollArrow(menuRect, RIGHT, True);
			for (i=gRightVisColumn; i>=1; i--)
				DrawColumn(theMenu, menuRect, i);
		}
	}
}


/* Draw the menu item with given number. Assumes that caller will save port's font and
size and set the port's font and size to the popup font before calling DrawItem. Afterwards,
it will restore the original font and size. */
 
static void DrawItem(short item, Rect *itemRect, MenuHandle theMenu, Boolean leaveBlack)
{
	register unsigned char theChar, *p;
	register GrafPtr	gp;
	register short		saveFontNum, saveFontSize;
	register Boolean	drawInColor = False;
	
	if (item < 1) return;
	
#if USE_COLOR
	if (gHasColorQD) {
		GetColors(theMenu, item);
		SaveCurColors();
		/* Only the whole menu can be disabled, since we don't use the menu's enableFlags */
		if (MenuEnabled(theMenu))
			SetColors(&gItemNameColor, &gMenuBgColor, leaveBlack);
		else
			SetColors(&gMyGrayColor, &gMenuBgColor, False);
			
		if (gPixelDepth > 1) drawInColor = True;
	}
#endif

	/* Prepare for drawing font chars. */
	GetPort(&gp);										/* Window Manager port */
	saveFontNum = GetPortTextFont(gp);
	saveFontSize = GetPortTextSize(gp);
	
	TextFont(gPopFontNum);
	LoadResource((Handle)gCharGridH);					/* just in case it's been purged */
	FIX_END((*gCharGridH)->numChars);
	FIX_END((*gCharGridH)->fontSize); 
	TextSize((*gCharGridH)->fontSize);					/* TextSize isn't supposed to move memory */

	p = (unsigned char *)*gCharGridH;					/* no need to lock handle here */
	p += gItemCharsOffset;								/* point at beginning of item chars */
	theChar = p[item-1];
LogPrintf(LOG_DEBUG, "DrawItem: item=%d theChar=%c itemRect->left=%d, ->bottom=%d\n", item,
			theChar, itemRect->left, itemRect->bottom);
	MoveTo(itemRect->left, itemRect->bottom);
	DrawChar(theChar);

#if USE_COLOR
	if (gHasColorQD) RestoreColors();
#endif

#if USE_COLOR
	if (!MenuEnabled(theMenu) && !drawInColor) {
#else
	if (!MenuEnabled(theMenu)) {
#endif
		SetGrayPat();
		PenMode(patBic);
		PaintRect(itemRect);
		PenNormal();
	}
	
	TextFont(saveFontNum);
	TextSize(saveFontSize);
}


/* DrawRow and DrawColumn should be called only from ScrollMenu. */

static void DrawRow(MenuHandle theMenu, Rect *menuR, short rowNum)
{
	register short firstItem, lastItem;
	register Rect itemR;
	register short i;
	
	firstItem = ((rowNum - 1) * gNumCols) + 1;
	lastItem = firstItem + gNumCols - 1;

	for (i = firstItem; i <= lastItem; i++) {
		GetItemRect(i, menuR, &itemR);
		DrawItem(i, &itemR, theMenu, False);
	}
}


static void DrawColumn(MenuHandle theMenu, Rect *menuR, short columnNum)
{
	register Rect itemR;
	register short i;

	for (i = columnNum; i <= gNumItems; i += gNumCols) {
		GetItemRect(i, menuR, &itemR);
		DrawItem(i, &itemR, theMenu, False);
	}
}


static void InvertItem(short item, Boolean leaveBlack, Rect *menuRect, MenuHandle theMenu)
{
	Rect				itemRect;
	register RgnHandle	saveClip;

#if USE_COLOR
	if (gHasColorQD) {
		SaveCurColors();
		GetColors(theMenu, item);						/* need to set item colors for EraseRect */
		SetColors(&gItemNameColor, &gMenuBgColor, leaveBlack);
	}
#endif

/* All this clipping stuff is probably unnecessary, but it doesn't hurt. */
	saveClip = NewRgn();
	GetClip(saveClip);
	GetItemRect(item, menuRect, &itemRect);
	EraseRect(&itemRect);	
#if USE_COLOR
	if (gHasColorQD) RestoreColors();
#endif

	ClipRect(&itemRect);
	DrawItem(item, &itemRect, theMenu, leaveBlack);	

#if USE_COLOR
	if (!gHasColorQD && leaveBlack)
#else
	if (leaveBlack)
#endif
		InvertRect(&itemRect);

	SetClip(saveClip);
	DisposeRgn(saveClip);
}

static void FindMenuItem(MenuHandle theMenu, Rect *menuRect, Point hitPt, MenuTrackingData *trackingData, CGContextRef /*context*/ )
{
	register short	newItem, vFromTop, hFromLeft, direction, row, col;
	Boolean			hitPtInMenuRect = False, doScroll = False;
	short			currentItem = trackingData->itemSelected;
	
	if (!MenuEnabled(theMenu)) {
		trackingData->itemSelected = 0;
		return;
	}

	hitPtInMenuRect = PtInRect(hitPt, menuRect);
	
	/* Scroll even when mouse is not in menu rect in certain circumstances, in
	   conformity to the way normal text menus work. */
	   
	if (!hitPtInMenuRect && gScrollStatus != NOSCROLL) {
		if (gScrollStatus & SCROLLUP || gScrollStatus & SCROLLDOWN) {
			if (hitPt.h > menuRect->left && hitPt.h < menuRect->right)
				doScroll = True;
		}
		if (gScrollStatus & SCROLLLEFT || gScrollStatus & SCROLLRIGHT) {
			if (hitPt.v > menuRect->top && hitPt.v < menuRect->bottom)
				doScroll = True;
		}
	}		

	if (hitPtInMenuRect || doScroll) {
		vFromTop = hitPt.v - menuRect->top;
		hFromLeft = hitPt.h - menuRect->left;
		
		/* adjust for any scroll arrows */
		if (gScrollStatus & SCROLLUP)
			vFromTop -= gArrowRectWid;
		else if (gScrollStatus & SCROLLDOWN)
			vFromTop += gArrowRectWid;
		if (gScrollStatus & SCROLLLEFT)
			hFromLeft -= gArrowRectWid;
		else if (gScrollStatus & SCROLLRIGHT)
			hFromLeft += gArrowRectWid;
		
		/* which item is the mouse over? */
		row = (vFromTop / gItemHt) + 1;
		col = (hFromLeft / gItemWid) + 1;
		newItem = ((row - 1) * gNumCols) + col;
		
		if (gScrollStatus != NOSCROLL) {
			if (InScrollRect(hitPt, menuRect)) {			/* scroll */
				
				/* set scrolling speed and direction */
				if (gScrollStatus & SCROLLUP) {
					gScrollTicks = (hitPt.v < menuRect->top + FST_SCROLL_PIXELS) ?
														FST_SCROLL_TICKS : SLO_SCROLL_TICKS;
					direction = UP;
				}
				else if (gScrollStatus & SCROLLDOWN) {
					gScrollTicks = (hitPt.v > menuRect->bottom - FST_SCROLL_PIXELS) ?
														FST_SCROLL_TICKS : SLO_SCROLL_TICKS;
					direction = DOWN;
				}
				if (gScrollStatus & SCROLLLEFT) {
					gScrollTicks = (hitPt.h < menuRect->left + FST_SCROLL_PIXELS) ?
														FST_SCROLL_TICKS : SLO_SCROLL_TICKS;
					direction = LEFT;
				}
				else if (gScrollStatus & SCROLLRIGHT) {
					gScrollTicks = (hitPt.h > menuRect->right - FST_SCROLL_PIXELS) ?
														FST_SCROLL_TICKS : SLO_SCROLL_TICKS;
					direction = RIGHT;
				}
				
				ScrollMenu(theMenu, menuRect, direction);
				newItem = 0;									/* no hiliting while scrolling */
			}
			else {												/* make adjustments to newItem */
				if (gScrollStatus & SCROLLUP)
					newItem += (gTopVisRow - 1) * gNumCols;
				else if (gScrollStatus & SCROLLDOWN)
					newItem -= (gNumRows - gBotVisRow) * gNumCols;
				if (gScrollStatus & SCROLLLEFT)
					newItem += gLeftVisColumn - 1;
				else if (gScrollStatus & SCROLLRIGHT)
					newItem -= gNumCols - gRightVisColumn;
				if (!ItemIsVisible(newItem) || !hitPtInMenuRect)
					newItem = 0;								/* can only happen to left-right scrolled items */
			}
		}
      trackingData->itemUnderMouse = newItem;
      trackingData->itemRect = *menuRect;
      //if ( IsMenuItemEnabled( theMenu, newItem ) )
          trackingData->itemSelected = newItem;
		
	}
	else {
		trackingData->itemSelected = currentItem;
	}
}

static void HiliteMenuItem( MenuRef theMenu, const Rect *menuRect, HiliteMenuItemData *hiliteData,
								CGContextRef /*context*/ )
{
	short previousItem = hiliteData->previousItem;
	short newItem = hiliteData->newItem;
	
	if (previousItem != newItem) {
		char *p;
		LoadResource((Handle)gCharGridH);					/* just in case it's been purged */
		FIX_END((*gCharGridH)->numChars);
		FIX_END((*gCharGridH)->fontSize);
		p = (char *)*gCharGridH;							/* no need to lock handle here */
		p += gItemCharsOffset;								/* point at beginning of item chars */
		if (newItem <= gNumItems && p[newItem-1] != '\0')
			InvertItem(newItem, True, (Rect *)menuRect, theMenu);
		else
			newItem = 0;										/* it's a blank item */
		InvertItem(previousItem, False, (Rect *)menuRect, theMenu);
		//LogPrintf(LOG_NOTICE, "currItem=%d, newItem=%d\n", previousItem, newItem);
	}
}

static Boolean ItemIsVisible(short item)
{
	register short	row, col;

	row = ((item - 1) / gNumCols) + 1;
	col = item - (gNumCols * (row - 1));
	
	if (row < gTopVisRow || row > gBotVisRow || col < gLeftVisColumn || col > gRightVisColumn)
		return False;
	else
		return True;
}


static Boolean InScrollRect(Point pt, Rect *menuRect)
{
	if (gScrollStatus & SCROLLUP)
		return (pt.v < menuRect->top + gArrowRectWid);
	if (gScrollStatus & SCROLLDOWN)
		return (pt.v > menuRect->bottom - gArrowRectWid);
	if (gScrollStatus & SCROLLLEFT)
		return (pt.h < menuRect->left + gArrowRectWid);
	if (gScrollStatus & SCROLLRIGHT)
		return (pt.h > menuRect->right - gArrowRectWid);
		
	return False;		/* in case we somehow got called while gScrollStatus==NOSCROLL */
}


static void ScrollMenu(MenuHandle theMenu, Rect *menuRect, short direction)
{
	register RgnHandle	updtRgn;
	register short		dh, dv, i;
	register long		startTicks;
	static long			endTicks = 0L;
	
	startTicks = TickCount();
	if (startTicks - endTicks < SLO_SCROLL_TICKS << 1) {
		while (TickCount() < startTicks + gScrollTicks) ;
	}
	endTicks = TickCount();

	dh = dv = 0;

	if (direction == UP) {
		dv = gItemHt;
		gTopVisRow--;
		if (gTopVisRow == 1)								/* can't scroll up any more */
			gScrollStatus -= SCROLLUP;
	}
	else if (direction == DOWN) {
		dv = -gItemHt;
		gBotVisRow++;
		if (gBotVisRow == gNumRows)							/* can't scroll down any more */
			gScrollStatus -= SCROLLDOWN;
	}		
	if (direction == LEFT) {
		dh = gItemWid;
		gLeftVisColumn--;
		if (gLeftVisColumn == 1)							/* can't scroll up any more */
			gScrollStatus -= SCROLLLEFT;
	}
	else if (direction == RIGHT) {
		dh = -gItemWid;
		gRightVisColumn++;
		if (gRightVisColumn == gNumCols)					/* can't scroll down any more */
			gScrollStatus -= SCROLLRIGHT;
	}

	DrawEraseScrollArrow(menuRect, direction, False);		/* erase before ScrollRect */

#if USE_COLOR
	if (gHasColorQD) {
		SaveCurColors();
		SetColors(&gItemNameColor, &gMenuBgColor, False);
	}
#endif

	updtRgn = NewRgn();
	ScrollRect(menuRect, dh, dv, updtRgn);
	DisposeRgn(updtRgn);

#if USE_COLOR
	if (gHasColorQD) RestoreColors();
#endif

	if (direction == UP) {
		if (gScrollStatus & SCROLLUP) {
			DrawRow(theMenu, menuRect, gTopVisRow);
			DrawEraseScrollArrow(menuRect, UP, True);
		}
		else {
			EraseMenu(menuRect);
			for (i=1; i<=gNumRows; i++)
				DrawRow(theMenu, menuRect, i);
		}
	}
	else if (direction == DOWN) {
		if (gScrollStatus & SCROLLDOWN) {
			DrawRow(theMenu, menuRect, gBotVisRow);
			DrawEraseScrollArrow(menuRect, DOWN, True);
		}
		else {
			EraseMenu(menuRect);
			for (i=gNumRows; i>=1; i--)
				DrawRow(theMenu, menuRect, i);
		}
	}

	if (direction == LEFT) {
		if (gScrollStatus & SCROLLLEFT) {
			DrawColumn(theMenu, menuRect, gLeftVisColumn);
			DrawEraseScrollArrow(menuRect, LEFT, True);
		}
		else {
			EraseMenu(menuRect);
			for (i=1; i<=gNumCols; i++)
				DrawColumn(theMenu, menuRect, i);
		}
	}
	else if (direction == RIGHT) {
		if (gScrollStatus & SCROLLRIGHT) {
			DrawColumn(theMenu, menuRect, gRightVisColumn);
			DrawEraseScrollArrow(menuRect, RIGHT, True);
		}
		else {
			EraseMenu(menuRect);
			for (i=gNumCols; i>=1; i--)
				DrawColumn(theMenu, menuRect, i);
		}
	}

	if (gScrollStatus == NOSCROLL) gArrowRectWid = 0;
}


/* Assumes colors have been set up correctly and will be restored afterwards. */

static void DrawEraseScrollArrow(Rect *menuRect, short arrow, Boolean draw)
{
	register short	dh, dv, sicnIndex;
	Rect			arrowRect, sicnRect;
	register Handle	SICNHdl;
	BitMap			bm;
	GrafPtr			gp;
	
	arrowRect = *menuRect;
	SetRect(&sicnRect, menuRect->left, menuRect->top,
					menuRect->left + SICN_WID, menuRect->top + SICN_WID);
	
	dh = dv = 0;	
	switch (arrow) {
		case UP:
			arrowRect.bottom = arrowRect.top + gArrowRectWid;
			dh = ((arrowRect.right - arrowRect.left) >> 1) - (SICN_WID >> 1);
			sicnIndex = 1;
			break;
		case DOWN:
			arrowRect.top = arrowRect.bottom - gArrowRectWid;
			dh = ((arrowRect.right - arrowRect.left) >> 1) - (SICN_WID >> 1);
			dv = (menuRect->bottom - menuRect->top) - SICN_WID;
			sicnIndex = 2;
			break;
		case LEFT:
			arrowRect.right = arrowRect.left + gArrowRectWid;
			dv = ((menuRect->bottom - menuRect->top) >> 1) - (SICN_WID >> 1);
			sicnIndex = 3;
			break;
		case RIGHT:
			arrowRect.left = arrowRect.right - gArrowRectWid;
			dh = (menuRect->right - menuRect->left) - SICN_WID;
			dv = ((menuRect->bottom - menuRect->top) >> 1) - (SICN_WID >> 1);
			sicnIndex = 4;
			break;
	}
	OffsetRect(&sicnRect, dh, dv);

	if (draw) {
		/* get the small icons for arrows */
		SICNHdl = Get1Resource('SICN', SICN_ID);
		if (SICNHdl == NULL) return;
	
		/* make it a bitmap */
		HNoPurge(SICNHdl);
		SetRect(&bm.bounds, 0, 0, 16, 16);				/* the size of a small icon */
		bm.rowBytes = 2;
	}
	
#if USE_COLOR
	if (gHasColorQD) {
		SaveCurColors();
		SetColors(&gItemNameColor, &gMenuBgColor, False);
	}
#endif

	EraseRect(&arrowRect);

	if (draw) {
		GetPort(&gp);										/* draw into window mgr port */
		HLock((Handle)SICNHdl);
		bm.baseAddr = (Ptr) (*SICNHdl + (32 * (sicnIndex-1)));
		const BitMap *gpPortBits = GetPortBitMapForCopyBits(gp);
		CopyBits(&bm, gpPortBits, &bm.bounds, &sicnRect, srcOr, (RgnHandle)NULL);
		HUnlock((Handle)SICNHdl);
		HPurge(SICNHdl);
	}
	
#if USE_COLOR
	if (gHasColorQD)
		RestoreColors();
#endif
}


static void EraseMenu(Rect *menuRect)
{
#if USE_COLOR
	if (gHasColorQD) {
		SaveCurColors();
		SetColors(&gItemNameColor, &gMenuBgColor, False);
	}
#endif
	
	EraseRect(menuRect);
	
#if USE_COLOR
	if (gHasColorQD) RestoreColors();
#endif
}


static void SizeMenu(MenuHandle theMenu, Point hitPt)
{
#pragma unused(hitPt)

	register short		charWid, charHt, numItems, fontSize;
	short				popFontNum, saveFontSize, saveFontNum;
	short				numColumns, numRows;
	register Handle		resH;
	register PCHARGRID	chgdP;
	FontInfo			finfo;
	GrafPtr				gp;
		
	/* We can set these globals here. popupmsg follows. However, make sure to call
	   CalcMenuSize before each click in the menu bar for any menus that use this
	   MDEF. (The user might have clicked a popup that uses this MDEF after the
	   initial call to SizeMenu, when the menu bar is drawn.) */
	
	gIsAPopUp = False;
	gScrollStatus = NOSCROLL;
	gArrowRectWid = 0;
	
	/* There's no point loading the 'chgd' resource for good now, because SizeMenu may
	   be called when the Menu Mgr sets up the menu bar, possibly long before any of
	   our menus are invoked. */
	
	resH = Get1Resource('chgd', GetMenuID(theMenu));
	if (resH==NULL) {
		SysBeep(10);									/* character grid rsrc missing */
		return;
	}
	chgdP = (PCHARGRID)*resH;
	FIX_END(chgdP->numChars); 
	FIX_END(chgdP->fontSize); 
	numItems = chgdP->numChars;							/* Can't put these in a global yet, because */
	fontSize = chgdP->fontSize;							/* another menu may use this code before this */
	numColumns = chgdP->numColumns;						/* one receives its drawMsg. */

	/* get maxWidth of popup char font */
	HLock((Handle)resH);
	GetFNum(chgdP->fontName, &popFontNum);				/* moves memory */
	if (popFontNum==0) {
		SysBeep(10);									/* font missing */
		HUnlock((Handle)resH);
		return;
	}
	HUnlock((Handle)resH);
	ReleaseResource(resH);

	/* determine number of rows and columns */
	if (numItems < numColumns) numItems = numColumns;
	numRows = numItems / numColumns;
	if (numItems % numColumns) numRows++;

	GetPort(&gp);										/* Window Manager port */
	//saveFontNum = gp->txFont;
	//saveFontSize = gp->txSize;
	saveFontNum = GetPortTextFont(gp);
	saveFontSize = GetPortTextSize(gp);
	TextFont(popFontNum);
	TextSize(fontSize);
	GetFontInfo(&finfo);
	charWid = finfo.widMax;
	charHt = finfo.ascent;
	TextFont(saveFontNum);
	TextSize(saveFontSize);
	
	/* calculate menu dimensions */
	SetMenuHeight(theMenu, charHt * numRows);
	SetMenuWidth(theMenu, charWid * numColumns);
LogPrintf(LOG_DEBUG, "SizeMenu: charHt=%d charWid=%d numRows=%d numColumns=%d\n", charHt, charWid,
			numRows, numColumns); 
}


static void GetItemRect(short item, Rect *menuRect, Rect *itemRect)
{
	register short	row, col, dh, dv;

	dh = dv = 0;	

	if (item > 0) {
		row = ((item - 1) / gNumCols) + 1;
		col = item - (gNumCols * (row - 1));
		
		*itemRect = *menuRect;
		itemRect->top += (row - 1) * gItemHt;
		itemRect->bottom = itemRect->top + gItemHt;
		itemRect->left += (col - 1) * gItemWid;
		itemRect->right = itemRect->left + gItemWid;
	}
	else
		SetRect(itemRect, 0, 0, 0, 0);
	
	/* adjust rect if scrolling in effect */
	if (gScrollStatus != NOSCROLL) {
		if (gScrollStatus & SCROLLUP) {
			dh = 0;
			dv = -((gItemHt * (gTopVisRow - 1)) - gArrowRectWid);
		}
		else if (gScrollStatus & SCROLLDOWN) {
			dh = 0;
			dv = (gItemHt * (gNumRows - gBotVisRow)) - gArrowRectWid;
		}
		if (gScrollStatus & SCROLLLEFT) {
			dv = 0;
			dh = -((gItemWid * (gLeftVisColumn - 1)) - gArrowRectWid);
		}
		else if (gScrollStatus & SCROLLRIGHT) {
			dv = 0;
			dh = (gItemWid * (gNumCols - gRightVisColumn)) - gArrowRectWid;
		}
		OffsetRect(itemRect, dh, dv);
	}
}


/* Show the menu under the cursor. This should be called after SizeMenu, but before
 * DrawMenu and ChooseItem.
 * See (ancient) Apple Tech Note 172, "Parameters for MDEF Message #3".
 * From Apple text MDEF:
 * 
 * Calc TopMenuItem and MenuRect so that the top of PopUpItem is at TopLeft
 * ----------------------------------------------------------------------------------------
 *  Begin by calculating how far the top of WhichItem is from the top of the menu.
 *  Then place the menu.  The various possibilities are:
 * 	1. The whole menu fits on the screen and PopUpItem lines up with TopLeft
 * 	2. The whole menu fits on the screen and PopUpItem cannot line up with TopLeft without
 * 		causing the menu to scroll.  In this case adjust menuRect so that complete menu is
 * 		is on the screen, but PopUpItem is as close to TopLeft as possible.
 * 	3. The whole menu is not on the screen.  In this case adjust the menuRect so that as much
 * 		of the menu is showing as possible on the screen, and position PopUpItem so that
 * 		it is showing and as close as possible to TopLeft.
 * 
 *  Return the MenuRect and TopMenuItem.  TopMenuItem is returned in the VAR PopUpItem param.
 * 
 *  Historical Note: It would be desireable to have popups change vertical size as they scrolled,
 * 	instead of having all that white space when the scrolling menu is first displayed.
 *	The reason for this is due to the design of the MBDF. The MBDF saves the bits behind
 *	and draws the drop shadow at the same time. If there were two messages instead, one
 *	to save the bits behind and one to draw the drop shadow, the we could save all of the
 *	bits behind the menurect, from the top of the screen to the bottom, and then change
 * 	the menu's vertical height without worrying about saving more bits each time it got bigger.
 */

static void PopUpMenu(MenuHandle theMenu, Rect *menuRect, short v, short h, short *item)
{
	register short	row, col, itemsToScroll;
	Rect			scr;
	
	InitGlobals(theMenu);										/* This hasn't been called yet! */

	/* Get main screen dimensions. FIXME: Is this guaranteed to be the screen with the popup?
		I doubt it. Should I get screen rect from GetGrayRgn? */
		
	//scr = screenBits.bounds; ??
	scr = GetQDScreenBitsBounds();

	scr.top += GetMBarHeight();
	InsetRect(&scr, POP_FROM_EDGE + gArrowRectWid,
						 POP_FROM_EDGE + gArrowRectWid);		/* reduce screen area by margin constant */

	if (*item < 1) *item = 1;
	row = ((*item - 1) / gNumCols) + 1;
	col = *item - (gNumCols * (row - 1));

	/* Figure out menu rect, ignoring whether it fits on the screen */
	menuRect->top = v - ((row - 1) * gItemHt);
	menuRect->left = h - ((col - 1) * gItemWid);
	menuRect->bottom = menuRect->top + GetMenuHeight(theMenu);		/* menu ht,wid already set by SizeMenu */
	menuRect->right = menuRect->left + GetMenuWidth(theMenu);

	/* If menu rect is too big for screen, just return. It's up to the programmer
	 * to make sure that the app's popup menus will fit on the smallest available
	 * screen. Someday I'll support double scroll arrows
	 */
	if ((GetMenuWidth(theMenu) > (scr.right-scr.left)) || (GetMenuHeight(theMenu) > (scr.bottom-scr.top))) {
		SysBeep(10);
		goto broken;
	}

	if (ContainedRect(menuRect, &scr))
		goto broken;												/* menu rect needs no adjustment */

	/* Popup is small enough to fit on screen, but part of it is offscreen now.
	 * Move the menu rect onscreen and set things up for menu scrolling.
	 */
	gTopVisRow = gLeftVisColumn = 1;
	gBotVisRow = gNumRows;
	gRightVisColumn = gNumCols;
	gScrollStatus = NOSCROLL;
	
	if (menuRect->top < scr.top) {
		itemsToScroll = ((scr.top - menuRect->top) / gItemHt) + 1;
		menuRect->top += (gItemHt * itemsToScroll) - gArrowRectWid;
		menuRect->bottom += (gItemHt * itemsToScroll) - gArrowRectWid;
		gScrollStatus += SCROLLUP;
		gTopVisRow = itemsToScroll + 1;
	}
	else if (menuRect->bottom > scr.bottom) {
		itemsToScroll = ((menuRect->bottom - scr.bottom) / gItemHt) + 1;
		menuRect->top -= (gItemHt * itemsToScroll) - gArrowRectWid;
		menuRect->bottom -= (gItemHt * itemsToScroll) - gArrowRectWid;
		gScrollStatus += SCROLLDOWN;
		gBotVisRow = gNumRows - itemsToScroll;
	}
	if (menuRect->left < scr.left) {
		itemsToScroll = ((scr.left - menuRect->left) / gItemWid) + 1;
		menuRect->left += (gItemWid * itemsToScroll) - gArrowRectWid;
		menuRect->right += (gItemWid * itemsToScroll) - gArrowRectWid;
		gScrollStatus += SCROLLLEFT;
		gLeftVisColumn = itemsToScroll + 1;
	}
	else if (menuRect->right > scr.right) {
		itemsToScroll = ((menuRect->right - scr.right) / gItemWid) + 1;
		menuRect->left -= (gItemWid * itemsToScroll) - gArrowRectWid;
		menuRect->right -= (gItemWid * itemsToScroll) - gArrowRectWid;
		gScrollStatus += SCROLLRIGHT;
		gRightVisColumn = gNumCols - itemsToScroll;
	}
	
	/* Can't handle scrolling in both dimensions at same time yet, so just
	 * insure that the menu is onscreen. The item that comes up under the
	 * mouse will not be the current one, though. Maybe moving the pointer
	 * is an acceptible solution in this case.
	 */
	if ((gScrollStatus & SCROLLLEFT || gScrollStatus & SCROLLRIGHT) &&
		 (gScrollStatus & SCROLLUP || gScrollStatus & SCROLLDOWN)) {
		gScrollStatus = NOSCROLL;
		/* move pointer to middle of current item */
	}
	
	
/*	*item = ; FIXME: ••••• What goes here? */

broken:
	gIsAPopUp = True;
}


#if USE_COLOR

/*
 * Here's what the 'mctb' resource specifies.
 * MenuBar:
 * 	mctRGB1	default title color
 * 	mctRGB2	default background
 * 	mctRGB3	default item color
 * 	mctRGB4	menu bar color
 * Menu Title:
 * 	mctRGB1	title color
 * 	mctRGB2	?
 * 	mctRGB3	default item color
 * 	mctRGB4	background color
 * Item:
 * 	mctRGB1	item mark color
 * 	mctRGB2	item name color
 * 	mctRGB3	cmd key color
 * 	mctRGB4	?
 */

void GetColors(MenuHandle theMenu, short menuItem)
{
	register MCEntryPtr	mbarPtr;
	register MCEntryPtr	titlePtr;
	register MCEntryPtr	itemPtr;
	RGBColor			WhiteRGB;
	RGBColor			BlackRGB;

	WhiteRGB.red = 0xFFFF;
	WhiteRGB.green = 0xFFFF;
	WhiteRGB.blue = 0xFFFF;
	BlackRGB.red = 0;
	BlackRGB.green = 0;
	BlackRGB.blue = 0;
	
	if (gPixelDepth == 1) {
		gMenuTitleColor = BlackRGB;
		gMenuBgColor = WhiteRGB;
		gItemNameColor = BlackRGB;
		gCmdKeyColor = BlackRGB;
		return;
	}

	mbarPtr = GetMCEntry(0, 0);
	titlePtr = GetMCEntry(GetMenuID(theMenu), 0);
	itemPtr = GetMCEntry(GetMenuID(theMenu), menuItem);
	//titlePtr = GetMCEntry((**theMenu).menuID, 0);
	//itemPtr = GetMCEntry((**theMenu).menuID, menuItem);
	
	/* Get defaults from mbar or default to B&W */
	if (mbarPtr) {
		gMenuTitleColor = mbarPtr->mctRGB1;
		gMenuBgColor = mbarPtr->mctRGB2;
		gItemNameColor = mbarPtr->mctRGB3;
		gCmdKeyColor = mbarPtr->mctRGB3;
	}
	else { 
		gMenuTitleColor = BlackRGB;
		gMenuBgColor = WhiteRGB;
		gItemNameColor = BlackRGB;
		gCmdKeyColor = BlackRGB;
	}
	
	/* Now override mbar defaults if titlePtr != NULL */
	if (titlePtr) {
		gMenuTitleColor = titlePtr->mctRGB1;
		gMenuBgColor = titlePtr->mctRGB4;
		gItemNameColor = titlePtr->mctRGB3;
		gCmdKeyColor = titlePtr->mctRGB3;
	}
	
	/* Override defaults if itemPtr != NULL */
	if (itemPtr) {
		gItemNameColor = itemPtr->mctRGB2;
		gCmdKeyColor = itemPtr->mctRGB3;
	}
}


void SetColors(RGBColor *foreColor, RGBColor *bgColor, Boolean inverse)
{
	if (inverse) {
		RGBForeColor(bgColor);
		RGBBackColor(foreColor);
	}
	else {
		RGBForeColor(foreColor);
		RGBBackColor(bgColor);
	}
}


/* Save the current fore/back colors and the device's pixel depth, if running on
 * a color machine.
 */
void SaveCurColors()
{
	register GDHandle	gdH;
	GrafPtr				gp;
	Rect				globalRect;
	
	if (!gHasColorQD) return;
	
	GetForeColor(&gSavedForeColor);
	GetBackColor(&gSavedBackColor);
	GetPort(&gp);
	//globalRect = gp->portRect;
	GetPortBounds(gp, &globalRect);
	gdH = GetMaxDevice(&globalRect);					/* Don't dispose this handle! It's in the sysheap */
	gPixelDepth = (**(**gdH).gdPMap).pixelSize;
}


/* Restore the current fore/back colors on both b&w and color machines. */
void RestoreColors(void)
{
	if (gHasColorQD) {
		RGBForeColor(&gSavedForeColor);
		RGBBackColor(&gSavedBackColor);
	}
	else {
		ForeColor(blackColor);
		BackColor(whiteColor);
	}
}

#endif		/* #if USE_COLOR */


/* Sets current port's pen pat to grey. Caller should call PenNormal later. */

#define SYS_GRAY	16

static void SetGrayPat()
{
	register PatHandle grayPat;
	
	/* get the grey pattern from the System file */
	grayPat = (PatHandle) GetResource('PAT ', SYS_GRAY);
	PenPat(*grayPat);
	ReleaseResource((Handle)grayPat);
}
