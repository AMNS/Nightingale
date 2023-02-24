/*	MIDIFOCmd.c for Nightingale: user interface for opening Standard MIDI Files */

/*
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* From OpcodeNightingaleData.h: variables shared with other Open MIDI File modules */

extern short infile;
extern long eofpos;


#define MAXTRACKS (MAXSTAVES+1)			/* So we can open any file we save: need 1 extra for timing track */

extern Word mfNTracks, mfTimeBase;
extern long qtrNTicks;					/* Ticks per quarter in Nightingale (NOT in the file!) */

extern Byte *pChunkMF;					/* MIDI file track pointer */
extern Word lenMF;						/* MIDI file track length */
extern DoubleWord locMF;				/* MIDI file track current position */

#ifndef MAXINPUTTYPE
#define MAXINPUTTYPE	4
#endif

static unsigned char *filename;

static OSErr errCode;

static Boolean OpenMIDIFile(void);


/* ---------------------------------------------------------- MFInfoDialog and helpers -- */
/* Display statistics for the individual tracks of the MIDI file. */

static void DMFDrawLine(char *);
void ShowMFInfoPage(short, short, TRACKINFO [], short [], short [], Boolean [][MAXCHANNEL],
							short [], Boolean [], long []);
static void MFInfoDialog(TRACKINFO [], short [], short [], Boolean [][MAXCHANNEL], short [],
						Boolean [], long []);

#define LEADING 11			/* Vertical dist. between lines displayed (pixels) */

extern char durStrs[][16];

static Rect textRect;
static short linenum;

/* Draw the specified C string on the next line in MIDI File Info dialog */

#define TAB_COUNT 21

static void DMFDrawLine(char *s)
{
	char *p;
	short nt;
	short tab[TAB_COUNT] =	{10,35,70,100,150,					/* In pixels: summary cols. */
							 207,217,227,237, 257,267,277,287,	/* First 8 channels */
							 307,317,330,343, 363,376,389,402};	/* Second 8 channels */

	char aStr[256], *q;
	if (DETAIL_SHOW) {
		p = s; q = aStr;
		while (*p) {
			*q = (*p=='\t'? ' ' : *p);
			p++; q++;
		}
		*q = '\0'; LogPrintf(LOG_NOTICE, aStr); LogPrintf(LOG_NOTICE, "\n");
	}

	MoveTo(textRect.left, textRect.top+linenum*LEADING);
	p = s; nt = 0;
	while (*p) {
		if (*p=='\t') {
			MoveTo(textRect.left+tab[nt], textRect.top+linenum*LEADING);
			if (nt++>=TAB_COUNT) nt--;
		}
		else
			DrawChar(*p);
		p++;
	}

	++linenum;
}


void ShowMFInfoPage(
			short pageNum, short linesPerPage,
			TRACKINFO trackInfo[],
			short nTrackNotes[], short nTooLong[],
			Boolean chanUsed[][MAXCHANNEL],
			short qTrLDur[],
			Boolean qTrTriplets[],
			long lastTrEvent[]
			)
{
	short first, t, i;
	char durStr[256], skippedStr[32];
	
	EraseRect(&textRect);
	TextFont(textFontNum); TextSize(textFontSmallSize);
	TextFace(0);												/* face 0 = plain */
	linenum = 1;

	GetIndCString(strBuf, MIDIFILE_STRS, 35); 					/* "Track  Notes TooLong  Grid..." */
	DMFDrawLine(strBuf);

	GetIndCString(skippedStr, MIDIFILE_STRS, 37); 				/* "SKIPPED" */

 	first = 1+(pageNum-1)*linesPerPage;
	for (t = first; t<=first+linesPerPage-1 && t<=mfNTracks; t++) {
 		if (!trackInfo[t].okay)
 			sprintf(strBuf, "\t%d.\t(%d) %s\t\t\t  %ld",
 						t, nTrackNotes[t], skippedStr, lastTrEvent[t]);
 		else if (nTrackNotes[t]>0) {
			if (qTrLDur[t]==UNKNOWN_L_DUR) {
				GetIndCString(durStr, MIDIFILE_STRS, 36); 		/* "(none)" */
			}
			else {
				strcpy(durStr, &durStrs[qTrLDur[t]][0]);
				if (qTrTriplets[t]) strcat(durStr, "*3");
			}
			sprintf(strBuf, "\t%d.\t%d\t%d\t%s\t  %ld",
 						t, nTrackNotes[t], nTooLong[t], durStr, lastTrEvent[t]);
		}
		else if (t==1) {
			strcpy(durStr, &durStrs[qTrLDur[t]][0]);
			sprintf(strBuf, "\t%d.\t%d\t\t%s\t  %ld",
 						t, nTrackNotes[t], durStr, lastTrEvent[t]);
		}
		else
 			sprintf(strBuf, "\t%d.\t%d\t\t\t  %ld",
 						t, nTrackNotes[t], lastTrEvent[t]);
 		
 		for (i = 0; i<MAXCHANNEL; i++)
 			if (chanUsed[t][i])
 				sprintf(&strBuf[strlen(strBuf)], "\t%d", i+1);
 			else
 				sprintf(&strBuf[strlen(strBuf)], "\t.");
		DMFDrawLine(strBuf);
 	}
}


enum {
	TEXT_DI=2,
	PREV_DI,
	NEXT_DI,
	TICKS_DI
};

static void MFInfoDialog(
					TRACKINFO trackInfo[],
					short nTrackNotes[], short nTooLong[],
					Boolean chanUsed[][MAXCHANNEL],
					short qTrLDur[],
					Boolean qTrTriplets[],
					long lastTrEvent[]
					)
{
	DialogPtr dialogp;  GrafPtr oldPort;
	Handle prevHdl, nextHdl, aHdl;
	Rect aRect, ticksRect;
	short ditem, aShort;
	short dialogOver, linesPerPage, nPages, pageNum, t;
	long lastEvTime;
	char fmtStr[256];
	 
	GetPort(&oldPort);
	dialogp = GetNewDialog(MFINFO_DLOG, NULL, BRING_TO_FRONT);
	if (!dialogp) { MissingDialog(MFINFO_DLOG); return; }
	SetPort(GetDialogWindowPort(dialogp));
	GetDialogItem(dialogp, TEXT_DI, &aShort, &aHdl, &textRect);
	linesPerPage = (textRect.bottom-textRect.top-5)/LEADING-1;			/* Allow a bit of space for title */

	GetDialogItem(dialogp, TICKS_DI, &aShort, &aHdl, &ticksRect);

	CenterWindow(GetDialogWindow(dialogp), 110);
	ShowWindow(GetDialogWindow(dialogp));
					
	ArrowCursor();
	OutlineOKButton(dialogp, True);

	nPages = mfNTracks/linesPerPage;
	if (mfNTracks%linesPerPage!=0) nPages++;
	GetDialogItem(dialogp, PREV_DI, &aShort, &prevHdl, &aRect);
	GetDialogItem(dialogp, NEXT_DI, &aShort, &nextHdl, &aRect);

	TextFont(textFontNum); TextSize(textFontSmallSize); TextFace(0);	/* face 0 = plain */
	MoveTo(ticksRect.left+1, ticksRect.top+9);
	GetIndCString(fmtStr, MIDIFILE_STRS, 10);							/* "%d ticks per quarter" */
	sprintf(strBuf, fmtStr, mfTimeBase); 
	DrawCString(strBuf);

	for (lastEvTime = 0, t =1; t<=mfNTracks; t++)
		lastEvTime = n_max(lastEvTime, lastTrEvent[t]);
	MoveTo(ticksRect.left+1, ticksRect.top+20);
	GetIndCString(fmtStr, MIDIFILE_STRS, 11);							/* "Approx. %ld quarters" */
	sprintf(strBuf, fmtStr, lastEvTime/mfTimeBase); 
	DrawCString(strBuf);

	pageNum = 1;
	ShowMFInfoPage(pageNum, linesPerPage, trackInfo, nTrackNotes, nTooLong,
							chanUsed, qTrLDur, qTrTriplets, lastTrEvent);
	HiliteControl((ControlHandle)prevHdl,(pageNum<=1? CTL_INACTIVE : CTL_ACTIVE));
	HiliteControl((ControlHandle)nextHdl,(pageNum>=nPages? CTL_INACTIVE : CTL_ACTIVE));
	
	dialogOver = 0;
	do {
		while (True) {
			ModalDialog(NULL, &ditem);
			if (ditem==OK || ditem==Cancel) { dialogOver = ditem; break; }
			switch (ditem) {
				case PREV_DI:
				case NEXT_DI:
					if (ditem==PREV_DI) pageNum--;
					if (ditem==NEXT_DI) pageNum++;
					ShowMFInfoPage(pageNum, linesPerPage, trackInfo, nTrackNotes, nTooLong,
											chanUsed, qTrLDur, qTrTriplets, lastTrEvent);
					HiliteControl((ControlHandle)prevHdl,(pageNum<=1? CTL_INACTIVE : CTL_ACTIVE));
					HiliteControl((ControlHandle)nextHdl,(pageNum>=nPages? CTL_INACTIVE : CTL_ACTIVE));
					break;
				default:
					;
			}
		}
	} while (!dialogOver);

	HideWindow(GetDialogWindow(dialogp));
	DisposeDialog(dialogp);
	SetPort(oldPort);
}


/* ================================ Open MIDI File dialog =============================== */ 

static GRAPHIC_POPUP	durPop0dot, *curPop;
static POPKEY			*popKeys0dot;
static short			popUpHilited=True;

/* ---------------------------------------------------------------- TranscribeMFDialog -- */

#define CurEditField(dlog) (((DialogPeek)(dlog))->editField+1)

enum {
	MFDURPOP_DI=4,												/* Item numbers */
	AUTOBEAM_DI,
	QUANTIZE_DI,
	NOQUANTIZE_DI,
	FILENAME_DI=9,
	NNOTES_DI,
	NTRACKS_DI=12,
	NEEDDUR_DI=14,
	INFO_DI=16,
	TRIPLETS_DI,
	MAXMEAS_DI=19,
	CLEFCHANGE_DI=21,
	MFDUMMY_DI
};


static pascal Boolean TransMFFilter(DialogPtr, EventRecord *, short *);
static pascal Boolean TransMFFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr	w;
	short		ch, field, ans;
	Point		where;
	GrafPtr		oldPort;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));	
				UpdateDialogVisRgn(dlog);		
				//UpdateDialog(dlog, dlog->visRgn);
				FrameDefault(dlog, OK, True);
				DrawGPopUp(curPop);		
				HiliteGPopUp(curPop, popUpHilited);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return True;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog)) SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			if (PtInRect(where, &curPop->box)) {
				DoGPopUp(curPop);
				*itemHit = MFDURPOP_DI;
				return True;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
			field = GetDialogKeyboardFocusItem(dlog);
			
			/* The Dialog Manager considers only EditText fields as candidates for being
			   activated by the tab key, so handle tabbing from field to field ourselves
			   so user can direct keystrokes to the pop-up as well as the EditText fields. */
			   
			if (ch=='\t') {
				field = field==MAXMEAS_DI? MFDUMMY_DI : MAXMEAS_DI;
				popUpHilited = (field==MAXMEAS_DI)? False : True;
				SelectDialogItemText(dlog, field, 0, ENDTEXT);
				HiliteGPopUp(curPop, popUpHilited);
				*itemHit = 0;
				return True;
			}
			else {
				if (field==MFDUMMY_DI) {
					ans = DurPopupKey(curPop, popKeys0dot, ch);
					*itemHit = ans? MFDURPOP_DI : 0;
					HiliteGPopUp(curPop, True);
					return True;							/* so no chars get through to MFDUMMY_DI edit field */
				}											/* NB: however, DlgCmdKey will let you paste into MFDUMMY_DI! */
			}
			break;
	}
	return False;
}

#define XACTIVEATE_CTLS	\
	HiliteControl((ControlHandle)beamHdl, (rButGroup==NOQUANTIZE_DI ? CTL_INACTIVE : CTL_ACTIVE));	\
	HiliteControl((ControlHandle)tripHdl, (rButGroup==NOQUANTIZE_DI ? CTL_INACTIVE : CTL_ACTIVE))

/* Run the very complex Transcribe MIDI File dialog. Parameters labeled "I+O" are both
input and output! */

static Boolean TranscribeMFDialog(TRACKINFO [],short [],short [], Boolean [][MAXCHANNEL],
				short [], Boolean [],long [],short, short, short, short *, Boolean *,
				Boolean *, Boolean *, short *);

static Boolean TranscribeMFDialog(
				TRACKINFO trackInfo[],
				short nTrackNotes[],					/* Specific track-by-track info */
				short nTooLong[],						/* Specific track-by-track info */
				Boolean chanUsed[][MAXCHANNEL],
				short qTrLDur[],
				Boolean qTrTriplets[],
				long lastTrEvent[],
				short nNotes,							/* Totals for all tracks */
				short /*nGoodTrs*/,
				short qAllLDur,
				short *qDurCode,						/* Duration to quantize to (I+O) */
				Boolean *autoBeam,						/* Beam automatically? (I+O) */
				Boolean *triplets,						/* Consider triplet rhythms? (I+O) */
				Boolean *clefChanges,					/* Generate clef changes? (I+O) */
				short *maxMeasures
				)
{
	DialogPtr dlog;  GrafPtr oldPort;
	short ditem, aShort;
	short rButGroup, lDurCode, oldLDurCode, maxMeas, t;
	short oldResFile; 
	Boolean done, autoBm, trips, clefs, needTrips; 
	short choice;
	Handle ndHdl, beamHdl, tripHdl;
	char durStr[256], tripletsStr[32];
	Handle hndl;
	Rect aRect, box;
	char fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(TransMFFilter);
	if (filterUPP == NULL) {
		MissingDialog(TRANSMIDI_DLOG);
		return False;
	}
	
	dlog = GetNewDialog(TRANSMIDI_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(TRANSMIDI_DLOG);
		return False;
	}
	
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);									/* popup code uses Get1Resource */
		
	durPop0dot.menu = NULL;										/* NULL makes any "goto broken" safe */
	durPop0dot.itemChars = NULL;
	popKeys0dot = NULL;

	GetDialogItem(dlog, MFDURPOP_DI, &aShort, &hndl, &box);
	if (!InitGPopUp(&durPop0dot, TOP_LEFT(box), NODOTDUR_MENU, 1)) goto broken;
	popKeys0dot = InitDurPopupKey(&durPop0dot);
	if (popKeys0dot==NULL) goto broken;
	curPop = &durPop0dot;
	
	Pstrcpy((StringPtr)strBuf, filename);
	PStrCat((StringPtr)strBuf, "\pÓ");
	PutDlgString(dlog, FILENAME_DI, (StringPtr)strBuf, False);

	PutDlgWord(dlog, NNOTES_DI, nNotes, False);
	PutDlgWord(dlog, NTRACKS_DI, mfNTracks, False);

	GetDialogItem(dlog, NEEDDUR_DI, &aShort, &ndHdl, &aRect);
	if (qAllLDur==UNKNOWN_L_DUR) {
		GetIndCString(strBuf, MIDIFILE_STRS, 13);			/* "Some attacks don't fit any metric grid Nightingale can handle: may be tuplets." */
		SetDialogItemCText(ndHdl, strBuf);
	} else {
		strcpy(durStr, &durStrs[qAllLDur][0]);
		for (needTrips = False, t = 1; t<=mfNTracks; t++)
			if (qTrLDur[t]!=UNKNOWN_L_DUR)
				if (qTrTriplets[t]) needTrips = True;
		if (needTrips) {
			GetIndCString(tripletsStr, MIDIFILE_STRS, 34); 	/* " with triplets" */
			strcat(durStr, tripletsStr);
		}
		GetIndCString(fmtStr, MIDIFILE_STRS, 12);			/* "All time signatures and attacks fit a grid of %s." */
		sprintf(strBuf, fmtStr, durStr); 
		SetDialogItemCText(ndHdl, strBuf);
	}

	/* Set up radio button group for quantize/noquantize */
	
	rButGroup = (*qDurCode==UNKNOWN_L_DUR? NOQUANTIZE_DI : QUANTIZE_DI);
	PutDlgChkRadio(dlog, rButGroup, True);

	/* Set the suggested (initial) quantization unit. Use the finest value among tracks
	   whose attacks fit a (non-tuplet) metric grid and time sig. denoms. But in no case
	   suggest anything coarser than eighths or finer than 32nds. */
		
	lDurCode = WHOLE_L_DUR;
	for (t = 1; t<=mfNTracks; t++)
		if (qTrLDur[t]!=UNKNOWN_L_DUR) lDurCode = n_max(lDurCode, qTrLDur[t]);
	if (lDurCode<EIGHTH_L_DUR) lDurCode = EIGHTH_L_DUR;
	if (lDurCode>THIRTY2ND_L_DUR) lDurCode = THIRTY2ND_L_DUR;
	choice = GetDurPopItem(curPop, popKeys0dot, lDurCode, 0);
	if (choice==NOMATCH) choice = 1;
	SetGPopUpChoice(curPop, choice);
	oldLDurCode = lDurCode;

	PutDlgChkRadio(dlog, AUTOBEAM_DI, *autoBeam);
	PutDlgChkRadio(dlog, TRIPLETS_DI, *triplets);
	PutDlgWord(dlog, MAXMEAS_DI, *maxMeasures, False);
	PutDlgChkRadio(dlog, CLEFCHANGE_DI, *clefChanges);

	GetDialogItem(dlog, AUTOBEAM_DI, &aShort, &beamHdl, &aRect);
	GetDialogItem(dlog, TRIPLETS_DI, &aShort, &tripHdl, &aRect);
	XACTIVEATE_CTLS;

	SelectDialogItemText(dlog, (popUpHilited? MFDUMMY_DI : MAXMEAS_DI), 0, ENDTEXT);
	CenterWindow(GetDialogWindow(dlog), 65);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	oldLDurCode = lDurCode;
	done = False;
	while (!done) {
		ModalDialog(filterUPP, &ditem);
		if (lDurCode!=oldLDurCode) {
			SwitchRadio(dlog, &rButGroup, QUANTIZE_DI);
			XACTIVEATE_CTLS;
		}
		oldLDurCode = lDurCode;
		switch (ditem) {
		case OK:
			done = True;
			autoBm = GetDlgChkRadio(dlog, AUTOBEAM_DI);
			trips = GetDlgChkRadio(dlog, TRIPLETS_DI);
			clefs = GetDlgChkRadio(dlog, CLEFCHANGE_DI);
			GetDlgWord(dlog, MAXMEAS_DI, &maxMeas);
			if (maxMeas<=0) {
				GetIndCString(strBuf, MIDIFILE_STRS, 2);    /* "The maximum number of measures must be greater than 0." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				done = False;
			}
			if (rButGroup==QUANTIZE_DI) {
				*qDurCode = lDurCode;
				if (trips && lDurCode<qTrLDur[1]) {
					StopInform(TOO_COARSE_QUANT_ALRT);
					done = False;
				}
				else if (!(ControlKeyDown()) && trips && lDurCode>SIXTEENTH_L_DUR) {
					StopInform(TOO_FINE_QUANT_ALRT);
					done = False;
				}
				else if (lDurCode<qAllLDur && qAllLDur!=UNKNOWN_L_DUR)
					done = (CautionAdvise(COARSE_QUANTIZE_ALRT)==OK);
			}
			else
				*qDurCode = UNKNOWN_L_DUR;
			break;
		case Cancel:
			done = True;
			break;
		case QUANTIZE_DI:
		case NOQUANTIZE_DI:
			if (ditem!=rButGroup) {
				SwitchRadio(dlog, &rButGroup, ditem);
				XACTIVEATE_CTLS;
			}
			break;
		case MFDURPOP_DI:
			lDurCode = popKeys0dot[curPop->currentChoice].durCode;
			SelectDialogItemText(dlog, MFDUMMY_DI, 0, ENDTEXT);
			HiliteGPopUp(curPop, popUpHilited = True);
			break;
		case AUTOBEAM_DI:
			PutDlgChkRadio(dlog, AUTOBEAM_DI, !GetDlgChkRadio(dlog, AUTOBEAM_DI));
			break;
		case INFO_DI:
			MFInfoDialog(trackInfo, nTrackNotes, nTooLong, chanUsed, qTrLDur, qTrTriplets,
								lastTrEvent);
			break;
		case TRIPLETS_DI:
			PutDlgChkRadio(dlog, TRIPLETS_DI, !GetDlgChkRadio(dlog, TRIPLETS_DI));
			break;
		case CLEFCHANGE_DI:
			PutDlgChkRadio(dlog, CLEFCHANGE_DI, !GetDlgChkRadio(dlog, CLEFCHANGE_DI));
			break;
		}
	}
			
	if (ditem==OK) {
		*autoBeam = autoBm;
		*triplets = trips;
		*maxMeasures = maxMeas;
		*clefChanges = clefs;
	}
	
broken:
	DisposeGPopUp(&durPop0dot);
	if (popKeys0dot) DisposePtr((Ptr)popKeys0dot);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}


/* ====================================================================================== */ 

/* ----------------------------------------------------------------------- NameMFScore -- */
/* Give the score a name based on the name of the Imported MIDI file it came from. */

static void NameMFScore(Document *);
static void NameMFScore(Document *doc)
{
	Str63 str;
	WindowPtr w=(WindowPtr)doc;

	/* Get the name of MIDI file and append a suffix to indicate converted MIDI file */

	Pstrcpy(doc->name, filename);
	
	GetIndString(str, MiscStringsID, 10);
	PStrCat(doc->name, str);

	SetWTitle(w, doc->name);
}


/* ------------------------------------------------------------------------ MFHeaderOK -- */

static Boolean MFHeaderOK(Byte, Word, Word);
static Boolean MFHeaderOK(Byte midiFileFormat, Word nTracks, Word timeBase)
{
	char fmtStr[256];

	if (midiFileFormat!=1) {
		GetIndCString(fmtStr, MIDIFILE_STRS, 14);    /* "This MIDI file is in format %d; Nightingale can read only format 1." */
		sprintf(strBuf, fmtStr, midiFileFormat); 
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return False;
	}
	if ((timeBase & 0x8000)!=0) {
		GetIndCString(strBuf, MIDIFILE_STRS, 3);    /* "Can't handle MIDI files with non-metric timing." */
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return False;
	}
	if (nTracks<= 0 || nTracks>MAXTRACKS) {
		if (nTracks<= 0) {
			GetIndCString(strBuf, MIDIFILE_STRS, 15);    /* "This MIDI file has no tracks." */
		} else {
			GetIndCString(fmtStr, MIDIFILE_STRS, 16);    /* "This MIDI file has %d tracks but Nightingale can only handle files with %d." */
			sprintf(strBuf, fmtStr, nTracks, MAXTRACKS); 
		}
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return False;
	}	

	return True;
}


/* ------------------------------------------------------------------- GetMIDIFileInfo -- */

static Boolean GetMIDIFileInfo(TRACKINFO [], long *, short [], short [],
							Boolean [][MAXCHANNEL], short [], Boolean [], long [], short *,
							short *, short *);
static Boolean GetMIDIFileInfo(
				TRACKINFO trackInfo[],
				long *pLastEvent,				/* Output, in MIDI file (not Nightingale!) ticks */
				short nTrackNotes[],			/* Output, trk-by-trk no. of notes */
				short nTooLong[],				/* Output, trk-by-trk no. of notes over max. dur. */
				Boolean chanUsed[][MAXCHANNEL],	/* Output, trk-by-trk: any notes on channel? */
				short qTrLDur[],				/* Output, trk-by-trk: code for coarsest grid */
				Boolean qTrTriplets[],			/* Output, trk-by-trk: need triplets to fit all? */
				long lastTrEvent[],				/* Output, trk-by-trk: in ticks */
				short *pNNotes,					/* Output, total for all trks */
				short *pNGoodTrs,				/* Output: total for all trks */
				short *pqAllLDur				/* Output: total for all trks */
				)
{
	Byte midiFileFormat;
	long fPos, qTrLPoint;
	short nNotes, qAllLDur, nGoodTrs, t, i, nChanUsed;
	short tsCount, nTSBad;
	char fmtStr[256];

	LogPrintf(LOG_INFO, "Getting info on and checking the MIDI file...  (GetMIDIFileInfo)\n"); 

	/* Read and check the MIDI file header. */
	
	if (!ReadMFHeader(&midiFileFormat, &mfNTracks, &mfTimeBase)) {
		GetIndCString(strBuf, MIDIFILE_STRS, 4);    /* "Unable to read MIDI file header." */
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return False;
	}
	if (!MFHeaderOK(midiFileFormat, mfNTracks, mfTimeBase))
		return False;
	
	/* Get information about the notes and check that the file can be read and parsed;
		then restore its previous position. */
	
	GetFPos(infile, &fPos);
	nNotes = 0;
	qAllLDur = WHOLE_L_DUR;
	*pLastEvent = 0L;
	for (t = 1; t<=mfNTracks; t++) {
		lenMF = ReadTrack(&pChunkMF);
		if (lenMF==0) return False;

		trackInfo[t].okay = True;
		if (GetTrackInfo(&nTrackNotes[t], &nTooLong[t], chanUsed[t], &qTrLDur[t],
							&qTrLPoint, &qTrTriplets[t], &lastTrEvent[t])) {

			if (DETAIL_SHOW) {
				if (nTrackNotes[t]<=0)	LogPrintf(LOG_DEBUG, "Track %d contains no notes.  (GetMIDIFileInfo)\n",
													t);
				else					LogPrintf(LOG_DEBUG, "Track %d finest attack grid point: qTrLDur=%d at time %ld  (GetMIDIFileInfo)\n",
													t, qTrLDur[t], qTrLPoint);
			}

			/* Timing tracks with an end time of zero in otherwise valid MIDI files used
			   to be somewhat common: cf. Peter Stone in the 1990s. So the below check
			   might produce unwarranted complaints. */
			   
			if (t!=1 && lastTrEvent[t]==0) {
				GetIndCString(fmtStr, MIDIFILE_STRS, 38);    /* "Track %d has an ending time of zero: this MIDI file may be damaged." */
				sprintf(strBuf, fmtStr, t); 
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
			}

			for (nChanUsed = 0, i = 0; i<MAXCHANNEL; i++)
				if (chanUsed[t][i]) nChanUsed++;
			if (nChanUsed>1) {
				trackInfo[t].okay = False;
				GetIndCString(fmtStr, MIDIFILE_STRS, 17);	/* "Track %d has notes on more than one channel: it will be skipped and its staff left blank." */
				sprintf(strBuf, fmtStr, t); 
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
			}
			else if (nTooLong[t]>0) {
				GetIndCString(fmtStr, MIDIFILE_STRS, 18);	/* "Durations of %d note(s) are too long for Nightingale: they will be truncated to 65535 ticks." */
				sprintf(strBuf, fmtStr, nTooLong[t]); 
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
			}
			nNotes += nTrackNotes[t];
			if (qAllLDur==UNKNOWN_L_DUR || qTrLDur[t]==UNKNOWN_L_DUR)
				qAllLDur = UNKNOWN_L_DUR;
			else
				qAllLDur = n_max(qAllLDur, qTrLDur[t]);
			*pLastEvent = n_max(*pLastEvent, lastTrEvent[t]);
		}
		else {
			trackInfo[t].okay = False;
			if (t==1) {
				GetIndCString(strBuf, MIDIFILE_STRS, 41);	/* "Track 1 (the timing track) appears to be damaged or incomplete." */
				break;										/* since we won't try to open the file */
			}
			else {
				GetIndCString(fmtStr, MIDIFILE_STRS, 19);	/* "Track %d appears to be damaged or incomplete after %d valid note%s: it will be skipped and its staff (staff %d) left blank." */
				sprintf(strBuf, fmtStr, t, nTrackNotes[t], (nTrackNotes[t]==1? "" : "s"), t-1); 
			}
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			LogPrintf(LOG_WARNING, "Problem note (track %d, after %d notes): note no.=track[%ld]=%d\n",
						t, nTrackNotes[t], locMF-2, pChunkMF[locMF-2]);
		}
		
		if (t==1 && trackInfo[t].okay) {
			GetTimingTrackInfo(&tsCount, &nTSBad, &qTrLDur[1], &lastTrEvent[1]);
			if (qAllLDur!=UNKNOWN_L_DUR)
				qAllLDur = n_max(qAllLDur, qTrLDur[1]);
		}
		if (pChunkMF) DisposePtr((Ptr)pChunkMF);
	}

	errCode = SetFPos(infile, fsFromStart, fPos);
	if (errCode!=noError) {
		GetIndCString(strBuf, MIDIFILE_STRS, 5);    /* "Error reading MIDI file header: SetFPos failed." */
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return False;
	}

	if (!trackInfo[1].okay) {
		GetIndCString(strBuf, MIDIFILE_STRS, 6);    /* "Can't open a MIDI file with a bad timing track." */
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return False;
	}
	
	for (nGoodTrs = 0, t = 2; t<=mfNTracks; t++)
		if (trackInfo[t].okay) nGoodTrs++;
	if (nGoodTrs==0) {
		GetIndCString(strBuf, MIDIFILE_STRS, 7);    /* "None of the tracks are good except the timing track." */
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return False;
	}
	
	*pNNotes = nNotes;
	*pNGoodTrs = nGoodTrs;
	*pqAllLDur = qAllLDur;
	
	return True;
}

/* ------------------------------------------------------------------- CheckAndConsult -- */

static Boolean CheckAndConsult(TRACKINFO [], short *, Boolean *,Boolean *, Boolean *,
					short *, long *);
static Boolean CheckAndConsult(
						TRACKINFO trackInfo[],
						short *pQuantCode,
						Boolean *pAutoBeam, Boolean *pTriplets, Boolean *pClefChanges,
						short *pMaxMeasures,
						long *pLastEvent						/* in MIDI file (not Nightingale!) ticks */
						)
{
	short nTrackNotes[MAXTRACKS+1], nTooLong[MAXTRACKS+1], qTrLDur[MAXTRACKS+1];
	Boolean chanUsed[MAXTRACKS+1][MAXCHANNEL];
	Boolean qTrTriplets[MAXTRACKS+1];
	long lastTrEvent[MAXTRACKS+1];
	short nNotes, qAllLDur, nGoodTrs;
	
	if (!GetMIDIFileInfo(trackInfo, pLastEvent, nTrackNotes, nTooLong, chanUsed, qTrLDur,
							qTrTriplets, lastTrEvent, &nNotes, &nGoodTrs, &qAllLDur))
			return False;
	
	/* Tell user what we found and ask them what to do now. */
	
	LogPrintf(LOG_NOTICE, "Last event time=%ld  (CheckAndConsult)\n", *pLastEvent);
	return TranscribeMFDialog(trackInfo, nTrackNotes, nTooLong, chanUsed, qTrLDur,
										qTrTriplets, lastTrEvent, nNotes, nGoodTrs, qAllLDur,
										pQuantCode, pAutoBeam, pTriplets, pClefChanges, pMaxMeasures);
}


/* ---------------------------------------------------------------------- OpenMIDIFile -- */
/* Create a new score and read a format 1 MIDI file into it, mapping each track (except
the timing track) to a one-staff part. Return True if successful. */

#define RESET_PLAYDURS True
#define COMMENT_MIDIFILE "(origin: imported MIDI file)"	/* FIXME: Should be in resource for I18N */

static Boolean OpenMIDIFile()
{
	static short quantCode=SIXTEENTH_L_DUR;
	Word t, stf, len, td;
	Document *doc;
	LINK partL;
	TRACKINFO trackInfo[MAXTRACKS+1];
	Byte *pChunk;
	short durQuantum, tripletBias, status;
	static Boolean autoBeam=False, triplets=False, clefChanges=True;
	static short maxMeasures=9999;
	long lastEvent;
	FSSpec fsSpec;
	
	WaitCursor();
	
	qtrNTicks = Code2LDur(QTR_L_DUR, 0);

	/* Read the file, check it, and ask the user what to do. */
	
	if (!CheckAndConsult(trackInfo, &quantCode, &autoBeam, &triplets, &clefChanges,
								&maxMeasures, &lastEvent))
			return False;

	WaitCursor();

	/* Create a score, then read the tracks and put their content into that score. */
	
	if (DoOpenDocumentX(NULL, 0, False, &fsSpec, &doc)) {
		AnalyzeWindows();
		InstallDoc(doc);
		//doc = (Document *)TopDocument;
		strcpy(doc->comment, COMMENT_MIDIFILE);

		durQuantum = (quantCode==UNKNOWN_L_DUR? 1 : Code2LDur(quantCode, 0));

		/* We now have a default score with one part of two staves. In a format 1 MIDI
		   file, the first track is the tempo map, which won't get a staff; for every
		   other track, add a part of one staff below all existing staves. Then get rid
		   of the default part. */
		   
		for (stf = 1; stf<mfNTracks; stf++) {
			partL = AddPart(doc, 2+(stf-1), 1, SHOW_ALL_LINES);
			if (partL==NILINK) return False;
			InitPart(partL, 2+stf, 2+stf);
		}
		DeletePart(doc, 1, 2);

		FixMeasRectYs(doc, NILINK, True, True, False);		/* Fix measure & system tops & bottoms */
		Score2MasterPage(doc);
		
		/* Read in each track and convert okay ones to our "MIDNight" intermediate form.
		   Note that neither pChunkMF nor pChunk is allocated here: ReadTrack and
		   MF2MIDNight are respectively responsible for that. */
		   
		for (t = 1; t<=mfNTracks; t++) {
			lenMF = ReadTrack(&pChunkMF);
			if (lenMF==0) {
				if (pChunkMF) DisposePtr((Ptr)pChunkMF);
				for (td = 1; td<t; td++)
					if (trackInfo[td].pChunk) DisposePtr((Ptr)trackInfo[td].pChunk);
				return False;
			}
			if (MORE_DETAIL_SHOW) {
				LogPrintf(LOG_DEBUG, "MTrk(%d) lenMF=%d:\n", t, lenMF);
				DHexDump(LOG_DEBUG, "OpenMIDIFile", pChunkMF, (lenMF>50L? 50L : lenMF), 5,
							20, False);
			}

			if (trackInfo[t].okay) {
				if (DETAIL_SHOW) LogPrintf(LOG_INFO, "Calling MF2MIDNight for track %d...  (OpenMIDIFile)\n", t);
				len = MF2MIDNight(&pChunk);
				if (MORE_DETAIL_SHOW) {
					LogPrintf(LOG_DEBUG, "OpenMIDIFile: MTrk(%d) MIDNightLen=%d:\n", t, len);
					DHexDump(LOG_DEBUG, "OpenMIDIFile", pChunk, (len>50L? 50L : len), 5, 20,
								False);
				}
				if (len==0) {
					if (pChunkMF) DisposePtr((Ptr)pChunkMF);
					if (pChunk) DisposePtr((Ptr)pChunk);
					for (td = 1; td<t; td++)
						if (trackInfo[td].pChunk) DisposePtr((Ptr)trackInfo[td].pChunk);
					return False;
				}
				trackInfo[t].len = len;
				trackInfo[t].pChunk = pChunk;
			}
			else
				trackInfo[t].pChunk = NULL;
			if (pChunkMF) DisposePtr((Ptr)pChunkMF);
		}

		NameMFScore(doc);

		/* Convert MIDNight form for all okay tracks to Nightingale data structure, format
		   it, and clean up. */
		   
		tripletBias = (triplets? -config.noTripletBias : -100);
		LogPrintf(LOG_INFO, "Calling MIDNight2Night with %d tracks...  (OpenMIDIFile)\n", mfNTracks);
		status = MIDNight2Night(doc,trackInfo, durQuantum, tripletBias,
									config.delRedundantAccs, clefChanges, RESET_PLAYDURS,
									maxMeasures, lastEvent);

		for (t = 1; t<=mfNTracks; t++)
			if (trackInfo[t].pChunk) DisposePtr((Ptr)trackInfo[t].pChunk);

		if (status==FAILURE) return False;
		
		if (status==OP_COMPLETE) {
			if (durQuantum>1)								/* Only if quantizing */
				FixTimeStamps(doc, doc->headL, NILINK);		/* Recompute all play times */
	
			ProgressMsg(RESPACE_PMSTR, "");
			MFRespAndRfmt(doc, quantCode);
			ProgressMsg(0, "");
	
			SelAllNoHilite(doc);
			if (autoBeam && durQuantum>1) {
				AutoBeam(doc);
				
				/*  Set bracket visibility to the conventional default. */
				
				SetBracketsVis(doc, doc->headL, doc->tailL);
			}
			DeselAllNoHilite(doc);
		}
	
		SetPort(GetWindowPort(TopDocument));				/* May be necessary since no update yet */

		GetAllSheets(doc);
		RecomputeView(doc);
		MEAdjustCaret(doc, False);
	}

	return True;
}


/* -------------------------------------------------------------------------------------- */

Boolean	ImportMIDIFile(FSSpec *fsSpec)
{
	Boolean okay = False;
	errCode =  FSpOpenDF(fsSpec, fsRdPerm, &infile);		/* Read-only is what we want */
	if (errCode!=noError) goto Done;

	errCode = GetEOF(infile,&eofpos);
	if (errCode!=noError) goto Done;
	
	filename = fsSpec->name;

	if (!OpenMIDIFile()) goto Done;
	FSClose(infile);
	okay = True;

Done:
	ReportIOError(errCode, READMIDI_ALRT);

	ArrowCursor();
	return okay;
}
