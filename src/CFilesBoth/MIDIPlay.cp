/***************************************************************************
*	FILE:	MIDIPlay.c																			*
*	PROJ:	Nightingale, rev.for v.2000													*
*	DESC:	MIDI playback routines															*
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <CoreAudio/HostTime.h>

#include "CoreMIDIDefs.h"

/* Nightingale supports both MIDI Manager and its own "built-in" MIDI routines.
The latter, used here and in MIDIRecord.c, are Altech Systems' MIDI Pascal 3.0.
We used to use a driver from Kirk Austin's articles in MacTutor, July and
December 1987, but those routines became quite buggy--according to Jeremy Sagan,
because they don't initialize enough registers. Anyway, MIDI Pascal is much more
powerful, and only six years old instead of nine (sigh). */

/* ================================== LOCAL STUFF ================================== */

static long			pageTurnTOffset;

static void		PlayMessage(Document *, LINK, INT16);
static Boolean	HiliteSyncRect(Document *doc, Rect *syncRect, Rect *rPaper, Boolean scroll);

#define CMDEBUG 1
#define TDEBUG 1
#define PLDEBUG 1

/* Print an error message.  If index is non-zero, then retrieve the index'th
string from our error strings resource. */

#define MIDIErrorStringsID 235

static INT16 DoGeneralAlert(unsigned char *str);

static void ErrorMsg(INT16 index)
	{
		Str255 str;
		
		if (index > 0) GetIndString(str,MIDIErrorStringsID,index);
		else		   	*str = 0;
		(void)DoGeneralAlert(str);
	}

static INT16 DoGeneralAlert(unsigned char *str)
	{
		ParamText(str,"\p","\p","\p");
		PlaceAlert(errorMsgID,NULL,0,40);
		return(StopAlert(errorMsgID,NULL));
	}


/* ------------------------------------------------------------------ PlayMessage -- */
/* Write a message into the message area saying what measure is playing. If <pL>
is NILINK, use <measNum>, else use <pL>'s Measure. */

static void PlayMessage(Document *doc, LINK pL, INT16 measNum)
{
	Rect			messageRect;

	if (pL)
		measNum = GetMeasNum(doc, pL);

	PrepareMessageDraw(doc,&messageRect, FALSE);
	GetIndCString(strBuf, MIDIPLAY_STRS, 1);					/* "Playing measure " */
	DrawCString(strBuf);					
	sprintf(strBuf, "%d", measNum);
	DrawCString(strBuf);
	GetIndCString(strBuf, MIDIPLAY_STRS, 2);					/* "    CLICK OR CMD-. TO STOP" */
	DrawCString(strBuf);					
	FinishMessageDraw(doc);
}


/* --------------------------------------------------------------- HiliteSyncRect -- */
/* Given a rectange, r, in paper-relative coordinates, hilite it in the current
window.  If it's not in view, "scroll" so its page is in the window (though r might
still not be in the window!) and return TRUE; else return FALSE.

Note that scrolling while playing screws up timing (simply by introducing a long
break) unless interrupt-driven, but after all, Nightingale is a notation program,
not a sequencer: the most important thing is that the music be in view while it's
being played.

NB: The "scrolling" code here can itself change doc->currentPaper. For this and
other reasons, we expect the appropriate doc->currentPaper as a parameter. */

static Boolean HiliteSyncRect(
						Document *doc,
						Rect *r,
						Rect *rPaper,				/* doc->currentPaper for r's page */
						Boolean scroll)
	{
		Rect result; INT16 x,y; Boolean turnedPage=FALSE;
		
		/* Temporarily convert r to window coords. Normally, we do this by
		 * offsetting by doc->currentPaper, but in this case, doc->currentPaper
		 * may have changed (if r's Sync was the last played on a page), so we
		 * have to use the currentPaper for r's Sync. */

		OffsetRect(r,rPaper->left,rPaper->top);
		
		/*
		 * Code to scroll while playing. See comment on timing above. */
		if (scroll) {
			if (!SectRect(r,&doc->viewRect,&result)) {
				/* Rect to be hilited is outside of window's view, so scroll paper */
				x = doc->currentPaper.left - config.hPageSep;
				y = doc->currentPaper.top  - config.vPageSep;
				QuickScroll(doc,x,y,FALSE,TRUE);
				turnedPage = TRUE;
				}
			}
		
#ifdef NOMORE	/* AVOID MYSTERIOUS HANGING IN PlaySequence */
	if (SectRect(r,&doc->viewRect,&result))
#endif
		HiliteRect(r);
		/* Convert back to paper coords */

		OffsetRect(r,-rPaper->left,-rPaper->top);
		
		return turnedPage;
	}


/* -------------------------------------------------------- AddBarlines functions -- */
/* Build up a list of places to add single barlines (actually Measure objects):
	InitAddBarlines() 		Initialize these functions
	AddBarline(pL)				Request adding a barline before <pL>
	CloseAddBarlines(doc)	Add the barlines at all requested places
Duplicate calls to AddBarline at the same link are ignored.*/

#define MAX_BARS 300			/* Maximum no. of barlines we can add in one set */

static LINK barBeforeL[MAX_BARS];
static INT16 nBars;

void InitAddBarlines(void);
void AddBarline(LINK);
static Boolean TupletProblem(Document *, LINK);
Boolean CloseAddBarlines(Document *);

static void SendMidiSustainOn(Document *doc, MIDIUniqueID destDevID, char channel);
static void SendMidiSustainOff(Document *doc,MIDIUniqueID destDevID, char channel);
static void SendMidiPan(Document *doc,MIDIUniqueID destDevID, char channel, Byte panSetting);
static void SendMidiSustainOn(Document *doc, MIDIUniqueID destDevID, char channel, MIDITimeStamp tStamp);
static void SendMidiSustainOff(Document *doc,MIDIUniqueID destDevID, char channel, MIDITimeStamp tStamp);
static void SendMidiPan(Document *doc,MIDIUniqueID destDevID, char channel, Byte panSetting, MIDITimeStamp tStamp);

#define PAN_EQUAL 64

static Boolean cmSustainOn[MAXSTAVES + 1];
static Boolean cmSustainOff[MAXSTAVES + 1];
static Byte cmPanSetting[MAXSTAVES + 1];

static Boolean cmAllSustainOn[MAXSTAVES + 1];
static Byte cmAllPanSetting[MAXSTAVES + 1];

#ifdef VIEWER_VERSION

void InitAddBarlines()
{
}

void AddBarline(LINK pL)
{
}

static Boolean TupletProblem(Document *doc, LINK insL)
{
}

Boolean CloseAddBarlines(Document *doc)
{
}

#else

void InitAddBarlines()
{
	nBars = 0;
}

void AddBarline(LINK pL)
{
	if (pL && (nBars==0 || pL!=barBeforeL[nBars-1])) {
		if (nBars<MAX_BARS)
			barBeforeL[nBars] = pL;
		nBars++;											/* Register attempt so we can tell user later */
	}
}

static Boolean TupletProblem(Document *doc, LINK insL)
{
	short s, tupStaff;
	char fmtStr[256];
	 
	for (tupStaff = 0, s = 1; s<=doc->nstaves; s++)
		if (HasTupleAcross(insL, s))
			{ tupStaff = s; break; }
			
	if (tupStaff<=0)
		return FALSE;
	else {
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 1);		/* "can't add a barline in middle of tuplet" */
		sprintf(strBuf, fmtStr, tupStaff);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return TRUE;
	}
}

Boolean CloseAddBarlines(Document *doc)
{
	short symIndex, n; CONTEXT context;
	LINK insL, newMeasL, firstInsL, lastInsL, saveSelStartL, saveSelEndL, prevSync;
	Boolean okay=TRUE;
	char fmtStr[256];
	
	if (nBars==0) return okay;
	
	if (NoteAdvise(ADDBAR_ALRT)!=OK) return okay;
	ArrowCursor();
	if (nBars>MAX_BARS) {
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 2);		/* "can add only %d barlines at once" */
		sprintf(strBuf, fmtStr, MAX_BARS, MAX_BARS);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		nBars = MAX_BARS;
	}
	
	WaitCursor();
	
	/*
	 *	Put barlines that would otherwise go at the beginning of a system at
	 * the end of the previous system instead.
	 */
	for (n = 0; n<nBars; n++) {
		prevSync = SSearch(LeftLINK(barBeforeL[n]), SYNCtype, GO_LEFT);
		if (prevSync && !SameSystem(prevSync, barBeforeL[n]))
			barBeforeL[n] = RightLINK(prevSync);
	}

	/*
	 *	Save current selection range so we can set range for PrepareUndo, then restore
	 *	it. Since Undo works on whole systems, LocateInsertPt can't affect it.
	 */
	saveSelStartL = doc->selStartL;
	saveSelEndL = doc->selEndL;
	doc->selStartL = LeftLINK(barBeforeL[0]);
	doc->selEndL = barBeforeL[nBars-1];
	
	PrepareUndo(doc, LeftLINK(barBeforeL[0]), U_TapBarlines, 29);	/* "Undo Tap-in Barlines" */
	
	doc->selStartL = saveSelStartL;
	doc->selEndL = saveSelEndL;
	symIndex = GetSymTableIndex(singleBarInChar);	/* single barline */
	for (n = 0; n<nBars; n++) {
		GetContext(doc, LeftLINK(barBeforeL[n]), 1, &context);
		insL = LocateInsertPt(barBeforeL[n]);
		if (TupletProblem(doc, insL))
			{ nBars = n; break; }
		if (n==0) firstInsL = insL;
		newMeasL = CreateMeasure(doc, insL, -1, symIndex, context);
		if (!newMeasL) return FALSE;
		DeselectNode(newMeasL);
	}

	if (nBars==0) return okay;

	lastInsL = insL;
	okay = RespaceBars(doc, LeftLINK(firstInsL), RightLINK(lastInsL), 
		RESFACTOR*(long)doc->spacePercent, FALSE, FALSE);
	InvalWindow(doc);
	return okay;
}

#endif /* VIEWER_VERSION */


/* ------------------------------------------------------------ SelAndHiliteSync -- */
/* Select and hilite the given Sync, and set the document's scaleCenter (for
magnification) to it. */

static void SelAndHiliteSync(Document *, LINK);
static void SelAndHiliteSync(Document *doc, LINK syncL)
{
	CONTEXT		context[MAXSTAVES+1];
	Boolean		found;
	INT16			index;
	STFRANGE		stfRange={0,0};
	DDIST			xd, yd;
	
	GetAllContexts(doc, context, syncL);
	CheckObject(doc, syncL, &found, NULL, context, SMSelect, &index, stfRange);

	xd = PageRelxd(syncL, &context[1]);
	doc->scaleCenter.h = context[1].paper.left+d2p(xd);
	yd = PageRelyd(syncL, &context[1]);
	doc->scaleCenter.v = context[1].paper.top+d2p(yd);
}


/* ----------------------------------------------------------------- CheckButton -- */

Boolean CheckButton(void);
Boolean CheckButton()
{
	Boolean button = FALSE;
	
	EventRecord evt; 

#ifdef USE_GETOSEVENT		
	if (!GetOSEvent(mDownMask,&evt)) return(button);	/* Not Wait/GetNextEvent so we do as little as possible */
#else
	if (!GetNextEvent(mDownMask,&evt)) return(button);	/* Not Wait/GetNextEvent so we do as little as possible */
#endif

	if (evt.what==mouseDown) button = TRUE;

 	return button;
}

#define DEBUG_KEEPTIMES 1
#if DEBUG_KEEPTIMES
#define MAXKEEPTIMES 20
long kStartTime[MAXKEEPTIMES];
INT16 nkt;
#endif

// From CoreMidiUtils.c

#define CM_PATCHNUM_BASE 1			/* Some synths start numbering at 1, some at 0 */
#define CM_CHANNEL_BASE 1	

/* ------------------------------------------------------------- GetPartPlayInfo -- */

static void SetPartPatch(INT16 partn, Byte partPatch[], Byte partChannel[], Byte patchNum)
{
	INT16 patch = patchNum + CM_PATCHNUM_BASE;

	partPatch[partn] = patch;	
}

/* ------------------------------------------------------------- SendMidiProgramChange -- */
/* Steps:

	1. Get the graphic
	
	2. Check that the object it is hanging off of is a sync
	
	3. Get the partn from the graphic's staff

	4. Update the part patch in the part patch array. See GetPartPlayInfo for this.
		Updates channelPatch[].
		
	6. Send the program packet
 		CMMIDIProgram(doc, partPatch, partChannel);
 		
 		with the progressively updated patch num.
*/

static Boolean ValidPanSetting(Byte panSetting) 
{
	SignedByte sbpanSetting = (SignedByte)panSetting;
	
	return sbpanSetting >= 0;
}

static Boolean PostMidiProgramChange(Document *doc, LINK pL, unsigned char *partPatch, unsigned char *partChannel) 
{
	Boolean posted = FALSE;
	PGRAPHIC p = GetPGRAPHIC(pL);
	LINK firstL = p->firstObj;
	if (ObjLType(firstL) == SYNCtype) {
		INT16 patchNum = p->info;
		INT16 stf = p->staffn;
		if (stf > 0) {
			LINK partL = Staff2PartL(doc, doc->headL, stf);
			INT16 partn = PartL2Partn(doc, partL);
			SetPartPatch(partn, partPatch, partChannel, patchNum);
			//CMMIDIProgram(doc, partPatch, partChannel);
			posted = TRUE;
		}
	}
	return posted;
}

static Boolean PostMidiSustain(Document *doc, LINK pL, Boolean susOn) 
{
	Boolean posted = FALSE;
						
	PGRAPHIC pGraphic = GetPGRAPHIC(pL);
	INT16 stf = GraphicSTAFF(pL);
	if (stf > 0) 
	{
		if (susOn) {
			cmSustainOn[stf] = cmAllSustainOn[stf] = TRUE;
		}		
		else {
			cmSustainOff[stf] = TRUE;			
		}
		posted = TRUE;
	}
	
	return posted;
}

static Boolean PostMidiPan(Document *doc, LINK pL)
{
	Boolean posted = FALSE;
	
	PGRAPHIC pGraphic = GetPGRAPHIC(pL);
	INT16 stf = GraphicSTAFF(pL);
	if (stf > 0) 
	{
		Byte panSetting = GraphicINFO(pL);
		cmPanSetting[stf] = cmAllPanSetting[stf] = panSetting;
		posted = TRUE;
	}
	
	return posted;
}

static void ClearMidiSustain(Boolean susOn) 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (susOn) {
			cmSustainOn[j] = FALSE;
		}		
		else {
			cmSustainOff[j] = FALSE;			
		}
	}	
}

static void ClearMidiPan() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmPanSetting[j] = -1;
	}
}

static void ClearAllMidiSustainOn() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmAllSustainOn[j] = FALSE;
	}
}

static void ClearAllMidiPan() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmAllPanSetting[j] = -1;
	}
}

#define kNanosPerSecond 1000000000ULL

static MIDITimeStamp TimeStampSecsFromNow(unsigned long long seconds) 
{
	unsigned long long nanos = kNanosPerSecond * seconds;
	
	MIDITimeStamp now = AudioGetCurrentHostTime();
	UInt64 nowNanos = AudioConvertHostTimeToNanos(now);
	UInt64 oneSecondFromNowNanos = nowNanos + nanos;
	MIDITimeStamp secondsFromNow = AudioConvertNanosToHostTime(oneSecondFromNowNanos);
	return secondsFromNow;
}

static long long GetSustainSecs() 
{
	long long secs = 0LL;
	char buf[64];
	
	char *midiSustainSecs = GetPrefsValue("MIDISustain");
	if (midiSustainSecs != NULL) {
		strcpy(buf, midiSustainSecs);
		CToPString(buf);
		secs = FindIntInString((unsigned char *)buf);
		if (secs < 0) secs = 0LL;
	}
	return secs;
}

static void ResetMidiSustain(Document *doc, unsigned char *partChannel) 
{
//	MIDITimeStamp tStamp = 5000L * kNanosToMillis;
	
	long long secs = GetSustainSecs();
	MIDITimeStamp tStamp = TimeStampSecsFromNow(secs);
	
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (cmAllSustainOn[j]) {
			INT16 partn = Staff2Part(doc,j);
			MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
			INT16 channel = CMGetUseChannel(partChannel, partn);
			SendMidiSustainOff(doc, partDevID, channel, tStamp);										
		}
	}
	
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmAllSustainOn[j] = FALSE;
	}
}

static void ResetMidiPan(Document *doc, unsigned char *partChannel) 
{
//	MIDITimeStamp tStamp = 5000L * kNanosToMillis;

	long long secs = GetSustainSecs();	
	MIDITimeStamp tStamp = TimeStampSecsFromNow(0);
	
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (ValidPanSetting(cmAllPanSetting[j])) {
			INT16 partn = Staff2Part(doc,j);
			MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
			INT16 channel = CMGetUseChannel(partChannel, partn);
			SendMidiPan(doc, partDevID, channel, PAN_EQUAL, tStamp);		
		}
	}
	
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmAllPanSetting[j] = -1;
	}
}

static void SendAllMidiSustains(Document *doc, unsigned char *partChannel, Boolean susOn) 
{
	if (susOn) {
		for (int j = 1; j<=MAXSTAVES; j++) {
			if (cmSustainOn[j]) {
				INT16 partn = Staff2Part(doc,j);
				MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
				INT16 channel = CMGetUseChannel(partChannel, partn);
				SendMidiSustainOn(doc, partDevID, channel);							
			}
		}
	}
	else {
		for (int j = 1; j<=MAXSTAVES; j++) {
			if (cmSustainOff[j]) {
				INT16 partn = Staff2Part(doc,j);
				MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
				INT16 channel = CMGetUseChannel(partChannel, partn);
				SendMidiSustainOff(doc, partDevID, channel);							
			}
		}		
	}
}

static void SendAllMidiPans(Document *doc, unsigned char *partChannel) 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (ValidPanSetting(cmPanSetting[j])) {
			INT16 partn = Staff2Part(doc,j);
			MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
			INT16 channel = CMGetUseChannel(partChannel, partn);
			SendMidiPan(doc, partDevID, channel, cmPanSetting[j]);							
		}
	}		
}

static void SendMidiProgramChange(Document *doc, unsigned char *partPatch, unsigned char *partChannel) 
{
	CMMIDIProgram(doc, partPatch, partChannel);	
}

static Boolean IsMidiPatchChange(LINK pL) 
{
	if (ObjLType(pL) == GRAPHICtype) {
		PGRAPHIC p = GetPGRAPHIC(pL);
		return (p->graphicType == GRMIDIPatch);
	}
		
	return FALSE;
}

static void SendMidiSustainOn(Document *doc, MIDIUniqueID destDevID, char channel) 
{
	CMMIDISustainOn(destDevID, channel);	
}

static void SendMidiSustainOff(Document *doc,MIDIUniqueID destDevID, char channel) 
{
	CMMIDISustainOff(destDevID, channel);	
}

static void SendMidiPan(Document *doc,MIDIUniqueID destDevID, char channel, Byte panSetting) 
{
	CMMIDIPan(destDevID, channel, panSetting);	
}

static void SendMidiSustainOn(Document *doc, MIDIUniqueID destDevID, char channel, MIDITimeStamp tStamp) 
{
	CMMIDISustainOn(destDevID, channel, tStamp);	
}

static void SendMidiSustainOff(Document *doc,MIDIUniqueID destDevID, char channel, MIDITimeStamp tStamp) 
{
	CMMIDISustainOff(destDevID, channel, tStamp);	
}

static void SendMidiPan(Document *doc,MIDIUniqueID destDevID, char channel, Byte panSetting, MIDITimeStamp tStamp) 
{
	CMMIDIPan(destDevID, channel, panSetting, tStamp);	
}

Boolean IsMidiSustainOn(LINK pL) 
{
	if (ObjLType(pL) == GRAPHICtype) {
		PGRAPHIC p = GetPGRAPHIC(pL);
		return (p->graphicType == GRMIDISustainOn);
	}
		
	return FALSE;
}

Boolean IsMidiSustainOff(LINK pL) 
{
	if (ObjLType(pL) == GRAPHICtype) {
		PGRAPHIC p = GetPGRAPHIC(pL);
		return (p->graphicType == GRMIDISustainOff);
	}
		
	return FALSE;
}

Boolean IsMidiPan(LINK pL) 
{
	if (ObjLType(pL) == GRAPHICtype) {
		PGRAPHIC p = GetPGRAPHIC(pL);
		return (p->graphicType == GRMIDIPan);
	}
		
	return FALSE;
}

Boolean IsMidiController(LINK pL) 
{
	if (IsMidiSustainOn(pL) ||
		 IsMidiSustainOff(pL) ||
		 IsMidiPan(pL)) 
	{
		return TRUE;
	}
	
	return FALSE;
}

Byte GetMidiControlNum(LINK pL) 
{
	if (IsMidiSustainOn(pL) || IsMidiSustainOff(pL)) {
		return MSUSTAIN;
	}
	if (IsMidiPan(pL)) {
		return MPAN;
	}
	return 0;	
}

Byte GetMidiControlVal(LINK pL) 
{
	Byte ctrlVal = 0;
	
	if (IsMidiSustainOn(pL)) {
		ctrlVal = 127;
	}
	if (IsMidiSustainOff(pL)) {
		ctrlVal = 0;		
	}
	if (IsMidiPan(pL)) {
		ctrlVal = GraphicINFO(pL);
	}

	return ctrlVal;
}

#define ERR_STOPNOTE -1000000
#define ERR_PLAYNOTE -1000001

/* ----------------------------------------------------------------- PlaySequence -- */
/*	Play [fromL,toL) of the given score and, if user hits the correct keys while
playing, add barlines.  While playing, we maintain a list of currently-playing notes,
which we use to decide when to stop playing each note. (With MIDI Manager v.2, it
would probably be better to do this with invisible input and output ports, as de-
scribed in the MIDI Manager manual.) */

#define CH_BARTAP 0x09								/* Character code for insert-barline key  */
#define MAX_TCONVERT 100							/* Max. no. of tempo changes we handle */

void PlaySequence(
				Document *doc,
				LINK fromL,	LINK toL,	/* range to be played */
				Boolean showit,			/* TRUE if we want visual feedback */
				Boolean selectedOnly		/* TRUE if we want to play selected notes only */
				)
{
	PPAGE			pPage;
	PSYSTEM		pSystem;
	LINK			pL, oldL, showOldL, aNoteL;
	LINK			systemL, pageL, measL, newMeasL;
	CursHandle	playCursor;
	INT16			i;
	INT16			useNoteNum,
					useChan, useVelo;
	long			t,
					toffset,										/* PDUR start time of 1st note played */
					playDur,
					plStartTime, plEndTime,					/* in PDUR ticks */
					startTime, oldStartTime, endTime;	/* in milliseconds */
	long			tBeforeTurn, tElapsed;
	Rect			syncRect, sysRect, r,
					oldPaper, syncPaper, pagePaper;
	Boolean		paperOnDesktop, moveSel, newPage,
					doScroll, tooManyTempi;
	SignedByte	partVelo[MAXSTAVES];
	Byte			partChannel[MAXSTAVES];
	short			partTransp[MAXSTAVES];
	Byte			channelPatch[MAXCHANNEL];

	short			useIORefNum;					/* NB: can be fmsUniqueID */
	Byte			partPatch[MAXSTAVES];
	Byte			partBankSel0[MAXSTAVES];	/* bank select for FreeMIDI only */
	Byte			partBankSel32[MAXSTAVES];
	short			partIORefNum[MAXSTAVES];
	fmsUniqueID partDevice[MAXSTAVES];

	INT16			oldCurrentSheet, tempoCount, barTapSlopMS;
	EventRecord	theEvent;
	char			theChar;
	TCONVERT		tConvertTab[MAX_TCONVERT];
	char			fmtStr[256];
	short			velOffset, durFactor, timeFactor;
	
	INT16			 notePartn;
	MIDIUniqueID partDevID;
	Boolean		patchChangePosted = FALSE;
	Boolean		sustainOnPosted = FALSE;
	Boolean		sustainOffPosted = FALSE;
	Boolean		panPosted = FALSE;
	

	WaitCursor();

/* Get initial system Rect, set all note play times, get part attributes, etc. */

#if DEBUG_KEEPTIMES
	nkt = 0;
#endif

	doScroll = config.turnPagesInPlay;

	systemL = LSSearch(fromL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	pSystem = GetPSYSTEM(systemL);
	D2Rect(&pSystem->systemRect, &sysRect);

	if (useWhichMIDI==MIDIDR_OMS) {
		OMSUniqueID omsPartDevice[MAXSTAVES];
		if (!GetOMSPartPlayInfo(doc, partTransp, partChannel, partPatch, partVelo,
									partIORefNum, omsPartDevice))
			return;	/* User cancelled playback. */
	}
	else if (useWhichMIDI==MIDIDR_FMS) {
		if (!GetFMSPartPlayInfo(doc, partTransp, partChannel, partPatch,
								partBankSel0, partBankSel32, partVelo, partDevice))
			return;
	}
	if (useWhichMIDI==MIDIDR_CM) {
#if CMDEBUG
		DebugPrintf("doc inputDev=%ld\n", doc->cmInputDevice);
#endif
		if (doc->cmInputDevice == kInvalidMIDIUniqueID)
			doc->cmInputDevice = gSelectedInputDevice;
#if CMDEBUG
		DebugPrintf("doc inputDev=%ld\n", doc->cmInputDevice);
#endif
		MIDIUniqueID cmPartDevice[MAXSTAVES];
		if (!GetCMPartPlayInfo(doc, partTransp, partChannel, partPatch, partVelo,
									partIORefNum, cmPartDevice)) {
#if CMDEBUG
			CMDebugPrintXMission();
#endif
			MayErrMsg("Unable to play score: can't get part device for every part");
			return;	/* User cancelled playback. */
		}
	}
	else
		GetPartPlayInfo(doc, partTransp, partChannel, channelPatch, partVelo);

	InitEventList();

	InitAddBarlines();

	showOldL = oldL = NILINK;									/* not yet playing anything */
	newMeasL = measL = SSearch(fromL, MEASUREtype, GO_LEFT);	/* starting measure */
	playCursor = GetCursor(MIDIPLAY_CURS);
	if (playCursor) SetCursor(*playCursor);
	moveSel = FALSE;												/* init. "move the selection" flag */
	
	pageL = LSSearch(fromL, PAGEtype, ANYONE, GO_LEFT, FALSE);
	pPage = GetPPAGE(pageL);

	oldCurrentSheet = doc->currentSheet;
	oldPaper = doc->currentPaper;
	doc->currentSheet = pPage->sheetNum;
	paperOnDesktop =
		(GetSheetRect(doc,doc->currentSheet,&doc->currentPaper)==INARRAY_INRANGE);
	pagePaper = doc->currentPaper;

	barTapSlopMS = 10*config.barTapSlop;
	oldStartTime = -999999L;
	
	tempoCount = MakeTConvertTable(doc, fromL, toL, tConvertTab, MAX_TCONVERT);
	tooManyTempi = (tempoCount<0);
	if (tooManyTempi) tempoCount = MAX_TCONVERT;				/* Table size exceeded */

	switch (useWhichMIDI) {
		case MIDIDR_FMS:
			FMSInitTimer();
			break;
		case MIDIDR_OMS:
			OMSInitTimer();
			break;
		case MIDIDR_MM:
			break;
		case MIDIDR_CM:
			CMSetup(doc, partChannel);
			break;
		case MIDIDR_BI:
			MayInitBIMIDI(BIMIDI_SMALLBUFSIZE, BIMIDI_SMALLBUFSIZE);	/* initialize serial chip */
			InitBIMIDITimer();													/* initialize millisecond timer */
			break;
		default:
			break;
	}

	for (pL = doc->headL; pL!=fromL; pL = RightLINK(pL)) 
	{
		if (IsMidiPatchChange(pL)) {
			PostMidiProgramChange(doc, pL, partPatch, partChannel);
		}
	}
	
	ClearMidiSustain(TRUE);
	ClearMidiSustain(FALSE);
	ClearMidiPan();
	
	ClearAllMidiSustainOn();
	ClearAllMidiPan();

	/*
	 *	If "play on instruments' parts" is set, send out program changes to the correct
	 *	patch numbers for all channels that have any instruments assigned to them.
	 */
	if (useWhichMIDI == MIDIDR_OMS) {
		OMSMIDIProgram(doc, partPatch, partChannel, partIORefNum);
	}
	else if (useWhichMIDI == MIDIDR_FMS) {
		FMSMIDIProgram(doc);
	}
	else if (useWhichMIDI == MIDIDR_CM) {
		OSErr err = CMMIDIProgram(doc, partPatch, partChannel);
		if (err != noErr) {
			ErrorMsg(20);
			//MayErrMsg("Unable to Play Score: go to the Instrument MIDI Settings dialog and make sure a device is set to all parts.");
			return;
		}
	}
	else {
		if (doc->polyTimbral && !doc->dontSendPatches) {
			for (i = 1; i<=MAXCHANNEL; i++)
				if (channelPatch[i]<=MAXPATCHNUM) SetMIDIProgram(i, channelPatch[i]);
		}
	}
	SleepTicks(10L);	/* Let synth settle after patch change, before playing notes. */

	PlayMessage(doc, fromL, -1);

	pageTurnTOffset = 0L;
	StartMIDITime();

	/* The stored play time of the first Sync we're going to play might be any
	 *	positive value but we want to start playing immediately, so we'll pick up
	 *	the first Sync's play time and use it as an offset on all play times.
	 */
	toffset = -1L;															/* Init. time offset to unknown */
	newPage = FALSE;
	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case PAGEtype:
				pPage = GetPPAGE(pL);
				doc->currentSheet = pPage->sheetNum;
				paperOnDesktop =
					(GetSheetRect(doc,doc->currentSheet,&doc->currentPaper)==INARRAY_INRANGE);
				pagePaper = doc->currentPaper;
				newPage = TRUE;
				break;
			case SYSTEMtype:												/* Remember system rectangles */
				pSystem = GetPSYSTEM(pL);
				D2Rect(&pSystem->systemRect, &sysRect);
				break;
			case MEASUREtype:
				newMeasL = measL = pL;
				break;
			case SYNCtype:
			  	if (LinkSEL(pL) || !selectedOnly) {
			  		if (SyncTIME(pL)>MAX_SAFE_MEASDUR)
			  			MayErrMsg("PlaySequence: pL=%ld has timeStamp=%ld", (long)pL,
			  							(long)SyncTIME(pL));
#ifdef TDEBUG
					if (toffset<0L) DebugPrintf("toffset=%ld => %ld\n", toffset, MeasureTIME(measL)+SyncTIME(pL));
#endif
			  		plStartTime = MeasureTIME(measL)+SyncTIME(pL);
					startTime = PDur2RealTime(plStartTime, tConvertTab, tempoCount);	/* Convert play time to millisecs. */
			  		if (toffset<0L) toffset = startTime;
					startTime -= toffset;
					aNoteL = FirstSubLINK(pL);
		
					/*
					 * Wait 'til it's time to play pL. While waiting, check for notes
					 * ending and take care of them. Also check for relevant keyboard
					 * events: the codes for Stop and Select what's playing, Cancel
					 * (just stop playing), and Insert Barline. For the latter, if we're
					 * about to start playing pL, assume the user wants the barline
					 * before pL; otherwise assume they want it before the previous Sync.
					 *
					 * NB: it seems as if the comment in WriteMFNotes about having to write
					 *	notes ending at a given time before those beginning at the same time
					 *	should apply here too, but we're not doing that here and I haven't
					 * noticed any problems. And doing it this way should help get notes
					 *	started as close as possible to their correct times.
					 */
					do {
						t = GetMIDITime(pageTurnTOffset);
						
						if (moveSel = UserInterruptAndSel()) goto done;	/* Check for Stop/Select */
						if (moveSel = CheckButton()) goto done;		/* Check for Stop/Select */
						if (UserInterrupt()) goto done;					/* Check for Cancel */
						
#ifdef USE_GETOSEVENT		
						GetOSEvent(keyDownMask, &theEvent);				/* Not Wait/GetNextEvent so we do as little as possible */
#else
						GetNextEvent(keyDownMask, &theEvent);			/* Not Wait/GetNextEvent so we do as little as possible */
#endif
						theChar = (char)theEvent.message & charCodeMask;
						if (theChar==CH_BARTAP)	{							/* Check for Insert Barline */
							LINK insL = (t-oldStartTime<barTapSlopMS? oldL : pL);
							AddBarline(insL);
						}
						CheckEventList(pageTurnTOffset);					/* Check for, turn off any notes done */
								
						if (useWhichMIDI==MIDIDR_CM)
							CMCheckEventList(pageTurnTOffset);					/* Check for, turn off any notes done */
						else
							CheckEventList(pageTurnTOffset);					/* Check for, turn off any notes done */
					} while (t<startTime);
		
					if (newMeasL) {
						PlayMessage(doc, newMeasL, -1);
						newMeasL = NILINK;
					}
					oldL = pL;
					oldStartTime = startTime;
					if (showit) {
						if (showOldL) HiliteSyncRect(doc,&syncRect,&syncPaper,FALSE);  /* unhilite old sync */
						if (paperOnDesktop) {
							syncRect = sysRect;
							/*
							 * We use the objRect to determine what to hilite. Problem: If this
							 * Sync isn't in view, its objRect may be empty or, worse, garbage!
							 * Should be fixed someday.
							 */
							r = LinkOBJRECT(pL);
							syncRect.left = r.left;
							syncRect.right = r.right;
							syncPaper = pagePaper;
							tBeforeTurn = GetMIDITime(pageTurnTOffset);
#ifdef PLDEBUG
DebugPrintf("pL=%ld: rect.l=%ld,r=%ld paper.l=%ld,r=%ld\n",
pL,syncRect.left,syncRect.right,syncPaper.left,syncPaper.right);
#endif
							HiliteSyncRect(doc,&syncRect,&syncPaper,newPage && doScroll); /* hilite new sync */
							tElapsed = GetMIDITime(pageTurnTOffset)-tBeforeTurn;
							pageTurnTOffset += tElapsed;
							showOldL = pL;										/* remember this sync */
							newPage = FALSE;
							}
						else showOldL = NILINK;
					}
					
					if (patchChangePosted) {
						CMMIDIProgram(doc, partPatch, partChannel);
						patchChangePosted = FALSE;
					}
					if (sustainOnPosted) {
						SendAllMidiSustains(doc, partChannel, TRUE);
						ClearMidiSustain(TRUE);
						sustainOnPosted = FALSE;
					}
					if (sustainOffPosted) {
						SendAllMidiSustains(doc, partChannel, FALSE);
						ClearMidiSustain(FALSE);
						sustainOffPosted = FALSE;
					}
					if (panPosted) {
						SendAllMidiPans(doc, partChannel);
						ClearMidiPan();
						panPosted = FALSE;
					}
		
		/* Play all the notes in <<pL> we're supposed to, adding them to <eventList[]> too */
		
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
						if (NoteSEL(aNoteL) || !selectedOnly) {
							/*
							 *	Get note's MIDI note number, including transposition; velocity,
							 *	limited to legal range; channel number; and duration, includ-
							 * ing any tied notes to right.
							 */
							if (useWhichMIDI==MIDIDR_OMS)
								GetOMSNotePlayInfo(doc, aNoteL, partTransp, partChannel, partVelo,
														partIORefNum,
														&useNoteNum, &useChan, &useVelo, &useIORefNum);
							else if (useWhichMIDI==MIDIDR_FMS) {
								fmsUniqueID deviceID;
								GetFMSNotePlayInfo(doc, aNoteL, partTransp, partChannel, partVelo,
														partDevice,
														&useNoteNum, &useChan, &useVelo, &deviceID);
								useIORefNum = (short)deviceID;
							}
							else if (useWhichMIDI==MIDIDR_CM) {
								CMGetNotePlayInfo(doc, aNoteL, partTransp, partChannel, partVelo,
														&useNoteNum, &useChan, &useVelo);
							}
							else
								GetNotePlayInfo(doc, aNoteL, partTransp, partChannel, partVelo,
														&useNoteNum, &useChan, &useVelo);
							
							playDur = TiedDur(doc, pL, aNoteL, selectedOnly);

							/* NOTE: We don't try to use timeFactor. */
							if (GetModNREffects(aNoteL, &velOffset, &durFactor, &timeFactor)) {
								useVelo += velOffset;
								useVelo = clamp(0, useVelo, 127);
								playDur = (long)(playDur * durFactor) / 100L;
							}

							plEndTime = plStartTime+playDur;						

							/* If it's a real note (not rest or continuation), send it out */
							
							if (useNoteNum>=0) {
								OSStatus err = noErr;
								notePartn = Staff2Part(doc,NoteSTAFF(aNoteL));
								partDevID = GetCMDeviceForPartn(doc, notePartn);
								
								if (useWhichMIDI==MIDIDR_CM)
									err = CMStartNoteNow(partDevID, useNoteNum, useChan, useVelo);
								else
									err = StartNoteNow(useNoteNum, useChan, useVelo, useIORefNum);
									
								endTime = PDur2RealTime(plEndTime, tConvertTab, tempoCount);	/* Convert time to millisecs. */
								endTime -= toffset;
								
								if (useWhichMIDI==MIDIDR_CM) {
									if (!CMEndNoteLater(useNoteNum, useChan, endTime, partDevID)) {
										err = ERR_STOPNOTE;
										goto done;										
									}
								}
								else {
									if (!EndNoteLater(useNoteNum, useChan, endTime, useIORefNum)) {
										err = ERR_STOPNOTE;
										goto done;										
									}
								}
									
								if (err != noErr) {
									err = ERR_PLAYNOTE;
									goto done;
								}
							}
						}
					}
				}
				break;
			case GRAPHICtype:
				if (useWhichMIDI==MIDIDR_CM) {				
					if (IsMidiPatchChange(pL)) {
						patchChangePosted = PostMidiProgramChange(doc, pL, partPatch, partChannel);						
					}
					else if (IsMidiSustainOn(pL)) 
					{
						sustainOnPosted = PostMidiSustain(doc, pL, TRUE);
					}
					else if (IsMidiSustainOff(pL)) 
					{
						sustainOffPosted = PostMidiSustain(doc, pL, FALSE);
					}
					else if (IsMidiPan(pL)) 
					{
						panPosted = PostMidiPan(doc, pL);
					}					
				}
				break;
			default:
				;
		}
	}

	if (useWhichMIDI==MIDIDR_CM) {
		while (!CMCheckEventList(pageTurnTOffset)) {				/* Wait til eventList[] empty... */
			if (moveSel = UserInterruptAndSel()) goto done;		/* Check for Stop/Select */
			if (moveSel = CheckButton()) goto done;				/* Check for Stop/Select */
			if (UserInterrupt()) goto done;							/* Check for Cancel */

	#ifdef USE_GETOSEVENT		
			GetOSEvent(keyDownMask, &theEvent);						/* Not Wait/GetNextEvent so we do as little as possible */
	#else
			GetNextEvent(keyDownMask, &theEvent);
	#endif
			theChar = (char)theEvent.message & charCodeMask;
			if (theChar==CH_BARTAP)										/* Check for Insert Barline */
			AddBarline(oldL);
		}
	}
	else {
		while (!CheckEventList(pageTurnTOffset)) {				/* Wait til eventList[] empty... */
			if (moveSel = UserInterruptAndSel()) goto done;		/* Check for Stop/Select */
			if (moveSel = CheckButton()) goto done;				/* Check for Stop/Select */
			if (UserInterrupt()) goto done;							/* Check for Cancel */

	#ifdef USE_GETOSEVENT		
			GetOSEvent(keyDownMask, &theEvent);						/* Not Wait/GetNextEvent so we do as little as possible */
	#else
			GetNextEvent(keyDownMask, &theEvent);
	#endif
			theChar = (char)theEvent.message & charCodeMask;
			if (theChar==CH_BARTAP)										/* Check for Insert Barline */
			AddBarline(oldL);
		}
	}
			
done:
	if (showit && showOldL) HiliteSyncRect(doc,&syncRect,&syncPaper,FALSE); /* unhilite last sync */

	doc->currentSheet = oldCurrentSheet;
	doc->currentPaper = oldPaper;
	
	// Sleep a half a second before turning off sustain and pan
	
	//SleepTicks(30L);
	ResetMidiSustain(doc, partChannel);
	ResetMidiPan(doc, partChannel);
	
	StopMIDI();
	CloseAddBarlines(doc);
	
	if (useWhichMIDI == MIDIDR_CM) {
		CMTeardown();
	}

	if (tooManyTempi) {
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 3);			/* "Nightingale can only play %d tempo changes" */
		sprintf(strBuf, fmtStr, MAX_TCONVERT);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}

	if (moveSel) {
	/*
	 *	We want to change the selection to only the notes we were just playing.
	 *	If the Sync that was playing is in the selection range, we deselect every-
	 *	thing else in the range, otherwise we deselect absolutely everything.  Then
	 *	we make sure that the correct notes in the Sync are selected.
	 */

		if (WithinRange(doc->selStartL, oldL, doc->selEndL)) {
			DeselRange(doc, doc->selStartL, oldL);
			DeselRange(doc, RightLINK(oldL), doc->selEndL);
		}
		else DeselAll(doc);

		if (!selectedOnly) SelAndHiliteSync(doc, oldL);

		LinkSEL(oldL) = TRUE;
		doc->selStartL = oldL;
		doc->selEndL = RightLINK(oldL);
	}
	EraseAndInvalMessageBox(doc);
	ArrowCursor();

#if DEBUG_KEEPTIMES
	{ INT16 nk; for (nk=0; nk<nkt; nk++)
		DebugPrintf("nk=%d kStartTime[]=%ld\n", nk, kStartTime[nk]-kStartTime[0]);
	}
#endif
}


/* ------------------------------------------------------------------ PlayEntire -- */
/*	Play the entire score. */

void PlayEntire(Document *doc)
{
	LINK firstPage;
	
	firstPage = LSSearch(doc->headL, PAGEtype, ANYONE, GO_RIGHT, FALSE);
	PlaySequence(doc, RightLINK(firstPage), doc->tailL, TRUE, FALSE);
}


/* --------------------------------------------------------------- PlaySelection -- */
/*	Play the current selection. */

void PlaySelection(Document *doc)
{
	PlaySequence(doc, doc->selStartL, doc->selEndL, TRUE, TRUE);
}
