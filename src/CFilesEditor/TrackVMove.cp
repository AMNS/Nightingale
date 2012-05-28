/* TrackVMove.c for Nightingale - rev. for v.3.1 */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1990-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "MidiMap.h"

/* ================================================= AltInsTrackPitch and helpers == */

#define NO_EMPTY_BOXES 0			/* always get rid of box when showing no accidental */
#define ACCELERATE 1

#define NO_ACCIDENTAL	0
#define MIN_ACCSIZE		14		/* Feedback accidentals will be no smaller than this pt size */
#define CH_MODE_CHNG		'\t'	/* Character code that will change note insertion mode. */
		/* ??? NB: belongs in NTypes.h, since accMode should be global. */

#define INCL_DBL_ACCS FALSE

#define ACC_DOWN			-1
#define ACC_UP				1
#define MAX_INDEX			5		/* same for local and global (SonataAcc[]) lists of accidentals */

static enum {
	DIATONIC_MODE,
	CHROMATIC_MODE
	} E_TrackVMode;

/* Local prototypes */
static void DrawAccInBox(INT16, INT16, INT16, Rect *);
static void CalcAccBox(Document *, PCONTEXT, Point *, Rect *, INT16 *);
static void AltCalcAccidental(INT16 *, INT16 *, SignedByte, Boolean, INT16, INT16, Boolean);
static void PaperMouse2Global(Point *, Rect *);
static void SetMouse(INT16 x, INT16 y, Boolean updateCrs);


/* ------------------------------------------------------------- AltInsTrackPitch -- */
/* For notes and rests. Tracks the cursor up and down the staff, giving appro-
priate feedback: audio and ledger lines, plus a rectangle sliding up and down
showing where the symbol would go.  The global <accMode>, toggled by hitting
a key on the keyboard (currently tab), determines whether the tracking mech-
anism will change the accidentals as well as the pitch letter name. Another
global, <dblAccidents> determines whether double sharps and flats will be
included in the sequence of accidentals displayed.
Returns in parameters the half-line pitch level and the index into the global
SonataAcc[] of the chosen accidental (0=none, 1...5=dbl-flat thru dbl-sharp).
If the cursor's final position is outside the music area, returns FALSE to
allow the caller to cancel the operation. If called with a rest, does minimal
tracking, allowing the user to cancel, but giving no feedback. If the cursor
is inside the music area, returns TRUE.

WARNING: This routine calls SetMouse, which stores into low-memory globals.

NOTE: The standard MIDIFBNoteOn and MIDIFBNoteOff include an intentional delay;
that was slowing this routine down AltInsTrackPitch in chromatic mode. So this
file used to contain special versions without the delay; of course, they got
out of sync with the standard versions. A better solution would be to add a
parameter to MIDIFBNoteOn and MIDIFBNoteOff to tell them whether to delay or
not, or possibly to have the delay controlled by config.altPitchCtl. */

Boolean AltInsTrackPitch(Document *, Point, INT16 *, LINK, INT16, INT16 *, INT16 *,
									INT16, Boolean);
Boolean AltInsTrackPitch(
				Document *doc,
				Point		downPt,			/* Location of mousedown */
				INT16		*symIndex,		/* Symbol table index of note/rest to be inserted */
				LINK		startL,			/* link to node before which note/rest will go */
				INT16		staff,
				INT16		*pHalfLn,		/* pitch level return value */
				INT16		*pAccident,		/* accidental return value */
				INT16		octType,			/* type of octava note is to be inserted into. */
				Boolean	inclDblAccs 	/* include dbl-sharp and dbl-flat in acc. sequence? */
				)
{
	INT16		noteNum,
				oldh, oldv, deltaV,
				accident, accidentOld, prevAccident,
				accSize, accBoxWid, boxVctr,
				halfLn, halfLnOld, halfLnOrig, tempHalfLn,
				halfLnLo, halfLnHi, middleCHalfLn, hiPitchLim, loPitchLim,
				durIndex, durIndexOld,
				partn, useChan, octTransp, transp;
	short    useIORefNum=0;	/* NB: both OMSUniqueID and fmsUniqueID are unsigned short */
	Boolean	outOfBounds, sharpKS, halfLnAccel = FALSE;
	SignedByte direction, accidentStep, extraTime;
	long		thisTime, lastTime, deltaTime;
	Point		pt, tempPt;
	Rect		accBox, saveBox, tRect;
	GrafPtr	savePort, ourPort;
	CONTEXT	context;
	LINK		partL, staffL;
	PPARTINFO	pPart;
	/* ??? accMode should be a global, set in evt loop as well as in this routine. */
	static SignedByte	accMode = CHROMATIC_MODE;
	EventRecord evt;
	
	const BitMap *ourPortBits = NULL;
	const BitMap *savePortBits = NULL;
	GWorldPtr gwPtr = NULL;
	
	
	ourPort = GetDocWindowPort(doc);

	GetContext(doc, LeftLINK(startL), staff, &context);				/* Get context */
	GetInsRect(doc, startL, &tRect);

	/* If it's a rest, enter this tracking loop now; we don't need any more info.
	 * The loop allows the user to cancel the rest insertion. For feedback, we show
	 * the crossbar at a fixed location (2nd space), then erase it whenever the
	 * mouse is outside of the music area.
	 */
	if (symtable[*symIndex].subtype!=0) {					/* Is it a rest? */
		outOfBounds = FALSE;
		SetCrossbar(3, downPt.h, &context);					/* Calculate ccrossbar rect */
		InvertCrossbar(doc);										/* Show it */
		do {
			GetPaperMouse(&pt, &context.paper);
			if (!PtInRect(pt, &tRect)) {						/* Mouse out of System? */
				outOfBounds = TRUE;
				InvertCrossbar(doc);
				do {
					GetPaperMouse(&pt, &context.paper);
				} while (!PtInRect(pt, &tRect) && WaitMouseUp());
				InvertCrossbar(doc);
			}
			else outOfBounds = FALSE;
		} while (WaitMouseUp()); 
		*pHalfLn = 0;
		*pAccident = 0;
		InvertCrossbar(doc);
		return !outOfBounds;
	}


	/* If it's a note, we need more info and a much more comprehensive tracking loop. */
	
	staffL = LSSearch(startL, STAFFtype, ANYONE, GO_LEFT, FALSE);
	halfLn = CalcPitchLevel(downPt.v, &context, staffL, staff);
	halfLnOrig = halfLn;
	hiPitchLim = GetPitchLim(doc, staffL, staff, TRUE);
	loPitchLim = GetPitchLim(doc, staffL, staff, FALSE);

	durIndex = *symIndex;
	partn = Staff2Part(doc, staff);
	useChan = UseMIDIChannel(doc, partn);									/* Set feedback channel no. */

	octTransp = (octType>0? noteOffset[octType-1] : 0);				/* Set feedback transposition */
	partL = FindPartInfo(doc, partn);
	pPart = GetPPARTINFO(partL);
	if (doc->transposed) transp = pPart->transpose;
	else						transp = 0;

	if (context.nKSItems==0) sharpKS = TRUE;						/* arbitrary for no key sig */
	else {
		if (context.KSItem[0].sharp) sharpKS = TRUE;				/* key sig begins with sharp */
		else sharpKS = FALSE;											/* key sig begins with flat */
	}
	
	pt.v = downPt.v;
	accident = prevAccident = NO_ACCIDENTAL;
	outOfBounds = FALSE;

	middleCHalfLn = ClefMiddleCHalfLn(context.clefType);
	if (ACC_IN_CONTEXT) {
		GetPitchTable(doc, accTable, startL, staff);						/* Get pitch mod. situation */
		prevAccident = accTable[halfLn-middleCHalfLn+ACCTABLE_OFF];	/* Effective acc. here */			
	}
	noteNum = Pitch2MIDI(middleCHalfLn-halfLn+octTransp, prevAccident)+transp;
	MIDIFBOn(doc);
	MIDIFBNoteOn(doc, noteNum, useChan, useIORefNum);
	InsertLedgers(p2d(downPt.h), halfLn, &context);
	halfLnLo = halfLnHi = halfLn;

	SetCrossbar(halfLn, downPt.h, &context);						/* Calculate ccrossbar rect */
	InvertCrossbar(doc);													/* Show it */

	CalcAccBox(doc, &context, &downPt, &accBox, &accSize);
	accBoxWid = accBox.right-accBox.left;
	boxVctr = downPt.v;
	/* Convert accBox to window coordinates */
	OffsetRect(&accBox,context.paper.left,context.paper.top);
	SetRect(&saveBox, 0, 0, accBox.right-accBox.left, accBox.bottom-accBox.top);

#ifdef USE_GRAFPORT
	savePort = NewGrafPort(saveBox.right+1, saveBox.bottom+1);

	ourPortBits = GetPortBitMapForCopyBits(ourPort);
	savePortBits = GetPortBitMapForCopyBits(savePort);
	CopyBits(ourPortBits,savePortBits, &accBox, &saveBox, srcCopy, NULL);
	
#else
	SaveGWorld();
	
	gwPtr = MakeGWorld(saveBox.right+1, saveBox.bottom+1, TRUE);
	savePort = gwPtr;

	ourPortBits = GetPortBitMapForCopyBits(ourPort);
	savePortBits = GetPortBitMapForCopyBits(savePort);
	CopyBits(ourPortBits,savePortBits, &accBox, &saveBox, srcCopy, NULL);
	
	RestoreGWorld();
#endif // USE_GRAFPORT

//	CopyBits(&ourPort->portBits, &savePort->portBits,
//				&accBox, &saveBox, srcCopy, NULL);

	oldh = downPt.h;
	oldv = downPt.v;
	
	lastTime = 0;
	
	/* Tracking loop. When anything changes, handle it, then go back to waiting for changes. */
	
	if (StillDown()) {
		if (accMode==CHROMATIC_MODE) {
			if (accident!=NO_ACCIDENTAL)
				DrawAccInBox(accident, accSize, context.paper.top+boxVctr, &accBox);
			HideCursor();
		}
		while (WaitMouseUp()) {
			halfLnOld = halfLn;
			accidentOld = accident;
			durIndexOld = durIndex;

			/* Watch for ANY change */
			do {
				GetPaperMouse(&pt, &context.paper);
				
				if (!PtInRect(pt, &tRect)) {					/* Mouse out of System? */
					outOfBounds = TRUE;
					MIDIFBNoteOff(doc, noteNum, useChan, useIORefNum);
					InvertCrossbar(doc);
					if (accMode==CHROMATIC_MODE) {
						ourPortBits = GetPortBitMapForCopyBits(ourPort);
						savePortBits = GetPortBitMapForCopyBits(savePort);
						CopyBits(savePortBits, ourPortBits, &saveBox, &accBox, srcCopy, NULL);
						
//						CopyBits(&savePort->portBits, &ourPort->portBits,
//									&saveBox, &accBox, srcCopy, NULL);
						ShowCursor();
					}
					do {
						GetPaperMouse(&pt, &context.paper);
					} while (!PtInRect(pt, &tRect) && WaitMouseUp());
					InvertCrossbar(doc);
					if (accMode==CHROMATIC_MODE) HideCursor();
					break;
				}
				else outOfBounds = FALSE;
				
				/* Change mode if we get a CH_MODE_CHNG key event */
				if (GetNextEvent(keyDownMask, &evt)) {
					if ((evt.what==keyDown) && ((unsigned char)evt.message==CH_MODE_CHNG)) {
						accMode = accMode==DIATONIC_MODE? CHROMATIC_MODE : DIATONIC_MODE;					
						if (accMode==CHROMATIC_MODE) {
							OffsetRect(&accBox, 0, pt.v-boxVctr);
							boxVctr = pt.v;
							
							ourPortBits = GetPortBitMapForCopyBits(ourPort);
							savePortBits = GetPortBitMapForCopyBits(savePort);
							CopyBits(ourPortBits, savePortBits, &accBox, &saveBox, srcCopy, NULL);
						
//							CopyBits(&ourPort->portBits, &savePort->portBits,
//										&accBox, &saveBox, srcCopy, NULL);
							accident = NO_ACCIDENTAL;					/* reset to no accidental */
							HideCursor();
						}
						else {
							ourPortBits = GetPortBitMapForCopyBits(ourPort);
							savePortBits = GetPortBitMapForCopyBits(savePort);
							CopyBits(savePortBits, ourPortBits, &saveBox, &accBox, srcCopy, NULL);
						
//							CopyBits(&savePort->portBits, &ourPort->portBits,
//										&saveBox, &accBox, srcCopy, NULL);
							ShowCursor();
						}
					}
				}
				
				/* ??Should allow only in DIATONIC_MODE? */
				if (CmdKeyDown())
					durIndex = CalcNewDurIndex(*symIndex, pt.h, oldh);
					
				else {
					if (accMode==CHROMATIC_MODE)	{
#if ACCELERATE
						/* Determine number of pixels mouse must move to change accidental.
						 * The quicker the mouse moves, the fewer the number of pixels. */
						thisTime = TickCount();
						deltaTime = thisTime-lastTime;
						lastTime = thisTime;
						extraTime = doc->feedback? 2 : 0;
						if		  (deltaTime<6+extraTime)  accidentStep = 1;
						else if (deltaTime<10+extraTime) accidentStep = 2;
						else if (deltaTime<20+extraTime) accidentStep = 3;
						else										accidentStep = 4;
/* say("%d\n", accidentStep); */
#else
						accidentStep = 2;
#endif				
						for (deltaV = 0; ABS(deltaV)<accidentStep && WaitMouseUp(); ) {
							if (EventAvail(keyDownMask, &evt)) break;		/* in case user hits mode change key */
							GetPaperMouse(&tempPt, &context.paper);
							deltaV = tempPt.v-pt.v;
						}
						direction = deltaV>0? ACC_DOWN : ACC_UP;
						
#if ACCELERATE
						/* Extra bump(s) for maximum acceleration. */
						if (accidentStep==1 && ABS(deltaV)>2) {
#if 0
							if      (ABS(deltaV)<5) halfLn -= direction;		/* jump by a second */
							else if (ABS(deltaV)<8) halfLn -= direction*2;	/* jump by a third */
							else if (ABS(deltaV)<10) halfLn -= direction*3;	/* etc. */
							else if (ABS(deltaV)<15) halfLn -= direction*4;
							else if (ABS(deltaV)<24) halfLn -= direction*5;
							else							 halfLn -= direction*(ABS(deltaV)>>2);
#else
							halfLn -= direction*(ABS(deltaV)>>1);
#endif
							halfLnAccel = TRUE;
						}
						else halfLnAccel = FALSE;

						SleepTicks(1L);			/* Rein in fast machines. (??Where should this go?) */
#endif

						/* must test deltaV in case we're here due to mouseUp in prev. for loop */
						if (ABS(deltaV)>=accidentStep) {
							tempHalfLn = halfLn;

							AltCalcAccidental(&accident, &tempHalfLn, direction,
												sharpKS, loPitchLim, hiPitchLim, inclDblAccs);
							if (halfLnAccel) accident = NO_ACCIDENTAL;			/* override AltCalcAccidental's accidental */
								
							/* Don't reset mouse, so user can move into cancel area. */
							if (halfLnAccel && (tempHalfLn==hiPitchLim || tempHalfLn==loPitchLim))
								halfLn = tempHalfLn;
								
							else if (tempHalfLn != halfLn) {
								halfLn = tempHalfLn;
								/* Move mouse to next halfLn. */
/* ??? But crossbar hasn't been moved yet! This often results in crossbar
	jumping by one halfLn when going from chromatic mode to diatonic mode. */
								tempPt.h = ccrossbar.left + ((ccrossbar.right-ccrossbar.left)>>1);
								tempPt.v = ccrossbar.top + ((ccrossbar.bottom-ccrossbar.top)>>1);
								PaperMouse2Global(&tempPt, &context.paper);
								SetMouse(tempPt.h, tempPt.v, FALSE);
							}
							else {
								tempPt = pt;
								PaperMouse2Global(&tempPt, &context.paper);
								/* Keep mouse "running in place" as we change accidentals. */
								SetMouse(tempPt.h, tempPt.v, FALSE);
							}
						}
					}
					else {								/* We're in diatonic mode. */
						accident = NO_ACCIDENTAL;
						halfLn = CalcPitchLevel(pt.v, &context, staffL, staff);
					}
					oldv = pt.v;
					durIndex = *symIndex;
				}
			} while (WaitMouseUp() &&
						halfLn==halfLnOld &&
						accident==accidentOld &&
						durIndex==durIndexOld); 

			if (WaitMouseUp()) {											/* Mouse still down? */						

				/* Change of duration? */
				if (durIndex!=durIndexOld) {
					PalKey(symtable[durIndex].symcode);
					currentCursor = cursorList[durIndex];
					SetCursor(*currentCursor);
					if (accMode==CHROMATIC_MODE) {		/* ??? This isn't enough! */
						accMode = DIATONIC_MODE;
						
						ourPortBits = GetPortBitMapForCopyBits(ourPort);
						savePortBits = GetPortBitMapForCopyBits(savePort);
						CopyBits(savePortBits, ourPortBits, &saveBox, &accBox, srcCopy, NULL);
						
//						CopyBits(&savePort->portBits, &ourPort->portBits,
//									&saveBox, &accBox, srcCopy, NULL);
						ShowCursor();
					}
				}
				
				/* Not dur.change, must be pitch change */
				else {
				
					/* Pitch level change? */
					if (halfLn!=halfLnOld)	{
						InvertCrossbar(doc);									/* Move ccrossbar to new pitch */
						SetCrossbar(halfLn, downPt.h, &context);
						if (halfLn<halfLnLo) {							/* do we need new ledger lines? */
							halfLnLo = halfLn;
							InsertLedgers(p2d(downPt.h), halfLn, &context);
						}
						else if (halfLn>halfLnHi) {
							halfLnHi = halfLn;
							InsertLedgers(p2d(downPt.h), halfLn, &context);
						}
						InvertCrossbar(doc);
					}
					
					/* Accidental change? */
					if (accMode==CHROMATIC_MODE /* && accident!=accidentOld */)	{			/* Change of accidental? */
#if NO_EMPTY_BOXES
						if (accident==NO_ACCIDENTAL)				/* get rid of box */
							CopyBits(&savePort->portBits, &ourPort->portBits,
										&saveBox, &accBox, srcCopy, NULL);
						else DrawAccInBox(accident, accSize, context.paper.top+boxVctr, &accBox);
#else
						DrawAccInBox(accident, accSize, context.paper.top+boxVctr, &accBox);
#endif
					}
					
					/* Play new MIDI note. */
					MIDIFBNoteOff(doc, noteNum, useChan, useIORefNum);
					if (accident==NO_ACCIDENTAL) {
						if (ACC_IN_CONTEXT)
							prevAccident = accTable[halfLn-middleCHalfLn+ACCTABLE_OFF];			
						noteNum = Pitch2MIDI(middleCHalfLn-halfLn+octTransp, prevAccident)
										+transp;
					}
					else
						noteNum = Pitch2MIDI(middleCHalfLn-halfLn+octTransp, accident)
										+transp;
					MIDIFBNoteOn(doc, noteNum, useChan, useIORefNum);
				}				
			}			
		}
	}
	
	InvertCrossbar(doc);
	MIDIFBNoteOff(doc, noteNum, useChan, useIORefNum);
	MIDIFBOff(doc);
	
	/* Restore bits under accbox so we'll never have to inval prev meas. */
	ourPortBits = GetPortBitMapForCopyBits(ourPort);
	savePortBits = GetPortBitMapForCopyBits(savePort);
	CopyBits(savePortBits, ourPortBits, &saveBox, &accBox, srcCopy, NULL);
						
//	CopyBits(&savePort->portBits, &ourPort->portBits,
//				&saveBox, &accBox, srcCopy, NULL);
#ifdef USE_GRAFPORT
	DisposGrafPort(savePort);										/* done with this */
#else
	DestroyGWorld(gwPtr);
#endif // USE_GRAFPORT

	if (accMode==CHROMATIC_MODE) ShowCursor();
	
	if (outOfBounds) {
		*pHalfLn = halfLnOrig;
		*pAccident = accident;
		return FALSE;
	}
	else {
		*pHalfLn = halfLn;
		*pAccident = accident;
		*symIndex = durIndex;
		return TRUE;
	}
}


/* ---------------------------------------------------------------- DrawAccInBox -- */
/* DrawAccInBox draws the given accidental, expressed as an index into the
global array of accidental characters (SonataAcc) in the given rect in the
current port using the given size of the Sonata font. */

void DrawAccInBox(INT16 accIndex, INT16 fontSize, INT16 baselineV, Rect *accBox)
{
	INT16 hoffset, boxWid;
	Rect	eraseBox;
	
	if (accIndex>MAX_INDEX) {
		MayErrMsg("DrawAccInBox: bad accidental index.");
		return;
	}
	boxWid = accBox->right - accBox->left;
	eraseBox = *accBox;
	InsetRect(&eraseBox, 1, 1);
	EraseRect(&eraseBox);
	FrameRect(accBox);

// FIXME: But probably we should use doc->musicFontNum here, and make other adjustments (MapMusChar, etc.)
	TextFont(sonataFontNum);					/* Set to Sonata font */
	TextSize(fontSize);
	hoffset = (boxWid-CharWidth(SonataAcc[accIndex]))>>1;
	MoveTo(accBox->left+hoffset, baselineV);
	DrawChar(SonataAcc[accIndex]);				/* Draw new accidental */
}


/* ------------------------------------------------------------------ CalcAccBox -- */
/* CalcAccBox determines the dimensions and placement of the accidental box and
the font size used to display the accidentals, both of which it passes back
to the caller.  NB: Currently no distinction based on whether box will include
doubleflat. */

void CalcAccBox(Document *doc, PCONTEXT pCont, Point *vCenterPt, Rect *accBoxR, 
						INT16 *accSize)
{
	INT16		accy, accBoxWid, accBoxHt, fontSize;
	FontInfo	fInfo;
	
	/* Vertically center box about point of mouseDown. */
	accy = vCenterPt->v;

	/* Size relative to magnification, but not smaller than MIN_ACCSIZE */
	fontSize = UseMagnifiedSize(pCont->fontSize, doc->magnify);
	*accSize = n_max(MIN_ACCSIZE, fontSize);

	TextFont(doc->musicFontNum);
	TextSize(*accSize);
	GetFontInfo(&fInfo);

#if 1
	/* Assign box widths for various font sizes */
	if (*accSize<=14)			{ accBoxWid = 12;	accBoxHt = 20; }
	else if (*accSize<=18)	{ accBoxWid = 13;	accBoxHt = 22; }
	else if (*accSize<=20)	{ accBoxWid = 16;	accBoxHt = 24; }
	else if (*accSize<=24)	{ accBoxWid = 15;	accBoxHt = 26; }
	else if (*accSize<=28)	{ accBoxWid = 18;	accBoxHt = 36; }
	else if (*accSize<=36)	{ accBoxWid = 23;	accBoxHt = 40; }
	else if (*accSize<=40)	{ accBoxWid = 28;	accBoxHt = 48; }
	else if (*accSize<=48)	{ accBoxWid = 26;	accBoxHt = 52; }
	else if (*accSize<=56)	{ accBoxWid = 38;	accBoxHt = 72; }
	else if (*accSize<=72)	{ accBoxWid = 48;	accBoxHt = 84; }
	else if (*accSize<=96)	{ accBoxWid = 56;	accBoxHt = 102; }
	else							{ accBoxWid = 56;	accBoxHt = 118; }
#else
/* Might be able to perform a general calculation for both width and height.
 * NB: would be relative to fontSize, not magnify, since fontSize will never
 * be smaller than MIN_ACCSIZE, regardless of magnify.
 */
	accBoxWid = *accSize-1;
	accBoxHt = (fInfo.ascent+fInfo.descent)/3;
#endif

	accBoxR->left = ccrossbar.left-2-accBoxWid;		
	accBoxR->right = ccrossbar.left-2;
	accBoxR->bottom = accy+(accBoxHt>>1)-1;
	accBoxR->top = accy-(accBoxHt>>1)-1;
}


/* Minimum and maximum indices into local accidental arrays, depending on whether
double-flats and -sharps are included. */

#define MAX_ACC_NORM		4
#define MAX_ACC_EXT		5
#define MIN_ACC_NORM		1
#define MIN_ACC_EXT		0

/* Local arrays of accidentals, depending on key signature in context. */
/* char SonataAcc[] = { ' ', 0xBA,'b','n','#',0xDC }; 	For comparison only! */
static char ShrpKSaccs[] = {0xBA,'b','n',' ','#',0xDC};
static char FlatKSaccs[] = {0xBA,'b',' ','n','#',0xDC};

/* ----------------------------------------------------------- AltCalcAccidental -- */
/* AltCalcAccidental takes the index into the global SonataAcc array of the
current accidental and changes it depending on the direction the mouse is
moving, the current key signature, and whether the user wants to see double
flats and double sharps.  If user reaches a minimum or maximum accidental,
we wrap around to the other end and bump the half line position, which we
pass back to the caller for further action.  Passes back an index into the
global SonataAcc array. */

void AltCalcAccidental(
			INT16		*pAcc,			/* Current index into global SonataAcc[] */
			INT16		*pHalfLn,		/* Current crossbar position, changed in this routine */
			SignedByte direction,	/* Which way is mouse going? (1: up, -1: down) */
			Boolean	sharpKS,			/* Does current key sig contain sharps? (What about C major?) */
			INT16		loPitchLim,		/* Pitch range in halfLines */
			INT16		hiPitchLim,
			Boolean	inclDblAccs 	/* Include dbl-sharps and dbl-flats? */
			)
{
	SignedByte minIndex, maxIndex, locIndex;
	INT16		accident, halfLn;
	char		*accList;
	Boolean	bumpHalfLn = FALSE;
	register INT16 i;

	if (*pAcc>MAX_INDEX) {
		MayErrMsg("AltCalcAccidental: bad accidental index.");
		return;
	}
	
	/* Initialize some local variables */
	halfLn = *pHalfLn;
	accident = *pAcc;
	accList = sharpKS? ShrpKSaccs : FlatKSaccs;
	minIndex = inclDblAccs? MIN_ACC_EXT : MIN_ACC_NORM;	
	maxIndex = inclDblAccs? MAX_ACC_EXT : MAX_ACC_NORM;
	
	/* Convert current accidental to an index into local accidental list. */
	for (i=0; i<=MAX_INDEX; i++)
		if (accList[i]==SonataAcc[accident]) break;
	locIndex = i;
	
#if 0 /* ???NECESSARY? */
	/* For dragging, note may have dbl-acc even if inclDblAccs==FALSE */
	if			(!inclDblAccs && locIndex==MAX_ACC_EXT) locIndex--;
	else if	(!inclDblAccs && locIndex==MIN_ACC_EXT) locIndex++;
#endif
	
	/* Bump index depending on direction. */
	locIndex += direction;
	
	/* If we've hit a maximum or minimum index, unless we're at either
	 * pitch limit, wrap around to accidental on the other end and bump
	 * halfLn so that crossbar will be moved by caller.
	 */
	if (halfLn<hiPitchLim) halfLn = hiPitchLim;		/* due to halfLn acceleration in InsTrackPitch */
	else if (halfLn>loPitchLim) halfLn = loPitchLim;
	if (direction==ACC_UP && locIndex>maxIndex) {
		if (halfLn==hiPitchLim)
			locIndex--;											/* restore *pAcc */
		else {
			locIndex = minIndex;
			bumpHalfLn = TRUE;
		}
	}
	else if (direction==ACC_DOWN && locIndex<minIndex) {
		if (halfLn==loPitchLim)
			locIndex++;
		else {
			locIndex = maxIndex;
			bumpHalfLn = TRUE;
		}
	}
	
	if (bumpHalfLn) halfLn -= direction;
	
	/* Convert local into global accidental index. */	
	for (i=0; i<=MAX_INDEX; i++)
		if (SonataAcc[i]==accList[locIndex]) break;
	accident = i;
	
	*pHalfLn = halfLn;
	*pAcc = accident;
}


/* ----------------------------------------------------------- PaperMouse2Global -- */
/* Convert paper-relative point to global coordinates.  Paper is a rectangle
in window-relative coordinates. */

void PaperMouse2Global(Point *pt, Rect *paper)
{	
	pt->h += paper->left;		/* Transform to window-relative coords */
	pt->v += paper->top;
	LocalToGlobal(pt);
}


/* -------------------------------------------------------------------- SetMouse -- */
/* Move the mouse cursor to <x>, <y> (in global coordinates), optionally updat-
ing the cursor position.  Takes effect immediately. Adapted from code by
Joe Holt of Adobe Systems, which is based on Apple Sample Code #17.
NB: Radius screen doesn't respond to updating the cursor; it catches up later
on its own accord.  They must be doing some mouse moving of their own. ??Belongs
in a new "system-specific" file. */

/* Low memory globals */
/*
extern Point		MTemp : 0x828;
extern Point		RawMouse : 0x82C;
extern Point		Mouse : 0x830;
extern Byte			CrsrNew : 0x8CE;
extern Byte			CrsrCouple : 0x8CF;
*/

// Use CoreGraphics for SetMouse (MAS, see http://lists.apple.com/archives/Carbon-development/2001/Aug/msg00738.html)
void SetMouse(INT16 x, INT16 y, Boolean updateCrs)
{
	//Point	newPt;
	CGPoint newPt;
	
	//newPt.h = x;	newPt.v = y;
	newPt.x = x;  newPt.y = y;
	
	/*	
		MTemp = newPt;
	 RawMouse = newPt;
	 if (!updateCrs) {
		 Mouse = newPt;
		 return;
	 }
	 CrsrNew = CrsrCouple;
	 */
	CGWarpMouseCursorPosition(newPt);
	//#if 0
	//	Delay(5, &dummy);				/* let cursor catch up (suggested by Mathew Mora) */
	//#endif
}

#define CM_CHANNEL_BASE 1

/* ---------------------------------------------------------------- InsTrackPitch -- */
/* For notes and rests. For notes (as indicated by *symIndex), track the cursor up
and down the staff, giving appropriate feedback: audio and ledger lines, plus a
rectangle sliding up and down showing where the symbol would go.  The shift key
invokes a "spring-loaded" accidental entry submode; if *symIndex is not negative,
the command key invokes a spring-loaded mode that changes duration when the mouse
is moved horizontally.

If the cursor's final position is outside the music area, returns FALSE to allow the
caller to cancel the operation. Otherwise, returns in parameters the new symbol table
index, half-line pitch level, and accidental value (0=none, 1...5=double-flat thru
double-sharp).

For rests (as indicated by *symIndex), does minimal tracking, allowing the user to
cancel, but giving no feedback. If the cursor's final position is outside the music
area, returns FALSE to allow the caller to cancel the operation. Otherwise, simply
returns TRUE. */

Boolean InsTrackPitch(
				register Document *doc,
				Point		downPt,			/* Location of mousedown */
				INT16		*symIndex,		/* Symbol table index of note/rest to be inserted (<0=unspecified note) */
				LINK		startL,			/* link to node where note/rest is or before it note/rest will go */
				INT16		staff,
				INT16		*pHalfLn,		/* pitch level return value */
				INT16		*pAccident,		/* accidental return value */
				INT16		octType 			/* type of octave sign note is to be inserted into. */
				)
{
	Point		pt;
	INT16		noteNum,
				oldh, oldv,
				accident, accidentOld,
				prevAccident, accy,
				middleCHalfLn;
	Boolean	outOfBounds;
	Rect		accBox, saveBox;
	GrafPtr	savePort, ourPort;
	INT16		halfLn, halfLnOld, halfLnOrig,
				halfLnLo, halfLnHi,
				durIndex, durIndexOld, useChan, useIORefNum,
				partn, octTransp, transp;
	Rect		tRect;
	CONTEXT	context;
	LINK		partL, staffL;
	PPARTINFO	pPart;
	
	MIDIUniqueID partDevID;
	Boolean	useFeedback = TRUE;
	
	SignedByte	partVelo[MAXSTAVES];
	Byte			partChannel[MAXSTAVES];
	short			partTransp[MAXSTAVES];
	Byte			partPatch[MAXSTAVES];
	short			partIORefNum[MAXSTAVES];
	MIDIUniqueID cmPartDevice[MAXSTAVES];
	
	const BitMap *ourPortBits = NULL;
	const BitMap *savePortBits = NULL;
	const BitMap *thePortBits = NULL;

	GWorldPtr gwPtr = NULL;
	
	Byte		patchNum;
	
	if (config.altPitchCtl)
		return AltInsTrackPitch(doc, downPt, symIndex, startL, staff, pHalfLn, pAccident,
								octType, INCL_DBL_ACCS);
		
	ourPort = GetDocWindowPort(doc);

	GetContext(doc, LeftLINK(startL), staff, &context);				/* Get context */
	GetInsRect(doc, startL, &tRect);

	/* If it's a rest, enter this tracking loop now; we don't need any more info.
	 * The loop allows the user to cancel the rest insertion. For feedback, we show
	 * the crossbar at a fixed location (2nd space), then erase it whenever the
	 * mouse is outside of the music area.
	 */
	if (*symIndex>=0 && symtable[*symIndex].subtype!=0) {	/* Is it a rest? */
		outOfBounds = FALSE;
		SetCrossbar(3, downPt.h, &context);					/* Calculate ccrossbar rect */
		InvertCrossbar(doc);										/* Show it */
		do {
			GetPaperMouse(&pt, &context.paper);
			if (!PtInRect(pt, &tRect)) {						/* Mouse out of System? */
				outOfBounds = TRUE;
				InvertCrossbar(doc);
				do {
					GetPaperMouse(&pt, &context.paper);
				} while (!PtInRect(pt, &tRect) && WaitMouseUp());
				InvertCrossbar(doc);
			}
			else outOfBounds = FALSE;
		} while (WaitMouseUp()); 
		*pHalfLn = 0;
		*pAccident = 0;
		InvertCrossbar(doc);
		return !outOfBounds;
	}


	/* If it's a note, we need more info and a much more comprehensive tracking loop. */
	
	staffL = LSSearch(startL, STAFFtype, ANYONE, GO_LEFT, FALSE);
	halfLn = CalcPitchLevel(downPt.v, &context, staffL, staff);
	halfLnOrig = halfLn;
	durIndex = *symIndex;
	partn = Staff2Part(doc, staff);
	
//#ifndef TARGET_API_MAC_CARBON_MIDI
	useChan = UseMIDIChannel(doc, partn) - CM_CHANNEL_BASE;			/* Set feedback channel no. */
	partDevID = GetCMDeviceForPartn(doc, partn);

	if (useWhichMIDI == MIDIDR_CM) {
		useIORefNum = 0;															/* unused */
		if (GetCMPartPlayInfo(doc, partTransp, partChannel, 
										partPatch, partVelo,
										partIORefNum, cmPartDevice)) 
		{
			CMSetup(doc, partChannel);			
			CMMIDIProgram(doc, partPatch, partChannel);
		}
		useFeedback = FALSE;
	}
//#endif

	octTransp = (octType>0? noteOffset[octType-1] : 0);				/* Set feedback transposition */
	partL = FindPartInfo(doc, partn);
	pPart = GetPPARTINFO(partL);
	if (doc->transposed) transp = pPart->transpose;
	else						transp = 0;
	
	pt.v = downPt.v;
	accident = prevAccident = 0;
	outOfBounds = FALSE;
	patchNum = pPart->patchNum;

	middleCHalfLn = ClefMiddleCHalfLn(context.clefType);
	if (ACC_IN_CONTEXT) {
		GetPitchTable(doc, accTable, startL, staff);						/* Get pitch mod. situation */
		prevAccident = accTable[halfLn-middleCHalfLn+ACCTABLE_OFF];	/* Effective acc. here */			
	}
	noteNum = Pitch2MIDI(middleCHalfLn-halfLn+octTransp, prevAccident)+transp;
	MIDIFBOn(doc);
#ifdef TARGET_API_MAC_CARBON_MIDI
	if (IsPatchMapped(doc, patchNum)) {
		noteNum = GetMappedNoteNum(doc, noteNum);		
	}
	CMMIDIFBNoteOn(doc, noteNum, useChan, partDevID);
#endif
	InsertLedgers(p2d(downPt.h), halfLn, &context);
	halfLnLo = halfLnHi = halfLn;

	SetCrossbar(halfLn, downPt.h, &context);						/* Calculate ccrossbar rect */
	InvertCrossbar(doc);													/* Show it */

	accy = d2p(context.staffTop+context.staffHeight/2);		/* Setup accidental feedback */
	accBox.left = ccrossbar.left-2-ACCBOXWIDE;		
	accBox.right = ccrossbar.left-2;
	accBox.bottom = accy+ACCBOXWIDE/2+1;
	accBox.top = accy-3*ACCBOXWIDE/4-1;								/* Allow for flat ascent */
	/* Convert accBox to window coordinates */
	OffsetRect(&accBox,context.paper.left,context.paper.top);
	SetRect(&saveBox, 0, 0, accBox.right-accBox.left, accBox.bottom-accBox.top);
#ifdef USE_GRAFPORT
	savePort = NewGrafPort(saveBox.right+1, saveBox.bottom+1);
	
	savePortBits = GetPortBitMapForCopyBits(savePort);
	thePortBits = GetPortBitMapForCopyBits(GetQDGlobalsThePort());
	CopyBits(thePortBits,savePortBits, &accBox, &saveBox, srcCopy, NULL);

#else
	SaveGWorld();
	
	gwPtr = MakeGWorld(saveBox.right+1, saveBox.bottom+1, TRUE);
	savePort = gwPtr;

	savePortBits = GetPortBitMapForCopyBits(savePort);
	thePortBits = GetPortBitMapForCopyBits(GetQDGlobalsThePort());
	CopyBits(thePortBits,savePortBits, &accBox, &saveBox, srcCopy, NULL);
	
	RestoreGWorld();
#endif // USE_GRAFPORT

//	CopyBits(&qd.thePort->portBits, &savePort->portBits,
//			 &accBox, &saveBox, srcCopy, NULL);


	oldh = downPt.h;
	oldv = downPt.v;
	
	/* Tracking loop. When anything changes, handle it, then go back to waiting for changes. */

	if (StillDown())
		while (WaitMouseUp()) {
			halfLnOld = halfLn;
			accidentOld = accident;
			durIndexOld = durIndex;
			do {																/* Watch for ANY change */
				GetPaperMouse(&pt,&context.paper);
				if (!PtInRect(pt, &tRect)) {							/* Mouse out of System? */
					outOfBounds = TRUE;
#ifdef TARGET_API_MAC_CARBON_MIDI
					CMMIDIFBNoteOff(doc, noteNum, useChan, partDevID);
#endif
					InvertCrossbar(doc);
					do {
						GetPaperMouse(&pt,&context.paper);
					} while (!PtInRect(pt, &tRect) && WaitMouseUp());
					InvertCrossbar(doc);
					break;
				}
				else
					outOfBounds = FALSE; 
				if (ShiftKeyDown())	
					accident = CalcAccidental(pt.v, oldv);
				else if (CmdKeyDown() && *symIndex>=0)
					durIndex = CalcNewDurIndex(*symIndex, pt.h, oldh);
				else {
					halfLn = CalcPitchLevel(pt.v, &context, staffL, staff);
					oldv = pt.v;
					accident = 0;
					durIndex = *symIndex;
				}
			} while (WaitMouseUp() &&
						halfLn==halfLnOld &&
						accident==accidentOld &&
						durIndex==durIndexOld); 

			if (WaitMouseUp()) {											/* Mouse still down? */						
				if (durIndex!=durIndexOld) {							/* Yes. Change of duration? */
					PalKey(symtable[durIndex].symcode);				/* Yes */
					currentCursor = cursorList[durIndex];
					SetCursor(*currentCursor);
				}
				else {														/* Not dur.change, must be pitch change */
					if (halfLn!=halfLnOld)	{							/* Pitch level change? */
						InvertCrossbar(doc);									/* Move ccrossbar to new pitch */
						SetCrossbar(halfLn, downPt.h, &context);
						if (halfLn<halfLnLo) {							/* do we need new ledger lines? */
							halfLnLo = halfLn;
							InsertLedgers(p2d(downPt.h), halfLn, &context);
						}
						else if (halfLn>halfLnHi) {
							halfLnHi = halfLn;
							InsertLedgers(p2d(downPt.h), halfLn, &context);
						}
						InvertCrossbar(doc);
					}
					if (accident!=accidentOld) {						/* Change of accidental? */
						if (accident==0) {
							ourPortBits = GetPortBitMapForCopyBits(ourPort);
							savePortBits = GetPortBitMapForCopyBits(savePort);
							CopyBits(savePortBits, ourPortBits, &saveBox, &accBox, srcCopy, NULL);
						
//							CopyBits(&savePort->portBits, &ourPort->portBits,
//									&saveBox, &accBox, srcCopy, NULL);
						}
						else {
							TextFont(doc->musicFontNum);						/* Set to music font */
							TextSize(ACCSIZE);
							EraseRect(&accBox);
							FrameRect(&accBox);
							MoveTo(accBox.left+4, context.paper.top+accy);
							DrawChar(SonataAcc[accident]);			/* Draw new accidental */
						}
					}
#ifdef TARGET_API_MAC_CARBON_MIDI
					CMMIDIFBNoteOff(doc, noteNum, useChan, partDevID);
#endif
					if (accident==0) {
						if (ACC_IN_CONTEXT)
							prevAccident = accTable[halfLn-middleCHalfLn+ACCTABLE_OFF];			
						noteNum = Pitch2MIDI(middleCHalfLn-halfLn+octTransp, prevAccident)
										+transp;
					}
					else {
						noteNum = Pitch2MIDI(middleCHalfLn-halfLn+octTransp, accident)
										+transp;
					}
#ifdef TARGET_API_MAC_CARBON_MIDI
					if (IsPatchMapped(doc, patchNum)) {
						noteNum = GetMappedNoteNum(doc, noteNum);		
					}
					CMMIDIFBNoteOn(doc, noteNum, useChan, partDevID);
#endif
				}
				
			}
		}

	InvertCrossbar(doc);
	
#ifdef TARGET_API_MAC_CARBON_MIDI
	CMMIDIFBNoteOff(doc, noteNum, useChan, partDevID);
	MIDIFBOff(doc);
#endif
	
	/* Restore bits under accBox so we'll never have to redraw previous Measure. */
	
	ourPortBits = GetPortBitMapForCopyBits(ourPort);
	savePortBits = GetPortBitMapForCopyBits(savePort);
	CopyBits(savePortBits, ourPortBits, &saveBox, &accBox, srcCopy, NULL);

//	CopyBits(&savePort->portBits, &ourPort->portBits,
//				&saveBox, &accBox, srcCopy, NULL);
#ifdef USE_GRAFPORT
	DisposGrafPort(savePort);										/* done with this */
#else
	DestroyGWorld(gwPtr);
#endif // USE_GRAFPORT

	if (outOfBounds) {
		*pHalfLn = halfLnOrig;
		*pAccident = accident;
		return FALSE;
	}
	else {
		*pHalfLn = halfLn;
		*pAccident = accident;
		*symIndex = durIndex;
		return TRUE;
	}
}


/* -------------------------------------------------------------- InsTrackUpDown -- */
/*	For symbol with variable y-position, track the cursor up and down the staff,
giving appropriate feedback: for now, a rectangle sliding up and down showing where
the symbol would go. If the symbol is a dynamic, it allows changing the dynamic by
holding	down the command key and moving sideways. Returns the half-line pitch
value.  If the final position is outside the music area, it	returns 0 to allow the
caller to cancel the current operation;	if the mouse is never moved enough to
change anything, it returns -1; else it returns 1. */

INT16 InsTrackUpDown(
		register Document *doc,
		Point		downPt,			/* Location of mousedown */
		INT16		*symIndex,		/* Symbol table index of item to be inserted */
		LINK		startL,			/* Node before which node will go, to determine staff size */
		INT16		staff,
		INT16		*pHalfLn 		/* pitch level return value */
		)
{
	Point		pt;
	INT16		oldh, oldv;
	Boolean	isDynamic, outOfBounds, anyChange;
	INT16		halfLn, halfLnOld, halfLnOrig,
				halfLnLo, halfLnHi,
				itemIndex, itemIndexOld;
	Rect		tRect;
	CONTEXT	context;					/* current context */
	LINK		staffL;
	
	GetContext(doc, LeftLINK(startL), staff, &context);					/* For staff size */
	staffL = LSSearch(startL, STAFFtype, ANYONE, GO_LEFT, FALSE);
	halfLn = CalcPitchLevel(downPt.v, &context, staffL, staff);
	halfLnOrig = halfLn;
	itemIndex = *symIndex;
	isDynamic = (symtable[*symIndex].objtype==DYNAMtype);
	
	pt.v = downPt.v;
	outOfBounds = FALSE;
	InsertLedgers(p2d(downPt.h), halfLn, &context);				/* Just to clarify which staff */
	halfLnLo = halfLnHi = halfLn;

	GetInsRect(doc, startL, &tRect);

	SetCrossbar(halfLn, downPt.h, &context);						/* Calculate ccrossbar rect */
	InvertCrossbar(doc);														/* Show it */

	anyChange = FALSE;
	oldh = downPt.h;
	oldv = downPt.v;
	
	/* Tracking loop. When anything changes, handle it, then go back to waiting for changes. */

	if (StillDown())
		while (WaitMouseUp()) {
			halfLnOld = halfLn;
			itemIndexOld = itemIndex;
			do {																/* Watch for mouseUp or pitch change */
				GetPaperMouse(&pt,&context.paper);
				if (!PtInRect(pt, &tRect)) {							/* Mouse still in System? */
					outOfBounds = TRUE;
					InvertCrossbar(doc);
					do {
						GetPaperMouse(&pt,&context.paper);
					} while (!PtInRect(pt, &tRect) && WaitMouseUp());
					InvertCrossbar(doc);
					break;
				}
				else
					outOfBounds = FALSE;
				if (isDynamic && CmdKeyDown())
					itemIndex = CalcNewDynIndex(*symIndex, pt.h, oldh);
				else {
					itemIndex = *symIndex;
					halfLn = CalcPitchLevel(pt.v, &context, staffL, staff);
					oldv = pt.v;
				}
			} while (WaitMouseUp() &&
						halfLn==halfLnOld &&
						itemIndex==itemIndexOld);
						
			if (WaitMouseUp()) {										/* Mouse still down? */						
				if (itemIndex!=itemIndexOld) {					/* Yes. Change of item? */
					PalKey(symtable[itemIndex].symcode);
					currentCursor = cursorList[itemIndex];
					SetCursor(*currentCursor);
					anyChange = TRUE;
				}
				else if (halfLn!=halfLnOld)	{					/* Pitch level change? */
					InvertCrossbar(doc);								/* Move ccrossbar to new halfLn */
					SetCrossbar(halfLn, downPt.h, &context);
					if (halfLn<halfLnLo) {							/* do we need new ledger lines? */
						halfLnLo = halfLn;
						InsertLedgers(p2d(downPt.h), halfLn, &context);
					}
					else if (halfLn>halfLnHi) {
						halfLnHi = halfLn;
						InsertLedgers(p2d(downPt.h), halfLn, &context);
					}
					InvertCrossbar(doc);
					anyChange = TRUE;
				}
			}
		}

	InvertCrossbar(doc);	
	if (outOfBounds) {
		*pHalfLn = halfLnOrig;
		return 0;
	}
	else {
		*pHalfLn = halfLn;
		*symIndex = itemIndex;
		return (anyChange? 1 : -1);
	}
}


/* ----------------------------------------------------------------- SetCrossbar -- */
/* Calculate new crossbar rectangle, in paper-relative coords */

void SetCrossbar(INT16 halfLn, INT16 h, PCONTEXT pCont)
{
	INT16		top, left, bottom, right,
				ledgerLen, barWidth;
	DDIST		lnSpace = LNSPACE(pCont);

	ledgerLen = d2p(LedgerLen(lnSpace)+LedgerOtherLen(lnSpace));
	barWidth = ledgerLen;

	top = d2p(pCont->staffTop+halfLn2d(halfLn-1, pCont->staffHeight, pCont->staffLines));
	bottom = d2p(pCont->staffTop+halfLn2d(halfLn+1, pCont->staffHeight, pCont->staffLines));
	left = h - barWidth/2;
	right = left+barWidth;
	SetRect(&ccrossbar, left, top+1, right+1, bottom);
}


/* -------------------------------------------------------------- InvertCrossbar -- */

void InvertCrossbar(Document *doc)
{
	Rect cb;
	
	cb = ccrossbar;
	OffsetRect(&cb,doc->currentPaper.left,doc->currentPaper.top);
	InvertRect(&cb);
}


/* --------------------------------------------------------------- CalcAccidental -- */

#define ACC_STEP 3								/* Pixels between accidentals */

INT16 CalcAccidental(INT16 newy, INT16 oldy)
{
	INT16	center, delta;
	
	center = ACC_STEP/2;
	delta = newy-oldy;
	if (delta<=-2*ACC_STEP+center)	 return AC_DBLSHARP;
	else if (delta<=-ACC_STEP+center) return AC_SHARP;
	else if (delta<=center)				 return AC_NATURAL;
	else if (delta<ACC_STEP+center)	 return AC_FLAT;
	else										 return AC_DBLFLAT;
}


/* ------------------------------------------------------------- CalcNewDurIndex -- */

#define DUR_STEP 4								/* Pixels between duration changes */

INT16 CalcNewDurIndex(INT16 oldIndex, INT16 newx, INT16 oldx)
{
	INT16 delta, newIndex;
	
	delta = newx-oldx;
	newIndex = oldIndex-(delta/DUR_STEP);
	if (newIndex<1) return 1;
	else if (newIndex>MAX_L_DUR) return MAX_L_DUR;
	else return newIndex;
}


/* ------------------------------------------------------------- CalcNewDynIndex -- */

#define DYN_STEP 4								/* Pixels between dynamic changes */

INT16 CalcNewDynIndex(INT16 oldIndex, INT16 newx, INT16 oldx)
{
	static Boolean firstCall=TRUE;
	static INT16	lowDynIndex, hiDynIndex;
	INT16 			delta, j, newIndex;
	
	if (firstCall) {
		for (j = 0; j<nsyms; j++)
			if (symtable[j].objtype==DYNAMtype) {
				lowDynIndex = j; break;
			}
		hiDynIndex = lowDynIndex+7;			/* Assume 8 non-hairpin dynamics */
		firstCall = FALSE;
	}
	
	delta = newx-oldx;
	newIndex = oldIndex+(delta/DYN_STEP);
	if (newIndex<lowDynIndex) return lowDynIndex;
	else if (newIndex>hiDynIndex) return hiDynIndex;
	else return newIndex;
}
