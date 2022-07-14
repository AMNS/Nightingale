/* SearchScoreDlog.c for Nightingale - dialog handler for searching a score for a melodic
pattern given by another score - new for OMRAS. Incorporates code by Resorcerer. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "SearchScore.h"
#include "SearchScorePrivate.h"

#define SEARCH_DLOG 960
#define SEARCH_FILES_DLOG 962
#define SEARCH_STRS 410					/* Search strings */

/* Symbolic Dialog Item Numbers */

enum {
	BUT1_Find=1,
	BUT2_Cancel,
	BUT5_Show=5,

	CHK6_Pitch=6,
	BUT7_PitchRelative,
	BUT8_PitchAbsolute,
	BUT9_PitchAbsoluteAnyOct,
	EDIT11_PitchTol=11,
	CHK13_PreserveContour=13,
	EDIT15_MaxTranspose=15,

	CHK17_Duration=17,
	BUT18_DurPreserveContour=18,
	BUT19_DurRelative,
	BUT20_DurAbsolute,

	BUT22_ChordNotesAll=22,
	BUT23_ChordNotesOuter,
	BUT24_ChordNotesTop,
	BUT26_RestsIgnore=26,
	BUT27_RestsMatch,
	BUT29_TiedExtend=29,
	BUT30_TiedMatch,
	USER31_PitchPanel=31,
	USER32_DurPanel,
	USER33_PitchRelPanel,
	BUT34_FindAll
};

/* Prototypes */

static void DimPitchPanel(DialogPtr, short);
static pascal void UserDimPitchPanel(DialogPtr d, short dItem);
static void DimDurationPanel(DialogPtr, short);
static pascal void UserDimDurationPanel(DialogPtr d, short dItem);
static DialogPtr OpenThisDialog(SEARCHPARAMS sParm, Boolean findInFiles, short *pGroupPitch,
									short *pGroupDur, short *pGroupChord, short *pGroupTied,
									short *pGroupRests);
static void CloseThisDialog(DialogPtr dlog);


Boolean	savePitchPreserveContour;


/* -------------------------------------------------- DimPitchPanel, UserDimPitchPanel -- */

/* Maintain state of items subordinate to the Match Pitch checkbox. If that
checkbox is not checked, un-hilite its subordinate radio buttons and checkbox,
hide the edit field, dim the whole area.  Otherwise, do the opposite, except that
the relative-pitch panel is inside the Match Pitch panel, so we handle that case:
if relative is not selected, dim its subordinate items. The userItem's handle
should be set to a pointer to this procedure so it'll be called automatically by
the dialog filter. */
		
static void DimPitchPanel(DialogPtr dlog, short pitchItem)
{
	short		type;
	Handle		hndl, hndlRel, hndlAbs, hndlAbsAny, hndlPreserve;
	Rect		box, tempR;
	Boolean		matchPitch, relPitch;
	
	matchPitch = GetDlgChkRadio(dlog, CHK6_Pitch);
	relPitch = GetDlgChkRadio(dlog, BUT7_PitchRelative);

	/* Restore "always preserve contour" (see comment below). */
	
	PutDlgChkRadio(dlog, CHK13_PreserveContour, savePitchPreserveContour);

	GetDialogItem(dlog, BUT7_PitchRelative, &type, &hndlRel, &box);
	GetDialogItem(dlog, BUT8_PitchAbsolute, &type, &hndlAbs, &box);
	GetDialogItem(dlog, BUT9_PitchAbsoluteAnyOct, &type, &hndlAbsAny, &box);
	GetDialogItem(dlog, CHK13_PreserveContour, &type, &hndlPreserve, &box);

	if (matchPitch) {
		HiliteControl((ControlHandle)hndlRel, CTL_ACTIVE);
		HiliteControl((ControlHandle)hndlAbs, CTL_ACTIVE);
		HiliteControl((ControlHandle)hndlAbsAny, CTL_ACTIVE);
		ShowDialogItem(dlog, EDIT11_PitchTol);

		if (relPitch) {
			HiliteControl((ControlHandle)hndlPreserve, CTL_ACTIVE);
			ShowDialogItem(dlog, EDIT15_MaxTranspose);
		}
		else {
			/* In this state, "always preserve contour" is ignored, so--to avoid
			   misleading users--before dimming it, save its state, then uncheck it. */
			   
			savePitchPreserveContour = GetDlgChkRadio(dlog, CHK13_PreserveContour);
			PutDlgChkRadio(dlog, CHK13_PreserveContour, False);

			HiliteControl((ControlHandle)hndlPreserve, CTL_INACTIVE);

			HideDialogItem(dlog, EDIT15_MaxTranspose);
			GetHiddenDItemBox(dlog, EDIT15_MaxTranspose, &tempR);
			InsetRect(&tempR, -3, -3);
			FrameRect(&tempR);

			//PenPat(&qd.gray);	
			PenPat(NGetQDGlobalsGray());
			PenMode(patBic);
			
			/* GetDialogItem should get the bounding box of the panel we want to dim, but
			   for some reason, it actually gets the top and bottom of the entire pitch
			   panel! I don't like it, but we can just kludge in the top and bottom of
			   the line we really want to dim. */
			   
			GetDialogItem(dlog, USER33_PitchRelPanel, &type, &hndl, &box);
			box.top = tempR.top;					/* Kludge */
			box.bottom =  tempR.bottom;				/* Kludge */
			PaintRect(&box);						/* Dim everything within userItem rect */
			PenNormal();
		}
	}
	else {
		HiliteControl((ControlHandle)hndlRel, CTL_INACTIVE);
		HiliteControl((ControlHandle)hndlAbs, CTL_INACTIVE);
		HiliteControl((ControlHandle)hndlAbsAny, CTL_INACTIVE);

		HideDialogItem(dlog, EDIT11_PitchTol);
		GetHiddenDItemBox(dlog, EDIT11_PitchTol, &tempR);
		InsetRect(&tempR, -3, -3);
		FrameRect(&tempR);
		
		HideDialogItem(dlog, EDIT15_MaxTranspose);
		GetHiddenDItemBox(dlog, EDIT15_MaxTranspose, &tempR);
		InsetRect(&tempR, -3, -3);
		FrameRect(&tempR);

		//PenPat(&qd.gray);	
		PenPat(NGetQDGlobalsGray());
		PenMode(patBic);	
		GetDialogItem(dlog, pitchItem, &type, &hndl, &box);
		PaintRect(&box);													/* Dim everything within userItem rect */
		PenNormal();
	}
}

/* User Item-drawing procedure to dim subordinate check boxes and text items */

static pascal void UserDimPitchPanel(DialogPtr d, short pitchItem)
{
	DimPitchPanel(d, pitchItem);
}


/* -------------------------------------------- DimDurationPanel, UserDimDurationPanel -- */

/* Maintain state of items subordinate to the Match Duration checkbox. If that checkbox
is not checked, un-hilite its subordinate radio buttons and checkbox, hide the edit
field, and dim the whole area.  Otherwise, do the  opposite. The userItem's handle
should be set to a pointer to this procedure so it'll be called automatically by the
dialog filter. */
		
static void DimDurationPanel(DialogPtr dlog, short item)
{
	short		type;
	Handle		hndl, hndl1, hndl2, hndl3;
	Rect		box;
	Boolean		checked;
	
	checked = GetDlgChkRadio(dlog, CHK17_Duration);

	GetDialogItem(dlog, BUT18_DurPreserveContour, &type, &hndl1, &box);
	GetDialogItem(dlog, BUT19_DurRelative, &type, &hndl2, &box);
	GetDialogItem(dlog, BUT20_DurAbsolute, &type, &hndl3, &box);

	if (checked) {
		HiliteControl((ControlHandle)hndl1, CTL_ACTIVE);
		HiliteControl((ControlHandle)hndl2, CTL_ACTIVE);
		HiliteControl((ControlHandle)hndl3, CTL_ACTIVE);
	}
	else {
		HiliteControl((ControlHandle)hndl1, CTL_INACTIVE);
		HiliteControl((ControlHandle)hndl2, CTL_INACTIVE);
		HiliteControl((ControlHandle)hndl3, CTL_INACTIVE);
		
		//PenPat(&qd.gray);	
		PenPat(NGetQDGlobalsGray());
		PenMode(patBic);	
		GetDialogItem(dlog, item, &type, &hndl, &box);
		PaintRect(&box);													/* Dim everything within userItem rect */
		PenNormal();
	}
}

/* User Item-drawing procedure to dim subordinate check boxes and text items */

static pascal void UserDimDurationPanel(DialogPtr d, short dItem)
{
	DimDurationPanel(d, dItem);
}


/* -------------------------------------------------------------------------------------- */

/* Display Search modal dialog.  On OK, return True with parameters set. On Cancel or
error, simply return False. */

Boolean SearchDialog(
			Boolean findInFiles,		/* True=Search in Files command, else current score */
			Boolean *findAll,			/* True=batch (IR-style) search, False=interactive */
			Boolean *pUsePitch,			/* True=match on pitch, False=ignore pitch */
			Boolean *pUseDuration,		/* RUE=match on duration, False=ignore duration */
			INT16 *pPitchSearchType,	/* TYPE_PITCHMIDI_* value */
			INT16 *pDurSearchType,		/* TYPE_DURATION_* value */
			Boolean *pIncludeRests,		/* True=match rests against rests, False=ignore rests */
			INT16 *pMaxTranspose,		/* Maximum transposition (semitones) */
			INT16 *pPitchTolerance,		/* Tolerance for pitch or interval (semitones) */
			Boolean *pKeepPContour,		/* True=pitch contour must match regardless of tolerance */
			INT16 *pChordNotes,			/* TYPE_CHORD_* value */
			Boolean *pMatchTiedDur		/* Tied notes: True=extend first note, False=match each */
			)
{
	short itemHit, groupP, groupD, groupC, groupT, groupR;
	Boolean okay=False,keepGoing=True;
	DialogPtr dlog=NULL; GrafPtr oldPort;
	ModalFilterUPP myFilterUPP;
	SEARCHPARAMS sParm;
	short aShort; Handle aHdl; Rect pitchPanelBox, durationPanelBox, pitchRelPanelBox;

	sParm.usePitch = *pUsePitch;
	sParm.useDuration = *pUseDuration;
	sParm.pitchSearchType = *pPitchSearchType;
	sParm.durSearchType = *pDurSearchType;
	sParm.includeRests = *pIncludeRests;
	sParm.maxTranspose = *pMaxTranspose;
	sParm.pitchTolerance = *pPitchTolerance;
	sParm.pitchKeepContour = *pKeepPContour;
	sParm.chordNotes = *pChordNotes;
	sParm.matchTiedDur = *pMatchTiedDur;

	GetPort(&oldPort);

	/* On PowerPC, need a RoutineDescriptor from heap; on 68K, no allocation */
	
	//myFilterUPP = NewModalFilterProc(OKButFilter);
	myFilterUPP = NewModalFilterUPP(OKButFilter);
	if (myFilterUPP==NULL) goto cleanUp;

	/* Build dialog window and install its item values */
	
	dlog = OpenThisDialog(sParm, findInFiles, &groupP, &groupD, &groupC, &groupT, &groupR);
	if (dlog==NULL) goto cleanUp;

	GetDialogItem(dlog, USER31_PitchPanel, &aShort, &aHdl, &pitchPanelBox);
	GetDialogItem(dlog, USER32_DurPanel, &aShort, &aHdl, &durationPanelBox);
	GetDialogItem(dlog, USER33_PitchRelPanel, &aShort, &aHdl, &pitchRelPanelBox);

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(myFilterUPP, &itemHit);
		switch (itemHit) {
			case BUT1_Find:
			case BUT34_FindAll:
				/* Pick up settings of dialog items to return. */
				
				switch (groupP) {
					case BUT7_PitchRelative:
						sParm.pitchSearchType = TYPE_PITCHMIDI_REL;
						break;
					case BUT8_PitchAbsolute:
						sParm.pitchSearchType = TYPE_PITCHMIDI_ABS;
						break;
					case BUT9_PitchAbsoluteAnyOct:
						sParm.pitchSearchType = TYPE_PITCHMIDI_ABS_TROCT;
						break;
					default:
						;
				}

				switch (groupD) {
					case BUT18_DurPreserveContour:
						sParm.durSearchType = TYPE_DURATION_CONTOUR;
						break;
					case BUT19_DurRelative:
						sParm.durSearchType = TYPE_DURATION_REL;
						break;
					case BUT20_DurAbsolute:
						sParm.durSearchType = TYPE_DURATION_ABS;
						break;
					default:
						;
				}
								
				sParm.usePitch = (GetDlgChkRadio(dlog, CHK6_Pitch)==1);
				GetDlgWord(dlog,EDIT15_MaxTranspose,&sParm.maxTranspose);
				GetDlgWord(dlog,EDIT11_PitchTol,&sParm.pitchTolerance);

				/* The displayed state of "always preserve contour" may be affected by
				   other options, so pick up the saved state instead. */
				   
				sParm.pitchKeepContour = savePitchPreserveContour;
				
				switch (groupC) {
					case BUT22_ChordNotesAll:
						sParm.chordNotes = TYPE_CHORD_ALL;
						break;
					case BUT23_ChordNotesOuter:
						sParm.chordNotes = TYPE_CHORD_OUTER;
						break;
					case BUT24_ChordNotesTop:
						sParm.chordNotes = TYPE_CHORD_TOP;
						break;
					default:
						;
				}
				
				sParm.useDuration = (GetDlgChkRadio(dlog, CHK17_Duration)==1);

				switch (groupT) {
					case BUT29_TiedExtend:
						sParm.matchTiedDur = True;
						break;
					case BUT30_TiedMatch:
						sParm.matchTiedDur = False;
						break;
					default:
						;
				}

				switch (groupR) {
					case BUT26_RestsIgnore:
						sParm.includeRests = False;
						break;
					case BUT27_RestsMatch:
						sParm.includeRests = True;
						break;
					default:
						;
				}

				keepGoing = False; okay = True;
				break;

			case Cancel:
				keepGoing = False;
				break;

#ifdef NOTYET
			case BUT5_Show:
				ShowSearchPatDocument();
				break;
#endif
			case CHK6_Pitch:
				PutDlgChkRadio(dlog, CHK6_Pitch, !GetDlgChkRadio(dlog, CHK6_Pitch));
				InvalWindowRect(GetDialogWindow(dlog),&pitchPanelBox);			/* force filter to call DimPitchPanel */
				break;
			case CHK17_Duration:
				PutDlgChkRadio(dlog, CHK17_Duration, !GetDlgChkRadio(dlog, CHK17_Duration));
				InvalWindowRect(GetDialogWindow(dlog),&durationPanelBox);		/* force filter to call DimDurationPanel */
				break;
			
			case BUT7_PitchRelative:
			case BUT8_PitchAbsolute:
			case BUT9_PitchAbsoluteAnyOct:
				if (itemHit!=groupP) {
					SwitchRadio(dlog, &groupP, itemHit);
					InvalWindowRect(GetDialogWindow(dlog),&pitchRelPanelBox);	/* force filter to call DimPitchPanel */					
				}
				break;
			
			case CHK13_PreserveContour:
				PutDlgChkRadio(dlog, CHK13_PreserveContour, !GetDlgChkRadio(dlog, CHK13_PreserveContour));
				savePitchPreserveContour = GetDlgChkRadio(dlog, CHK13_PreserveContour);
				break;
			
			case BUT18_DurPreserveContour:
			case BUT19_DurRelative:
			case BUT20_DurAbsolute:
				if (itemHit!=groupD)
					SwitchRadio(dlog, &groupD, itemHit);
				break;
			
			case BUT22_ChordNotesAll:
			case BUT23_ChordNotesOuter:
			case BUT24_ChordNotesTop:
				if (itemHit!=groupC)
					SwitchRadio(dlog, &groupC, itemHit);
				break;

			case BUT29_TiedExtend:
			case BUT30_TiedMatch:
				if (itemHit!=groupT)
					SwitchRadio(dlog, &groupT, itemHit);
				break;

			case BUT26_RestsIgnore:
			case BUT27_RestsMatch:
				if (itemHit!=groupR)
					SwitchRadio(dlog, &groupR, itemHit);
				break;

			default:
				;
		}
	}
	
	/* No final processing of item values is needed in this dialog. */
	
	okay = (itemHit!=Cancel);

cleanUp:		
	if (dlog) CloseThisDialog(dlog);
	if (myFilterUPP) DisposeModalFilterUPP(myFilterUPP);
	SetPort(oldPort);
	
	*pUsePitch = sParm.usePitch;
	*pUseDuration = sParm.useDuration;
	*pPitchSearchType = sParm.pitchSearchType;
	*pDurSearchType = sParm.durSearchType;
	*pIncludeRests = sParm.includeRests;
	*pMaxTranspose = sParm.maxTranspose;
	*pPitchTolerance = sParm.pitchTolerance;
	*pKeepPContour = sParm.pitchKeepContour;
	*pChordNotes = sParm.chordNotes;
	*pMatchTiedDur = sParm.matchTiedDur;
	
	*findAll = (findInFiles || itemHit==BUT34_FindAll);
	return okay;
}


/* Build this dialog's window on desktop, and install initial item values. Return the
dlog opened, or NULL if error (no resource, no memory); return in parameters the radio-
button group settings. */

static DialogPtr OpenThisDialog(SEARCHPARAMS sParm, Boolean findInFiles, short *pGroupPitch,
									short *pGroupDur, short *pGroupChord, short *pGroupTied,
									short *pGroupRests)
{
	short patLen, dlogResID;
	short aShort;  Handle aHdl;
	Rect pitchPanelBox, durationPanelBox, pitchRelPanelBox;
	Boolean haveRest;
	GrafPtr oldPort;
	DialogPtr dlog;
	char fmtStr[256], str[256];
	UserItemUPP userDimPitchUPP, userDimDurUPP;

	dlogResID = (findInFiles? SEARCH_FILES_DLOG : SEARCH_DLOG);
	userDimPitchUPP = NewUserItemUPP(UserDimPitchPanel);
	if (userDimPitchUPP==NULL) {
		MissingDialog((long)dlogResID);  /* Missleading, but this isn't likely to happen. */
		return NULL;
	}
	
	userDimDurUPP = NewUserItemUPP(UserDimDurationPanel);
	if (userDimDurUPP == NULL) {
		DisposeUserItemUPP(userDimPitchUPP);
		MissingDialog((long)dlogResID);
		return NULL;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(dlogResID, NULL, BRING_TO_FRONT);
	if (dlog==NULL) { MissingDialog(dlogResID); return(NULL); }

	CenterWindow(GetDialogWindow(dlog), 70);
	SetPort(GetDialogWindowPort(dlog));
	//SetPort(dlog);

	/* Fill in dialog's initial values */

	patLen = N_SearchPatternLen(&haveRest);
	if (findInFiles)
		GetIndCString(fmtStr, SEARCH_STRS, (haveRest? 3 : 4));   	/* "Search scores in the folder for the %d..." */
	else
		GetIndCString(fmtStr, SEARCH_STRS, (haveRest? 1 : 2));   	/* "Search the front window for the %d..." */
	sprintf(str, fmtStr, patLen);
	CParamText(str, "", "", "");

	PutDlgChkRadio(dlog, CHK6_Pitch, sParm.usePitch? 1 : 0);
	PutDlgWord(dlog, EDIT15_MaxTranspose, sParm.maxTranspose, False);
	PutDlgWord(dlog, EDIT11_PitchTol, sParm.pitchTolerance, True);
	PutDlgChkRadio(dlog, CHK13_PreserveContour, sParm.pitchKeepContour? 1 : 0);
	savePitchPreserveContour = sParm.pitchKeepContour;

	PutDlgChkRadio(dlog, CHK17_Duration, sParm.useDuration? 1 : 0);

	*pGroupPitch = BUT7_PitchRelative;								/* Just in case */

	/* Set up radio button groups. */
	
	switch (sParm.pitchSearchType) {
		case TYPE_PITCHMIDI_REL:
			*pGroupPitch = BUT7_PitchRelative;
			break;
		case TYPE_PITCHMIDI_ABS:
			*pGroupPitch = BUT8_PitchAbsolute;
			break;
		case TYPE_PITCHMIDI_ABS_TROCT:
			*pGroupPitch = BUT9_PitchAbsoluteAnyOct;
			break;
	}
	PutDlgChkRadio(dlog, *pGroupPitch, True);

	switch (sParm.durSearchType) {
		case TYPE_DURATION_CONTOUR:
			*pGroupDur = BUT18_DurPreserveContour;
			break;
		case TYPE_DURATION_REL:
			*pGroupDur = BUT19_DurRelative;
			break;
		case TYPE_DURATION_ABS:
			*pGroupDur = BUT20_DurAbsolute;
			break;
	}
	PutDlgChkRadio(dlog, *pGroupDur, True);

	switch (sParm.chordNotes) {
		case TYPE_CHORD_ALL:
			*pGroupChord = BUT22_ChordNotesAll;
			break;
		case TYPE_CHORD_OUTER:
			*pGroupChord = BUT23_ChordNotesOuter;
			break;
		case TYPE_CHORD_TOP:
			*pGroupChord = BUT24_ChordNotesTop;
			break;
	}
	PutDlgChkRadio(dlog, *pGroupChord, True);

	*pGroupTied = (sParm.matchTiedDur==1? BUT29_TiedExtend : BUT30_TiedMatch);
	PutDlgChkRadio(dlog, *pGroupTied, True);

	*pGroupRests = (sParm.includeRests==0? BUT26_RestsIgnore : BUT27_RestsMatch);
	PutDlgChkRadio(dlog, *pGroupRests, True);

	/* Set userItem listeners. The relative-pitch panel is inside the Match Pitch
	   panel, so use the same listener for both. */
	   
	GetDialogItem(dlog, USER31_PitchPanel, &aShort, &aHdl, &pitchPanelBox);
	SetDialogItem(dlog, USER31_PitchPanel, userItem, (Handle)userDimPitchUPP,
						&pitchPanelBox);
	GetDialogItem(dlog, USER32_DurPanel, &aShort, &aHdl, &durationPanelBox);
	SetDialogItem(dlog, USER32_DurPanel, userItem, (Handle)userDimDurUPP,
						&durationPanelBox);
	GetDialogItem(dlog, USER33_PitchRelPanel, &aShort, &aHdl, &pitchRelPanelBox);
	SetDialogItem(dlog, USER33_PitchRelPanel, userItem, (Handle)userDimPitchUPP,
						&pitchPanelBox);

	ShowWindow(GetDialogWindow(dlog));
	DimPitchPanel(dlog, USER31_PitchPanel);		/* Prevent flashing if subordinates need to be dimmed right away */
	DimDurationPanel(dlog, USER32_DurPanel);	/* Prevent flashing if subordinates need to be dimmed right away */
		
	DisposeUserItemUPP(userDimPitchUPP);
	DisposeUserItemUPP(userDimDurUPP);
	return dlog;
}

/* Clean up any allocated stuff, and return dialog to primordial mists */

static void CloseThisDialog(DialogPtr dlog)
{
	if (dlog) {
		DisposeDialog(dlog);
	}
}
