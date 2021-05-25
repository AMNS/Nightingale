/******************************************************************************************
*	FILE:	MIDIPlay.c
*	PROJ:	Nightingale
*	DESC:	MIDI playback routines
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <CoreAudio/HostTime.h>

#include "CoreMIDIDefs.h"

/* As of version 5.4, if not earlier, Nightingale supports Apple's Core MIDI and no
other MIDI driver. It formerly supported, in more-or-less this order, the "MacTutor"
driver, MIDI Pascal, Apple MIDI Manager, Opcode's OMS, and Mark of the Unicorn's
FreeMIDI. The earlier drivers all predate OS X, and if it's even possible, I seriously
doubt if it's desirable to revive any of the earlier drivers. Unfortunately, there's
still a lot of code here and in other MIDI-related files left over from earlier drivers.

The following comment dates back to the 20th century: "Nightingale supports both MIDI
Manager and its own 'built-in' MIDI routines. The latter, used here and in MIDIRecord.c,
are Altech Systems' MIDI Pascal 3.0. We used to use a driver from Kirk Austin's articles
in MacTutor, July and December 1987, but those routines became quite buggy--according to
Jeremy Sagan, because they don't initialize enough registers. Anyway, MIDI Pascal is
much more powerful, and slightly less ancient (sigh)."

FIXME: The old code should be removed!  --DAB */


/* ==================================== LOCAL STUFF ==================================== */

static long		pageTurnTOffset;

static void		MPErrorMsg(short index);
static short	MPDoGeneralAlert(unsigned char *str);
static void		PlayMessage(Document *, LINK, short);
static Boolean	HiliteSyncRect(Document *doc, Rect *syncRect, Rect *rPaper, Boolean scroll);
static long		ScaleDurForVariableSpeed(long dur);

#define CMDEBUG 0
#define PLDEBUG 0

/* Print an error message.  If index is non-zero, then retrieve the index'th string from
our error strings resource. */

static void MPErrorMsg(short index)
{
	Str255 str;
	
	if (index > 0)	GetIndString(str, MIDIPLAYERRS_STRS, index);
	else			*str = 0;
	(void)MPDoGeneralAlert(str);
}

static short MPDoGeneralAlert(unsigned char *str)
{
	ParamText(str, "\p", "\p", "\p");
	PlaceAlert(errorMsgID, NULL, 0, 40);
	return(StopAlert(errorMsgID, NULL));
}


/* ----------------------------------------------------------------------- PlayMessage -- */
/* Write a message into the message area saying what measure is playing; if not playing
at the "correct" (marked) tempo, what the relative speed is; and, if a part is muted,
saying that. If <pL> is NILINK, use <measNum> as measure no., else get it from <pL>'s
Measure. */

static void PlayMessage(Document *doc, LINK pL, short measNum)
{
	Rect messageRect;

	if (pL!=NILINK) measNum = GetMeasNum(doc, pL);

	PrepareMessageDraw(doc,&messageRect, False);
	GetIndCString(strBuf, MIDIPLAY_STRS, 1);				/* "Playing m. " */
	DrawCString(strBuf);					
	sprintf(strBuf, "%d", measNum);
	DrawCString(strBuf);
	if (doc->mutedPartNum!=0) {
		TextFace(bold);
		sprintf(strBuf, " M");
		DrawCString(strBuf);
		TextFace(0);										/* Plain */
	}
	if (playTempoPercent!=100) {
		TextFace(bold);
		sprintf(strBuf, "  T%d%%", playTempoPercent);
		DrawCString(strBuf);
		TextFace(0);										/* Plain */
	}
	GetIndCString(strBuf, MIDIPLAY_STRS, 2);				/* "   CLICK OR CMD-. TO STOP" */
	DrawCString(strBuf);					
	FinishMessageDraw(doc);
}


/* -------------------------------------------------------------------- HiliteSyncRect -- */
/* Given a rectange, r, in paper-relative coordinates, hilite it in the current
window.  If it's not in view, "scroll" so its page is in the window (though r might
still not be in the window!) and return True; else return False.

Note that scrolling while playing screws up timing (simply by introducing a break)
unless interrupt-driven, but after all, Nightingale is a notation program, not a
sequencer: it's more important that the music being played is in view.

NB: The "scrolling" code here can itself change doc->currentPaper. For this and
other reasons, we expect the appropriate doc->currentPaper as a parameter. */

static Boolean HiliteSyncRect(
					Document *doc,
					Rect *r,
					Rect *rPaper,				/* doc->currentPaper for r's page */
					Boolean scroll)
{
	Rect result;  short x, y;
	Boolean turnedPage=False;
	
	/* Temporarily convert r to window coords. Normally, we do this by offsetting by
	   doc->currentPaper, but in this case, doc->currentPaper may have changed (if r's
	   Sync was the last played on a page); so we have to use the currentPaper for r's
	   Sync. */

	OffsetRect(r, rPaper->left, rPaper->top);
	
	/* Code to scroll while playing. See comment on timing above. */
	
	if (scroll) {
		if (!SectRect(r, &doc->viewRect, &result)) {
			/* Rect to be hilited is outside of window's view, so scroll paper */
			
			x = doc->currentPaper.left - config.hPageSep;
			y = doc->currentPaper.top  - config.vPageSep;
			QuickScroll(doc, x, y, False, True);
			turnedPage = True;
			}
		}
	
	HiliteRect(r);
	
	/* Convert back to paper coords */
	
	OffsetRect(r, -rPaper->left, -rPaper->top);
	
	return turnedPage;
}


/* ------------------------------------------------------------- AddBarlines functions -- */
/* For the "add barlines while playing" feature, build up a list of places to add
single barlines (actually Measure objects):
	InitAddBarlines()			Initialize these functions
	AddBarline(pL)				Request adding a barline before <pL>
	CloseAddBarlines(doc)		Add the barlines at all requested places
Duplicate calls to AddBarline at the same link are ignored.*/

#define MAX_BARS 300			/* Maximum no. of barlines we can add in one set */

static LINK barBeforeL[MAX_BARS];
static short nBars;

void InitAddBarlines(void);
void AddBarline(LINK);
static Boolean TupletProblem(Document *, LINK);
Boolean CloseAddBarlines(Document *);

void InitAddBarlines()
{
	nBars = 0;
}

void AddBarline(LINK pL)
{
	if (pL && (nBars==0 || pL!=barBeforeL[nBars-1])) {
		if (nBars<MAX_BARS)
			barBeforeL[nBars] = pL;
		nBars++;										/* Register attempt so we can tell user later */
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
		return False;
	else {
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 1);	/* "can't add a barline in middle of tuplet" */
		sprintf(strBuf, fmtStr, tupStaff);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return True;
	}
}

Boolean CloseAddBarlines(Document *doc)
{
	short symIndex, n;
	CONTEXT context;
	LINK insL, newMeasL, firstInsL, lastInsL, saveSelStartL, saveSelEndL, prevSync;
	Boolean okay=True;
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
	
	/* Put barlines that would otherwise go at the beginning of a system at the end of
	   of the previous system instead. */
	
	for (n = 0; n<nBars; n++) {
		prevSync = SSearch(LeftLINK(barBeforeL[n]), SYNCtype, GO_LEFT);
		if (prevSync && !SameSystem(prevSync, barBeforeL[n]))
			barBeforeL[n] = RightLINK(prevSync);
	}

	/* Save current selection range so we can set range for PrepareUndo, then restore
	   it. Since Undo works on whole systems, FindInsertPt can't affect it. */
	
	saveSelStartL = doc->selStartL;
	saveSelEndL = doc->selEndL;
	doc->selStartL = LeftLINK(barBeforeL[0]);
	doc->selEndL = barBeforeL[nBars-1];
	
	PrepareUndo(doc, LeftLINK(barBeforeL[0]), U_TapBarlines, 29);	/* "Undo Tap-in Barlines" */
	
	doc->selStartL = saveSelStartL;
	doc->selEndL = saveSelEndL;
	symIndex = GetSymTableIndex(singleBarInChar);					/* single barline */
	for (n = 0; n<nBars; n++) {
		GetContext(doc, LeftLINK(barBeforeL[n]), 1, &context);
		insL = FindInsertPt(barBeforeL[n]);
		if (TupletProblem(doc, insL))
			{ nBars = n; break; }
		if (n==0) firstInsL = insL;
		newMeasL = CreateMeasure(doc, insL, -1, symIndex, context);
		if (!newMeasL) return False;
		DeselectNode(newMeasL);
	}

	if (nBars==0) return okay;

	lastInsL = insL;
	okay = RespaceBars(doc, LeftLINK(firstInsL), RightLINK(lastInsL), 
		RESFACTOR*(long)doc->spacePercent, False, False);
	InvalWindow(doc);
	return okay;
}


/* ------------------------------------------------------------------ SelAndHiliteSync -- */
/* Select and hilite the given Sync, and set the document's scaleCenter (for
magnification) to it. */

static void SelAndHiliteSync(Document *, LINK);
static void SelAndHiliteSync(Document *doc, LINK syncL)
{
	CONTEXT		context[MAXSTAVES+1];
	Boolean		found;
	short		index;
	STFRANGE	stfRange={0,0};
	DDIST		xd, yd;
	
	GetAllContexts(doc, context, syncL);
	CheckObject(doc, syncL, &found, NULL, context, SMSelect, &index, stfRange);

	xd = PageRelxd(syncL, &context[1]);
	doc->scaleCenter.h = context[1].paper.left+d2p(xd);
	yd = PageRelyd(syncL, &context[1]);
	doc->scaleCenter.v = context[1].paper.top+d2p(yd);
}


/* ----------------------------------------------------------------------- CheckButton -- */

static Boolean CheckButton(void);
static Boolean CheckButton()
{
	Boolean button = False;
	
	EventRecord evt; 

	if (!GetNextEvent(mDownMask,&evt)) return(button);	/* Not Wait/GetNextEvent so we do as little as possible */

	if (evt.what==mouseDown) button = True;

 	return button;
}

#define DEBUG_KEEPTIMES 1
#if DEBUG_KEEPTIMES
#define MAXKEEPTIMES 20
long kStartTime[MAXKEEPTIMES];
short nkt;
#endif

/* -------------------------------------------------------------------------------------- */

static void SetPartPatch(short partn, Byte partPatch[], Byte /* partChannel */ [],
						Byte patchNum)
{
	short patch = patchNum + CM_PATCHNUM_BASE;

	partPatch[partn] = patch;	
}


#define PAN_CENTER 64						/* Pan to both sides equally */

static void SendMIDISustainOn(Document *doc, MIDIUniqueID destDevID, char channel);
static void SendMIDISustainOff(Document *doc, MIDIUniqueID destDevID, char channel);
static void SendMIDIPan(Document *doc, MIDIUniqueID destDevID, char channel, Byte panSetting);
static void SendMIDISustainOn(Document *doc, MIDIUniqueID destDevID, char channel, MIDITimeStamp tStamp);
static void SendMIDISustainOff(Document *doc, MIDIUniqueID destDevID, char channel, MIDITimeStamp tStamp);
static void SendMIDIPan(Document *doc, MIDIUniqueID destDevID, char channel, Byte panSetting, MIDITimeStamp tStamp);

static Boolean cmSustainOn[MAXSTAVES + 1];
static Boolean cmSustainOff[MAXSTAVES + 1];
static Byte cmPanSetting[MAXSTAVES + 1];

static Boolean cmAllSustainOn[MAXSTAVES + 1];
static Byte cmAllPanSetting[MAXSTAVES + 1];

/* --------------------------------------------------------------- SendMIDIPatchChange -- */
/* Steps:
	1. Get the Graphic
	2. Check that the object it's attached to is a Sync
	3. Get the partn from the Graphic's staff
	4. Update the part patch in the part patch array. See GetPartPlayInfo for this.
		Updates channelPatch[].
	6. Send the program packet
			CMMIDIProgram(doc, partPatch, partChannel);
 		with the progressively updated patch number.
*/

static Boolean PostMIDIPatchChange(Document *doc, LINK pL, unsigned char *partPatch,
									 unsigned char *partChannel) 
{
	Boolean posted = False;
	PGRAPHIC p = GetPGRAPHIC(pL);
	LINK firstL = p->firstObj;
	if (ObjLType(firstL) == SYNCtype) {
		short patchNum = p->info;
		short stf = p->staffn;
		if (stf > 0) {
			LINK partL = Staff2PartL(doc, doc->headL, stf);
			short partn = PartL2Partn(doc, partL);
			SetPartPatch(partn, partPatch, partChannel, patchNum);
			//CMMIDIProgram(doc, partPatch, partChannel);
			posted = True;
		}
	}
	return posted;
}

static Boolean PostMIDISustain(Document * /* doc */, LINK pL, Boolean susOn) 
{
	Boolean posted = False;
						
	short stf = GraphicSTAFF(pL);
	if (stf > 0) {
		if (susOn) {
			cmSustainOn[stf] = cmAllSustainOn[stf] = True;
		}		
		else {
			cmSustainOff[stf] = True;			
		}
		posted = True;
	}
	
	return posted;
}

static Boolean PostMIDIPan(Document * /* doc */, LINK pL)
{
	Boolean posted = False;
	
	short stf = GraphicSTAFF(pL);
	if (stf > 0) {
		Byte panSetting = GraphicINFO(pL);
		cmPanSetting[stf] = cmAllPanSetting[stf] = panSetting;
		posted = True;
	}
	
	return posted;
}

static void ClearMIDISustain(Boolean susOn) 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (susOn) {
			cmSustainOn[j] = False;
		}		
		else {
			cmSustainOff[j] = False;			
		}
	}	
}

static void ClearMIDIPan() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmPanSetting[j] = -1;
	}
}

static void ClearAllMIDISustainOn() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmAllSustainOn[j] = False;
	}
}

static void ClearAllMIDIPan() 
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

static void ResetMIDISustain(Document *doc, unsigned char *partChannel) 
{
	long long secs = GetSustainSecs();
	MIDITimeStamp tStamp = TimeStampSecsFromNow(secs);
	
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (cmAllSustainOn[j]) {
			short partn = Staff2Part(doc,j);
			MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
			short channel = CMGetUseChannel(partChannel, partn);
			SendMIDISustainOff(doc, partDevID, channel, tStamp);										
		}
	}
	
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmAllSustainOn[j] = False;
	}
}


static Boolean ValidPanSetting(Byte panSetting) 
{
	SignedByte sbpanSetting = (SignedByte)panSetting;
	
	return sbpanSetting >= 0;
}

static void ResetMIDIPan(Document *doc, unsigned char *partChannel) 
{
	MIDITimeStamp tStamp = TimeStampSecsFromNow(0);
	
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (ValidPanSetting(cmAllPanSetting[j])) {
			short partn = Staff2Part(doc,j);
			MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
			short channel = CMGetUseChannel(partChannel, partn);
			SendMIDIPan(doc, partDevID, channel, PAN_CENTER, tStamp);		
		}
	}
	
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmAllPanSetting[j] = -1;
	}
}

static void SendAllMIDISustains(Document *doc, unsigned char *partChannel, Boolean susOn) 
{
	if (susOn) {
		for (int j = 1; j<=MAXSTAVES; j++) {
			if (cmSustainOn[j]) {
				short partn = Staff2Part(doc,j);
				MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
				short channel = CMGetUseChannel(partChannel, partn);
				SendMIDISustainOn(doc, partDevID, channel);							
			}
		}
	}
	else {
		for (int j = 1; j<=MAXSTAVES; j++) {
			if (cmSustainOff[j]) {
				short partn = Staff2Part(doc,j);
				MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
				short channel = CMGetUseChannel(partChannel, partn);
				SendMIDISustainOff(doc, partDevID, channel);							
			}
		}		
	}
}

static void SendAllMIDIPans(Document *doc, unsigned char *partChannel) 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (ValidPanSetting(cmPanSetting[j])) {
			short partn = Staff2Part(doc, j);
			MIDIUniqueID partDevID = GetCMDeviceForPartn(doc, partn);
			short channel = CMGetUseChannel(partChannel, partn);
			SendMIDIPan(doc, partDevID, channel, cmPanSetting[j]);							
		}
	}		
}

static void SendMIDIPatchChange(Document *doc, unsigned char *partPatch, unsigned char
								  *partChannel) 
{
	CMMIDIProgram(doc, partPatch, partChannel);	
}

static Boolean IsMIDIPatchChange(LINK pL) 
{
	if (ObjLType(pL) == GRAPHICtype) {
		PGRAPHIC p = GetPGRAPHIC(pL);
		return (p->graphicType == GRMIDIPatch);
	}
		
	return False;
}

static void SendMIDISustainOn(Document * /* doc */, MIDIUniqueID destDevID, char channel) 
{
	CMMIDISustainOn(destDevID, channel);	
}

static void SendMIDISustainOff(Document * /* doc */, MIDIUniqueID destDevID, char channel) 
{
	CMMIDISustainOff(destDevID, channel);	
}

static void SendMIDIPan(Document * /* doc */, MIDIUniqueID destDevID, char channel, Byte panSetting) 
{
	CMMIDIPan(destDevID, channel, panSetting);	
}

static void SendMIDISustainOn(Document * /* doc */, MIDIUniqueID destDevID, char channel,
							  MIDITimeStamp tStamp) 
{
	CMMIDISustainOn(destDevID, channel, tStamp);	
}

static void SendMIDISustainOff(Document * /* doc */, MIDIUniqueID destDevID, char channel,
							   MIDITimeStamp tStamp) 
{
	CMMIDISustainOff(destDevID, channel, tStamp);	
}

static void SendMIDIPan(Document * /* doc */, MIDIUniqueID destDevID, char channel, Byte panSetting,
						MIDITimeStamp tStamp) 
{
	CMMIDIPan(destDevID, channel, panSetting, tStamp);	
}

Boolean IsPedalDown(LINK pL) 
{
	if (ObjLType(pL) == GRAPHICtype) {
		PGRAPHIC p = GetPGRAPHIC(pL);
		return (p->graphicType == GRSusPedalDown);
	}
		
	return False;
}

Boolean isPedalUp(LINK pL) 
{
	if (ObjLType(pL) == GRAPHICtype) {
		PGRAPHIC p = GetPGRAPHIC(pL);
		return (p->graphicType == GRSusPedalUp);
	}
		
	return False;
}

Boolean IsMIDIPan(LINK pL) 
{
	if (ObjLType(pL) == GRAPHICtype) {
		PGRAPHIC p = GetPGRAPHIC(pL);
		return (p->graphicType == GRMIDIPan);
	}
		
	return False;
}

Boolean IsMIDIController(LINK pL) 
{
	if (IsPedalDown(pL) || isPedalUp(pL) || IsMIDIPan(pL)) 
		return True;
	
	return False;
}

Byte GetMIDIControlNum(LINK pL) 
{
	if (IsPedalDown(pL) || isPedalUp(pL))
		return MSUSTAIN;
	if (IsMIDIPan(pL))
		return MPAN;

	return 0;	
}

Byte GetMIDIControlVal(LINK pL) 
{
	Byte ctrlVal = 0;
	
	if (IsPedalDown(pL)) ctrlVal = 127;
	if (isPedalUp(pL)) ctrlVal = 0;		
	if (IsMIDIPan(pL)) ctrlVal = GraphicINFO(pL);

	return ctrlVal;
}


/* --------------------------------------------------------------- PlaySequence et al. -- */

/* Scale the given real-time duration by some factor. Intended to support playback at
"variable speed", modifying the tempi marked in the score. */

static long ScaleDurForVariableSpeed(long rtDur)
{
	return rtDur*(100.0/playTempoPercent);
}

/* Display in the log file all notes that are to be played. */

static void ListNotesToPlay(Document *doc, LINK fromL, LINK toL, Boolean selectedOnly)
{
	LINK pL, aNoteL;
	short iVoice;
	long playDur;
	
	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL) && AnyNoteToPlay(doc, pL, selectedOnly)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteToBePlayed(doc, aNoteL, selectedOnly)) {
					iVoice = NoteVOICE(aNoteL);
					playDur = TiedDur(doc, pL, aNoteL, selectedOnly);
					LogPrintf(LOG_DEBUG, "Note to play: pL=%u voice=%d noteNum=%d playDur=%ld  (ListNotesToPlay)\n",
								pL, iVoice, NoteNUM(aNoteL), playDur);
				}
			}
		}
	}
}


#define ERR_STOPNOTE -1000000
#define ERR_PLAYNOTE -1000001

/*	Play [fromL,toL) of the given score and, if user hits the correct keys while
playing, add barlines.  While playing, we maintain a list of currently-playing notes,
which we use to decide when to stop playing each note. (With the ancient MIDI Manager v.2,
it would probably be better to do this with invisible input and output ports, as de-
scribed in the MIDI Manager manual. I don't know if this also applies to Core MIDI.)

If <selectedOnly>, we play only the selected notes. The selection need not be continuous.
Notes are played at their correct relative times, even if that means there's silence until
the next selected note's time. */

#define CH_BARTAP 0x09						/* Character code for insert-barline key  */
#define MAX_PLAY_TEMPOCHANGE 500			/* Max. no. of tempo changes we handle */

void PlaySequence(
			Document *doc,
			LINK fromL,	LINK toL,			/* range to be played */
			Boolean showit,					/* True to hilite notes as they're played */
			Boolean selectedOnly			/* True if we want to play selected notes only */
			)
{
	PPAGE		pPage;
	PSYSTEM		pSystem;
	LINK		objL, oldL, showOldL, aNoteL;
	LINK		systemL, pageL, measL, newMeasL;
	CursHandle	playCursor;
	short		useNoteNum,
				useChan, useVelo;
	long		t,
				toffset,								/* PDUR start time of 1st note played */
				playDur,
				plStartTime, plEndTime,					/* in PDUR ticks */
				startTimeNorm, endTimeNorm,				/* in millisec. at tempi marked */
				startTime, oldStartTime, endTime;		/* in actual milliseconds */
	long		tBeforeTurn, tElapsed;
	Rect		syncRect, sysRect, r,
				oldPaper, syncPaper, pagePaper;
	Boolean		paperIsOnDesktop, moveSel, newPage,
				doScroll, tooManyTempi;
	SignedByte	partVelo[MAXSTAVES];
	Byte		partChannel[MAXSTAVES];
	short		partTransp[MAXSTAVES];
	Byte		channelPatch[MAXCHANNEL];

	/* FIXME: _useIORefNum_ is never used with CoreMIDI; it and code that uses it should go away! */
	short		useIORefNum=0;							/* NB: can be fmsUniqueID */
	Byte		partPatch[MAXSTAVES];
	short		partIORefNum[MAXSTAVES];

	short		oldCurrentSheet, tempoCount, barTapSlopMS;
	EventRecord	theEvt;
	char		theChar;
	TCONVERT	tConvertTab[MAX_PLAY_TEMPOCHANGE];
	char		fmtStr[256];
	short		velOffset, durFactor, timeFactor;

	short		notePartn;
	MIDIUniqueID partDevID;
	Boolean		patchChangePosted = False;
	Boolean		sustainOnPosted = False, sustainOffPosted = False;
	Boolean		panPosted = False;
	OSErr		err;
	
	WaitCursor();

	/* Get initial system Rect, set all note play times, get part attributes, etc. */

#if DEBUG_KEEPTIMES
	nkt = 0;
#endif

	doScroll = config.turnPagesInPlay;

	systemL = LSSearch(fromL, SYSTEMtype, ANYONE, GO_LEFT, False);
	pSystem = GetPSYSTEM(systemL);
	D2Rect(&pSystem->systemRect, &sysRect);

	if (useWhichMIDI==MIDIDR_CM) {
#if CMDEBUG
		LogPrintf(LOG_DEBUG, "PlaySequence (1): doc inputDev=%ld\n", doc->cmInputDevice);
#endif
		if (doc->cmInputDevice == kInvalidMIDIUniqueID)
			doc->cmInputDevice = gSelectedInputDevice;
#if CMDEBUG
		LogPrintf(LOG_DEBUG, "PlaySequence (2): doc inputDev=%ld\n", doc->cmInputDevice);
#endif
		MIDIUniqueID cmPartDevice[MAXSTAVES];
		if (!GetCMPartPlayInfo(doc, partTransp, partChannel, partPatch, partVelo,
									partIORefNum, cmPartDevice)) {
#if CMDEBUG
			CMDebugPrintXMission();
#endif
			MayErrMsg("Unable to play score: can't get part device for every part.  (PlaySequence)");
			return;
		}
	}
	else
		GetPartPlayInfo(doc, partTransp, partChannel, channelPatch, partVelo);

	InitEventList();

	InitAddBarlines();

	tempoCount = MakeTConvertTable(doc, fromL, toL, tConvertTab, MAX_PLAY_TEMPOCHANGE);
	tooManyTempi = (tempoCount<0);
	if (tooManyTempi) {
		LogPrintf(LOG_WARNING, "Over %d tempo changes, too many to play correctly.  (PlaySequence)\n", MAX_PLAY_TEMPOCHANGE);
		tempoCount = MAX_PLAY_TEMPOCHANGE;						/* Table size exceeded */
	}

	switch (useWhichMIDI) {
		case MIDIDR_CM:
			CMSetup(doc, partChannel);
			break;
		default:
			break;
	}

	for (objL = doc->headL; objL!=fromL; objL = RightLINK(objL)) {
		if (IsMIDIPatchChange(objL)) {
			PostMIDIPatchChange(doc, objL, partPatch, partChannel);
		}
	}
	
	ClearMIDISustain(True);
	ClearMIDISustain(False);
	ClearMIDIPan();
	
	ClearAllMIDISustainOn();
	ClearAllMIDIPan();

	/* If "play on instruments' parts" is set, send out program changes to the correct
	   patch numbers for all channels that have any instruments assigned to them. */
		
	if (useWhichMIDI == MIDIDR_CM) {
		err = CMMIDIProgram(doc, partPatch, partChannel);
		if (err != noErr) {
			MPErrorMsg(20);
			MayErrMsg("Can't send patch changes (error %d). Try Instrument MIDI Settings > Set Device To All Parts.  (PlaySequence)",
						err);
			return;
		}
	}
	SleepTicks(10L);			/* Let synth settle after patch change, before playing notes. */

	if (DETAIL_SHOW) ListNotesToPlay(doc, fromL, toL, selectedOnly);

	/* The stored play time of the first Sync we're going to play might be any positive
	   value, but we want to start playing immediately, so pick up our first Sync's play
	   time to use as an offset on all play times. */
	
	measL = SSearch(fromL, MEASUREtype, GO_LEFT);				/* starting measure */
	toffset = -1L;												/* Init. time offset to unknown */
	for (objL = fromL; objL!=toL; objL = RightLINK(objL))
		switch (ObjLType(objL)) {
			case MEASUREtype:
				measL = objL;
				break;
			case SYNCtype:
			  	if (!AnyNoteToPlay(doc, objL, selectedOnly)) break;
				if (SyncTIME(objL)>MAX_SAFE_MEASDUR)
					AlwaysErrMsg("objL L%ld has illegal timeStamp=%ld.  (PlaySequence)",
									(long)objL, (long)SyncTIME(objL));
				
				/* For 1st note to play, convert play time to nominal millisecs. using
					tempi marked in the score; then, to handle variable-speed playback,
					convert that time to actual millisec. */
					
				if (toffset<0L) {
					plStartTime = MeasureTIME(measL)+SyncTIME(objL);
					startTimeNorm = PDur2RealTime(plStartTime, tConvertTab, tempoCount);
					toffset = startTimeNorm;
					LogPrintf(LOG_INFO, "tempoCount=%d. t=%ld => toffset=%ld playTempoPercent=%d mutedPart=%d  (PlaySequence)\n",
						tempoCount, plStartTime, toffset, playTempoPercent, doc->mutedPartNum);
				}
				break;
			default:
				;
		}
	
	/* Make final preparations and enter the main loop to play everything. */
	
	pageL = LSSearch(fromL, PAGEtype, ANYONE, GO_LEFT, False);
	pPage = GetPPAGE(pageL);
	oldCurrentSheet = doc->currentSheet;
	oldPaper = doc->currentPaper;
	doc->currentSheet = pPage->sheetNum;
	paperIsOnDesktop =
		(GetSheetRect(doc, doc->currentSheet, &doc->currentPaper)==INARRAY_INRANGE);
	pagePaper = doc->currentPaper;

	barTapSlopMS = 10*config.barTapSlop;
	oldStartTime = -999999L;
	
	PlayMessage(doc, fromL, -1);
	pageTurnTOffset = 0L;
	StartMIDITime();

	showOldL = oldL = NILINK;									/* not yet playing anything */
	moveSel = False;											/* init. "move the selection" flag */
	playCursor = GetCursor(MIDIPLAY_CURS);
	if (playCursor) SetCursor(*playCursor);
	newMeasL = measL = SSearch(fromL, MEASUREtype, GO_LEFT);	/* starting measure */
	newPage = False;
	
	/* Start playing. */
	
	for (objL = fromL; objL!=toL; objL = RightLINK(objL)) {
		switch (ObjLType(objL)) {
			case PAGEtype:
				pPage = GetPPAGE(objL);
				doc->currentSheet = pPage->sheetNum;
				paperIsOnDesktop =
					(GetSheetRect(doc, doc->currentSheet, &doc->currentPaper)==INARRAY_INRANGE);
				pagePaper = doc->currentPaper;
				newPage = True;
				break;
			case SYSTEMtype:									/* Remember system rectangles */
				pSystem = GetPSYSTEM(objL);
				D2Rect(&pSystem->systemRect, &sysRect);
				break;
			case MEASUREtype:
				newMeasL = measL = objL;
				break;
			case SYNCtype:
			  	if (AnyNoteToPlay(doc, objL, selectedOnly)) {
			  		if (SyncTIME(objL)>MAX_SAFE_MEASDUR)
						AlwaysErrMsg("objL L%ld has illegal timeStamp=%ld.  (PlaySequence)",
										(long)objL, (long)SyncTIME(objL));
			  		plStartTime = MeasureTIME(measL)+SyncTIME(objL);
					
					/* Convert play time to nominal millisecs. using tempi marked in the
					   score; then, to handle variable-speed playback, convert that time
					   to actual millisec. */
						
					startTimeNorm = PDur2RealTime(plStartTime, tConvertTab, tempoCount);
#ifdef TDEBUG
					LogPrintf(LOG_DEBUG, "PlaySequence: toffset=%ld. plStartTime=%ld startTimeNorm=%ld\n",
								toffset, plStartTime, startTimeNorm);
#endif
					startTimeNorm -= toffset;
					startTime = ScaleDurForVariableSpeed(startTimeNorm);
							
					/*
					 * Wait 'til it's time to play objL. While waiting, check for notes
					 * ending and take care of them. Also check for relevant keyboard
					 * events: the codes for Stop and Select what's playing, Cancel
					 * (just stop playing), and Insert Barline. For the latter, if we're
					 * about to start playing objL, assume the user wants the barline
					 * before objL; otherwise assume they want it before the previous Sync.
					 *
					 * NB: it seems as if the comment in WriteMFNotes about having to write
					 * notes ending at a given time before those beginning at the same time
					 * should apply here too, but we're not doing that here and I haven't
					 * noticed any problems. And doing it this way should help get notes
					 * started as close as possible to their correct times.
					 */
					 
					do {
						t = GetMIDITime(pageTurnTOffset);
						
						if ((moveSel = UserInterruptAndSel())) goto done;	/* Check for Stop/Select */
						if ((moveSel = CheckButton())) goto done;			/* Check for Stop/Select */
						if (UserInterrupt()) goto done;					/* Check for Cancel */
						
						GetNextEvent(keyDownMask, &theEvt);				/* Not Wait/GetNextEvent so we do as little as possible */
						theChar = (char)theEvt.message & charCodeMask;
						if (theChar==CH_BARTAP)	{						/* Check for Insert Barline */
							LINK insL = (t-oldStartTime<barTapSlopMS? oldL : objL);
							AddBarline(insL);
						}
						CheckEventList(pageTurnTOffset);				/* Check for, turn off any notes done */
								
						if (useWhichMIDI==MIDIDR_CM)
							CMCheckEventList(pageTurnTOffset);			/* Check for, turn off any notes done */
						else
							CheckEventList(pageTurnTOffset);			/* Check for, turn off any notes done */
					} while (t<startTime);
		
					if (newMeasL) {
						PlayMessage(doc, newMeasL, -1);
						newMeasL = NILINK;
					}
					oldL = objL;
					oldStartTime = startTime;
					if (showit) {
						if (showOldL) HiliteSyncRect(doc, &syncRect, &syncPaper, False);  /* unhilite old Sync */
						if (paperIsOnDesktop) {
							syncRect = sysRect;
							
							/* We use the objRect to determine what to hilite. FIXME: If
							   this Sync isn't in view, its objRect may be empty or, worse,
							   garbage! */
							   
							r = LinkOBJRECT(objL);
							syncRect.left = r.left;
							syncRect.right = r.right;
							syncPaper = pagePaper;
							tBeforeTurn = GetMIDITime(pageTurnTOffset);
#if PLDEBUG
LogPrintf(LOG_DEBUG, "objL=%ld: rect.l=%ld,r=%ld paper.l=%ld,r=%ld  (PlaySequence)\n",
objL,syncRect.left,syncRect.right,syncPaper.left,syncPaper.right);
#endif
							HiliteSyncRect(doc, &syncRect, &syncPaper, newPage && doScroll); /* hilite new Sync */
							tElapsed = GetMIDITime(pageTurnTOffset)-tBeforeTurn;
							pageTurnTOffset += tElapsed;
							showOldL = objL;								/* remember this Sync */
							newPage = False;
							}
						else
							showOldL = NILINK;
					}
					
					if (patchChangePosted) {
						CMMIDIProgram(doc, partPatch, partChannel);
						patchChangePosted = False;
					}
					if (sustainOnPosted) {
						SendAllMIDISustains(doc, partChannel, True);
						ClearMIDISustain(True);
						sustainOnPosted = False;
					}
					if (sustainOffPosted) {
						SendAllMIDISustains(doc, partChannel, False);
						ClearMIDISustain(False);
						sustainOffPosted = False;
					}
					if (panPosted) {
						SendAllMIDIPans(doc, partChannel);
						ClearMIDIPan();
						panPosted = False;
					}
		
					/* Play all the notes in <objL> we're supposed to, adding them to
					   <eventList[]> too */
		
					aNoteL = FirstSubLINK(objL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
						if (NoteToBePlayed(doc, aNoteL, selectedOnly)) {
						
							/* Get note's MIDI note number, including transposition; velocity,
							   limited to legal range; channel number; and duration, includ-
							   ing any tied notes to right. */
							   
							if (useWhichMIDI==MIDIDR_CM) {
								CMGetNotePlayInfo(doc, aNoteL, partTransp, partChannel, partVelo,
														&useNoteNum, &useChan, &useVelo);
							}
							else
								GetNotePlayInfo(doc, aNoteL, partTransp, partChannel, partVelo,
														&useNoteNum, &useChan, &useVelo);
							
							playDur = TiedDur(doc, objL, aNoteL, selectedOnly);

							/* NOTE: We don't try to use timeFactor. */
							
							if (GetModNREffects(aNoteL, &velOffset, &durFactor, &timeFactor)) {
								useVelo += velOffset;
								useVelo = clamp(0, useVelo, 127);
								playDur = (long)(playDur * durFactor) / 100L;
							}

							plEndTime = plStartTime+playDur;						

							/* If it's a "real" note (not rest or continuation), send it out */
							
							if (useNoteNum>=0) {
								err = noErr;
								notePartn = Staff2Part(doc,NoteSTAFF(aNoteL));
								partDevID = GetCMDeviceForPartn(doc, notePartn);
								
								if (useWhichMIDI==MIDIDR_CM)
									err = CMStartNoteNow(partDevID, useNoteNum, useChan, useVelo);
								else
									err = StartNoteNow(useNoteNum, useChan, useVelo);
									
								/* Convert time to nominal millisecs. using tempi marked;
								   then convert that to actual millisec. to handle
								   variable-speed playback. */
									
								endTimeNorm = PDur2RealTime(plEndTime, tConvertTab, tempoCount);	/* Convert time to millisecs. */
								endTimeNorm -= toffset;
								endTime = ScaleDurForVariableSpeed(endTimeNorm);
								
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
					if (IsMIDIPatchChange(objL)) {
						patchChangePosted = PostMIDIPatchChange(doc, objL, partPatch, partChannel);						
					}
					else if (IsPedalDown(objL)) {
						sustainOnPosted = PostMIDISustain(doc, objL, True);
					}
					else if (isPedalUp(objL)) {
						sustainOffPosted = PostMIDISustain(doc, objL, False);
					}
					else if (IsMIDIPan(objL)) {
						panPosted = PostMIDIPan(doc, objL);
					}					
				}
				break;
			default:
				;
		}
	}

	if (useWhichMIDI==MIDIDR_CM) {
		while (!CMCheckEventList(pageTurnTOffset)) {			/* Wait til eventList[] empty... */
			if ((moveSel = UserInterruptAndSel())) goto done;	/* Check for Stop/Select */
			if ((moveSel = CheckButton())) goto done;			/* Check for Stop/Select */
			if (UserInterrupt()) goto done;						/* Check for Cancel */

			GetNextEvent(keyDownMask, &theEvt);
			theChar = (char)theEvt.message & charCodeMask;
			if (theChar==CH_BARTAP)								/* Check for Insert Barline */
			AddBarline(oldL);
		}
	}
	else {
		while (!CheckEventList(pageTurnTOffset)) {				/* Wait til eventList[] empty... */
			if ((moveSel = UserInterruptAndSel())) goto done;	/* Check for Stop/Select */
			if ((moveSel = CheckButton())) goto done;			/* Check for Stop/Select */
			if (UserInterrupt()) goto done;						/* Check for Cancel */

			GetNextEvent(keyDownMask, &theEvt);
			theChar = (char)theEvt.message & charCodeMask;
			if (theChar==CH_BARTAP)								/* Check for Insert Barline */
			AddBarline(oldL);
		}
	}
			
done:
	if (err) MayErrMsg("Can't play the score (error %d). Try Instrument MIDI Settings > Set Device To All Parts.  (PlaySequence)\n",
				err);
	if (showit && showOldL) HiliteSyncRect(doc, &syncRect, &syncPaper, False);	/* unhilite last Sync */

	doc->currentSheet = oldCurrentSheet;
	doc->currentPaper = oldPaper;
	
	// Wait for half a second before turning off sustain and pan.
	//SleepTicks(30L);
	ResetMIDISustain(doc, partChannel);
	ResetMIDIPan(doc, partChannel);
	
	StopMIDI();
	CloseAddBarlines(doc);
	
	if (useWhichMIDI == MIDIDR_CM) CMTeardown();

	if (tooManyTempi) {
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 3);			/* "Nightingale can only play %d tempo changes" */
		sprintf(strBuf, fmtStr, MAX_PLAY_TEMPOCHANGE);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}

	if (moveSel) {
	
		/* We want to change the selection to only the notes we were just playing. If the
		   Sync that was playing is in the selection range, we deselect everything else
		   in the range, otherwise we deselect absolutely everything.  Then we make sure
		   the correct notes in the Sync are selected. */
	   
		if (WithinRange(doc->selStartL, oldL, doc->selEndL)) {
			DeselRange(doc, doc->selStartL, oldL);
			DeselRange(doc, RightLINK(oldL), doc->selEndL);
		}
		else DeselAll(doc);

		if (!selectedOnly) SelAndHiliteSync(doc, oldL);

		LinkSEL(oldL) = True;
		doc->selStartL = oldL;
		doc->selEndL = RightLINK(oldL);
	}
	EraseAndInvalMessageBox(doc);
	ArrowCursor();

#if DEBUG_KEEPTIMES
	{ short nk; for (nk=0; nk<nkt; nk++)
		LogPrintf(LOG_DEBUG, "nk=%d kStartTime[]=%ld (PlaySequence)\n", nk, kStartTime[nk]-kStartTime[0]);
	}
#endif
}


/* ------------------------------------------------------------------------ PlayEntire -- */
/*	Play the entire score. */

void PlayEntire(Document *doc)
{
	LINK firstPage;
	
	firstPage = LSSearch(doc->headL, PAGEtype, ANYONE, GO_RIGHT, False);
	PlaySequence(doc, RightLINK(firstPage), doc->tailL, True, False);
}


/* --------------------------------------------------------------------- PlaySelection -- */
/*	Play the currently selected notes. */

void PlaySelection(Document *doc)
{
	PlaySequence(doc, doc->selStartL, doc->selEndL, True, True);
}
