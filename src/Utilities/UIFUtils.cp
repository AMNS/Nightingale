/******************************************************************************************
*	FILE:	UIFUtils.c
*	PROJ:	Nightingale
*	DESC:	General-purpose utility routines for implementing the user interface.
		WaitCursor				ArrowCursor				FixCursor
		XableItem				UpdateMenu				UpdateMenuBar
		CenterRect				PullInsideRect			ContainedRect
		ZoomRect				
		GetGlobalPort			GetIndWinPosition
		AdjustWinPosition		GetMyScreen	PlaceAlert	PlaceWindow
		CenterWindow			EraseAndInval
		KeyIsDown				CmdKeyDown				OptionKeyDown
		ShiftKeyDown			CapsLockKeyDown			ControlKeyDown
		CommandKeyDown			CheckAbort				IsDoubleClick
		GetStaffLim				InvertSymbolHilite		InvertTwoSymbolHilite
		HiliteAttPoints			HiliteRect				FrameShadowRect
		FlashRect				SamePoint
		Advise					NoteAdvise				CautionAdvise
		StopAdvise				Inform					NoteInform
		CautionInform			StopInform				ProgressMsg
		UserInterrupt			UserInterruptAndSel
		NameHeapType			NameObjType				NameGraphicType
		DynamicToString			ClefToString
		SmartenQuote			DrawBox					DrawGrowBox
		DrawTheSelection		
		Voice2UserStr			Staff2UserStr
		VLogPrintf				LogPrintf				InitLogPrintf	
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* --------------------------------------------------------------------------- Cursors -- */

static void FixCursorLogPrint(char *str);
static void FixCursorLogPrint(char * /* str */) 
{
#ifdef DEBUG_CURSOR
	LogPrintf(LOG_DEBUG, str);
#endif
}

/* Display the watch cursor to indicate lengthy operations. */

void WaitCursor()
{
	static CursHandle watchHandle;
	static Boolean once = True;
	
	if (once) {
		watchHandle = GetCursor(watchCursor);		/* From system resources */
		once = False;
	}
	if (watchHandle && *watchHandle) SetCursor(*watchHandle);
}

void ArrowCursor()
{
	Cursor arrow;
	
	GetQDGlobalsArrow(&arrow);

	SetCursor(&arrow);
}


/*	Set cursor shape based on window and mouse position. This version allows "shaking
off the mode".	*/

#define MAXSHAKES	4		/* Changes in X direction before a good shake */
#define CHECKMIN	2		/* Number ticks before next dx check */
#define SWAPMIN		30		/* Number ticks between consecutive shakeoffs */

void FixCursor()
{
	Point			mousept, globalpt;
	WindowPtr		wp;
	CursHandle		newCursor;
	Document		*doc;
	static short	x, xOld, dx, dxOld, shaker, shookey;
	static long 	now, soon, nextcheck;
	PaletteGlobals *toolPalette;
	char			message[256];
	Boolean			foundPalette = False;
	
	toolPalette = *paletteGlobals[TOOL_PALETTE];
	
	/* <currentCursor> is a global that Nightingale uses elsewhere to keep track of
	   the cursor. <newCursor> is a temporary variable used solely to set the cursor
	   here. They'll always be the same except when a modifier key is down that forces
	   a specific cursor temporarily: at the moment, only the <genlDragCursor>. */
	   
	GetMouse(&mousept);
	globalpt = mousept;
	LocalToGlobal(&globalpt);
	
	/* If no windows at all, use arrow */
	
	if (TopWindow == NULL) {
		holdCursor = False;
		ArrowCursor();
		return;
	}
	
	/* If mouse is over any palette, use arrow unless a tool was just chosen */

	if (GetWindowKind(TopWindow) == PALETTEKIND) {
		holdCursor = True;
		for (wp=TopPalette; wp!=NULL && GetWindowKind(wp)==PALETTEKIND; wp=GetNextWindow(wp)) {
			RgnHandle strucRgn = NewRgn();
			GetWindowRegion(wp, kWindowStructureRgn, strucRgn);
			if (PtInRgn(globalpt, strucRgn)) {
				//if (!holdCursor) ArrowCursor();
				DisposeRgn(strucRgn);
				foundPalette = True;
			}
			else {
				DisposeRgn(strucRgn);
			}
		}
	}

	holdCursor = False;			/* OK to change cursor back to arrow when it's over a palette */
	
	/* If no Documents, use arrow */
	
	if (TopDocument==NULL) { FixCursorLogPrint("2. TopDocument is null\n");  ArrowCursor();  return; }

	/* If mouse is over any kind of window other than a Document, use arrow */
	
	FindWindow(globalpt, (WindowPtr *)&wp);
	
	if (wp==NULL) { FixCursorLogPrint("3-1. FoundWindow is null\n");  ArrowCursor();  return; }
	if (GetWindowKind(wp)==PALETTEKIND) { FixCursorLogPrint("3-2. FoundWindow is a palette\n");  return; }	
	if (GetWindowKind(wp)!=DOCUMENTKIND) {
		FixCursorLogPrint("3-3. FoundWindow not our Document\n"); ArrowCursor();  return;
	}
	
	doc = GetDocumentFromWindow(wp);
	
	if (doc==NULL) { ArrowCursor(); FixCursorLogPrint("4. FixCursor: doc is null\n");  return; }
	sprintf(message, "4.1 found document %s\n", doc->name);
	FixCursorLogPrint(message);
	
	/* If mouse not over the Document's viewRect, use arrow */
	
	if (!PtInRect(mousept,&doc->viewRect)) { FixCursorLogPrint("5. Not in viewRect\n");  ArrowCursor(); return; }
	
	/* If Document showing Master Page or Work on Format, use arrow */
	
	if (doc->masterView || doc->showFormat) { FixCursorLogPrint("6. MasterPage or ShowFormat\n");  ArrowCursor();  return; }
	
	/* Mouse is over the viewRect of a Document that is showing the "normal" (not Master
	   Page or Work on Format) view. Now we have to think harder. */
	 
	newCursor = currentCursor;
	
	/* At this point, we check for shakeoffs.  We do this by keeping track of the times
	at which the mouse changes horizontal direction, but no oftener than CHECKMIN ticks,
	since otherwise we won't pick up the direction changes (dx == 0 most of the time).
	Each time the direction does change we check to see if it has happened within
	mShakeThresh ticks of the last change. If it hasn't, we start over. If it has, we
	keep track of the number of consecutive shakes, each occurring within mShakeThresh
	ticks of each other, until there are MAXSHAKES of them, in which case we change the
	cursor to the arrow cursor. We save the previous cursor (in shook), so that on the
	next shakeoff, we can go back to whatever used to be there, if it's still the arrow.
	Finally, we don't allow consecutive shakeoffs to occur more often than SWAPMIN
	ticks. Yours truly, Doug McKenna. */

	now = TickCount();
	if (now > nextcheck) {
		x = mousept.h;
		dx = x - xOld;
		nextcheck = now + CHECKMIN;
		
		if ((dx<0 && dxOld>0) || (dx>0 && dxOld<0))
		
			if (shaker++ == 0)
				soon = now + config.mShakeThresh;					/* Start timing */
			 else
				if (now < soon)										/* A quick enough shake? */
					if (shaker >= MAXSHAKES) {
						if (shookey==0 || currentCursor!=arrowCursor) {
							shookey = GetPalChar((*paletteGlobals[TOOL_PALETTE])->currentItem);
							shook = currentCursor;
							PalKey(CH_ENTER);
							newCursor = arrowCursor;
						}
						 else {
							PalKey(shookey);
							newCursor = shook;
							shookey = 0;
						}
						shaker = 0;
						currentCursor = newCursor;
						nextcheck += SWAPMIN;
					}
					else
						soon = now + config.mShakeThresh;
				 else
					shaker = 0;										/* Too late: reset */

		dxOld = dx; xOld = x;
	}
	
	/* If option key is down and shift key is NOT down, use "general drag" cursor no
	   matter what. The shift key test is unfortunately necessary so we can use
	   shift-option-E (for example) as the keyboard equivalent for eighth grace note
	   without confusing users by having the wrong cursor as long they actually have the
	   option key down. */

	if (OptionKeyDown() && !ShiftKeyDown()) newCursor = genlDragCursor;
	
	FixCursorLogPrint("7. Installing current cursor\n");
	
	/* Install the new cursor, whatever it may be. */
	
	SetCursor(*newCursor);
}


/* ----------------------------------------------------------------------------- Menus -- */

/* Enable or disable a menu item. */

void XableItem(MenuHandle menu, short item, Boolean enable)
{
	if (enable) EnableMenuItem(menu,item);
	 else		DisableMenuItem(menu,item);
}

/* Enable or disable an entire menu. Call this any number of times before calling
UpdateMenuBar() to make the changes all at once (if any). */

static Boolean menuBarChanged;

void UpdateMenu(MenuHandle menu, Boolean enable)
{
	if (menu)
		if (enable) {
			if (!IsMenuItemEnabled(menu, 0)) {
//				if (!((**menu).enableFlags & 1)) {		/* Bit 0 is for whole menu */
				EnableMenuItem(menu,0);
				menuBarChanged = True;
			}
		}
		 else
			if (IsMenuItemEnabled(menu, 0)) {
//				if ((**menu).enableFlags & 1) {
				DisableMenuItem(menu,0);
				menuBarChanged = True;
			}
}

void UpdateMenuBar()
{
	if (menuBarChanged) DrawMenuBar();
	menuBarChanged = False;
}


/* ------------------------------------------------------------------------ Rectangles -- */

/* Set ansR to be the centered version of r with respect to inside.  We do this by
computing the centers of each rectangle, and offseting the centeree by the distance
between the two centers. If insideR is NULL, we assume it to be the main screen. */

void CenterRect(Rect *r, Rect *insideR, Rect *ansR)
{
	short rx, ry, ix, iy;
	Rect insR;

	rx = r->left + ((r->right - r->left) / 2);
	ry = r->top + ((r->bottom - r->top) / 2);
	
	if (insideR == NULL) { insR = GetQDScreenBitsBounds();  *insideR = insR; }

	ix = insideR->left + ((insideR->right - insideR->left) / 2);
	iy = insideR->top + ((insideR->bottom - insideR->top) / 2);
	
	*ansR = *r;
	OffsetRect(ansR, ix-rx, iy-ry);
}

/* Force a rectangle to be entirely enclosed by another (presumably) larger one. If the
"enclosed" rectangle is larger, then we leave the bottom right within. margin is a slop
factor to bring the rectangle a little farther in than just to the outer rectangle's
border. */

void PullInsideRect(Rect *r, Rect *inside, short margin)
{
	Rect ansR, tmp;
	
	tmp = *inside;
	InsetRect(&tmp,margin,margin);
	
	SectRect(r, &tmp, &ansR);
	if (!EqualRect(r, &ansR)) {
		if (r->top < tmp.top) OffsetRect(r, 0, tmp.top-r->top);
		 else if (r->bottom > tmp.bottom) OffsetRect(r, 0, tmp.bottom-r->bottom);
			
		if (r->left < tmp.left) OffsetRect(r, tmp.left-r->left, 0);
		 else if (r->right > tmp.right) OffsetRect(r, tmp.right-r->right, 0);
	}
}

/*	Return True if r is completely inside the bounds rect; False if outside or partially
outside. */

Boolean ContainedRect(Rect *r, Rect *bounds)
{
	Rect tmp;
	
	UnionRect(r, bounds, &tmp);
	return (EqualRect(bounds, &tmp));
}


/*	Draw a series of rectangles interpolated between two given rectangles, from small
to big if <zoomUp>, from big to small if not. As described in TechNote 194, we draw not
into WMgrPort, but into a new port covering the same area. Both rectangles should be
specified in global screen coordinates. */

static short blend1(short sc, short bc);
static Fixed fract, factor, one;

static short blend1(short smallCoord, short bigCoord)
{
	Fixed smallFix, bigFix, tmpFix;
	
	smallFix = one * smallCoord;		/* Scale up to fixed point */
	bigFix	 = one * bigCoord;
	tmpFix	 = FixMul(fract, bigFix) + FixMul(one-fract, smallFix);
	
	return(FixRound(tmpFix));
}

void ZoomRect(Rect *smallRect, Rect *bigRect, Boolean zoomUp)
{
	short i, zoomSteps = 16;
	Rect r1, r2, r3, r4, rgnBBox;
	GrafPtr oldPort, deskPort;
	
	RgnHandle grayRgn = NewRgn();
	RgnHandle deskPortVisRgn = NewRgn();		
		
	GetPort(&oldPort);
	deskPort = CreateNewPort();
	if (!deskPort) { LogPrintf(LOG_WARNING, "ZoomRect: can't create deskPort\n"); return; }
		
	GetPortVisibleRegion(deskPort, deskPortVisRgn);
	grayRgn = GetGrayRgn();
	if (!grayRgn) { LogPrintf(LOG_WARNING, "ZoomRect: can't create grayRgn\n"); return; }
	
	CopyRgn(grayRgn, deskPortVisRgn);
	
	GetRegionBounds(grayRgn, &rgnBBox);  		
	SetPortBounds(deskPort, &rgnBBox);
	
	DisposeRgn(grayRgn);
	DisposeRgn(deskPortVisRgn);
	
	SetPort(deskPort);
	PenPat(NGetQDGlobalsGray());
	PenMode(notPatXor);

	one = 65536;							/* fixed point 'const' */
	if (zoomUp) {
		r1 = *smallRect;
		factor = FixRatio(6, 5);			/* Make bigger each time */
		fract =  FixRatio(541, 10000);		/* 5/6 ^ 16 = 0.540877 */
	}
	 else {
		r1 = *bigRect;
		factor = FixRatio(5, 6);			/* Make smaller each time */
		fract  = one;
	}
	
	r2 = r1;
	r3 = r1;
	FrameRect(&r1);							/* Draw initial image */

	for (i=0; i<zoomSteps; i++) {
		r4.left   = blend1(smallRect->left, bigRect->left);
		r4.right  = blend1(smallRect->right, bigRect->right);
		r4.top	  = blend1(smallRect->top, bigRect->top);
		r4.bottom = blend1(smallRect->bottom, bigRect->bottom);
		
		FrameRect(&r4);						/* Draw newest */
		FrameRect(&r1);						/* Erase oldest */
		r1 = r2; r2 = r3; r3 = r4;
		
		fract = FixMul(fract, factor);		/* Bump interpolation factor */
	}
	
	FrameRect(&r1);							/* Erase final image */
	FrameRect(&r2);
	FrameRect(&r3);
	
	PenNormal();
	DisposePort(deskPort);
	SetPort(oldPort);
}


/* --------------------------------------------------------------------------- Windows -- */

/*	Given a window, compute the global coords of its port. */

void GetGlobalPort(WindowPtr w, Rect *r)
{
	GrafPtr oldPort;  Point pt;
	
	GetWindowPortBounds(w, r);
	pt.h = r->left;  pt.v = r->top;
	GetPort(&oldPort);
	SetPort(GetWindowPort(w));
	LocalToGlobal(&pt);
	SetPort(oldPort);
	SetRect(r, pt.h, pt.v, pt.h+r->right-r->left, pt.v+r->bottom-r->top);
}


/* Given an origin, orig, 0 <= i < n, compute the offset position, ans, for that index
into n.  All coords are expected to be global, and all windows are expected to be
standard sized. This can be used to position multiple windows being opened at the same
time; however, it's not used in v. 5.9.x. */

void GetIndWinPosition(WindowPtr w, short i, short n, Point *ans)
{
	short x, y;  Rect box;
	
	GetGlobalPort(w, &box);
	ans->h = box.left;
	ans->v = box.top;
	
	if (n <= 0) n = 8;
	x = i / n;
	y = i % n;
	
	ans->h += 5 + 30*x + 5*y;
	ans->v += 20 + 20*y + 5*x;
}

/* Find a nice spot in global coordinates to place a window by looking down the window
list nLook windows for any conflicts. Larger values of nLook should give nicer results
at the expense of slightly more running time. */

void AdjustWinPosition(WindowPtr w)
{
	short nLook=7, i, k, xx, yy;
	Rect box;  WindowPtr ww;
	
	GetGlobalPort(w, &box);
	xx = box.left;
	yy = box.top;
	
	i = 0;
	while (i < nLook) {
		k = 0;
		for (ww=FrontWindow(); ww!=NULL && k<nLook; ww=GetNextWindow(ww))
			if (ww!=w && IsWindowVisible(ww) && GetWindowKind(w)==DOCUMENTKIND) {
				k++;
				GetGlobalPort(ww, &box);
				if (box.left==xx && box.top==yy) {
					xx -= 5;
					yy += 20;
					i++;
					break;
				}
			}
		if (ww==NULL || k>=nLook) break;
	}
		
	if (i < nLook) MoveWindow(w, xx, yy, False);
}


/* Given a global rect, r, deliver the bounds of the screen device that best "contains"
that rect. If we're running on a machine that that <hasColorQD> (almost certainly any
machine Nightingale can run on!), we do this by scanning the device list, and for each
active screen device we intersect the rect with the device's bounds, keeping track of
that device whose intersection's area is the largest. If the machine doesn't
<hasColorQD>, we assume it has only one screen and just deliver the usual screenBits.bounds.
The one problem with this is it doesn't handle some ancient Macs with more than one
screen, e.g., SE/30s with third-party large monitors (ca. 1990); who cares.

The two arguments can point to the same rectangle. Returns the number of screens with
which the given rectangle intersects unless it's a non-<hasColorQD> machine, in which
case it returns 0.

NB: An earlier version of this routine apparently had a feature whereby, if the
caller wanted the menu bar area included, it could set a global <pureScreen> True
before call; this could easily be added again. */

short GetMyScreen(Rect *r, Rect *bnds)
{
	GDHandle dev;  Rect test, ans, bounds;
	long area, maxArea=0L;  short nScreens = 0;
	
	bounds = GetQDScreenBitsBounds();
	bounds.top += GetMBarHeight();						/* Exclude menu bar */
	if (thisMac.hasColorQD)
		for (dev=GetDeviceList(); dev; dev=GetNextDevice(dev))
			if (TestDeviceAttribute(dev, screenDevice) &&
										TestDeviceAttribute(dev, screenActive)) {
				test = (*dev)->gdRect;
				if (TestDeviceAttribute(dev, mainScreen))
					test.top += GetMBarHeight();
				if (SectRect(r, &test, &ans)) {
					nScreens++;
					OffsetRect(&ans, -ans.left, -ans.top);
					area = (long)ans.right * (long)ans.bottom;
					if (area > maxArea) { maxArea = area;  bounds = test; }
				}
			}
	*bnds = bounds;
	return(nScreens);
}

/* Given an Alert Resource ID, get its port rectangle and center it within a given window
or with a given offset if (left,top)!=(0,0).  If w is NULL, use screen bounds.  This
routine should be called immediately before displaying the alert, since it might be
purged.  For multi-screen devices, we look for that device on which a given window
is most affiliated, and we don't allow alert outside of that device's global bounds. */

void PlaceAlert(short id, WindowPtr w, short left, short top)
{
	Handle alrt;
	Rect r, inside, bounds;
	Boolean sect;
	
	/* This originally used Get1Resource, but it should get the ALRT from any available
	   resource file. */
	     
	alrt = GetResource('ALRT', id);
	if (alrt!=NULL && *alrt!=NULL && ResError()==noErr) {
		ArrowCursor();
		LoadResource(alrt);
		r = *(Rect *)(*alrt);
		if (w) {
			GetGlobalPort(w, &inside);
		}
		 else {
			inside = GetQDScreenBitsBounds();
			inside.top += 36;
		}
		CenterRect(&r, &inside, &r);
		if (top) OffsetRect(&r, 0, (inside.top+top)-r.top);
		if (left) OffsetRect(&r, (inside.left+left)-r.left, 0);
		
		/* If we're centering w/r/t a window, don't let alert get out of bounds. We
		   look for that graphics device that contains the window more than any other,
		   and use its bounds to bring the alert back into if its outside. */
		   
		if (w) {
			GetMyScreen(&inside, &bounds);
			sect = SectRect(&r, &bounds, &inside);
			if ((sect && !EqualRect(&r, &inside)) || !sect) {
				/* if (!sect) */ inside = bounds;
				PullInsideRect(&r, &inside, 16);
			}
		}
		LoadResource(alrt);			/* Probably not necessary, but we do it for good luck */
		*((Rect *)(*alrt)) = r;
	}
}

/* Given a window, dlog, place it with respect to another window, w, or the screen if w
is NULL. If (left,top) == (0,0) center it; otherwise place it at offset (left,top) from
the centering rectangle's upper left corner.  Deliver True if position changed. */

Boolean PlaceWindow(WindowPtr dlog, WindowPtr w, short left, short top)
{
	Rect r, s, inside;
	short dx=0, dy=0;
	
	if (dlog) {
		GetGlobalPort(dlog, &r);
		s = r;
		if (w) GetGlobalPort(w, &inside);
		 else  GetMyScreen(&r, &inside);
		CenterRect(&r, &inside, &r);
		if (top) OffsetRect(&r, 0, (inside.top+top)-r.top);
		if (left) OffsetRect(&r, (inside.left+left)-r.left, 0);
		
		GetMyScreen(&r, &inside);
		PullInsideRect(&r, &inside, 16);
		
		MoveWindow(dlog, r.left, r.top, False);
		dx = r.left - s.left;
		dy = r.top  - s.top;
		}
	return(dx!=0 || dy!=0);
}

/* Center w horizontally and place it <top> pixels from the top of the screen. */

void CenterWindow(WindowPtr w,
					short top)	/* 0=center vertically, else put top at this position */
{
	Rect scr, portRect;
	Point pt;
	short margin;
	short rsize,size;

	scr = GetQDScreenBitsBounds();
	SetPort(GetWindowPort(w));
	GetWindowPortBounds(w, &portRect);
	pt.h = portRect.left; pt.v = portRect.top;
	LocalToGlobal(&pt);

	size = scr.right - scr.left;
	rsize = portRect.right - portRect.left;
	margin = size - rsize;
	if (margin > 0) {
		margin >>= 1;
		pt.h = scr.left + margin;
	}
	size = scr.bottom - scr.top;
	rsize = portRect.bottom - portRect.top;
	margin = size - rsize;
	if (margin > 0) {
		margin >>= 1;
		pt.v = scr.top + margin;
	}
	MoveWindow(w, pt.h, (top? top : pt.v), False);
}


/* Erase and invalidate a rectangle in the current port. */

void EraseAndInval(Rect *r)
{
	GrafPtr port;
	WindowPtr w;
	
	GetPort(&port);
	w = GetWindowFromPort(port);
	
	EraseRect(r);
	InvalWindowRect(w, r);
}


/* ------------------------------------------------- Checking if various keys are down -- */

/* KeyIsDown (from the THINK C class library of the 1990's)
 
Determine whether or not the specified key is being pressed. Keys are specified by
hardware-specific key code, NOT by the character. Charts of key codes appear in Inside
Macintosh, p. V-191. */

Boolean KeyIsDown(short theKeyCode)
{
	KeyMap theKeys;
	
	GetKeys(theKeys);					/* Get state of each key */
										
	/* Ordering of bits in a KeyMap is truly bizarre. A KeyMap is a 16-byte (128
	   bits) array where each bit specifies the state of a key (0 = up, 1 = down). We
	   isolate the bit for the specified key code by first determining the byte
	   position in the KeyMap and then the bit position within that byte. Key codes
	   0-7 are in the first byte (offset 0 from the start), codes 8-15 are in the
	   second, etc. The BitTst() trap counts bits starting from the high-order bit of
	   the byte. For example, for key code 58 (the option key), we look at the 8th
	   byte (7 offset from the first byte) and the 5th bit within that byte. */
		
	return( BitTst( ((char*) &theKeys) + theKeyCode / 8,
					(long) 7 - (theKeyCode % 8) )!=0 );
}

Boolean CmdKeyDown() {
	return (KeyIsDown(55));
}

Boolean OptionKeyDown() {
	return (GetCurrentKeyModifiers() & optionKey) != 0;
}

Boolean ShiftKeyDown() {
	return (GetCurrentKeyModifiers() & shiftKey) != 0;
}

Boolean CapsLockKeyDown() {
	return (GetCurrentKeyModifiers() & alphaLock) != 0;
}

Boolean ControlKeyDown() {
	return (GetCurrentKeyModifiers() & controlKey) != 0;
}
	
/* FIXME: As of v. 5.8b10, CommandKeyDown() is never used; instead, CmdKeyDown() is used.
I don't know why, but CommandKeyDown() is the one that's analogous to the other key-down
functions, and it looks less system-dependent. So we should probably switch. */

Boolean CommandKeyDown() {
	return (GetCurrentKeyModifiers() & cmdKey) != 0;
}


/* ----------------------------------------------------------- Special keyboard things -- */

/* Return True iff the next event is a keystroke of period and the command key is down.
FIXME: This appears to effectively do the same thing as UserInterrupt(), though it might
not in some circumstances; anyway, consider removing one or the other. */

Boolean CheckAbort()
{
	EventRecord evt;  short ch;
	Boolean quit = False;

	if (!WaitNextEvent(keyDownMask+app4Mask, &evt, 0, NULL)) return(quit);

	switch(evt.what) {
		case keyDown:
			ch = evt.message & charCodeMask;
			if (ch=='.' && (evt.modifiers&cmdKey)!=0) quit = True;
			break;
		case app4Evt:
			DoSuspendResume(&evt);
			break;
	}
	
	return(quit);
}

/* Determine if a mouse click is a double click, using given position tolerance, all in
current port. */

Boolean IsDoubleClick(Point pt, short tol, long now)
{
	Rect box;  Boolean ans=False;
	static long lastTime;  static Point thePt;
	
	if ((unsigned long)(now-lastTime)<GetDblTime()) {
		SetRect(&box, thePt.h-tol, thePt.v-tol, thePt.h+tol+1, thePt.v+tol+1);
		ans = PtInRect(pt, &box);
		lastTime = 0;
		thePt.h = thePt.v = 0;
	}
	else {
		lastTime = now;
		thePt = pt;
	}
	return ans;
}


/* ----------------------------------------------------------------------- GetStaffLim -- */
/* Return a rough top or bottom staff limit: 6 half-spaces above the top line, or 6 half-
spaces below the bottom line. Intended for use in InvertSymbolHilite, for which the
crudeness is not a problem. */

static DDIST GetStaffLim(Document *doc, LINK pL, short s, Boolean top, PCONTEXT pContext);
static DDIST GetStaffLim(
					Document *doc, LINK pL,
					short staffn,
					Boolean top,					/* True if getting top staff limit. */
					PCONTEXT pContext)
{
	DDIST	blackTop, blackBottom,
			dhalfSp;								/* Distance between staff half-spaces */

	GetContext(doc, pL, staffn, pContext);
	dhalfSp = pContext->staffHeight/(2*(pContext->staffLines-1));
	if (top) {
		blackTop = pContext->staffTop;
		blackTop -= 6*dhalfSp;
		return blackTop;
	}

	blackBottom = pContext->staffTop+pContext->staffHeight;
	blackBottom += 6*dhalfSp;
	return blackBottom;
}


/* ======================================================== Hilite, Frame, Flash Rects == */

/* ---------------------------------------------------------------- InvertSymbolHilite -- */
/* Hilite a symbol, the graphical representation of an object, in a distinctive way to
show it's the one into which a subobject is about to be inserted, or to which something
is or will be attached. The hiliting we do is a vertical dotted line thru the object,
thickened slightly at the "special" staff if there is one. If pL is a structural element
-- SYSTEM, PAGE, STAFF, etc. -- do nothing. */

void InvertSymbolHilite(
			Document *doc,
			LINK pL,
			short staffn,			/* No. of "special" staff to emphasize or NOONE */
			Boolean flash			/* True=flash to emphasize special staff */
			)
{
	short	xd, xp, ypTop, ypBot;
	DDIST	blackTop=0, blackBottom=0;
	CONTEXT	context;

	if (!pL) return;											/* Just in case */
	if (objTable[ObjLType(pL)].justType==J_STRUC) return;
	
	if (staffn==NOONE)
		GetContext(doc, pL, 1, &context);
	else {
		blackTop = GetStaffLim(doc, pL, staffn, True, &context);
		blackBottom = GetStaffLim(doc, pL, staffn, False, &context);
	}
	xd = SysRelxd(pL)+context.systemLeft;					/* abs. origin of object */
	
	/* Draw with gray pattern (to make dotted lines), in XOR mode (to invert the pixels'
	   existing colors). */
		
	PenMode(patXor);
	PenPat(NGetQDGlobalsGray());
	xp = context.paper.left+d2p(xd);
	ypTop = context.paper.top+d2p(context.systemTop)+1;
	ypBot = context.paper.top+d2p(context.systemBottom)-1;
	MoveTo(xp+1, ypTop);									/* Draw thru whole system */
	LineTo(xp+1, ypBot);
	
	if (staffn!=NOONE) {
		PenSize(3, 1);										/* Draw thicker on special staff */
		MoveTo(xp, context.paper.top+d2p(blackTop));
		LineTo(xp, context.paper.top+d2p(blackBottom));
		
		if (flash) {
			/* Pause; erase the thickened line; pause; and redraw it. */
			SleepTicks(6);
			MoveTo(xp, context.paper.top+d2p(blackTop));
			LineTo(xp, context.paper.top+d2p(blackBottom));
			
			SleepTicks(6);
			MoveTo(xp, context.paper.top+d2p(blackTop));
			LineTo(xp, context.paper.top+d2p(blackBottom));
		}
	}
	PenNormal();											/* Restore normal drawing */
}


/* ------------------------------------------------------------- InvertTwoSymbolHilite -- */
/* Hilite two symbols, the graphical representations of objects, in a distinctive way
to show where a subobject is about to be inserted, or where something is or will be
attached. */

void InvertTwoSymbolHilite(Document *doc, LINK node1, LINK node2, short staffn)
{	
	InvertSymbolHilite(doc, node1, staffn, True);
	if (node2 && node2!=node1) InvertSymbolHilite(doc, node2, staffn, True);
}


/* ------------------------------------------------------------------- HiliteAttPoints -- */
/* Hilite attachment point(s) and wait as long as the mouse button is down, or in any
case for a short minimum time; then erase the hiliting. If <firstL> is NILINK, do
nothing at all. <lastL> may be NILINK (to indicate there's really only one attachment
point) or may be identical to <firstL>. In the latter case, simply hilite that link
twice. */

void HiliteAttPoints(
			Document *doc,
			LINK firstL, LINK lastL,		/* lastL may be NILINK */
			short staffn)
{
	if (!firstL) return;
	
	if (lastL) InvertTwoSymbolHilite(doc, firstL, lastL, staffn);			/* On */
	else	   InvertSymbolHilite(doc, firstL, staffn, True);				/* On */

	SleepTicks(HILITE_TICKS);
	while (Button()) ;
	
	InvertSymbolHilite(doc, firstL, staffn, False);								/* Off */
	if (lastL && firstL!=lastL) InvertSymbolHilite(doc, lastL, staffn, False);	/* Off */
}


/* ------------------------------------------------------------------------ HiliteRect -- */
/*	Toggle the hiliting of the given rectangle, using the selection color. Intended
for object/subobject hiliting. */

void HiliteRect(Rect *r)
{
	LMSetHiliteMode(LMGetHiliteMode() & 0x7F);	/* Clear top bit = hilite in selection color */

	InsetRect(r,-config.enlargeHilite,-config.enlargeHilite);
	InvertRect(r);
	InsetRect(r,config.enlargeHilite,config.enlargeHilite);
}


/* ------------------------------------------------------------------- FrameShadowRect -- */

void FrameShadowRect(Rect *pBox)
{
	FrameRect(pBox);
	PenSize(2, 2);
	MoveTo(pBox->left, pBox->bottom);			/* drop shadow */
	LineTo(pBox->right, pBox->bottom);
	LineTo(pBox->right, pBox->top);
	PenSize(1, 1);
}


/* ------------------------------------------------------------------------- FlashRect -- */

#define FLASH_RECT_COUNT 1

void FlashRect(Rect *pRect)
{	
	for (short j=0; j<FLASH_RECT_COUNT; j++) {
		InvertRect(pRect);
		SleepTicks(20L);
		InvertRect(pRect);
		SleepTicks(20L);		
	}
}


/* ------------------------------------------------------------------------- SamePoint -- */
/* Check to see if two Points (presumably on the screen) are within slop of each other.
Intended for editing beams, lines, hairpins, etc. */

#define SAME_POINT_SLOP 2		/* in pixels */

Boolean SamePoint(Point p1, Point p2)
{
	short dx, dy;
	
	dx = p1.h - p2.h; if (dx < 0) dx = -dx;
	dy = p1.v - p2.v; if (dy < 0) dy = -dy;
	
	return (dx<=SAME_POINT_SLOP && dy<=SAME_POINT_SLOP);
}
	

/* ------------------------------------------------------------------ Advise functions -- */
/* Give alerts of varlous types and return value. */

short Advise(short alertID)
{
	LogPrintf(LOG_NOTICE, "* Advise alertID=%d.\n\n", alertID);
	ArrowCursor();
	return Alert(alertID, NULL);
}

short NoteAdvise(short alertID)
{
	LogPrintf(LOG_NOTICE, "* NoteAdvise alertID=%d.\n", alertID);
	ArrowCursor();
	return NoteAlert(alertID, NULL);
}

short CautionAdvise(short alertID)
{
	LogPrintf(LOG_NOTICE, "* CautionAdvise alertID=%d!\n", alertID);
	ArrowCursor();
	return CautionAlert(alertID, NULL);
}

short StopAdvise(short alertID)
{
	LogPrintf(LOG_WARNING, "* StopAdvise alertID=%d!\n", alertID);
	ArrowCursor();
	return StopAlert(alertID, NULL);
}


/* ------------------------------------------------------------------ Inform functions -- */
/* Give valueless alerts of various types. */
							
void Inform(short alertID)
{
	short dummy;
	
	LogPrintf(LOG_NOTICE, "* Inform alertID=%d\n", alertID);
	ArrowCursor();
	dummy = Alert(alertID, NULL);
}
						
void NoteInform(short alertID)
{
	short dummy;
	
	LogPrintf(LOG_NOTICE, "* NoteInform alertID=%d\n", alertID);
	ArrowCursor();
	dummy = NoteAlert(alertID, NULL);
}
						
void CautionInform(short alertID)
{
	short dummy;
	
	LogPrintf(LOG_WARNING, "* CautionInform alertID=%d\n", alertID);
	ArrowCursor();
	dummy = CautionAlert(alertID, NULL);
}

void StopInform(short alertID)
{
	short dummy;
	
	LogPrintf(LOG_WARNING, "* StopInform alertID=%d\n", alertID);
	ArrowCursor();
	dummy = StopAlert(alertID, NULL);
}


/* ----------------------------------------------------------------------- ProgressMsg -- */

#define MAX_MESSAGE_STR 19		/* Index of last message string */

#define MESSAGE_DI 1

/* If which!=0, put up a small window (actually a dialog) containing the message with
that number, and pause for a brief time (HILITE_TICKS ticks). If which=0, remove the
window. If which!=0 and there's already a ProgressMsg window up, remove it before
putting up the new one. */

Boolean ProgressMsg(short which,
					char *moreInfo)				/* C string */
{
	static short lastWhich=-999;
	static DialogPtr dialogp=NULL;  GrafPtr oldPort;
	char str[256];
	short aShort;  Handle tHdl;  Rect aRect;
	 
	GetPort(&oldPort);

	if (which>0 && which<=MAX_MESSAGE_STR) {
	
		if (dialogp && which!=lastWhich) ProgressMsg(0, "");	/* Remove existing msg window */
		lastWhich = which;
		
		if (!dialogp) {
			dialogp = GetNewDialog(MESSAGE_DLOG, NULL, BRING_TO_FRONT);
			if (!dialogp) return False;
		}

		SetPort(GetDialogWindowPort(dialogp));

		GetIndCString(str, MESSAGE_STRS, which);
		strcat(str, moreInfo);
		GetDialogItem(dialogp, MESSAGE_DI, &aShort, &tHdl, &aRect);
		SetDialogItemCText(tHdl, str);

		CenterWindow(GetDialogWindow(dialogp),100);
		ShowWindow(GetDialogWindow(dialogp));
		UpdateDialogVisRgn(dialogp);
		SleepTicks(HILITE_TICKS);								/* So it's not erased TOO quickly */
	}
	else if (which==0) {
		if (dialogp) {
			HideWindow(GetDialogWindow(dialogp));
			DisposeDialog(dialogp);								/* Free heap space */
			dialogp = NULL;
		}
	}
	else {
		MayErrMsg("ProgressMsg: Progress message %ld doesn't exist.", which);
		return False;
	}

	SetPort(oldPort);
	return True;
}


/* ------------------------------------------------------------ UserInterrupt, -AndSel -- */
/* Returns True if COMMAND and PERIOD (.) keys are both currently down; all other keys
are ignored. */

Boolean UserInterrupt()
{
	if (!CmdKeyDown()) return False;
	if (!KeyIsDown(47)) return False;
	
	return True;
}

/* Returns True if COMMAND and SLASH (/) keys are both currently down; all other keys
are ignored. */

Boolean UserInterruptAndSel()
{
	if (!CmdKeyDown()) return False;
	if (!KeyIsDown(44)) return False;
	
	return True;
}


/* --------------------------------------------------------- NameHeapType, NameObjType -- */
/* Given a "heap index" or object type, return the name of the corresponding object. */

const char *NameHeapType(
			short heapIndex,
			Boolean friendly)		/* True=give user-friendly names, False=give "real" names */
{
	const char *ps;

	switch (heapIndex) {
		case HEADERtype:	ps = "HEAD"; break;
		case TAILtype:		ps = "TAIL"; break;
		case SYNCtype:		ps = (friendly? "note and rest" : "SYNC"); break;
		case RPTENDtype:	ps = (friendly? "repeat bar/segno" : "REPEATEND"); break;
		case ENDINGtype:	ps = (friendly? "ending" : "ENDING"); break;
		case PAGEtype:		ps = (friendly? "page" : "PAGE"); break;
		case SYSTEMtype:	ps = (friendly? "system" : "SYSTEM"); break;
		case STAFFtype:		ps = (friendly? "staff" : "STAFF"); break;
		case MEASUREtype:	ps = (friendly? "measure" : "MEASURE"); break;
		case PSMEAStype:	ps = (friendly? "pseudomeasure" : "PSEUDOMEAS"); break;
		case CLEFtype:		ps = (friendly? "clef" : "CLEF"); break;
		case KEYSIGtype:	ps = (friendly? "key signature" : "KEYSIG"); break;
		case TIMESIGtype:	ps = (friendly? "time signature" : "TIMESIG"); break;
		case BEAMSETtype:	ps = (friendly? "beamed group" : "BEAMSET"); break;
		case TUPLETtype:	ps = (friendly? "tuplet" : "TUPLET"); break;
		case CONNECTtype:	ps = (friendly? "staff bracket" : "CONNECT"); break;
		case DYNAMtype:		ps = (friendly? "dynamic" : "DYNAMIC"); break;
		case MODNRtype:		ps = (friendly? "note modifier" : "MODNR"); break;
		case GRAPHICtype:	ps = (friendly? "Graphic" : "GRAPHIC"); break;
		case OTTAVAtype:	ps = (friendly? "octave sign" : "OTTAVA"); break;
		case SLURtype:		ps = (friendly? "slur and tie" : "SLUR"); break;
		case GRSYNCtype:	ps = (friendly? "grace note" : "GRSYNC"); break;
		case TEMPOtype:		ps = (friendly? "tempo/MM" : "TEMPO"); break;
		case SPACERtype:	ps = (friendly? "spacer" : "SPACER"); break;
		case OBJtype:		ps = "OBJECT"; break;
		default:			ps = "**UNKNOWN**";
	}

	return ps;
}


/* Given an object, return its "real" (short but not user-friendly) name. */

const char *NameObjType(LINK pL)
{
	return NameHeapType(ObjLType(pL), False);
}


/* ------------------------------------------------------------------- NameGraphicType -- */
/* Given an object of type GRAPHIC, return its subtype. */

const char *NameGraphicType(
			LINK pL,
			Boolean friendly)		/* True=give user-friendly names, False=give "real" names */
{
	const char *ps;

	if (!GraphicTYPE(pL)) {											/* Just in case */
		ps = "*NOT A GRAPHIC*";
		return ps;
	}

	switch (GraphicSubType(pL)) {
		case GRPICT:			ps = (friendly? "PICT" : "GRPICT");  break;
		case GRString:			ps = (friendly? "Text" : "GRString");  break;
		case GRLyric:			ps = (friendly? "Lyric" : "GRLyric");  break;
		case GRDraw:			ps = (friendly? "Draw" : "GRDraw");  break;
		case GRRehearsal:		ps = (friendly? "Rehearsal" : "GRRehearsal");  break;
		case GRChordSym:		ps = (friendly? "Chord Sym" : "GRChordSym");  break;
		case GRArpeggio:		ps = (friendly? "Arpeggio" : "GRArpeggio");  break;
		case GRChordFrame:		ps = (friendly? "Chord Frame" : "GRChordFrame");  break;
		case GRMIDIPatch:		ps = (friendly? "MIDI Patch" : "GRMIDIPatch");  break;
		case GRSusPedalDown:	ps = (friendly? "Pedal Down" : "GRSusPedalDown");  break;
		case GRSusPedalUp:		ps = (friendly? "Pedal Up" : "GRSusPedalUp");  break;
		case GRMIDIPan:			ps = (friendly? "Pan" : "GRMIDIPan");  break;
		default:				ps = (friendly? "*UNKNOWN*" : "*UNKNOWN*");
	}

	return ps;
}


/* ----------------------------------------------------- DynamicToString, ClefToString -- */

Boolean DynamicToString(short dynamicType, char dynStr[])
{
	switch (dynamicType) {
		case PPP_DYNAM: strcpy(dynStr, "ppp"); return True;
		case PP_DYNAM: strcpy(dynStr, "pp"); return True;
		case P_DYNAM: strcpy(dynStr, "p"); return True;
		case MP_DYNAM: strcpy(dynStr, "mp"); return True;
		case MF_DYNAM: strcpy(dynStr, "mf"); return True;
		case F_DYNAM: strcpy(dynStr, "f"); return True;
		case FF_DYNAM: strcpy(dynStr, "ff"); return True;
		case FFF_DYNAM: strcpy(dynStr, "fff"); return True;
		case DIM_DYNAM: strcpy(dynStr, "hairpin dim."); return True;
		case CRESC_DYNAM: strcpy(dynStr, "hairpin cresc."); return True;
		default: strcpy(dynStr, "unknown"); return False;
	}
}


Boolean ClefToString(short clefType, char clefStr[])
{
	switch (clefType) {
		case TREBLE8_CLEF: strcpy(clefStr, "treble-8va"); return True;
		case FRVIOLIN_CLEF: strcpy(clefStr, "French violin"); return True;
		case TREBLE_CLEF: strcpy(clefStr, "treble"); return True;
		case SOPRANO_CLEF: strcpy(clefStr, "soprano"); return True;
		case MZSOPRANO_CLEF: strcpy(clefStr, "mezzo-soprano"); return True;
		case ALTO_CLEF: strcpy(clefStr, "alto"); return True;
		case TRTENOR_CLEF: strcpy(clefStr, "treble-tenor"); return True;
		case TENOR_CLEF: strcpy(clefStr, "tenor"); return True;
		case BARITONE_CLEF: strcpy(clefStr, "baritone"); return True;
		case BASS_CLEF: strcpy(clefStr, "bass"); return True;
		case BASS8B_CLEF: strcpy(clefStr, "bass-8vb"); return True;
		case PERC_CLEF: strcpy(clefStr, "percussion"); return True;
		
		default: strcpy(clefStr, "unknown"); return False;
	}
}


/* ---------------------------------------------------------------------- SmartenQuote -- */
/*	Given a TextEdit handle and a character, presumably one being inserted, if that
character is a (neutral) double quote or apostrophe, change it to its open/close
equivalent character according to the type of character that precedes it, if any;
return the converted character.  If the character not a neutral quote or apostrophe,
then just return it unchanged.

The conversion to open quote is done if the previous char was a space, tab, option-space,
or return; closed quote otherwise, unless it's the first character in the string, in
which case it's always open. If you want to allow the typing of standard ASCII single or
double quote, do it as a Command key char. */

/* NB: The following codes are for Macintosh Roman, as required by the Carbon toolkit. */

#define MRCH_OPEN_DOUBLE_QUOTE	0xD2
#define MRCH_CLOSE_DOUBLE_QUOTE	0xD3
#define MRCH_OPEN_SINGLE_QUOTE	0xD4
#define MRCH_CLOSE_SINGLE_QUOTE	0xD5
#define MRCH_OPTION_SPACE		0xCA

short SmartenQuote(TEHandle textH, short ch)
{
	short n;  unsigned char prev;

	if (textH)
		if (ch=='"' || ch=='\'') {
			n = (*textH)->selStart;
			prev = (n > 0) ? ((unsigned char) *(*((*textH)->hText) + n-1)) : 0;
			if (prev=='\r' || prev==' ' || prev==MRCH_OPTION_SPACE || prev=='\t' || n==0)
				ch = (ch=='"' ? MRCH_OPEN_DOUBLE_QUOTE : MRCH_OPEN_SINGLE_QUOTE);
			 else
				ch = (ch=='"' ? MRCH_CLOSE_DOUBLE_QUOTE : MRCH_CLOSE_SINGLE_QUOTE);
		}

	return ch;
}


/* --------------------------------------------------------------------------- DrawBox -- */
/* Use a wide line to draw a filled box centered at the given point.  Size should
normally be 4 so that the special drag cursor fits in with it. Intended to draw handles
for dragging.*/

void DrawBox(Point pt, short size)
{
	PenSize(size, size);
	size >>= 1;
	MoveTo(pt.h -= size, pt.v -= size);
	LineTo(pt.h, pt.v);
	PenSize(1, 1);
}


/* ------------------------------------------------------------------ DrawTheSelection -- */

/* Frame the current selection box, using the index'th pattern, or 0 for the last
pattern used.  */

static void	DrawSelBox(short index);
static void DrawSelBox(short index)
{
	static Pattern pat;
			
	if (index) GetIndPattern(&pat,MarchingAntsID,index);
	PenPat(&pat);
	
	FrameRect(&theSelection);
}


/* Draw the current selection box into the current port.  This should be called during
update and redraw events.  Does nothing if the current selection rectangle is empty. */

void DrawTheSelection()
{
	switch (theSelectionType) {
		case MARCHING_ANTS:
			PenMode(patXor);
			DrawSelBox(0);
			PenNormal();
			break;
		case SWEEPING_RECTS:
			PenMode(patXor);
			DrawTheSweepRects();
			PenNormal();
			break;
		case SLURSOR:
			PenMode(patXor);
			DrawTheSlursor();
			PenNormal();
			break;
		default:
			;
	}
}


/* --------------------------------------------------------------- Voice/Staff2UserStr -- */

/* Given an (internal) voice no., return its part and part-relative voice no. in
user-friendly format, e.g., "voice 2 of Piano".
FIXME: Based on code in HasSmthgAcross: that and similar code in 3-4 places should
be replaced with calls to this. */

void Voice2UserStr(Document *doc,
						short voice,
						char str[])			/* user-friendly string describing the voice */
{
	short userVoice;
	LINK partL; PPARTINFO pPart;
	char partName[256], fmtStr[256];

	Int2UserVoice(doc, voice, &userVoice, &partL);
	pPart = GetPPARTINFO(partL);
	strcpy(partName, (strlen(pPart->name)>6? pPart->shortName : pPart->name));
	GetIndCString(fmtStr, MISCERRS_STRS, 28);						/* "voice %d of %s" */
	sprintf(str, fmtStr, userVoice, partName);
}

/* Given an (internal) staff no., return its part and part-relative staff no. in
user-friendly format, e.g., "staff 3 (Clarinet)" or "staff 5 (staff 2 of Piano)".
FIXME: Based on code in StfHasSmthgAcross: that and similar code in 3-4 places should
be replaced with calls to this. */

void Staff2UserStr(Document *doc,
						short staffn,
						char str[])			/* user-friendly string describing the staff */
{
	short relStaff;
	LINK partL;  PPARTINFO pPart;
	char fmtStr[256];  short len;

	partL = Staff2PartL(doc, doc->headL, staffn);
	pPart = GetPPARTINFO(partL);
	GetIndCString(fmtStr, MISCERRS_STRS, 29);						/* "staff %d" */
	sprintf(str, fmtStr, staffn);
	relStaff = staffn-pPart->firstStaff+1;
	len = strlen((char *)str);
	if (pPart->lastStaff>pPart->firstStaff) {
		GetIndCString(fmtStr, MISCERRS_STRS, 30);					/* "(staff %d of %s)" */
		sprintf(&str[len], fmtStr, relStaff, pPart->name);
	}
	else
		sprintf(&str[len], " (%s)", pPart->name);
}


/* ----------------------------------------------------------------- QuickDraw Globals -- */

Rect GetQDPortBounds()
{
	Rect bounds;
	
	GetPortBounds(GetQDGlobalsThePort(), &bounds);
	
	return bounds;
}

Rect GetQDScreenBitsBounds()
{
	BitMap screenBits;
	
	GetQDGlobalsScreenBits(&screenBits);
	
	return screenBits.bounds;
}

Pattern *NGetQDGlobalsDarkGray()
{
	static Pattern sDkGray;
	static Boolean once = True;
	
	if (once) {	
		GetQDGlobalsDarkGray(&sDkGray);
		once = False;
	}
	
	return &sDkGray;
}

Pattern *NGetQDGlobalsLightGray()
{
	static Pattern sLtGray;
	static Boolean once = True;
	
	if (once) {	
		GetQDGlobalsLightGray(&sLtGray);
		once = False;
	}
	
	return &sLtGray;
}

Pattern *NGetQDGlobalsGray()
{
	static Pattern sGray;
	static Boolean once = True;
	
	if (once) {	
		GetQDGlobalsGray(&sGray);
		once = False;
	}
	
	return &sGray;
}

Pattern *NGetQDGlobalsBlack()
{
	static Pattern sBlack;
	static Boolean once = True;
	
	if (once) {	
		GetQDGlobalsBlack(&sBlack);
		once = False;
	}
	
	return &sBlack;
}

Pattern *NGetQDGlobalsWhite()
{
	static Pattern sWhite;
	static Boolean once = True;
	
	if (once) {	
		GetQDGlobalsWhite(&sWhite);
		once = False;
	}
	
	return &sWhite;
}


/* -------------------------------------------------------------------------------------- */

/* Documents */

/* See also GetDocumentFromWindow(WindowPtr w) in Documents.c */

GrafPtr GetDocWindowPort(Document *doc)
{
	return GetWindowPort(doc->theWindow);
}

/* Ports */

GrafPtr GetDialogWindowPort(DialogPtr dlog)
{
	return GetWindowPort(GetDialogWindow(dlog));
}

void GetDialogPortBounds(DialogPtr d, Rect *boundsR)
{
	GetWindowPortBounds(GetDialogWindow(d), boundsR);
}

//	bounds = (*(((WindowPeek)doc))->strucRgn)->rgnBBox;
//	
// GetWindowRegionBounds(w,kWindowStructureRgn,&bounds);

void GetWindowRgnBounds(WindowPtr w, WindowRegionCode rgnCode, Rect *bounds)
{
	RgnHandle wRgn = NewRgn();
	
	GetWindowRegion(w, rgnCode, wRgn);
	GetRegionBounds(wRgn,bounds);
	DisposeRgn(wRgn);
}
	
/* Dialogs */

void UpdateDialogVisRgn(DialogPtr d)
{
	RgnHandle visRgn;
	Rect portRect;

	visRgn = NewRgn();
	
	GetPortVisibleRegion(GetDialogWindowPort(d), visRgn);
	GetDialogPortBounds(d,&portRect);

	BeginUpdate(GetDialogWindow(d));
	
	UpdateDialog(d, visRgn);	
	FlushPortRect(&portRect);
	EndUpdate(GetDialogWindow(d));
	
	DisposeRgn(visRgn);

	DrawDialog(d);	
}

/* Lists */

void LUpdateDVisRgn(DialogPtr d, ListHandle lHdl)
{
	RgnHandle visRgn;

	visRgn = NewRgn();
	GetPortVisibleRegion(GetDialogWindowPort(d), visRgn);
	LUpdate(visRgn, lHdl);			
	DisposeRgn(visRgn);
}

void LUpdateWVisRgn(WindowPtr w, ListHandle lHdl)
{
	RgnHandle visRgn;

	visRgn = NewRgn();
	GetPortVisibleRegion(GetWindowPort(w), visRgn);
	LUpdate(visRgn, lHdl);			
	DisposeRgn(visRgn);
}

void FlushPortRect(Rect *r)
{
	GrafPtr port;
	RgnHandle rgn;
	GetPort(&port);
	if (QDIsPortBuffered(port)) {
		rgn = NewRgn();
		RectRgn(rgn, r);
		QDFlushPortBuffer(port, rgn);
		DisposeRgn(rgn);
	}
}

void FlushWindowPortRect(WindowPtr w)
{
	Rect portRect;
	GrafPtr port;
	RgnHandle rgn;
	port = GetWindowPort(w);
	if (QDIsPortBuffered(port)) {
		GetPortBounds(port, &portRect);
		rgn = NewRgn();
		RectRgn(rgn, &portRect);
		QDFlushPortBuffer(port, rgn);
		DisposeRgn(rgn);
	}
}

void FlushDialogPortRect(DialogPtr d)
{
	FlushWindowPortRect(GetDialogWindow(d));
}

/* Port font, face, size, mode */

short GetPortTxFont()
{
	GrafPtr port = GetQDGlobalsThePort();
		
	return GetPortTextFont(port);
}

short GetPortTxFace()
{
	GrafPtr port = GetQDGlobalsThePort();
		
	return GetPortTextFace(port);
}

short GetPortTxSize()
{
	GrafPtr port = GetQDGlobalsThePort();
		
	return GetPortTextSize(port);
}

short GetPortTxMode()
{
	GrafPtr port = GetQDGlobalsThePort();
		
	return GetPortTextMode(port);
}


/* Window font, face, size */

short GetWindowTxFont(WindowPtr w)
{
	return GetPortTextFont(GetWindowPort(w));
}

short GetWindowTxFace(WindowPtr w)
{
	return GetPortTextFace(GetWindowPort(w));
}

short GetWindowTxSize(WindowPtr w)
{
	return GetPortTextSize(GetWindowPort(w));
}

void SetWindowTxFont(WindowPtr w, short font)
{
	SetPortTextFont(GetWindowPort(w), font);
}

void SetWindowTxFace(WindowPtr w, short face)
{
	SetPortTextFace(GetWindowPort(w), face);
}

void SetWindowTxSize(WindowPtr w, short size)
{
	SetPortTextSize(GetWindowPort(w), size);
}


/* Dialog font, face, size */

short GetDialogTxFont(DialogPtr d)
{
	return GetWindowTxFont(GetDialogWindow(d));
}

short GetDialogTxFace(DialogPtr d)
{
	return GetWindowTxFace(GetDialogWindow(d));
}

short GetDialogTxSize(DialogPtr d)
{
	return GetWindowTxSize(GetDialogWindow(d));
}

void SetDialogTxFont(DialogPtr d, short font)
{
	SetWindowTxFont(GetDialogWindow(d), font);
}

void SetDialogTxFace(DialogPtr d, short face)
{
	SetWindowTxFace(GetDialogWindow(d), face);
}

void SetDialogTxSize(DialogPtr d, short size)
{
	SetWindowTxSize(GetDialogWindow(d), size);
}


/* Window kinds */

short IsPaletteKind(WindowPtr w)
{
	return (GetWindowKind(w) == PALETTEKIND);
}

short IsDocumentKind(WindowPtr w)
{
	return (GetWindowKind(w) == DOCUMENTKIND);
}

void SetPaletteKind(WindowPtr w)
{
	SetWindowKind(w, PALETTEKIND);
}

void SetDocumentKind(WindowPtr w)
{
	SetWindowKind(w, DOCUMENTKIND);
}


/* Controls */

void OffsetContrlRect(ControlRef ctrl, short dx, short dy)
{
	Rect contrlRect;
	
	GetControlBounds(ctrl, &contrlRect);
	OffsetRect(&contrlRect, dx, dy);
	SetControlBounds(ctrl, &contrlRect);
}


/* ----------------------------------------------------------------- Log-file handling -- */
/* NB: LOG_DEBUG is the lowest level (=_highest_ internal code: caveat!), so --
according to the man page for syslog(3) -- the call to setlogmask in InitLogPrintf()
_should_ cause all messages to go to the log regardless of level. But on my G5 running
OS 10.5.x (Xcode 2.5), I rarely if ever get anything below LOG_NOTICE; I have no idea
why. That doesn't seem to happen on my MacBook with OS 10.6.x (Xcode 3.2).

To avoid this problem, LogPrintf() changes anything with a priority level lower (=internal
code higher) than MIN_LOG_LEVEL to that level. To disable that feature, set MIN_LOG_LEVEL
to a large number (not a small one!), say 999.  --DAB, July 2017. */

#define MIN_LOG_LEVEL LOG_NOTICE			/* Force lower levels (higher codes) to this */

#define LOG_TO_STDERR False					/* Print to stderr as well as system log? */

#ifdef NOTYET
//FIXME: This just doesn't work.
static Boolean HaveNewline(const char *str);
static Boolean HaveNewline(const char *str)
{
	while (*str!='\0') {
		if (*str=='\n') return True;
		str++;
	}
	return False;
}
#endif

/* There's a weird bug in the Console utility in OS 10.5 where if messages are written
to the log at too high a rate, some just disappear. To make it worse, the ones that
disappear seem to be random, and there's no indication there's a problem! With the 10.6
Console, random messages still disappear, but you at least get a warning after 500
messages in a second; presumably Apple decided that was good enough, and easier than
fixing the bug.

To sidestep the bug, call this function with <doDelay> True to add a delay between
messages. Note that this can make some operations -- e.g., converting large files --
much slower. */
   
static Boolean addLogMsgDelay = False;

void KludgeOS10p5LogDelay(Boolean doDelay)
{
	if (doDelay!=addLogMsgDelay) {
		addLogMsgDelay = doDelay;
		if (addLogMsgDelay)
				LogPrintf(LOG_DEBUG, "****** WORKING SLOWLY! DELAYING TO AID DEBUGGING.  (KludgeOS10p5LogDelay) ******\n");
		else	LogPrintf(LOG_DEBUG, "****** NO LONGER WORKING SLOWLY TO AID DEBUGGING.  (KludgeOS10p5LogDelay) ******\n");
	}
	SleepMS(3);
}

char inStr[1000], outStr[1000];

Boolean VLogPrintf(const char *fmt, va_list argp)
{
	//if (strlen(inStr)+strlen(str)>=1000) return False;	FIXME: How to check for buffer overflow?
	vsprintf(inStr, fmt, argp);
	return True;
}


const char *NameLogLevel(short priLevel, Boolean friendly);
const char *NameLogLevel(
			short priLevel,
			Boolean friendly)		/* True=give user-friendly names, False=give "real" names */
{
	const char *ps;

	switch (priLevel) {
		case LOG_ALERT:		ps = (friendly? "ALERT" : "ALERT");  break;
		case LOG_CRIT:		ps = (friendly? "CRITICAL" : "CRIT");  break;
		case LOG_ERR:		ps = (friendly? "ERROR" : "ERR");  break;
		case LOG_WARNING:	ps = (friendly? "Warning" : "Warn");  break;
		case LOG_NOTICE:	ps = (friendly? "Notice" : "Notice");  break;
		case LOG_INFO:		ps = (friendly? "info" : "info");  break;
		case LOG_DEBUG:		ps = (friendly? "debug" : "debug");  break;
		default:			ps = (friendly? "*UNKNOWN*" : "*UNKNOWN*");
	}

	return ps;
}


/* Display in the system log and maybe on stderr the message described by the second and
 following parameters. The message may contain at most one "\n". There are two cases:
 (1) It may be terminated by "\n", i.e., it may be a full line or a chunk ending a line.
 (2) If it's not terminated by "\n", it's a partial line, to be completed on a later call.
 In case #1, we send the complete line to syslog(); in case #2, just add it to a buffer.
 FIXME: Instances of "\n" other than at the end of the message aren't handled correctly! */

Boolean LogPrintf(short priLevel, const char *fmt, ...)
{
	Boolean okay, endLine;
	const char *ps;
	char levelStr[32];
		
	if (addLogMsgDelay) SleepMS(3);

	/* If we're starting a new line, prefix a code for the level. */
	
	if (strlen(outStr)==0) {
		ps = NameLogLevel(priLevel, True);
		sprintf(levelStr, "%s. ", ps);
		strcat(outStr, levelStr);
	}
	
	if (priLevel>MIN_LOG_LEVEL) priLevel = LOG_NOTICE;		/* Override low priorities */
	va_list argp; 
	va_start(argp, fmt); 
	okay = VLogPrintf(fmt, argp); 
	va_end(argp);
	
	/* inStr now contains the results of this call. Handle both cases. */
	
	strcat(outStr, inStr);
	endLine = (inStr[strlen(inStr)-1]=='\n');
	if (endLine) {
		syslog(priLevel, outStr);
		outStr[0] = '\0';									/* Set <outStr> to empty */
	}
	
	return okay;
}


#define NoTEST_LOGGING_FUNCS
#define NoTEST_LOGGING_LEVELS

short InitLogPrintf()
{
	int logopt = 0;
	if (LOG_TO_STDERR) logopt = LOG_PERROR;
	openlog("Ngale", logopt, LOG_USER);
	
	int oldLevelMask = setlogmask(LOG_UPTO(LOG_DEBUG));
	outStr[0] = '\0';										/* Set <outStr> to empty */
	
#ifdef TEST_LOGGING_FUNCS
	LogPrintf(LOG_INFO, "A simple one-line LogPrintf.\n");
	LogPrintf(LOG_INFO, "Another one-line LogPrintf.\n");
	LogPrintf(LOG_INFO, "This line is ");
	LogPrintf(LOG_INFO, "built from 2 pieces.\n");
	LogPrintf(LOG_INFO, "This line was ");
	LogPrintf(LOG_INFO, "built from 3 ");
	LogPrintf(LOG_INFO, "pieces.\n");
#endif
	
	LogPrintf(LOG_INFO, "oldLevelMask = %d, MIN_LOG_LEVEL=%d  (InitLogPrintf)\n", oldLevelMask, MIN_LOG_LEVEL);
	
#ifdef TEST_LOGGING_LEVELS
	SysBeep(1);
	LogPrintf(LOG_ALERT, "LogPrintf LOG_ALERT message. Magic no. %d  (InitLogPrintf)\n", 1729);
	LogPrintf(LOG_CRIT, "LogPrintf LOG_CRIT message. Magic no. %d  (InitLogPrintf)\n", 1729);
	LogPrintf(LOG_ERR, "LogPrintf LOG_ERR message. Magic no. %d  (InitLogPrintf)\n", 1729);
	LogPrintf(LOG_WARNING, "LogPrintf LOG_WARNING message. Magic nos. %d, %f.  (InitLogPrintf)\n", 1729, 3.14159265359);
	LogPrintf(LOG_NOTICE, "LogPrintf LOG_NOTICE message. Magic no. %d  (InitLogPrintf)\n", 1729);
	LogPrintf(LOG_INFO, "LogPrintf LOG_INFO message. Magic no. %d  (InitLogPrintf)\n", 1729);
	LogPrintf(LOG_DEBUG, "LogPrintf LOG_DEBUG message. Magic no. %d  (InitLogPrintf)\n\n", 1729);
#endif
	
	return oldLevelMask;
}
