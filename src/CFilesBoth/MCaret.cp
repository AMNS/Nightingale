/* MCaret.c for Nightingale */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* These routines are tools for the rest of Nightingale to use to manipulate
the insertion caret.  Each Document has its own caret, which is either
active or inactive, usually in parallel with the Document.  When the caret
is inactive, it is always invisible.  When it is active, it is either
visible or invisible, depending on what part of its blink cycle it's in.
When the caret is visible, it is said to be ON; otherwise OFF. 

We show the caret in the usual way, by alternating the pixels at its location
between normal and inverted (black/white). Originally, we did something very
different, namely alternating the pixels between normal and all black; that
required much more machinery, as described in this comment: "When the caret
goes from OFF to ON, the background bits that are about to be covered are
saved. When it goes back OFF, they are restored.  Since only one caret should
be visible at a time (presumably in the frontmost Document), we can store the
background bits in one global place, which we can initialise regardless of
whether there are any documents open yet." Aside from its general complexity,
this "CopyBits" method crashed mysteriously on IIci's and fx's. Also, it's
not clear that it looks better than the usual way. So to hell with it. */

#define CARET_WIDTH 	1							/* In pixels; normally 1 */

/* MAX_CARET_HEIGHT must be enough for (ascent+descent) in MEMoveCaret in the
greatest possible magnification on the largest possible staff. Currently
(ascent+descent) = 2*staff height, and the largest possible staff is 
rastral 0, whose size is defined by config.rastral0size, and currently can be
up to 72 points. */

#define MAX_CARET_HEIGHT	UseMagnifiedSize(2*72, MAX_MAGNIFY)	/* Maximum caret ht, in pixels */

static GrafPtr caretPort;

/* Prototypes for private tools */

static void METurnCaret(Document *doc, Boolean on);
static void MEMoveCaret(Document *doc, LINK pL, short selx, Boolean now);

/* Initialise whatever is needed to implement the blinking caret.  The CopyBits
method entails allocating an offscreen port to hold a bitmap that contains
background bits to be restored whenever the caret turns OFF.  Delivers
FALSE if out of memory or other error.  The offscreen port is large enough
to hold the largest (tallest) caret. */

Boolean MEInitCaretSystem()
	{
#ifndef COPYBITS_CARET
		return TRUE;
#else
		/* Allocate an offscreen port to hold all background bits */
		
		/* !! THIS NEEDS TO BE A COLOR OFFSCREEN PORT FOR COLOR SYSTEMS !! */
		
		caretPort = NewGrafPort(CARET_WIDTH,MAX_CARET_HEIGHT);
		if (caretPort==NULL) return(FALSE);
#endif
	}

/*
 *	Activate or deactivate (according to 'active') the given Document's caret.
 *	When deactivating, turn caret OFF if it was ON.
 */

void MEActivateCaret(Document *doc, Boolean active)
	{
		if ((doc->caretActive!=0) ^ (active!=0)) {
			if (!active) METurnCaret(doc,active);
			doc->caretActive = active;
			}
	}

/*
 *	Redraw the caret whereever it should appear, but only if it is active
 *	and in its ON blink state.  The current port is assumed to be set to
 *	the document.
 */

void MEUpdateCaret(Document *doc)
	{
		if (doc->caretOn) {			/* Always FALSE if caret inactive */
#ifndef COPYBITS_CARET
			InvertRect(&doc->caret);
#else
			PaintRect(&doc->caret);
#endif
			}
	}

/*
 *	Turn caret OFF if it is ON, but don't deactivate it, so it may become
 *	visible as soon as the next call to MEIdleCaret (presumably in the event
 *	loop).  This is for temporarily hiding the caret during screen activity
 *	resulting from any user commands, e.g., alerts that might cover half of
 *	the caret, leaving us with a caret that's half on and half off.
 */

void MEHideCaret(Document *doc)
	{
		if (doc==NULL) return;
		
		METurnCaret(doc,FALSE);
		doc->nextBlink = TickCount();
	}

/*
 *	Set a given document's active caret to either ON or OFF.
 */

static void METurnCaret(Document *doc, Boolean on)
	{
		GrafPtr oldPort;
		
		if (!doc->caretActive) return;
		
		if ((doc->caretOn!=0) ^ (on!=0)) {
			
			GetPort(&oldPort); SetPort(GetWindowPort(doc->theWindow));
			
#ifndef COPYBITS_CARET
			InvertRect(&doc->caret);
			doc->caretOn = on;
#else
			/* Get document's caret position in offscreen port's coord system */
			caretBox = doc->caret;
			OffsetRect(&caretBox,-caretBox.left,-caretBox.top);
			
			/* Temporarily convert to current sheet-relative coordinate system */
			
			if (doc->caretOn = on) {
				/* Copy onscreen background bits into offscreen caret port */
				CopyBits(&qd.thePort->portBits, &caretPort->portBits,
				 			&doc->caret, &caretBox, srcCopy, NULL);
				/* Draw caret over saved onscreen bits */
				PaintRect(&doc->caret);
				}
			 else
				/* Restore background bits */
				CopyBits(&caretPort->portBits, &qd.thePort->portBits,
				 			&caretBox, &doc->caret, srcCopy, NULL);
			
			/* Restore to background window coordinate system */
			
#ifdef SPASM
			SetBackground(doc);
#endif

#endif
			
			SetPort(oldPort);
			}
		
		doc->nextBlink = TickCount() + GetCaretTime();
	}


/* Change a document's caret from its current state to the opposite if enough
time has passed since the last change and if there's no current selection.
Like TEIdle, should be called frequently from the event loop. */

void MEIdleCaret(Document *doc)
	{
		if (doc->selStartL == doc->selEndL)
			if (TickCount() >= doc->nextBlink) METurnCaret(doc,!doc->caretOn);
	}


/* Given a Document that currently has an insertion point, compute the new caret
position that represents that object (selStartL) and update the caret to place
it there. If the Document does not have an insertion point, e.g., the selection
range is not empty, this will NOT fix it!

Note that FixEmptySelection tries to avoid putting the caret in the middle of
a symbol; we don't do that here but maybe we should. */

void MEAdjustCaret(Document *doc, Boolean moveNow)
	{
	   CONTEXT context;
	   DDIST xObj;
	   short selx, xoffset;
	   LINK pL;
	 
		if (doc->masterView || doc->showFormat)
			return;

		if (doc->selStaff<1 || doc->selStaff>doc->nstaves) {
			MayErrMsg("MEAdjustCaret: selStaff=%ld is illegal.", (long)doc->selStaff);
			return;
			}

		switch(ObjLType(doc->selStartL)) {
			/*
			 * In most cases, put the caret just before the object at the insertion pt.
			 */
			case SYNCtype:
			case GRSYNCtype:
		   case CLEFtype:
			case DYNAMtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case GRAPHICtype:
			case TEMPOtype:
			case SPACEtype:
			case RPTENDtype:
			case ENDINGtype:
			case PSMEAStype:
		   case MEASUREtype:
				GetContext(doc, doc->selStartL, doc->selStaff, &context);
				xObj = PageRelxd(doc->selStartL, &context);
		      break;
		      
			case PAGEtype:
		   case SYSTEMtype:
		   case TAILtype:
		   	/*
		   	 *	For these types, putting the caret before the object at the insertion
		   	 * point won't work. For Systems and Pages, that would be the beginning of
		   	 * the next System; for the tail of the object list, its position is usually
		   	 * unknown (though we could call UpdateTailxd here but it doesn't work that
		   	 * well as of v.995). Instead, try to position the caret after the preceding
		   	 * object. For now, put it a fixed distance after the left edge of that obj:
		   	 * this will usually be OK but may be too far right and past the end of the
		   	 *	staff, or too far left and overlapping the obj if it's very wide. Sigh.
		   	 */
				GetContext(doc, LeftLINK(doc->selStartL), doc->selStaff, &context);
				xObj = PageRelxd(LeftLINK(doc->selStartL), &context);
				xObj += 3*LNSPACE(&context);
		      break;
		      
		  	case BEAMSETtype:
		  	case SLURtype:
		  	case TUPLETtype:
		  	case OCTAVAtype:
				pL = FirstValidxd(RightLINK(doc->selStartL), FALSE);
				GetContext(doc, pL, doc->selStaff, &context);
				xObj = PageRelxd(pL, &context);
		      break;
		      
			default:
#ifndef PUBLIC_VERSION
				MayErrMsg("MEAdjustCaret: can't handle type %ld",
							(long)(ObjLType(doc->selStartL)));
#endif
				return;		/* Don't call MEMoveCaret() below */
	  		}

		/*
		 * Compute an offset to put the caret a few points to the left of the position
		 *	we just found.
		 */
		xoffset = d2p(LNSPACE(&context));
		
		selx = d2p(xObj)-xoffset;
		MEMoveCaret(doc, doc->selStartL, selx, moveNow);
	}


/* Assuming the given point <selPt> in window-relative pixels is a new insertion
point, find an object representing that position in the data structure. Use the
staff <selPt.v> is on or closest to. */

LINK Point2InsPt(Document *doc, Point selPt)
{
	LINK pL;
	
	/* If GSSearch finds a LINK, get the head of its slot; otherwise call FindSymRight
		to get the next symbol (presumably the following System or Page). */

	pL = GSSearch(doc, selPt, ANYTYPE, ANYONE, GO_RIGHT, FALSE, FALSE, FALSE);	 /* #1 */
	if (pL) 	pL = LocateInsertPt(pL);
	else		pL = FindSymRight(doc, selPt, FALSE, TRUE);
	
	return pL;
}


/*
 *	Move the given document's caret to a new point whose x-coord. in window-relative
 * pixels is <selPt.h> and whose y-coord. is the staff <selPt.v> is on or closest to.
 *	If moveNow is TRUE, draw the caret in its new position immediately; otherwise, it
 *	will presumably be drawn at MEIdle time. Delivers the link to an object representing
 *	the new insertion position in the data structure.
 *
 * #1. Next-to-last parameter to GSSearch is useJD.
 * Previous situation: If FALSE, which was passed to FindSymRight before v.97,
 * sets selStartL to link following a J_D object when user sets insertion point
 * immediately before that object, as in a click just before a beamset. This
 * caused a bug in pasting, because the pasted-in range is inserted between
 * the J_D object and its owned Syncs.
 * Current situation: As of v.997, all J_D symbols (except CONNECTs, whose J_Type
 * should be changed) are located in a d.s. slot before the J_IT/J_IP objs
 * they depend on. Call GSSearch with useJD FALSE, and then call LocateInsertPt
 * to get the insertion point at the head of the slot, prior to any J_D objects
 * which depend on the J_IT/J_IP obj found.
 */

LINK MESetCaret(Document *doc, Point selPt, Boolean moveNow)
	{
		LINK pL;
		
		pL = Point2InsPt(doc, selPt);
		MEMoveCaret(doc, pL, selPt.h, moveNow);
		
		return pL;
	}


/* Move the given document's caret to a new point whose x-coord. in window-relative
pixels is <selx> and whose y-coord. is determined by doc->selStaff.  If moveNow
is TRUE, draw the caret in its new position immediately; otherwise, it will
presumably be drawn at MEIdle time. */

static void MEMoveCaret(Document *doc, LINK pL, short selx, Boolean moveNow)
	{
		short			yd, ascent, descent;
		CONTEXT		context;
		
		MEHideCaret(doc);
		 	
		GetContext(doc, pL, doc->selStaff, &context);
		yd = context.staffTop;

		/* If you increase ascent or descent, you may need to increase MAX_CARET_HEIGHT. */
		
		ascent = .5*context.staffHeight;
		descent = 1.5*context.staffHeight;
		SetRect(&doc->caret,selx,d2p(yd-ascent), selx+CARET_WIDTH, d2p(yd+descent));
		/* doc->caret is kept in window coordinates */
		OffsetRect(&doc->caret,context.paper.left,context.paper.top);
		if (moveNow) METurnCaret(doc,TRUE);
	}
