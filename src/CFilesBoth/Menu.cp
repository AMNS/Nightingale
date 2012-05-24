/* Menu.c for Nightingale - general menu routines */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "CarbonPrinting.h"
#include "CarbonTemplates.h" // MAS
#include "MidiMap.h"

#ifdef COPY_PROTECT
#include "Eve.h"
#endif

static void	MovePalette(WindowPtr whichPalette,Point position);

static Boolean IsSafeToQuit(void);
static Boolean SetTranspKeyStaves(Document *, Boolean []);
static void EditPartMIDI(Document *doc);

static void	DoAppleMenu(INT16 choice);
static void	DoTestMenu(INT16 choice);
static void	DoScoreMenu(INT16 choice);
static void	DoNotesMenu(INT16 choice);
static void	DoGroupsMenu(INT16 choice);

static void MPInstrument(Document *);

static void	DoMasterPgMenu(INT16 choice);
static void	DoFormatMenu(INT16 choice);
static void	DoMagnifyMenu(INT16 choice);

static void	FMPreferences(Document *doc);

static void	SMLeftEnd(Document *doc);
static void	SMTextStyle(Document *doc);
static void	SMMeasNum(Document *doc);
static void	SMRespace(Document *doc);
static void	SMRealign(Document *doc);
static void	SMReformat(Document *doc);

static void	NMSetDuration(Document *doc);
static void NMDoubleHalveDurations(Document *doc, Boolean doubleDur);
static void	NMInsertByPos(Document *doc);
static void	NMSetMBRest(Document *doc);
static void	NMFillEmptyMeas(Document *doc);
static void 	NMAddModifiers(Document *doc);
static void 	NMStripModifiers(Document *doc);
static INT16 CountSelVoices(Document *);
static void 	NMMultiVoice(Document *doc);

static void	VMLookAt(void);
static void	VMLookAtAll(void);
static void	VMShowDurProblems(void);
static void	VMShowSyncs(void);
static void	VMShowInvisibles(void);
static void	VMColorVoices(void);
static void	VMPianoRoll(void);
static void	VMActivate(INT16);

static void	PLRecord(Document *doc, Boolean merge);
static void	PLStepRecord(Document *doc, Boolean merge);
static void	PLMIDIDynPrefs(Document *);
static void	PLMIDIModPrefs(Document *);
static void	PLTransposeScore(Document *);

static void InstallDebugMenuItems(Boolean installAll);

static void	FixFileMenu(Document *doc, INT16 nSel);
static void	FixEditMenu(Document *doc, INT16 nInSelRange, INT16 nSel, Boolean isDA);
static void	FixTestMenu(Document *doc, INT16 nSel);
static void	DisableSMMove(void);
static void	FixMoveMeasSys(Document *doc);
static void	FixScoreMenu(Document *doc, INT16 nSel);
static LINK	MBRestSel(Document *);
static void	FixNotesMenu(Document *doc, Boolean continSel);
static Boolean SelRangeChkOct(INT16 staff,LINK	staffStartL,LINK staffEndL);
static void	FixGroupsMenu(Document *doc, Boolean continSel);
static void	AddWindowList(void);
static void	FixViewMenu(Document *doc);
static void	FixPlayRecordMenu(Document *doc, INT16 nSel);
static void	FixMasterPgMenu(Document *doc);
static void	FixFormatMenu(Document *doc);

INT16 NumOpenDocuments(void);

static Boolean cmdIsBeam, cmdIsTuplet, cmdIsOctava;

static Boolean	goUp=TRUE;								/* For "Transpose" dialog */
static INT16 octaves=0, steps=0, semiChange=0;
static Boolean slashes=FALSE;

static Boolean	dGoUp=TRUE;								/* For "Diatonic Transpose" dialog */
static INT16 dOctaves=0, dSteps=0;
static Boolean dSlashes=FALSE;

static Boolean	kGoUp=TRUE;								/* For "Transpose Key" dialog */
static INT16 kOctaves=0, kSteps=0, kSemiChange=0;
static Boolean	kSlashes=FALSE, kNotes=TRUE, kChordSyms=TRUE;

static Boolean	beforeFirst;

static Boolean showDbgWin = FALSE;

static Boolean debugItemsNeedInstall = TRUE;
				
void DeleteObj(Document *, LINK);
void DeleteSelObjs(Document *);


Boolean DoMenu(long menuChoice)
	{
		static Boolean copyProtProblem=FALSE;
		register INT16 choice; INT16 menu;
		Boolean keepGoing = TRUE;
		
		menu = HiWord(menuChoice); choice = LoWord(menuChoice);
		if (TopDocument) MEHideCaret(GetDocumentFromWindow(TopDocument));

#ifdef COPY_PROTECT
		/*
		 *	To discourage a user from launching Nightingale, then giving their copy-
		 * protection key to someone else while they're running, check the key every
		 * now and then. But if we do find a problem with it, it's possible that 
		 * something went wrong and they're not trying to cheat, so don't be too
		 * severe--just ignore all following non-File-menu commands; they can still
		 * save files and use the Quit command (as well as some commands we'd rather
		 * they didn't)!
		 */

		if (menu!=fileID) {
			static long checkCopyProtTime=0L;
			long timeNow;

			if (copyProtProblem) goto Cleanup;
		
			timeNow = TickCount();
			if (timeNow>checkCopyProtTime) {
				checkCopyProtTime = timeNow+(3L*60L*60L);			/* Check at most every 3 minutes */
				if (!CopyProtectionOK(FALSE)) {
					GetIndCString(strBuf, COPYPROTECT_STRS, 4);	/* "Nightingale will now ignore all commands but File menu commands." */
					CParamText(strBuf, "", "", "");
					PlaceAlert(GENERIC_ALRT, NULL, 0, 80);
					StopInform(GENERIC_ALRT);
					copyProtProblem = TRUE;
					goto Cleanup;
				}
			}
		}
		
#endif
		switch(menu) {
			case appleID:
				DoAppleMenu(choice);
				break;
			case fileID:
				keepGoing = DoFileMenu(choice);
				break;
			case editID:
				DoEditMenu(choice);
				break;
			case testID:
				DoTestMenu(choice);
				break;
			case scoreID:
				DoScoreMenu(choice);
				break;
			case notesID:
				DoNotesMenu(choice);
				break;
			case groupsID:
				DoGroupsMenu(choice);
				break;
			case viewID:
				DoViewMenu(choice);
				break;
			case playRecID:
				DoPlayRecMenu(choice);
				break;
			case masterPgID:
				DoMasterPgMenu(choice);
				break;
			case formatID:
				DoFormatMenu(choice);
				break;
			case magnifyID:
				DoMagnifyMenu(choice);
				break;
			}
		
Cleanup:
		if (keepGoing) HiliteMenu(0);
		return(keepGoing);
	}

/*
 *	Handle a choice from the Apple Menu.
 */

static void DoAppleMenu(INT16 choice)
	{
//		Str255 accName;
		
		switch(choice) {
			case AM_About:
#ifdef PUBLIC_VERSION
				DoAboutBox(FALSE);
#else
				if (ShiftKeyDown() && OptionKeyDown() && CmdKeyDown())
					Debugger();
				 else
					DoAboutBox(FALSE);
#endif
				break;
			case AM_Help:
				Do_Help(appVRefNum, appDirID);
				UseResFile(appRFRefNum);				/* a precaution */
				break;
			default:
#ifndef TARGET_API_MAC_CARBON
				GetMenuItemText(appleMenu,choice,accName);
				OpenDeskAcc(accName);
#endif
				break;
			}
	}

Boolean IsSafeToQuit()
	{
		Document *doc; Boolean inMasterPage=FALSE;
		
		for (doc=documentTable; doc<topTable; doc++)
			if (doc->inUse)
				if (doc->masterView)
					{ inMasterPage = TRUE; break; }
		
		if (inMasterPage) {
			GetIndCString(strBuf, MENUCMDMSGS_STRS, 1);			/* "You can't quit now because at least one score is in Master Page." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
		else
			return TRUE;
	}

#ifdef TARGET_API_MAC_CARBON_FILEIO

static Boolean GetNotelistFile(Str255 macfName, NSClientDataPtr pNSD);
static Boolean GetNotelistFile(Str255 macfName, NSClientDataPtr pNSD)
{
	OSStatus 	err = noErr;
	OSType 		inputType[MAXINPUTTYPE];
	
	inputType[0] = 'TEXT';
	
	err = OpenFileDialog(kNavGenericSignature, 1, inputType, pNSD);
	
	if (err != noErr || pNSD->nsOpCancel) {
		return FALSE;
	}		
	
	FSSpec fsSpec = pNSD->nsFSSpec;
	PStrCopy(fsSpec.name, macfName);
	return TRUE;
}
#else
static Boolean GetNotelistFile(Str255 macfName, short *vRefNum);
static Boolean GetNotelistFile(Str255 macfName, short *vRefNum)
{
	OSType 		myTypes[MAXINPUTTYPE];
	SFReply		reply;
	Point		dialogWhere = { 90, 82 };
	
	myTypes[0] = 'TEXT';
	
	SFGetFile(dialogWhere, "\p", 0L, 1, myTypes, 0L, &reply);
	if (reply.good) {
		PStrCopy(reply.fName, macfName);
		*vRefNum = reply.vRefNum;
		return TRUE;
	}

	return FALSE;
}
#endif



#ifdef TEST_SEARCH_NG
#include "SearchScoreDBFormat.h"

static Boolean usePitch = TRUE;
static Boolean useDuration = TRUE;
static FASTFLOAT pitchWeight = 0.75;
static INT16 pitchSearchType = TYPE_PITCHMIDI_REL;
static INT16 durSearchType = TYPE_DURATION_REL;
static Boolean includeRests = TRUE;
static INT16 maxTranspose = 127;
static INT16 pitchTolerance = 0;
static Boolean keepContour = TRUE;
static INT16 chordNotes = TYPE_CHORD_OUTER;
static Boolean matchTiedDur = TRUE;

static void EMSearchMelody(Document *doc, Boolean again);
static void EMSearchMelody(Document *doc, Boolean again)
{

	Document *theDoc = (Document *)TopDocument;
	Boolean findAll;

	if (theDoc==searchPatDoc) {
		CParamText("The Search Pattern is the front window: you can't search it for itself!",
						"", "", "");											// ??I18N BUG
		StopInform(GENERIC_ALRT);
		return;
	}
	
	if (again) {
		(void)DoSearchScore(doc, TRUE, usePitch, useDuration, pitchSearchType,
							durSearchType, includeRests, maxTranspose, pitchTolerance,
							pitchWeight, keepContour, chordNotes, matchTiedDur);
		return;
	}

	if (SearchDialog(FALSE, &findAll, &usePitch, &useDuration, &pitchSearchType,
						&durSearchType, &includeRests, &maxTranspose, &pitchTolerance,
						&keepContour, &chordNotes, &matchTiedDur)) {
		if (findAll)
			(void)DoIRSearchScore(doc, usePitch, useDuration, pitchSearchType,
								durSearchType, includeRests, maxTranspose, pitchTolerance,
								pitchWeight, keepContour, chordNotes, matchTiedDur);
		else
			(void)DoSearchScore(doc, FALSE, usePitch, useDuration, pitchSearchType,
								durSearchType, includeRests, maxTranspose, pitchTolerance,
								pitchWeight, keepContour, chordNotes, matchTiedDur);
	}
}


/* Ask user for an input file. Return 1 if okay, 0 not (user cancelled). Note that this
code does not use Navigation Services, so it won't work on Mac OS X or Carbon apps, but
it assumes system 7.0 or later. */

short FSpGetInputFileFromUser(FSSpec *theFile);
short FSpGetInputFileFromUser(FSSpec *theFile)
{	
#if 0
	short ans;

	StandardFileReply reply;
	SFTypeList types;
	StandardGetFile(NULL,-1,types,&reply);
	if (reply.sfGood)
		*theFile = reply.sfFile;
	ans = reply.sfGood;

	return(ans);
#else			
	NSClientData nscd;
	short vrefnum; short ans;
	char str[256];
	
	if (ans = GetInputName(str,FALSE,tmpStr,&vrefnum,&nscd)) {
		*theFile = nscd.nsFSSpec;
		vrefnum = nscd.nsFSSpec.vRefNum;		
	}
	
	return ans;
#endif
						
}


static void FMSearchFilesMelody(void);
static void FMSearchFilesMelody()
{
	short vrefnum; INT16 returnCode; char str[256];
	Boolean findAllDummy;
	FSSpec theFile;
	Str255 filename;
#ifdef SEARCH_DBFORMAT_MEF
	if (!GetTextFile(filename, &vrefnum)) return;

	if (SearchDialog(TRUE, &findAllDummy, &usePitch, &useDuration, &pitchSearchType,
						&durSearchType, &includeRests, &maxTranspose, &pitchTolerance,
						&keepContour, &chordNotes, &matchTiedDur)) {
		(void)DoIRSearchFiles(filename, vrefnum, usePitch, useDuration, pitchSearchType,
							durSearchType, includeRests, maxTranspose, pitchTolerance,
							pitchWeight, keepContour, chordNotes, matchTiedDur);
	}
#else
	if (!FSpGetInputFileFromUser(&theFile)) return;
	
	if (SearchDialog(TRUE, &findAllDummy, &usePitch, &useDuration, &pitchSearchType,
						&durSearchType, &includeRests, &maxTranspose, &pitchTolerance,
						&keepContour, &chordNotes, &matchTiedDur)) {
		(void)DoIRSearchFiles(&theFile, usePitch, useDuration, pitchSearchType,
							durSearchType, includeRests, maxTranspose, pitchTolerance,
							pitchWeight, keepContour, chordNotes, matchTiedDur);
	}
#endif
}

#endif




/*
 *	Handle a choice from the File Menu.  Return FALSE if it's time to quit.
 */

Boolean DoFileMenu(INT16 choice)
	{
		Boolean keepGoing = TRUE, doSymbol; short vrefnum; INT16 returnCode;
		register Document *doc=GetDocumentFromWindow(TopDocument); char str[256];
		NSClientData nscd; FSSpec fsSpec;
		
		switch(choice) {
#ifndef VIEWER_VERSION
			case FM_New:
				doSymbol = TopDocument == NULL;
				DoOpenDocument(NULL,0,FALSE,NULL);
				if (doSymbol && !IsWindowVisible(palettes[TOOL_PALETTE])) {
					AnalyzeWindows();
					DoViewMenu(VM_SymbolPalette);
					}
				break;
			case FM_Open:
				if (!FirstFreeDocument()) TooManyDocs();
				 else {
					UseStandardType(documentType);
					GetIndCString(str, MENUCMDMSGS_STRS, 4);			/* "Which score do you want to open?" */
					if (returnCode = GetInputName(str,TRUE,tmpStr,&vrefnum,&nscd)) {
						if (returnCode == OP_OpenFile) {
							fsSpec = nscd.nsFSSpec;
							vrefnum = nscd.nsFSSpec.vRefNum;
   						DoOpenDocument(tmpStr,vrefnum,FALSE,&fsSpec);
						 }
						 else if (returnCode == OP_NewFile)
						 	keepGoing = DoFileMenu(FM_New);
						 else if (returnCode == OP_QuitInstead)
						 	keepGoing = DoFileMenu(FM_Quit);
						}
					}
				break;
#endif
			case FM_OpenReadOnly:
				if (!FirstFreeDocument()) TooManyDocs();
				 else {
					UseStandardType(documentType);
#ifdef VIEWER_VERSION
					UseStandardType(DOCUMENT_TYPE_NORMAL);				/* Also accept Nightingale's <documentType> */
					GetIndCString(str, MENUCMDMSGS_STRS, 4);			/* "Which score do you want to open?" */
#else
					GetIndCString(str, MENUCMDMSGS_STRS, 5);			/* "Which score do you want to open read-only?" */
#endif
					if (returnCode = GetInputName(str,FALSE,tmpStr,&vrefnum,&nscd))
						fsSpec = nscd.nsFSSpec;
						vrefnum = nscd.nsFSSpec.vRefNum;
						DoOpenDocument(tmpStr,vrefnum,TRUE, &fsSpec);
					}
				break;
			case FM_Close:
				if (doc) DoCloseWindow(doc->theWindow);
				break;
			case FM_CloseAll:
				DoCloseAllDocWindows();
				break;
#ifndef VIEWER_VERSION
			case FM_Save:
				if (doc) DoSaveDocument(doc);
				break;
			case FM_SaveAs:
				if (doc) DoSaveAs(doc);
				break;
			case FM_Revert:
				if (doc) DoRevertDocument(doc);
				break;
			case FM_Import:
			
				UseStandardType('Midi');
				ClearStandardTypes();
				
				GetIndCString(str, MIDIFILE_STRS, 1);					/* "What MIDI file do you want to Import?" */
				if (returnCode = GetInputName(str,FALSE,tmpStr,&vrefnum,&nscd)) {
					fsSpec = nscd.nsFSSpec;
					ImportMIDIFile(&fsSpec);
				}
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(ImportMIDIFile);
#endif
				break;
			case FM_Export:
				if (doc) SaveMIDIFile(doc);
				break;
			case FM_Extract:
				if (doc) DoExtract(doc);
				break;
#endif
#ifndef LIGHT_VERSION
			case FM_GetETF:
				// ETF (Finale Enigma Transportable File) support removed, so do nothing
				break;
#endif
#ifndef VIEWER_VERSION
#ifdef JG_NOTELIST
			case FM_GetNotelist:
				NSClientData nsData;
				Str255 filename;
				
				if (GetNotelistFile(filename, &nsData))
					OpenNotelistFile(filename, &nsData);

				break;
#endif
#endif
#ifndef VIEWER_VERSION
#ifdef USE_NL2XML
			case FM_Notelist2XML:
				NSClientData nsData;
				Str255 filename;
				
				if (GetNotelistFile(filename, &nsData))
					NL2XMLParseNotelistFile(fileName, &nsData);

				break;
#endif
#endif
			case FM_OpenMidiMap:
				MidiMapInfo();
				break;

			case FM_SheetSetup:
				if (doc) DoSheetSetup(doc);
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(DoSheetSetup);
#endif
				break;
#ifndef VIEWER_VERSION
			case FM_PageSetup:
#if TARGET_API_MAC_CARBON

#if 0
				if (doc) NDoCustomPageSetup(doc);
#else
				if (doc) NDoPageSetup(doc);
#endif

#else
				if (doc) DoPageSetup(doc);
#endif
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(DoPageSetup);
#endif
				break;
			case FM_Print:
#if TARGET_API_MAC_CARBON
				if (doc) NDoPrintScore(doc);
#else
				if (doc) DoPrintScore(doc);
#endif
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(DoPrintScore);
#endif
				break;
			case FM_SaveEPSF:
#if TARGET_API_MAC_CARBON
				if (doc) NDoPostScript(doc);
#else
				if (doc) DoPostScript(doc);
#endif
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(DoPostScript);
#endif
				break;
			case FM_SaveText:
				if (doc) SaveNotelist(doc, ANYONE, TRUE);
				break;
			case FM_ScoreInfo:
				printf("File Menu: Score Info\n");
				
				if (OptionKeyDown()) {
				//if (1) {
					InstallDebugMenuItems(ControlKeyDown());
				}
				else {					
					ScoreInfo();
				}
				break;
			case FM_Preferences:
				if (doc) FMPreferences(doc);
				break;
#endif

#ifdef TEST_SEARCH_NG
			case FM_SearchInFiles:
				FMSearchFilesMelody();
				break;
#endif
			case FM_Quit:
				if (!IsSafeToQuit()) break;
				if (!SaveToolPalette(TRUE)) break;
				ShowHidePalettes(FALSE);
				quitting = TRUE;
				break;
			}
		
		if (keepGoing) HiliteMenu(0);
		return(keepGoing);
	}

#ifdef VIEWER_VERSION

void DoEditMenu(choice)
	INT16 choice;
	{
		SysBeep(1);
	}

#else

/*
 *	Handle a choice from the Edit Menu.
 */

//#define TEST_SEARCH_NG				// ??TEMPORARY! BELONGS IN Nightingale.precomp.c!!!!!!!!
void DoEditMenu(INT16 choice)
	{
		register Document *doc=GetDocumentFromWindow(TopDocument);
		if (doc==NULL) return;
		
DebugPrintf("DoEditMenu: choice=%ld\n", (long)choice);					
		switch(choice) {
			case EM_Undo:
				DoUndo(doc);
				break;
			case EM_Cut:
				lastCopy = COPYTYPE_CONTENT;
				DoCut(doc);
				break;
			case EM_Copy:
				lastCopy = COPYTYPE_CONTENT;
				DoCopy(doc);
				break;
			case EM_Paste:
				if (lastCopy==COPYTYPE_CONTENT)
					DoPaste(doc);
				else if (lastCopy==COPYTYPE_SYSTEM)
					PasteSystem(doc);
				else if (lastCopy==COPYTYPE_PAGE)
					PastePages(doc);
				break;
			case EM_Clear:
				DoClear(doc);
				break;
			case EM_Merge:
				DoMerge(doc);
#ifdef CURE_MACKEYS_DISEASE
				DCheckNEntries(doc);
#endif
				break;
			case EM_Double:
				Double(doc);
				break;
			case EM_ClearSystem:
				ClearSystem(doc);
				break;
			case EM_CopySystem:
				lastCopy = COPYTYPE_SYSTEM;
				CopySystem(doc);
				break;
			case EM_ClearPage:
				ClearPages(doc);
				break;
			case EM_CopyPage:
				lastCopy = COPYTYPE_PAGE;
				CopyPages(doc);
				break;
			case EM_SelAll:
				SelectAll(doc);
				break;
			case EM_ExtendSel:
				ExtendSelection(doc);
				break;
			case EM_GetInfo:
				InfoDialog(doc);
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(InfoDialog);
#endif
				break;
			case EM_ModifierInfo:
				ModNRDialog(doc);
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(InfoDialog);
#endif
				break;
			case EM_Set:
				DoSet(doc);
				break;

#ifdef TEST_SEARCH_NG
			case EM_SearchMelody:
				EMSearchMelody(doc, FALSE);
				break;
			case EM_SearchAgain:
				EMSearchMelody(doc, TRUE);
				break;
#endif
			case EM_Browser:
				if (OptionKeyDown())
					Browser(doc,doc->masterHeadL, doc->masterTailL);
				else if (ShiftKeyDown())
					Browser(doc,doc->undo.headL, doc->undo.tailL);
				else
					Browser(doc,doc->headL, doc->tailL);
				break;
			case EM_Debug:
				printf("printf: running debug command\n");
				DebugPrintf("DebugPrintf: running debug command\n");
				
				DoDebug("M");
				fflush(stdout);
				break;
			case EM_DeleteObjs:
				if (doc) DeleteSelObjs(doc);
				break;
		}
	}

#endif

//#ifndef PUBLIC_VERSION
#if 1


/* Functions to delete selected objects from the score "stupidly", i.e., with almost
no attempt to maintain consistency. This is intended to allow wizards to repair
messed-up files and to help debug Nightingale, but it's obviously very dangerous:
ordinary users should never be allowed to do it! */
 
void DeleteObj(Document *, LINK);
void DeleteSelObjs(Document *);

void DeleteObj(Document *doc, LINK pL)
{
	DebugPrintf("About to DeleteNode(%d) of type=%d...", pL, ObjLType(pL));

	if (InDataStruct(doc, pL, MAIN_DSTR)) {
		DeleteNode(doc, pL);
		DebugPrintf("done.\n");
		doc->changed = TRUE;
	}
	else
		DebugPrintf("NODE NOT IN MAIN OBJECT LIST.\n");
}

void DeleteSelObjs(Document *doc)
{
	INT16 nInRange, nSelFlag; LINK pL, nextL, firstMeasL;
	
	CountSelection(doc, &nInRange, &nSelFlag);
	if (nSelFlag<=0) { SysBeep(1); return; }
	
	sprintf(strBuf, "%d", nSelFlag);
	CParamText(strBuf, "", "", "");
	if (CautionAdvise(STUPID_KILL_ALRT)==Cancel) return;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = nextL) {
		nextL = RightLINK(pL);
		if (LinkSEL(pL)) DeleteObj(doc, pL);
	}

	/* Our only attempt to preserve consistency is to make the selection range legal. */
	
	if (!InDataStruct(doc, doc->selStartL, MAIN_DSTR)
	||  !InDataStruct(doc, doc->selEndL, MAIN_DSTR)) {
		firstMeasL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, FALSE);
		doc->selStartL = doc->selEndL = RightLINK(firstMeasL);
	}
}

void SetMeasNumPos(Document *doc, LINK startL, LINK endL, INT16 xOffset, INT16 yOffset);
void ResetAllMeasNumPos(Document *doc);

void SetMeasNumPos(Document *doc,
							LINK startL, LINK endL,
							INT16 xOffset, INT16 yOffset)
{
	PAMEASURE	aMeasure;
	LINK			pL, aMeasureL;

	for (pL = startL; pL!=endL; pL=RightLINK(pL)) {
		if (MeasureTYPE(pL)) {
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
				aMeasure = GetPAMEASURE(aMeasureL);
				aMeasure->xMNStdOffset = xOffset;
				aMeasure->yMNStdOffset = yOffset;
			}
			doc->changed = TRUE;
		}
	}
}


void ResetAllMeasNumPos(Document *doc)
{
	SetMeasNumPos(doc, doc->headL, doc->tailL, 0, 0);
	InvalWindow(doc);
}

#endif /* PUBLIC_VERSION */

/*
 *	Handle a choice from the Test Menu.
 */

static void DoTestMenu(INT16 choice)
	{
#ifndef PUBLIC_VERSION
		register Document *doc = GetDocumentFromWindow(TopDocument);			

		switch(choice) {
			case TS_Browser:
				if (doc) {
					if (OptionKeyDown())
						Browser(doc,doc->masterHeadL, doc->masterTailL);
					else if (ShiftKeyDown())
						Browser(doc,doc->undo.headL, doc->undo.tailL);
					else
						Browser(doc,doc->headL, doc->tailL);
#ifndef TARGET_API_MAC_CARBON
					UnloadSeg(Browser);
#endif
					}
				break;
			case TS_HeapBrowser:
				HeapBrowser();
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(HeapBrowser);
#endif
				break;
			case TS_Context:
				if (doc) {
					ShowContext(doc);
#ifndef TARGET_API_MAC_CARBON
					UnloadSeg(ShowContext);
#endif
					}
				break;
			case TS_ClickErase:
				clickMode = ClickErase;
				DebugPrintf("ClickMode set to ClickErase\n");
				break;
			case TS_ClickSelect:
				clickMode = ClickSelect;
				DebugPrintf("ClickMode set to ClickSelect\n");
				break;
			case TS_ClickFrame:
				clickMode = ClickFrame;
				DebugPrintf("ClickMode set to ClickFrame\n");
				break;
			case TS_Debug:
#ifndef PUBLIC_VERSION
				DoDebug("M");
#endif				
				break;
			case TS_ShowDebug:
//				static Boolean showDbgWin = FALSE;
				
				if (showDbgWin = !showDbgWin) {
//					ShowHideDebugWindow(TRUE);
					DebugPrintf("Showing Debug Window\n");					
				}
				else {				
//					ShowHideDebugWindow(FALSE);
				}
				
				break;
			case TS_DeleteObjs:
				if (doc) DeleteSelObjs(doc);
				break;
			case TS_ResetMeasNumPos:
				if (doc) ResetAllMeasNumPos(doc);
				break;
			case TS_PrintMusFontTables:
				if (doc) PrintMusFontTables(doc);
				break;
			default:
				break;
			}
#endif //ndef PUBLIC_VERSION
	}

/*
 *	Handle a choice from the Score Menu.
 */

#ifdef VIEWER_VERSION

static void DoScoreMenu(INT16 choice)
	{
		SysBeep(1);
	}

/*
 *	Handle a choice from the Notes Menu.
 */

static void DoNotesMenu(INT16 choice)
	{
		SysBeep(1);
	}

/*
 *	Handle a choice from the Groups Menu.
 */

void DoGroupsMenu(INT16 choice)
	{
		SysBeep(1);
	}

#else

static void DoScoreMenu(INT16 choice)
	{
		register Document *doc=GetDocumentFromWindow(TopDocument); LINK insertL; INT16 where;
		if (doc==NULL) return;
		
		switch(choice) {
			case SM_MasterSheet:
				DoMasterSheet(doc);
				break;
			case SM_CombineParts:
				if (doc) DoCombineParts(doc);
				break;
			case SM_ShowFormat:
				DoShowFormat(doc);
				break;
			case SM_LeftEnd:
				SMLeftEnd(doc);
				break;
			case SM_TextStyle:
				SMTextStyle(doc);
				break;
			case SM_FlowIn:
				DoTextFlowIn(doc);
				break;
			case SM_MeasNum:
				SMMeasNum(doc);
				break;
			case SM_PageNum:
				if (HeaderFooterDialog(doc)) {
					InvalWindow(doc);
					EraseAndInvalMessageBox(doc);
				}
				break;
			case SM_AddSystem:
				insertL = AddSysInsertPt(doc, doc->selStartL, &where);
				AddSystem(doc, insertL, where);
				break;
			case SM_AddPage:
				insertL = AddPageInsertPt(doc, doc->selStartL);
				AddPage(doc, insertL);
				break;
			case SM_Respace:
				SMRespace(doc);
				break;
			case SM_Realign:
				SMRealign(doc);
				break;
			case SM_AutoRespace:
				doc->autoRespace = !doc->autoRespace;
				CheckMenuItem(scoreMenu, SM_AutoRespace, doc->autoRespace);
				break;
			case SM_Justify:
				JustifySystems(doc, doc->selStartL, doc->selEndL);
				break;
			case SM_Reformat:
				SMReformat(doc);
				break;
			case SM_MoveMeasUp:
				MoveBarsUp(doc, doc->selStartL, doc->selEndL);
				break;
			case SM_MoveMeasDown:
				MoveBarsDown(doc, doc->selStartL, doc->selEndL);
				break;
			case SM_MoveSystemUp:
				MoveSystemUp(doc, doc->selStartL);
				break;
			case SM_MoveSystemDown:
				MoveSystemDown(doc, doc->selStartL);
				break;
			default:
				break;
			}
	}

/* Collect information for the Transpose Key command: fill the <trStaff> array with
flags indicating which staff nos. should be transposed (because they have something
selected in the reserved area at the start of the selection). To keep users from
getting confused with the Transpose Chromatic and Transpose Diatonic commands (which
work very differently), if anything after the reserved area is selected, return FALSE
to indicate Transpose Key shouldn't be allowed. */

static Boolean SetTranspKeyStaves(Document *doc, Boolean trStaff[])
	{
		INT16 s; LINK pL, aClefL, aKeySigL, aTimeSigL, measL;
		
		/* If anything is selected after the initial reserved area, don't allow
			Transpose Key. */
		
		measL = LSSearch(LeftLINK(doc->selEndL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
		if (measL) return FALSE;

		/* Decide which staves will be affected. */
		
		for (s = 1; s<=MAXSTAVES; s++)
			trStaff[s] = FALSE;
			
		for (pL = doc->selStartL; pL && !MeasureTYPE(pL); pL = RightLINK(pL))
			switch (ObjLType(pL)) {
				case CLEFtype:
					aClefL = FirstSubLINK(pL);
					for ( ; aClefL; aClefL = NextCLEFL(aClefL))
						if (ClefSEL(aClefL)) trStaff[ClefSTAFF(aClefL)] = TRUE;
					break;
				case KEYSIGtype:
					aKeySigL = FirstSubLINK(pL);
					for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
						if (KeySigSEL(aKeySigL)) trStaff[KeySigSTAFF(aKeySigL)] = TRUE;
					break;
				case TIMESIGtype:
					aTimeSigL = FirstSubLINK(pL);
					for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
						if (TimeSigSEL(aTimeSigL)) trStaff[TimeSigSTAFF(aTimeSigL)] = TRUE;
					break;
				default:
					;
			}

		return TRUE;
	}


/*
 *	Handle a choice from the Notes Menu.
 */

static void DoNotesMenu(INT16 choice)
	{
		register Document *doc=GetDocumentFromWindow(TopDocument);
		INT16 delAccCode;
		Boolean canTranspKey, trStaff[MAXSTAVES+1];
		short	addAccCode;
		if (doc == NULL) return;
		
		switch(choice) {
			case NM_SetDuration:
				NMSetDuration(doc);
				break;
			case NM_CompactV:
				DoCompactVoices(doc);
#ifdef CURE_MACKEYS_DISEASE
				DCheckNEntries(doc);
#endif
				break;
			case NM_InsertByPos:
				NMInsertByPos(doc);
				break;
			case NM_ClarifyRhythm:
				ClarifyRhythm(doc);
				break;
			case NM_SetMBRest:
				NMSetMBRest(doc);
				break;
			case NM_FillEmptyMeas:
				NMFillEmptyMeas(doc);
				break;
			case NM_TransposeKey:
				canTranspKey = SetTranspKeyStaves(doc, trStaff);
				if (!canTranspKey) {
					StopInform(TRANSKEY_ALRT);
					break;
				}
				if (TransKeyDialog(trStaff, &kGoUp, &kOctaves, &kSteps, &kSemiChange, &kSlashes,
											&kNotes, &kChordSyms)) {
					PrepareUndo(doc, doc->selStartL, U_TranspKey, 15);    /* "Transpose Key" */
					if (TransposeKey(doc, kGoUp, kOctaves, kSteps, kSemiChange, trStaff,
											kSlashes, kNotes, kChordSyms))
						InvalSelRange(doc);
					}
				break;
			case NM_Transpose:
				if (TransposeDialog(&goUp, &octaves, &steps, &semiChange, &slashes)) {
					PrepareUndo(doc, doc->selStartL, U_Transpose, 16);    /* "Transpose" */
					if (Transpose(doc, goUp, octaves, steps, semiChange, slashes))
						InvalSelRange(doc);
					}
				break;
			case NM_DiatonicTranspose:
				if (DTransposeDialog(&dGoUp, &dOctaves, &dSteps, &dSlashes)) {
					PrepareUndo(doc, doc->selStartL, U_DiatTransp, 17);	/* "Transpose Diatonic" */
					if (DTranspose(doc, dGoUp, dOctaves, dSteps, dSlashes))
						InvalSelRange(doc);
					}
				break;
			case NM_Respell:
				PrepareUndo(doc, doc->selStartL, U_Respell, 18);    		/* "Respell" */
				if (Respell(doc))
					InvalSelRange(doc);
				break;
			case NM_Accident:
				delAccCode = Advise(ACCIDENTAL_ALRT);
				if (delAccCode!=Cancel) {
					PrepareUndo(doc, doc->selStartL, U_DelRedundAcc, 19);    /* "Delete Redundant Accidentals" */
					if (DelRedundantAccs(doc, ANYONE, delAccCode))
						InvalSelRange(doc);
				}
				break;
			case NM_AddAccident:
				addAccCode = Advise(ADDACCIDENTAL_ALRT);
				if (addAccCode!=Cancel) {
					PrepareUndo(doc, doc->selStartL, U_AddRedundAcc, 42);		/* "Add Redundant Accidentals" */
					if (AddRedundantAccs(doc, ANYONE, addAccCode, TRUE))
						InvalSelRange(doc);
				}
				break;
			case NM_CheckRange:
				CheckRange(doc);
				break;
			case NM_AddModifiers:
				NMAddModifiers(doc);
				break;
			case NM_StripModifiers:
				NMStripModifiers(doc);
				break;
			case NM_Multivoice:
				NMMultiVoice(doc);
				break;
			case NM_Flip:
				FlipSelDirection(doc);
				break;
			default:
				break;
			}
	}

/*
 *	Handle a choice from the Groups Menu.
 */

void DoGroupsMenu(INT16 choice)
	{
		register Document *doc=GetDocumentFromWindow(TopDocument);
		TupleParam tParam; Boolean okay;
		if (doc==NULL) return;
		
		switch(choice) {
			case GM_AutomaticBeam:
				DoAutoBeam(doc);
#ifndef TARGET_API_MAC_CARBON
				UnloadSeg(DoAutoBeam);
#endif
				break;
			case GM_Beam:
				if (cmdIsBeam) DoBeam(doc);
				else				DoUnbeam(doc);
				break;
			case GM_BreakBeam:
				DoBreakBeam(doc);
				break;
			case GM_FlipFractional:
				DoFlipFractionalBeam(doc);
				break;
			case GM_Tuplet:
				if (cmdIsTuplet) {
					tParam.isFancy = FALSE;
					DoTuple(doc, &tParam);
#ifndef TARGET_API_MAC_CARBON
					UnloadSeg(DoTuple);
#endif
#ifdef CURE_MACKEYS_DISEASE
					DCheckNEntries(doc);
#endif
					}
				else {
					DoUntuple(doc);
#ifndef TARGET_API_MAC_CARBON
					UnloadSeg(DoUntuple);
#endif
#ifdef CURE_MACKEYS_DISEASE
					DCheckNEntries(doc);
#endif
					}
				break;
			case GM_FancyTuplet:
				if (FTupletCheck(doc, &tParam)) {
					tParam.isFancy = TRUE;
					okay = TupletDialog(doc, &tParam, TRUE);
					if (okay) DoTuple(doc, &tParam);
					}
#ifdef CURE_MACKEYS_DISEASE
				DCheckNEntries(doc);
#endif
				break;
			case GM_Octava:
				if (cmdIsOctava) {
					DoOctava(doc);
#ifndef TARGET_API_MAC_CARBON
					UnloadSeg(DoOctava);
#endif
				}
				else {
					DoRemoveOctava(doc);
#ifndef TARGET_API_MAC_CARBON
					UnloadSeg(DoRemoveOctava);
#endif
				}
				break;
			default:
				break;
			}
	}

#endif

/*
 *	Handle a choice from the View Menu.
 */

void		ShowSearchDocument(void);			

void DoViewMenu(INT16 choice)
	{
		register Document *doc=GetDocumentFromWindow(TopDocument); INT16 palIndex;
		Rect screen, pal, docRect;
		
		switch(choice) {
			case VM_GoTo:
				if (doc) GoTo(doc);
				break;
			case VM_GotoSel:
				if (doc) GoToSel(doc);
				break;
			case VM_Reduce:
				if (doc) SheetMagnify(doc, -1);
				break;
			case VM_Enlarge:
				if (doc) SheetMagnify(doc, 1);
				break;
			case VM_ReduceEnlargeTo:
				SysBeep(1);							/* A heirarchic menu: we should never get here */
				break;
			case VM_LookAtV:
				VMLookAt();
				break;
			case VM_LookAtAllV:
				VMLookAtAll();
				break;
#ifndef VIEWER_VERSION
			case VM_ShowDurProb:
				VMShowDurProblems();
				break;
			case VM_ShowSyncL:
				VMShowSyncs();
				break;
			case VM_ShowInvis:
				VMShowInvisibles();
				break;
			case VM_ShowSystems:
				if (doc) {
					CheckMenuItem(viewMenu, VM_ShowSystems, doc->frameSystems);
					doc->frameSystems = !doc->frameSystems;
					InvalWindow(doc);
					}
				break;
#endif
			case VM_ColorVoices:
				VMColorVoices();
				break;
			case VM_RedrawScr:
				RefreshScreen();
				break;
#ifndef VIEWER_VERSION
			case VM_PianoRoll:
				VMPianoRoll();
				break;
			case VM_ShowClipboard:
				ShowClipDocument();
				break;
			case VM_SymbolPalette:
				palIndex = TOOL_PALETTE;
				GetWindowPortBounds(palettes[palIndex], &pal);
				OffsetRect(&pal,-pal.left, -pal.right);
				if (TopDocument) {
					/*
					 *	Initial position is the offset in config.toolsPosition from
					 *	the upper left corner of the document's window, but not
					 *	offscreen.
					 */					 
					//docRect = (*((WindowPeek)TopDocument)->contRgn)->rgnBBox;
					GetWindowRgnBounds(TopDocument, kWindowContentRgn, &docRect);					
					docRect.right = docRect.left + pal.right;
					docRect.bottom = docRect.top + pal.bottom;
					GetMyScreen(&docRect,&screen);
					OffsetRect(&docRect,config.toolsPosition.h,config.toolsPosition.v);
					PullInsideRect(&docRect,&screen,2);
					}
				 else {
					/* Initial position is upper left corner of main screen */
					GetQDScreenBitsBounds(&docRect);
				 	InsetRect(&docRect, 10, GetMBarHeight()+16);
				 	}
				(*paletteGlobals[palIndex])->position.h = docRect.left;
				(*paletteGlobals[palIndex])->position.v = docRect.top;
				MovePalette(palettes[palIndex],(*paletteGlobals[palIndex])->position);
				palettesVisible[TOOL_PALETTE] = TRUE;
				break;
			case VM_ShowSearchPattern:
				ShowSearchDocument();
				break;
#endif
			default:
				VMActivate(choice);
				break;
			}
	}

/*
 * Handle the "MIDI Dynamics Preferences" command.
 */

void PLMIDIDynPrefs(Document *doc)
{
#ifdef VIEWER_VERSION
	static Boolean apply=TRUE;
#else
	static Boolean apply=FALSE;
#endif
	Boolean okay;
	
	okay = MIDIDynamDialog(doc, &apply);
	if (okay)
		DisableUndo(doc, FALSE);
	if (okay && apply) {
		SetVelFromContext(doc, FALSE);
		doc->changed = TRUE;
	}
}


/*
 * Handle the "MIDI Dynamics Preferences" command.
 */

void PLMIDIModPrefs(Document *doc)
{
	Boolean okay;
	
	okay = MIDIModifierDialog(doc);
}


/*
 * Toggle the document's "transposed score" flag.
 */

void PLTransposeScore(Document *doc)
{
	static Boolean warnedTransp = FALSE;
	INT16 partn, nparts; LINK partL;
	PPARTINFO pPart;

	if (!warnedTransp) {
		nparts = LinkNENTRIES(doc->headL)-1;
		partL = NextPARTINFOL(FirstSubLINK(doc->headL));
		for (partn = 1; partn<=nparts; partn++, partL = NextPARTINFOL(partL)) {
			pPart = GetPPARTINFO(partL);
			if (pPart->transpose!=0) {
				NoteInform(TRANSP_ALRT);
				warnedTransp = TRUE;
				break;
			}
		}
	}
	doc->transposed = !doc->transposed;
	doc->changed = TRUE;
}


#include "MIDIPASCAL3.h"

/* Handle the "Part MIDI Settings" dialog. */

void EditPartMIDI(Document *doc)
{
	LINK partL, pMPartL; PPARTINFO pPart, pMPart;
	PARTINFO partInfo; short firstStaff;
	INT16 partn;
	Boolean allParts = FALSE;

	partL = GetSelPart(doc);
	firstStaff = PartFirstSTAFF(partL);
	pPart = GetPPARTINFO(partL);
	partInfo = *pPart;
	if (useWhichMIDI == MIDIDR_OMS) {
		OMSUniqueID device;
		partn = (INT16)PartL2Partn(doc, partL);
		device = GetOMSDeviceForPartn(doc, partn);
		if (OMSPartMIDIDialog(doc, &partInfo, &device)) {
			pPart = GetPPARTINFO(partL);
			*pPart = partInfo;
			pMPartL = Staff2PartL(doc, doc->masterHeadL, firstStaff);
			pMPart = GetPPARTINFO(pMPartL);
			*pMPart = partInfo;
			SetOMSDeviceForPartn(doc, partn, device);
			doc->changed = TRUE;
		}
	}
	else if (useWhichMIDI == MIDIDR_CM) {
		MIDIUniqueID device;
		partn = (INT16)PartL2Partn(doc, partL);
		device = GetCMDeviceForPartn(doc, partn);
		if (CMPartMIDIDialog(doc, &partInfo, &device, &allParts)) {
		
			pPart = GetPPARTINFO(partL);
			*pPart = partInfo;
			pMPartL = Staff2PartL(doc, doc->masterHeadL, firstStaff);
			pMPart = GetPPARTINFO(pMPartL);
			*pMPart = partInfo;
			SetCMDeviceForPartn(doc, partn, device);
			doc->changed = TRUE;
			
			if (allParts) {
				INT16 partNum;
				
				partL = FirstSubLINK(doc->headL);
				for (partNum=0; partNum<=LinkNENTRIES(doc->headL)-1; partNum++, partL = NextPARTINFOL(partL)) 
				{
					if (partNum==0) continue;					/* skip dummy partn = 0 */
										
					SetCMDeviceForPartn(doc, partNum, device);
					
					//pPart = GetPPARTINFO(partL);
					//pPart->channel = partInfo.channel;
					
					//pPart->patchNum = partInfo.patchNum;
					//pPart->partVelocity = partInfo.partVelocity;
					
					// This causes the instrument description to be illegal
					//pPart->transpose = partInfo.transpose;
				}
			}
		}
	}
	else if (useWhichMIDI == MIDIDR_FMS) {
		fmsUniqueID device;
		device = GetFMSDeviceForPartL(doc, partL);
		if (FMSPartMIDIDialog(doc, &partInfo, &device)) {
			pPart = GetPPARTINFO(partL);
			*pPart = partInfo;
			FMSSetOutputDestinationMatch(doc, partL);
			partInfo = *pPart;				/* includes updated destinationMatch rec */
			pMPartL = Staff2PartL(doc, doc->masterHeadL, firstStaff);
			pMPart = GetPPARTINFO(pMPartL);
			*pMPart = partInfo;
			doc->changed = TRUE;
		}
	}
	else if (PartMIDIDialog(doc, &partInfo, &allParts)) {
		/* Update the PARTINFO in both the main object list and Master Page. */
		pPart = GetPPARTINFO(partL);
		*pPart = partInfo;
		pMPartL = Staff2PartL(doc, doc->masterHeadL, firstStaff);
		pMPart = GetPPARTINFO(pMPartL);
		*pMPart = partInfo;
		doc->changed = TRUE;
	}
}

/* If we're using built-in MIDI and the port currently chosen for built-in MIDI is
already in use, this function gives an error message and returns TRUE, else it just
returns FALSE. Call it before starting a MIDI operation that might use built-in MIDI. */

static Boolean BIMIDIPortIsBusy(void);
static Boolean BIMIDIPortIsBusy()
{
#ifdef ENABLE_BIMIDI
	if (useWhichMIDI!=MIDIDR_BI) return FALSE;
	
	if (ControlKeyDown()) return FALSE;							/* Skip checking */

	if (portSettingBIMIDI==MODEM_PORT) {
		if (!PORT_IS_FREE(MLM_PortAUse)) {
			GetIndCString(strBuf, MENUCMDMSGS_STRS, 7);		/* "...Modem port already in use." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return TRUE;
		}
	}
	else {
		if (!PORT_IS_FREE(MLM_PortBUse)) {
			GetIndCString(strBuf, MENUCMDMSGS_STRS, 8);		/* "...Printer port already in use." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return TRUE;
		}
	}
#endif	
	return FALSE;
}

/*
 *	Handle a choice from the Play/Record Menu
 */

void DoPlayRecMenu(INT16 choice)
	{		
		register Document *doc=GetDocumentFromWindow(TopDocument);
//		short newPortSetting, newInterfaceSpeed;
//		INT16 oldMIDIThru;

		switch(choice) {
		
			case PL_PlayEntire:
				if (BIMIDIPortIsBusy()) break;
				MEHideCaret(doc);
				if (doc) PlayEntire(doc);
				break;
			case PL_PlaySelection:
				if (BIMIDIPortIsBusy()) break;
				MEHideCaret(doc);
				if (doc) PlaySelection(doc);
				break;
			case PL_PlayFromSelection:
				if (BIMIDIPortIsBusy()) break;
				MEHideCaret(doc);
				if (doc) PlaySequence(doc, doc->selStartL, doc->tailL, TRUE, FALSE);
				break;
#ifndef VIEWER_VERSION
			case PL_RecordInsert:
				if (BIMIDIPortIsBusy()) break;
				if (doc) PLRecord(doc, FALSE);
				break;
			case PL_Quantize:
				if (doc) DoQuantize(doc);
				break;
			case PL_RecordMerge:
				if (BIMIDIPortIsBusy()) break;
				if (doc) PLRecord(doc, TRUE);
				break;
			case PL_StepRecInsert:
				if (BIMIDIPortIsBusy()) break;
				if (doc) PLStepRecord(doc, FALSE);
				break;
			case PL_StepRecMerge:
				if (BIMIDIPortIsBusy()) break;
				if (doc) PLStepRecord(doc, TRUE);
				break;
#endif
			case PL_AllNotesOff:
				if (useWhichMIDI == MIDIDR_OMS)
					OMSAllNotesOff2();
				else if (useWhichMIDI == MIDIDR_FMS)
					FMSAllNotesOff();
				else if (useWhichMIDI == MIDIDR_CM)
					CMAllNotesOff();
				else {
					if (BIMIDIPortIsBusy()) break;
					AllNotesOff();
				}
				break;
			case PL_MIDISetup:
				MEHideCaret(doc);
//				oldMIDIThru = config.midiThru;
				if (doc) MIDIDialog(doc);
				if (useWhichMIDI==MIDIDR_FMS)
					FMSSetInputDestinationMatch(doc);
#ifdef NOMORE
				if (useWhichMIDI==MIDIDR_BI) {
					if (config.midiThru!=oldMIDIThru) {
						/*
						 * I don't think this code can ever be executed: as of v.3.0, there's
						 *	no control of MIDI Thru in the MIDI dialog.
						 */
						if (config.midiThru==1) {
							MayInitBIMIDI(BIMIDI_SMALLBUFSIZE, BIMIDI_SMALLBUFSIZE);	/* be sure low-level stuff is initialized */
							MidiThru(1);
						}
						else {
							MidiThru(0);
							MayResetBIMIDI(FALSE);
						}
					}
				}
#endif
				break;
#ifndef VIEWER_VERSION
			case PL_Metronome:
				if (useWhichMIDI==MIDIDR_OMS)
					OMSMetroDialog(&config.metroViaMIDI, &config.metroChannel, &config.metroNote,
									&config.metroVelo, &config.metroDur, &config.metroDevice);
				else if (useWhichMIDI==MIDIDR_CM)
					CMMetroDialog(&config.metroViaMIDI, &config.metroChannel, &config.metroNote,
									&config.metroVelo, &config.metroDur, &config.cmMetroDevice);
				else if (useWhichMIDI==MIDIDR_FMS)
					FMSMetroDialog(&config.metroViaMIDI, &config.metroChannel, &config.metroNote,
									&config.metroVelo, &config.metroDur, &config.metroDevice);
				else
					MetroDialog(&config.metroViaMIDI, &config.metroChannel, &config.metroNote,
									&config.metroVelo, &config.metroDur);
				break;
#endif
			case PL_MIDIThru:
				if (useWhichMIDI==MIDIDR_FMS) {
					if (MIDIThruDialog())
						FMSSetMIDIThruDeviceFromConfig();
				}
				break;
			case PL_MIDIDynPrefs:
				if (doc) PLMIDIDynPrefs(doc);
				break;
			case PL_MIDIModPrefs:
				if (doc) PLMIDIModPrefs(doc);
				break;
#ifdef ENABLE_BIMIDI
			case PL_MIDIDriverSetup:
				if (useWhichMIDI!=MIDIDR_BI) {
					GetIndCString(strBuf, MENUCMDMSGS_STRS, 6);	/* "You can't set up the MIDI Driver because you aren't using built-in MIDI." */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					break;
				}
				newPortSetting = portSettingBIMIDI;
				newInterfaceSpeed = interfaceSpeedBIMIDI;
				if (MIDIDriverDialog(&newPortSetting, &newInterfaceSpeed)) {
					portSettingBIMIDI = newPortSetting;
					interfaceSpeedBIMIDI = newInterfaceSpeed;
				}
				break;
#endif
			case PL_PartMIDI:
				if (doc) EditPartMIDI(doc);
				break;
			case PL_Transpose:
				if (doc) PLTransposeScore(doc);
				break;

			default:
				break;
			}
#ifndef TARGET_API_MAC_CARBON
		UnloadSeg(PlayEntire);			/* MIDIGeneral.c segment should contain all of the above */
#endif
	}

#ifdef VIEWER_VERSION

static void DoMasterPgMenu(INT16 choice)
	{		
		SysBeep(1);
	}

static void DoFormatMenu(INT16 choice)
	{		
		SysBeep(1);
	}

#else

void MPInstrument(Document *doc)
	{
		LINK staffL, aStaffL; INT16 partStaffn;
		
		staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	
		aStaffL = FirstSubLINK(staffL);
		for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
			if (StaffSEL(aStaffL)) {
				partStaffn = StaffSTAFF(aStaffL);
				break;
				}

		if (SDInstrDialog(doc,partStaffn)) {
			doc->masterChanged = TRUE;
			InvalWindow(doc);
		}
	}
	
/*
void MPCombineParts(Document *doc)
	{
		LINK firstPartL, lastPartL;
		
		if (!GetPartSelRange(doc, &firstPartL, &lastPartL)) return;
		
		if (MPCombineParts(doc, firstPartL, lastPartL)) {
			doc->masterChanged = TRUE;
			InvalWindow(doc);
		}
		

	}
*/

/*
 *	Handle a choice from the MasterPage Menu
 */

static void DoMasterPgMenu(INT16 choice)
	{		
		Document *doc=GetDocumentFromWindow(TopDocument);
		if (doc==NULL) return;
		
		switch(choice) {
			case MP_AddPart:
				MPAddPart(doc);
				break;
			case MP_DeletePart:
				MPDeletePart(doc);
				break;
			case MP_GroupParts:
				DoGroupParts(doc);
				break;
			case MP_UngroupParts:
				DoUngroupParts(doc);
				break;
			case MP_SplitPart:
				DoMake1StaffParts(doc);
				break;
			case MP_DistributeStaves:
				MPDistributeStaves(doc);
				break;
			case MP_StaffSize:
				DoMasterStfSize(doc);
				break;
			case MP_StaffLines:
				DoMasterStfLines(doc);
				break;
			case MP_EditMargins:
				MPEditMargins(doc);
				break;
			case MP_Instrument:
				MPInstrument(doc);
				break;
//			case MP_CombineParts:
//				MPCombineParts(doc);
			default:
				break;
			}
	}

/*
 *	Handle a choice from the Show Format Menu
 */

static void DoFormatMenu(INT16 choice)
	{		
		Document *doc=GetDocumentFromWindow(TopDocument);
		if (doc==NULL) return;
		
#ifndef LIGHT_VERSION
		switch(choice) {
			case FT_Invisify:
				SFInvisify(doc);
				break;
			case FT_ShowInvis:
				SFVisify(doc);
				break;
			default:
				break;
			}
#endif
	}

#endif

/*
 *	Handle a choice from the Reduce/Enlarge To (Magnify) Menu
 */

static void DoMagnifyMenu(INT16 choice)
	{		
		/*
		 * Assume the menu simply contains a list of all possible <doc->magnify>
		 * values, starting with the smallest (MIN_MAGNIFY).
		 */
		Document *doc=GetDocumentFromWindow(TopDocument);
		INT16 newMagnify, magnifyDiff;

		if (doc) {
			newMagnify = MIN_MAGNIFY+(choice-1);
			magnifyDiff = newMagnify-(doc->magnify);
			SheetMagnify(doc, magnifyDiff);
			}
	}


static void MovePalette(WindowPtr whichPalette, Point position)
	{
		PaletteGlobals *pg; GrafPtr oldPort; Rect portRect;
		
		if (GetWRefCon(whichPalette) == TOOL_PALETTE) {
			ShowHide(whichPalette,FALSE);
			MoveWindow(whichPalette, position.h, position.v, FALSE);
			pg = *paletteGlobals[TOOL_PALETTE];
			ChangeToolSize(pg->firstAcross,pg->firstDown,TRUE);
			}
		 else {
			MoveWindow(whichPalette, position.h, position.v, FALSE);
			GetPort(&oldPort); SetPort(GetWindowPort(whichPalette));
			GetWindowPortBounds(whichPalette,&portRect);
			InvalWindowRect(whichPalette,&portRect);
			SetPort(oldPort);
			}
		
		if (TopWindow!=NULL && GetWindowKind(TopWindow)<OURKIND)
			/* The TopWindow is a non-application document. */
			if (IsWindowVisible(whichPalette))
				/* The palette is visible. Bring it to the front and generate an activate event. */
				SelectWindow(whichPalette);
			 else {
				BringToFront(whichPalette);
				/* Make the palette visible and generate an activate event. */
				ShowWindow(whichPalette);
				}
		 else {
			/* The TopWindow is an application window or it is equal to NULL */
			BringToFront(whichPalette);
			if (TopWindow == NULL)
				/* Make the palette visible and generate an activate event. */
				ShowWindow(whichPalette);
			 else {
				/* Make the palette visible but don't generate an activate event. */
				ShowHide(whichPalette, TRUE);
				HiliteWindow(whichPalette, TRUE);
				}
			}
	}

#ifndef VIEWER_VERSION

/*
 *	Get user preferences from dialog.
 */

void FMPreferences(Document *doc)
	{
		static INT16 section=1;			/* Initially show General preferences */
		
		PrefsDialog(doc, doc->used, &section);
#ifndef TARGET_API_MAC_CARBON
		UnloadSeg(PrefsDialog);
#endif
		FillSpaceMap(doc, doc->spaceTable);
	}


/*
 * Change indents and showing/not showing part names at left end of systems.
 */
 
void SMLeftEnd(Document *doc)
{
	INT16	changeFirstIndent, changeOtherIndent;
	INT16	firstNames, firstDist, otherNames, otherDist;
	
	firstNames = doc->firstNames;
	firstDist = d2pt(doc->firstIndent);
	otherNames = doc->otherNames;
	otherDist = d2pt(doc->otherIndent);
	if (LeftEndDialog(&firstNames, &firstDist, &otherNames, &otherDist)) {
		if (firstNames!=doc->firstNames) {
			doc->firstNames = firstNames;
			doc->changed = TRUE;
		}
		changeFirstIndent = pt2d(firstDist)-doc->firstIndent;
		if (changeFirstIndent!=0) {
			doc->firstIndent = pt2d(firstDist);
			IndentSystems(doc, changeFirstIndent, TRUE);
		}

		if (otherNames!=doc->otherNames) {
			doc->otherNames = otherNames;
			doc->changed = TRUE;
		}
		changeOtherIndent = pt2d(otherDist)-doc->otherIndent;
		if (changeOtherIndent!=0) {
			doc->otherIndent = pt2d(otherDist);
			IndentSystems(doc, changeOtherIndent, FALSE);
		}

		if (changeFirstIndent || changeOtherIndent) InvalWindow(doc);
	}
}

/*
 *	Set global font characteristics.
 */
 
static void SMTextStyle(Document *doc)
	{
		static Boolean firstCall=TRUE;
		static Str255 string;
		Boolean okay;
		CONTEXT context; LINK firstMeas;

		firstMeas = LSSearch(doc->headL, MEASUREtype, ANYONE, FALSE, FALSE);
		GetContext(doc, firstMeas, 1, &context);

		if (firstCall) {
			GetIndString(tmpStr,MiscStringsID,6);	/* Get default sample string */
			Pstrcpy(string,tmpStr);
			firstCall = FALSE;
		}
		okay = DefineStyleDialog(doc, string, &context);
	}


/*
 *	Get measure-numbering parameters for <doc> from user.
 */
	
static void SMMeasNum(Document *doc)
	{
		if (MeasNumDialog(doc)) {
			EraseAndInvalMessageBox(doc);
			InvalWindow(doc);
		}
	}

/*
 * Respace selected measures to tightness set by user from dialog.
 */

static void SMRespace(Document *doc)
	{
		INT16 spMin, spMax, dval;

		GetMSpaceRange(doc, doc->selStartL, doc->selEndL, &spMin, &spMax);
		dval = SpaceDialog(RESPACE_DLOG, spMin, spMax);
		if (dval>0) {				
			WaitCursor();
			InitAntikink(doc, doc->selStartL, doc->selEndL);
			if (RespaceBars(doc, doc->selStartL, doc->selEndL, 
									RESFACTOR*(long)dval, TRUE, FALSE))
				doc->spacePercent = dval;
				
			/* NB: It would be much better not to Antikink if RespaceBars failed, but we
				have no other way to free its data structures! This should be fixed. */
			Antikink();
		}
	}
	
/*
 * For each object in the selection range with subobjects, align all of its
 *	subobjects with its first selected subobject.
 */

static void SMRealign(Document *doc)
	{
		LINK pL; Boolean didSomething=FALSE;

		for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
			if (RealignObj(pL, TRUE)) didSomething = TRUE;
			
		if (didSomething) doc->changed = TRUE;
		InvalSelRange(doc);
	}

/*
 * Reformat first System of selection thru last System of selection in accordance
 * with parameters set by user in dialog.
 */

static void SMReformat(Document *doc)
	{
		static Boolean firstCall=TRUE, changeSBreaks, careMeasPerSys, exactMPS,
							careSysPerPage, justify;
		INT16 useMeasPerSys;
		static INT16 measPerSys, sysPerPage, titleMargin;
		INT16 endSysNum, status;
		LINK startSysL, endSysL;
		
		if (firstCall) {
			changeSBreaks = TRUE;
			careMeasPerSys = exactMPS = FALSE;
			measPerSys = 4;
			
			careSysPerPage = FALSE;
			sysPerPage = 6;
			justify = FALSE;
			titleMargin = config.titleMargin;
			
			firstCall = FALSE;
		}

		GetSysRange(doc, doc->selStartL, doc->selEndL, &startSysL, &endSysL);
		endSysNum = SystemNUM(endSysL);
		if (SystemNUM(startSysL)==1 && endSysNum==doc->nsystems) endSysNum = -1;
		if (ReformatDialog(SystemNUM(startSysL),endSysNum,&changeSBreaks,&careMeasPerSys,
								&exactMPS,&measPerSys,&justify,&careSysPerPage,&sysPerPage,
								&titleMargin)) {
			PrepareUndo(doc, doc->selStartL, U_Reformat, 20);    		/* "Reformat" */
			if (careMeasPerSys) {
				useMeasPerSys = measPerSys;
				if (exactMPS) useMeasPerSys = -useMeasPerSys;
			}
			else
				useMeasPerSys = 9999;
			
			status = Reformat(doc, doc->selStartL, doc->selEndL, changeSBreaks,
							useMeasPerSys, justify, (careSysPerPage? sysPerPage : 999),
							titleMargin);
			if (status==NOTHING_TO_DO) {
				GetIndCString(strBuf, MENUCMDMSGS_STRS, 2);			/* "The selected systems are already formatted as you specified." */
				CParamText(strBuf, "", "", "");
				NoteInform(GENERIC_ALRT);
			}
#ifdef LIGHT_VERSION
			EnforcePageLimit(doc);
#endif
		}
	}


/* Invoke Add Modifiers dialog to add modifiers to all selected notes. (JGG, 4/21/01) */
	
static void NMAddModifiers(Document *doc)
	{
		LINK	pL, aNoteL, mainNoteL, aModNRL;
		INT16	voice;
		char	data = 0;
		static Byte	modCode = MOD_STACCATO;
		
		if (AddModNRDialog(&modCode)) {
			WaitCursor();
			PrepareUndo(doc, doc->selStartL, U_AddMods, 49);    		/* "Add Modifiers" */
			for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
				if (SyncTYPE(pL))
					for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL)) {
							if (NoteREST(aNoteL) && modCode!=MOD_FERMATA)
								continue;
							/* If not the main note, and the main note is selected, skip it. */
							voice = NoteVOICE(aNoteL);
							mainNoteL = FindMainNote(pL, voice);
							if (aNoteL!=mainNoteL && NoteSEL(mainNoteL))
								continue;
							aModNRL = FIInsertModNR(doc, modCode, data, pL, aNoteL);
							if (aModNRL==NILINK) {
								MayErrMsg("NMAddModifiers: FIInsertModNR failed.");
								goto stop;
							}
							doc->changed = TRUE;
						}
stop:
			InvalSelRange(doc);
			ArrowCursor();
		}
	}


/*
 * Strip all modifiers from all selected notes.
 */
	
static void NMStripModifiers(Document *doc)
	{
		register LINK pL,aNoteL; PANOTE aNote;
		
		PrepareUndo(doc, doc->selStartL, U_StripMods, 21);    	/* "Remove Modifiers" */
		for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
			if (SyncTYPE(pL))
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->selected && aNote->firstMod) {
						DelModsForSync(pL, aNoteL);
						aNote->firstMod = NILINK;
						doc->changed = TRUE;
					}
				}
		InvalSelRange(doc);
	}


static INT16 CountSelVoices(Document *doc)
	{
		Boolean voiceUsed[MAXVOICES+1];
		INT16 v, nSelVoices;
		LINK pL, aNoteL, aGRNoteL;
		
		for (v = 1; v<=MAXVOICES; v++)
			voiceUsed[v] = FALSE;

		for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
			switch (ObjLType(pL)) {
				case SYNCtype:
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
						if (NoteSEL(aNoteL))
							voiceUsed[NoteVOICE(aNoteL)] = TRUE;
					}
					break;
				case GRSYNCtype:
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
						if (GRNoteSEL(aGRNoteL))
							voiceUsed[GRNoteVOICE(aGRNoteL)] = TRUE;
					}
					break;
				default:
					;
			}
			
		for (nSelVoices = 0, v = 1; v<=MAXVOICES; v++)
			if (voiceUsed[v]) nSelVoices++;
		return nSelVoices;
	}
	
/*
 * Apply multivoice notation rules as specified by user in dialog.
 */
	
static void NMMultiVoice(Document *doc)
	{
		static INT16 nSelVoices, voiceRole; static Boolean measMulti, assume;

		nSelVoices = CountSelVoices(doc);
		if (MultivoiceDialog(nSelVoices, &voiceRole, &measMulti, &assume))
			DoMultivoiceRules(doc, voiceRole, measMulti, assume);
	}
	

/*
 * Set logical and/or physical duration of selected notes/rests as specified by user.
 */
	
static void NMSetDuration(Document *doc)
	{
		static INT16 lDur=QTR_L_DUR, nDots=0, pDurPct=-1, lDurAction=SET_DURS_TO;
		static Boolean setLDur=TRUE, setPDur=TRUE, cptV=TRUE;
		Boolean unbeam, didAnything=FALSE;
		
		if (pDurPct<0) pDurPct = config.legatoPct;
		
		if (SetDurDialog(doc,&setLDur,&lDurAction,&lDur,&nDots,&setPDur,&pDurPct,&cptV,&unbeam)) {
			if (unbeam) Unbeam(doc);
			PrepareUndo(doc, doc->selStartL, U_SetDuration, 22);    	/* "Set Duration" */

			if (setLDur) {
				switch (lDurAction) {
					case HALVE_DURS:
						didAnything = SetDoubleHalveSelNoteDur(doc, FALSE, FALSE);
						break;
					case DOUBLE_DURS:
						didAnything = SetDoubleHalveSelNoteDur(doc, TRUE, FALSE);
						break;
					case SET_DURS_TO:
						didAnything = SetSelNoteDur(doc, lDur, nDots, FALSE);
						break;
					default:
						break;
				}
				if (didAnything) {
					if (cptV) SetDurCptV(doc);		/* ??Bug: this changes sel range, screwing up FixTimeStamps! */
					FixTimeStamps(doc, doc->selStartL, doc->selEndL);
					if (doc->autoRespace)
						RespaceBars(doc, doc->selStartL, doc->selEndL,
									RESFACTOR*(long)doc->spacePercent, FALSE, FALSE);
					else
						InvalMeasures(doc->selStartL, LeftLINK(doc->selEndL), ANYONE);	/* Force redrawing */
				}
			}
			if (setPDur)
				SetPDur(doc, pDurPct, TRUE);
		}
	}

	
/*
 * Choose whether notes should be inserted graphically or temporally.
 */

static void NMInsertByPos(Document *doc)
	{
		doc->insertMode = !doc->insertMode;
	}


/*
 * Set multibar rests to the number of measures specified by user.
 */
	
static void NMSetMBRest(Document *doc)
	{
		static INT16 newNMeas=4;

		if (SetMBRestDialog(doc, &newNMeas)) {
			PrepareUndo(doc, doc->selStartL, U_SetDuration, 23);    	/* "Set Multibar Rest" */
			if (SetSelMBRest(doc, newNMeas)) {
				FixTimeStamps(doc, doc->selStartL, doc->selEndL);
				UpdateMeasNums(doc, NILINK);
				if (doc->autoRespace)
					RespaceBars(doc, doc->selStartL, doc->selEndL,
								RESFACTOR*(long)doc->spacePercent, FALSE, FALSE);
				else
					InvalMeasures(doc->selStartL, LeftLINK(doc->selEndL), ANYONE);	/* Force redrawing */
			}
		}
	}


/*
 * Fill empty measures in user-specified range with whole-measure rests.
 */
	
static void NMFillEmptyMeas(Document *doc)
	{
		INT16 startMN, endMN; LINK measL, startL, endL; PAMEASURE aMeasure;
		
		/* Default start and end measures are the first and last of the score. */
		
		startMN = doc->firstMNNumber;
		measL = MNSearch(doc, doc->tailL, ANYONE, GO_LEFT, TRUE);
		aMeasure = GetPAMEASURE(FirstSubLINK(measL));
		endMN = aMeasure->measureNum+doc->firstMNNumber;

		if (FillEmptyDialog(doc, &startMN, &endMN)) {
			startL = MNSearch(doc, doc->headL, startMN-doc->firstMNNumber, GO_RIGHT, TRUE);
			endL = MNSearch(doc, doc->headL, endMN-doc->firstMNNumber, GO_RIGHT, TRUE);
			if (startL && endL) {
				/* Not sure fiddling with selection range is a good idea, but what else can we do?  -JGG */
				LINK saveSelStartL, saveSelEndL;
				saveSelStartL = doc->selStartL; saveSelEndL = doc->selEndL;
				doc->selStartL = startL; doc->selEndL = endL;
				PrepareUndo(doc, doc->selStartL, U_FillEmptyMeas, 24);	/* "Fill Empty Measures" */
				doc->selStartL = saveSelStartL; doc->selEndL = saveSelEndL;

				WaitCursor();
				if (FillEmptyMeas(doc, startL, endL)) {
					FixTimeStamps(doc, startL, endL);
					UpdateMeasNums(doc, NILINK);
				}
			}
		}
	}

#endif

/*
 * Get voice to look at from dialog, initialized with voice and part of first selected
 * object or, if nothing is selected, default voice and part of the insertion point's
 * staff. In either case, add the user-specified voice to the voice mapping table and
 * set document's <lookVoice> to it.
 */
 
static void VMLookAt()
	{
		INT16			dval, newLookV, userVoice, saveStaff;
		Document		*doc=GetDocumentFromWindow(TopDocument);
		LINK			partL;
		PPARTINFO 	pPart;

		if (doc==NULL) return;
		
		if (doc->selStartL!=doc->selEndL) {
			newLookV = GetVoiceFromSel(doc);
			if (!Int2UserVoice(doc, newLookV, &userVoice, &partL))
				MayErrMsg("VMLookAt: Int2UserVoice(%ld) failed.",
							(long)newLookV);
			pPart = GetPPARTINFO(partL);
		}
		else {
			partL = FindPartInfo(doc, Staff2Part(doc, doc->selStaff));
			pPart = GetPPARTINFO(partL);
			userVoice = doc->selStaff-pPart->firstStaff+1;
		}
		
		dval = LookAtDialog(doc, userVoice, partL);
		if (dval>0) {
			saveStaff = doc->selStaff;
			DeselAll(doc);									/* Deselect all BEFORE changing lookVoice! */
			newLookV = User2IntVoice(doc, dval, partL);
			if (!newLookV) MayErrMsg("VMLookAt: voice table full.");
			doc->lookVoice = newLookV;
			doc->selStaff = saveStaff;
			InvalWindow(doc);
			EraseAndInvalMessageBox(doc);
		}
	}

	
/*
 *	Look at all voices by setting document's <lookVoice> to -1.
 */

static void VMLookAtAll()
	{
		Document *doc=GetDocumentFromWindow(TopDocument);

		if (doc) {
			doc->lookVoice = -1;								/* Negative no. means all voices */
			InvalWindow(doc);
			EraseAndInvalMessageBox(doc);
			}
	}

#ifndef VIEWER_VERSION

/*
 *	Toggle showing/hiding rhythm problems.
 */

static void VMShowDurProblems()
	{
		Document *doc=GetDocumentFromWindow(TopDocument);

		if (doc) {
			doc->showDurProb = !doc->showDurProb;
			CheckMenuItem(viewMenu, VM_ShowDurProb, doc->showDurProb);
			InvalWindow(doc);
			}
	}

/*
 *	Toggle showing/hiding sync lines.
 */

static void VMShowSyncs()
	{
		Document *doc=GetDocumentFromWindow(TopDocument);

		if (doc) {
			doc->showSyncs = !doc->showSyncs;
			CheckMenuItem(viewMenu, VM_ShowSyncL, doc->showSyncs);
			InvalWindow(doc);
			}
	}
	
/*
 *	Toggle showing/hiding invisible symbols.
 */

static void VMShowInvisibles()
	{
		Document *doc=GetDocumentFromWindow(TopDocument);

		if (doc) {
			doc->showInvis = !doc->showInvis;
			CheckMenuItem(viewMenu, VM_ShowInvis, doc->showInvis);
			InvalWindow(doc);
			}
	}
	
#endif /* VIEWER_VERSION */

/*
 *	Toggle coloring of voices: non-default voices in color or everything in black.
 */

static void VMColorVoices()
	{
		Document *doc=GetDocumentFromWindow(TopDocument);

		/*
		 * Toggle with care: I'm not sure ISO C guarantees the "!" operator with
		 * multibit fields like <colorVoices>.
		 */
		if (doc) {
			if (doc->colorVoices==0)
				doc->colorVoices = ((CapsLockKeyDown() && ShiftKeyDown())? 1 : 2);
			else
				doc->colorVoices = 0;
			CheckMenuItem(viewMenu, VM_ColorVoices, doc->colorVoices);
			InvalWindow(doc);
			}
	}

#ifndef VIEWER_VERSION

/*
 * Toggle showing score in pianoroll or normal form.
 */
 
static void VMPianoRoll()
	{
		Document *doc=GetDocumentFromWindow(TopDocument);

		if (doc) {
			doc->pianoroll = !doc->pianoroll;
			CheckMenuItem(viewMenu, VM_PianoRoll, doc->pianoroll);
			InvalWindow(doc);
			}
	}

	
#endif /* VIEWER_VERSION */

#define SPEC_DOC_COUNT 1	/* No. of special Documents (e.g., clipboard) at start of <documentTable> */

static void VMActivate(INT16 choice)
	{
	INT16 itemNum; Document *doc;
	
		/* The items at the end of the View menu (after the dividing line after
		 *	VM_LastItem) correspond to Documents in documentTable. Search for the
		 * Document we want, starting after any special Documents, which do not
		 * appear in the list.
		 */
		for (itemNum = VM_LastItem+2, doc = documentTable+SPEC_DOC_COUNT; doc<topTable; doc++)
			if (doc->inUse) {
				if (itemNum==choice) { SelectWindow(doc->theWindow); break; }
				else						itemNum++;
				}
	}

#ifndef VIEWER_VERSION

static Boolean OKToRecord(Document *);
static Boolean OKToRecord(Document *doc)
{
	INT16 anInt; LINK lookPartL, selPartL; char str[256];
	
	if (HasSmthgAcross(doc, doc->selStartL, str)) {
		CParamText(str, "", "", "");
		CautionAlert(EXTOBJ_ALRT, NULL);
		return FALSE;
	}
	
	if (doc->lookVoice>=0) {
		Int2UserVoice(doc, doc->lookVoice, &anInt, &lookPartL);
		Int2UserVoice(doc, doc->selStaff, &anInt, &selPartL);		/* since staff no.=default voice no. */
		if (lookPartL!=selPartL) {
			GetIndCString(strBuf, MENUCMDMSGS_STRS, 3);				/* "You can't Record onto a staff in one part while Looking..." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}
	
	return TRUE;
}


/*
 * Record MIDI data in real time.
 */

static void PLRecord(Document *doc, Boolean merge)
	{
		if (!OKToRecord(doc)) return;

		DebugPrintf("0. Starting to record\n");
		
		if (merge) {
			PrepareUndo(doc, doc->selStartL, U_RecordMerge, 25);    	/* "Record Merge" */
			RecTransMerge(doc);
		}
		else {
			PrepareUndo(doc, doc->selStartL, U_Record, 26);    		/* "Record" */
			
			DebugPrintf("0.1. Ready to record\n");
			Record(doc);
		}
	}


/*
 * Step-record MIDI data.
 */

static void PLStepRecord(Document *doc, Boolean merge)
	{
		INT16 spaceProp;
		
		if (!OKToRecord(doc)) return;

		PrepareUndo(doc, doc->selStartL, U_Record, 27);    			/* "Step Record" */
		if (StepRecord(doc, merge)) {
			spaceProp = MeasSpaceProp(doc->selStartL);
			/*
			 * Reformatting (even if done by RespaceBars calling Reformat) may destroy
			 * selection status, so we use notes' tempFlags to save and restore it.
			 */
			NotesSel2TempFlags(doc);
			if (merge) {
				if (RespaceBars(doc, doc->selStartL, doc->selEndL, spaceProp, FALSE, TRUE))
					TempFlags2NotesSel(doc);
			}
			else {
				if (RespAndRfmtRaw(doc, doc->selStartL, doc->selEndL, spaceProp))
					TempFlags2NotesSel(doc);
			}
		}
	}

#endif

/*
 *	When activating a document, ensure that the correct menu is installed for
 *	the current mode of viewing that document: Play/Record menu, Master Page
 *	menu, or Show Format menu.
 */

void InstallDocMenus(Document *doc)
	{
		INT16 mid; MenuHandle oldMh, mh;
	
		if (oldMh = GetMenuHandle(playRecID))
			DeleteMenu(playRecID);
		 else if (oldMh = GetMenuHandle(masterPgID))
			DeleteMenu(masterPgID);
		 else if (oldMh = GetMenuHandle(formatID))
			DeleteMenu(formatID);
	
		/*
		 *	GetMenuHandle may give NULL for the new menu (if that menu was never installed?
		 *	I dunno), so instead pass one of our stored menu handles to InsertMenu.
		 */
#if 1
		if (doc->masterView)		  { mid = masterPgID; mh = masterPgMenu; }
		else if (doc->showFormat) { mid = formatID; mh = formatMenu; }
		else							  { mid = playRecID; mh = playRecMenu; }
		
#ifdef LIGHT_VERSION
		if (!doc->showFormat)
			InsertMenu(mh,0);
#else
		InsertMenu(mh,0);
#endif
		DrawMenuBar();

		if (oldMh) {
			/* Flash to call user's attention to the fact that there's a new menu. */
			
			HiliteMenu(mid);
			SleepTicks(1);
			HiliteMenu(0);
		}
#else
		if (doc->masterView)
			InsertMenu(masterPgMenu,0);
		 else if (doc->showFormat)
			InsertMenu(formatMenu,0);
		 else InsertMenu(playRecMenu,0);
	
		DrawMenuBar();
#endif
	}
	
static void InstallDebugMenuItems(Boolean installAll) 
	{
		printf("installing debug menu items..\n");
		
		if (debugItemsNeedInstall)
		{
			printf("installing .. \n");
			AppendMenu(editMenu, "\p(-");
			AppendMenu(editMenu, "\pBrowser");
			if (installAll) {
				AppendMenu(editMenu, "\pDebug...");
//				AppendMenu(editMenu, "\pShow Debug Window");
				AppendMenu(editMenu, "\pDelete Objects");
			}
		}
		else {
			printf("removing .. \n");
			DeleteMenuItem(editMenu, EM_DeleteObjs);			
			DeleteMenuItem(editMenu, EM_Debug);
			DeleteMenuItem(editMenu, EM_Browser);
			DeleteMenuItem(editMenu, EM_Browser-1);
		}
		
		debugItemsNeedInstall = !debugItemsNeedInstall;
	}


/* ============================ MENU-UPDATING ROUTINES ============================ */

/*
 *	Force all menus to reflect internal state, since we're about to let user
 *	peruse them.
 */

void FixMenus()
	{
		Boolean isDA; WindowPtr w;
		register Document *theDoc;
		INT16 nInRange=0,nSel=0;
		LINK firstMeasL;
		Boolean continSel=FALSE;
		
		w = TopWindow;
		if (w == NULL) {
			theDoc = NULL;
			isDA = FALSE;
			}
		 else {
#ifdef TARGET_API_MAC_CARBON
			isDA = FALSE;
#else
			isDA = ((WindowPeek)w)->windowKind < 0;
#endif
			theDoc = GetDocumentFromWindow(TopDocument);
			}
		
		/*
		 *	Precompute information about the current score and the clipboard for the
		 *	benefit of the various menu fixing routines about to be called.
		 *	We have to install theDoc first because, if the active window has
		 *	changed, the resulting Activate event (which will install it) may
		 *	not have been handled yet.
		 */
		
		if (theDoc) {
			if (theDoc!=clipboard && theDoc != gResListDocument) {
				InstallDoc(theDoc);
				CountSelection(theDoc, &nInRange, &nSel);
				beforeFirst = BeforeFirstMeas(theDoc->selStartL);

				continSel = ContinSelection(theDoc, config.strictContin!=0);
				theDoc->canCutCopy = continSel;
				}
			 else {							/* Clipboard is top document */
			 	nInRange = nSel = 0;
			 	beforeFirst = FALSE;
			 }
			InstallDoc(clipboard);
			firstMeasL = SSearch(clipboard->headL, MEASUREtype, FALSE);
			clipboard->canCutCopy = RightLINK(firstMeasL)!=clipboard->tailL;
			InstallDoc(theDoc);
			}
		
		FixFileMenu(theDoc, nSel);
		FixEditMenu(theDoc, nInRange, nSel, isDA);
		FixTestMenu(theDoc, nSel);
		FixScoreMenu(theDoc, nSel);
		FixNotesMenu(theDoc, continSel);
		FixGroupsMenu(theDoc, continSel);
		FixViewMenu(theDoc);
		FixPlayRecordMenu(theDoc,nSel);
		FixMasterPgMenu(theDoc);
		FixFormatMenu(theDoc);
		
		UpdateMenuBar();		/* Will do anything only if UpdateMenu was called by above */
	}

/* Enable or disable all items in the File menu */

static void FixFileMenu(Document *doc, INT16 nSel)
	{
		//always disable Finale ETF import until it has been removed from menu (rsrc) entirely
		XableItem(fileMenu,FM_GetETF,FALSE);        
        
		XableItem(fileMenu,FM_Close,doc!=NULL);
		XableItem(fileMenu,FM_CloseAll,doc!=NULL);

#ifndef VIEWER_VERSION
		XableItem(fileMenu,FM_Save,doc!=NULL && !doc->readOnly && doc->changed
										&& doc!=clipboard && !doc->masterView);
		XableItem(fileMenu,FM_SaveAs,doc!=NULL && doc!=clipboard && !doc->masterView);
		XableItem(fileMenu,FM_Extract,doc!=NULL && doc!=clipboard);
		INT16 numOpenDocs = NumOpenDocuments();
		//XableItem(fileMenu,FM_Import,numOpenDocs == 0);

		// always disable NoteScan import until it has been removed from menu (rsrc) entirely
		XableItem(fileMenu,FM_GetScan,FALSE);
		XableItem(fileMenu,FM_SaveEPSF,doc!=NULL && doc!=clipboard
										&& !doc->masterView && !doc->showFormat);
		XableItem(fileMenu,FM_SaveText,doc!=NULL && doc!=clipboard
										&& !doc->masterView && !doc->showFormat && nSel>0);
		XableItem(fileMenu,FM_Revert,doc!=NULL && doc->changed && !doc->readOnly &&
										doc!=clipboard && doc->named &&
										!doc->converted);
		XableItem(fileMenu,FM_Export,doc!=NULL && doc!=clipboard && !doc->masterView);
		XableItem(fileMenu,FM_OpenMidiMap,doc!=NULL && doc!=clipboard);
#endif
		XableItem(fileMenu,FM_SheetSetup,doc!=NULL && doc!=clipboard && !doc->overview);
#ifndef VIEWER_VERSION
		XableItem(fileMenu,FM_PageSetup,doc!=NULL && doc!=clipboard);
		XableItem(fileMenu,FM_Print,doc!=NULL && doc!=clipboard && !doc->showFormat);

		XableItem(fileMenu,FM_Preferences,doc!=NULL && doc!=clipboard);
		XableItem(fileMenu,FM_ScoreInfo,doc!=NULL);
#endif
	}


short CountSelPages(Document *);
short CountSelPages(Document *doc)
	{
		Boolean selAcrossFirst; INT16 nPages; LINK pL;
		
		if (!doc) return 0;
		
		/*
		 * Count one Page that's not in the selection range for the Page the selection
		 * starts on, unless the selection starts on or before the first Page and ends
		 *	after the first Page of the score.
		 */
		selAcrossFirst = FALSE;	
		for (pL = doc->headL; pL; pL = RightLINK(pL)) {
			/* selStartL is in the selection, so test it before considering breaking */
			if (pL==doc->selStartL) selAcrossFirst = TRUE;
			if (PageTYPE(pL)) break;
			if (pL==doc->selEndL) selAcrossFirst = FALSE;
		}

		nPages = (selAcrossFirst? 0 : 1);
		for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
			if (PageTYPE(pL))
				nPages++;
		
		return nPages;
	}


static void GetUndoString(Document *doc, char undoMenuItem[]);
static void GetUndoString(Document *doc, char undoMenuItem[])
	{
		char fmtStr[256];
		
		if (doc->undo.redo)
			GetIndCString(fmtStr, UNDOWORD_STRS, 2);				/* "Redo %s" */
		else
			GetIndCString(fmtStr, UNDOWORD_STRS, 1);				/* "Undo %s" */
		sprintf(undoMenuItem, fmtStr, doc->undo.menuItem);
	}

#ifdef VIEWER_VERSION

static void FixEditMenu(Document *doc, INT16 nInRange, INT16 nSel, Boolean isDA)
	{
		XableItem(editMenu,EM_Undo,isDA);
		XableItem(editMenu,EM_Cut,isDA);
		XableItem(editMenu,EM_Copy,isDA);
		XableItem(editMenu,EM_Clear,isDA);
		XableItem(editMenu,EM_Paste,isDA);
	}
	
#else

/*
 *	Enable or disable all items in the Edit menu; disable entire menu if the front
 * window isn't a DA and we're looking at the Master Page or Showing Format.
 */

static void FixEditMenu(Document *doc, INT16 /*nInRange*/, INT16 nSel, Boolean isDA)
	{
		Boolean mergeable,enablePaste,enableClearSys,enableClearPage;
		char str[256], undoMenuItem[256], fmtStr[256];
		short nPages;

		UpdateMenu(editMenu, doc!=NULL && !doc->masterView && !doc->showFormat);

		/*
		 * Unlike most other menus, we can't just disable the whole menu if there's
		 * no score open, so the code below has to work even if doc==NULL!
		 */
		 
		/*
		 *	Handle clipboard commands if either doc==NULL, or it exists and is in the
		 * normal view (not Master Page or Work on Format).
		 */
		
		if (doc==NULL || (!doc->masterView && !doc->showFormat)) {
			XableItem(editMenu,EM_Undo,isDA || 
				(doc!=NULL && doc->undo.lastCommand!=U_NoOp && doc->undo.canUndo));
			if (doc) {
				GetUndoString(doc, undoMenuItem);
				SetMenuItemCText(editMenu, EM_Undo, undoMenuItem);
			}
	
			XableItem(editMenu,EM_Cut,isDA ||
				(doc!=NULL && doc!=clipboard && doc->canCutCopy && !beforeFirst));
			XableItem(editMenu,EM_Copy,isDA ||
				(doc!=NULL && doc!=clipboard && doc->canCutCopy && !beforeFirst));
			XableItem(editMenu,EM_Clear,isDA ||
				(doc!=NULL && doc!=clipboard && doc->canCutCopy
						&& BFSelClearable(doc, beforeFirst)));
					
			nPages = CountSelPages(doc);
			if (nPages>1) {
				GetIndCString(fmtStr, MENUCMD_STRS, 1);					/* "Copy %d Pages" */
				sprintf(str, fmtStr, nPages);
			}
			else
				GetIndCString(str, MENUCMD_STRS, 2);						/* "Copy Page" */
			SetMenuItemCText(editMenu, EM_CopyPage, str);
			if (nPages>1) {
				GetIndCString(fmtStr, MENUCMD_STRS, 21);					/* "Clear %d Pages" */
				sprintf(str, fmtStr, nPages);
			}
			else
				GetIndCString(str, MENUCMD_STRS, 22);						/* "Clear Page" */
			SetMenuItemCText(editMenu, EM_ClearPage, str);

			/* ??How can you "Paste Insert" anything into a DA? (CER asks?) */

			switch (lastCopy) {
				case COPYTYPE_CONTENT:
					GetIndCString(str, MENUCMD_STRS, 3);					/* "Paste Insert" */
					SetMenuItemCText(editMenu, EM_Paste, str);
					enablePaste = (isDA ||
						(doc!=NULL && doc!=clipboard && clipboard->canCutCopy && !beforeFirst));
					XableItem(editMenu,EM_Paste,enablePaste);
					break;
				case COPYTYPE_SYSTEM:
					GetIndCString(str, MENUCMD_STRS, 4);					/* "Paste System" */
					SetMenuItemCText(editMenu, EM_Paste, str);
					enablePaste = (doc!=NULL && doc!=clipboard && !beforeFirst);
					XableItem(editMenu,EM_Paste,enablePaste);
					break;
				case COPYTYPE_PAGE:
					nPages = clipboard->numSheets;
					if (nPages>1) {
						GetIndCString(fmtStr, MENUCMD_STRS, 5);			/* "Paste %d Pages" */
						sprintf(str, fmtStr, nPages);
					}
					else
						GetIndCString(str, MENUCMD_STRS, 6);				/* "Paste Page" */
					SetMenuItemCText(editMenu, EM_Paste, str);
					enablePaste = (doc!=NULL && doc!=clipboard && !beforeFirst);
					XableItem(editMenu,EM_Paste,enablePaste);
					break;
			}
													
			mergeable = (lastCopy==COPYTYPE_CONTENT);
			
			XableItem(editMenu,EM_Merge,(doc!=NULL && doc!=clipboard && mergeable &&
													clipboard->canCutCopy && !beforeFirst));

			XableItem(editMenu,EM_Double,
				(doc!=NULL && doc!=clipboard && !beforeFirst && nSel>0));

			enableClearSys = FALSE;
			
			if (doc) {
				LINK firstMeasL;

				firstMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
				if (IsAfter(doc->selStartL,firstMeasL))
					enableClearSys = FALSE;
				else {
					enableClearSys = (doc!=NULL && doc!=clipboard);
				}
			}

			XableItem(editMenu,EM_ClearSystem,enableClearSys);
			XableItem(editMenu,EM_CopySystem,doc!=NULL && doc!=clipboard);
			
			enableClearPage = FALSE;
			
			if (doc) {
				LINK firstMeasL;

				firstMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
				if (IsAfter(doc->selStartL,firstMeasL))
					enableClearPage = FALSE;
				else {
					enableClearPage = (doc!=NULL && doc!=clipboard);
				}
			}

			XableItem(editMenu,EM_ClearPage,enableClearPage);
			XableItem(editMenu,EM_CopyPage,doc!=NULL && doc!=clipboard);

			XableItem(editMenu, EM_SelAll,
				(!isDA) && (doc!=NULL && doc!=clipboard && (RightLINK(doc->headL)!=doc->tailL)));
			XableItem(editMenu, EM_ExtendSel,
				(!isDA) && (doc!=NULL && doc!=clipboard && nSel>0));

			XableItem(editMenu, EM_GetInfo, doc!=NULL && nSel==1);

			if (doc==NULL || nSel!=1)
				XableItem(editMenu, EM_ModifierInfo, FALSE);
			else
				XableItem(editMenu, EM_ModifierInfo, SyncTYPE(doc->selStartL));

			XableItem(editMenu, EM_Set, doc!=clipboard && IsSetEnabled(doc));
			
			XableItem(editMenu,EM_SearchMelody,searchPatDoc != NULL);
		}
	}

#endif

static void FixTestMenu(Document *doc, INT16 nSel)
	{
#ifndef PUBLIC_VERSION
		if (clickMode==ClickErase)
			SetItemMark(testMenu, TS_ClickErase, '¥');
		else
			SetItemMark(testMenu, TS_ClickErase, noMark);
		if (clickMode==ClickSelect)
			SetItemMark(testMenu, TS_ClickSelect, '¥');
		else
			SetItemMark(testMenu, TS_ClickSelect, noMark);
		if (clickMode==ClickFrame)
			SetItemMark(testMenu, TS_ClickFrame, '¥');
		else
			SetItemMark(testMenu, TS_ClickFrame, noMark);

		XableItem(testMenu, TS_DeleteObjs, doc!=NULL && nSel>0);
		XableItem(testMenu, TS_ResetMeasNumPos, doc!=NULL);
#endif
	}

static void DisableSMMove()
	{
		DisableMenuItem(scoreMenu, SM_MoveMeasUp);
		DisableMenuItem(scoreMenu, SM_MoveMeasDown);
		DisableMenuItem(scoreMenu, SM_MoveSystemUp);
		DisableMenuItem(scoreMenu, SM_MoveSystemDown);
	}

static void FixMoveMeasSys(Document *doc)
	{
		LINK pageL,sysL,startBarL,endBarL,nextBarL,measAfterSelStart;
		Boolean measSysEnd,moveMeasUp;

		if (doc->masterView) {
			DisableSMMove();
			return;
		}

		/*
		 *	Enable "Move Measures Up" iff selStartL and LeftLINK(selEndL) are in the same
		 *	System; that System is not the first; selStartL is in the first Measure of that
		 *	System; and there's at least one Measure in the System after selStartL.
		 *
		 *	Enable "Move Measures Down" iff selStartL and LeftLINK(selEndL) are in the same
		 *	System, and LeftLINK(selEndL) is in the last Measure of that System.
		 */

		if (!SameSystem(doc->selStartL, LeftLINK(doc->selEndL))) {
			DisableSMMove();
			return;
		}
		
		sysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);

		if (!sysL) {
			DisableSMMove();
			return;
		}

		if (LinkLSYS(sysL)!=NILINK)
			startBarL = LSSearch(LeftLINK(doc->selStartL), MEASUREtype, ANYONE, GO_LEFT, FALSE);

		if (startBarL && LinkLSYS(sysL)!=NILINK) {
			nextBarL = LinkRMEAS(startBarL);
			measAfterSelStart = nextBarL ? (MeasSYSL(nextBarL)==sysL) : FALSE;
			moveMeasUp = ( !beforeFirst && FirstMeasInSys(startBarL) &&
					 			IsAfter(sysL,startBarL) && measAfterSelStart);
			XableItem(scoreMenu, SM_MoveMeasUp, moveMeasUp);
		}
		else
			DisableMenuItem(scoreMenu, SM_MoveMeasUp);

		endBarL = LSISearch(LeftLINK(doc->selEndL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
		if (endBarL)
			measSysEnd = LastUsedMeasInSys(doc, endBarL);
		XableItem(scoreMenu, SM_MoveMeasDown, !beforeFirst && endBarL && measSysEnd);

		/*
		 *	Enable "Move System Up" iff selStartL and selEndL are in the same System, that
		 *	System is the first in its Page, and that Page is not the first in the score.
		 *
		 *	Enable "Move System Down" iff selStartL and selEndL are in the same System, that
		 *	System is the last in its Page, and that Page is not the last in the score.
		 */

		pageL = SysPAGE(sysL);
		XableItem(scoreMenu, SM_MoveSystemUp,
			(FirstSysInPage(sysL) && LinkLPAGE(pageL)!=NILINK));
		XableItem(scoreMenu, SM_MoveSystemDown, 
			(LastSysInPage(sysL) && LinkRPAGE(pageL)!=NILINK));
	}

#ifdef VIEWER_VERSION

static void FixScoreMenu(Document *doc, INT16 nSel)
	{
	}
	
static void FixNotesMenu(doc, continSel)
	register Document *doc; Boolean continSel;
	{
	}
	
static void FixGroupsMenu(doc, continSel)
	register Document *doc; Boolean continSel;
	{
	}

#else

/*
 *	Fix all items in the Score menu; disable it entirely if there is no score
 *	or it's the clipboard.
 */

static void FixScoreMenu(Document *doc, INT16 nSel)
	{
		Boolean canAddSystem=FALSE; LINK insertL; INT16 where;
		Str255 str;
	
		UpdateMenu(scoreMenu, doc!=NULL && doc!=clipboard);
		
		if (doc==NULL || doc==clipboard) return;

		/* Context switching directly between showFormat and masterView presents 
			as-yet unanalyzed problems, so for now, prevent user from going directly
			from either one to the other. */

		XableItem(scoreMenu, SM_MasterSheet, !doc->showFormat);
		CheckMenuItem(scoreMenu, SM_MasterSheet, doc->masterView);
		
		XableItem(fileMenu,SM_CombineParts, doc!=NULL && doc!=clipboard);

		XableItem(scoreMenu, SM_ShowFormat, !doc->masterView);
		CheckMenuItem(scoreMenu, SM_ShowFormat, doc->showFormat);
		
		if (!doc->masterView) {
			insertL = AddSysInsertPt(doc, doc->selStartL, &where);
			GetIndString(str, MENUCMD_STRS,
				IsAfter(insertL, doc->selStartL)? 7 : 8);			/* "Add System Before/After" */
			SetMenuItemText(scoreMenu, SM_AddSystem, str);
			EnableMenuItem(scoreMenu, SM_AddSystem);
			}
		else {
			GetIndString(str, MENUCMD_STRS, 9);						/* "Add System" */
			SetMenuItemText(scoreMenu, SM_AddSystem, str);
			DisableMenuItem(scoreMenu, SM_AddSystem);
			}

		if (!doc->masterView) {
			insertL = AddPageInsertPt(doc, doc->selStartL);
			GetIndString(str, MENUCMD_STRS,
				IsAfter(insertL, doc->selStartL)? 10 : 11);		/* "Add Page Before/After" */
			SetMenuItemText(scoreMenu, SM_AddPage, str);
			EnableMenuItem(scoreMenu, SM_AddPage);
			}
		else {
			GetIndString(str, MENUCMD_STRS, 12);						/* "Add Page" */
			SetMenuItemText(scoreMenu, SM_AddPage, str);
			DisableMenuItem(scoreMenu, SM_AddPage);
			}

		XableItem(scoreMenu, SM_FlowIn, !doc->masterView);

		XableItem(scoreMenu, SM_Respace, !doc->masterView);
		XableItem(scoreMenu, SM_Realign, !doc->masterView && nSel>0);
		XableItem(scoreMenu, SM_AutoRespace, !doc->masterView);
		CheckMenuItem(scoreMenu,SM_AutoRespace, doc!=NULL && doc->autoRespace);
		XableItem(scoreMenu, SM_Justify, !doc->masterView);
		XableItem(scoreMenu, SM_Reformat, !doc->masterView);
		
		FixMoveMeasSys(doc);
	}

LINK MBRestSel(Document *doc)
{
	LINK	pL, aNoteL;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteREST(aNoteL))
					if (NoteType(aNoteL)<=WHOLE_L_DUR) return pL;
			
	return NILINK;
}

/*
 *	Enable or disable all items in the Notes menu; disable entire menu if there is no
 *	score, if it's the clipboard, or if we're looking at the Master Page.
 */

static void FixNotesMenu(Document *doc, Boolean /*continSel*/)
	{
		register Boolean noteSel, GRnoteSel;
		Boolean keySigSel, chordSymSel, slurSel, disableWholeMenu;

		disableWholeMenu = (doc==NULL || doc==clipboard || doc->masterView);
		UpdateMenu(notesMenu, !disableWholeMenu);
		if (disableWholeMenu) return;
		
		keySigSel = ObjTypeSel(doc, KEYSIGtype, 0)!=NILINK;
		noteSel = ObjTypeSel(doc, SYNCtype, 0)!=NILINK;
		GRnoteSel = ObjTypeSel(doc, GRSYNCtype, 0)!=NILINK;
		chordSymSel = ObjTypeSel(doc, GRAPHICtype, GRChordSym)!=NILINK;
		slurSel = ObjTypeSel(doc, SLURtype, 0)!=NILINK;
		
		XableItem(notesMenu, NM_SetDuration, noteSel);

		XableItem(notesMenu,NM_CompactV, (doc!=NULL && doc!=clipboard
													&& ObjTypeSel(doc, SYNCtype, 0)!=NILINK));

		XableItem(notesMenu,NM_InsertByPos,doc!=NULL && doc!=clipboard);
		CheckMenuItem(notesMenu,NM_InsertByPos,doc!=NULL && doc->insertMode);
		
		XableItem(notesMenu, NM_ClarifyRhythm, noteSel);
		if (!noteSel)
			DisableMenuItem(notesMenu, NM_SetMBRest);
		else
			XableItem(notesMenu, NM_SetMBRest, (MBRestSel(doc)!=NILINK));

		XableItem(notesMenu, NM_TransposeKey, beforeFirst);
		XableItem(notesMenu, NM_Transpose, noteSel || GRnoteSel || chordSymSel);
		XableItem(notesMenu, NM_DiatonicTranspose, noteSel || GRnoteSel || chordSymSel);
		XableItem(notesMenu, NM_Respell, noteSel || GRnoteSel || chordSymSel);
		XableItem(notesMenu, NM_Accident, noteSel || GRnoteSel);
		XableItem(notesMenu, NM_AddAccident, noteSel || GRnoteSel);
		XableItem(notesMenu, NM_CheckRange, noteSel || GRnoteSel);

		XableItem(notesMenu, NM_AddModifiers, noteSel);
		XableItem(notesMenu, NM_StripModifiers, noteSel);
		XableItem(notesMenu, NM_Multivoice, noteSel);
		XableItem(notesMenu, NM_Flip, slurSel || noteSel || GRnoteSel);
	}


static Boolean SelRangeChkOct(INT16 staff, LINK staffStartL, LINK staffEndL)
{
	LINK pL,aNoteL,aGRNoteL;
	PANOTE aNote; PAGRNOTE aGRNote;
	
	for (pL = staffStartL; pL!=staffEndL; pL = RightLINK(pL))
		if (LinkSEL(pL))
			if (SyncTYPE(pL)) {
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staff) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->inOctava) return TRUE;
						else break;
					}
				}
			else if (GRSyncTYPE(pL)) {
				for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					if (GRNoteSTAFF(aGRNoteL)==staff) {
						aGRNote = GetPAGRNOTE(aGRNoteL);
						if (aGRNote->inOctava) return TRUE;
						else break;
					}
			}

	return FALSE;
}

static void FixBeamCommands(Document *);
static void FixTupletCommands(Document *);
static void FixOctavaCommands(Document *, Boolean);

/*
 *	Handle menu command Enable/Disable for Beams
 */

static void FixBeamCommands(Document *doc)
{
	Boolean hasBeam, hasGRBeam; LINK pL, aNoteL, aGRNoteL;
	INT16 voice, nBeamable, nGRBeamable; Str255 str;
	
	hasBeam = FALSE;			
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteBEAMED(aNoteL))
					{ hasBeam = TRUE; goto knowBeamed; }
		
knowBeamed:
	/*
	 * Handle menu Enable/Disable for grace note beams: if range is
	 * either unBeam-able or unGRBeam-able, Enable GM_Unbeam; if range
	 * is beamable/not beamable, Xable accordingly; then check for
	 * GRbeam-able; if it is, enable it.
	 */
	hasGRBeam = FALSE;			
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && GRSyncTYPE(pL))
			for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
				if (GRNoteSEL(aGRNoteL) && GRNoteBEAMED(aGRNoteL))
					{ hasGRBeam = TRUE; goto knowGRBeamed; }

knowGRBeamed:
	/* Enable/disable the Break Beam cmd (easy) and the Beam/Unbeam cmd (not easy). */
	
	XableItem(groupsMenu, GM_BreakBeam, hasBeam);
	XableItem(groupsMenu, GM_FlipFractional, hasBeam);
	
	DisableMenuItem(groupsMenu, GM_Beam);
	
	if (hasBeam || hasGRBeam) {
		cmdIsBeam = FALSE;
		GetIndString(str, MENUCMD_STRS, 13);							/* "Unbeam Notes" */
		SetMenuItemText(groupsMenu, GM_Beam, str);
		EnableMenuItem(groupsMenu, GM_Beam);
		return;
	}
	
	/*
	 *	There are no beamed notes or grace notes selected, so nothing to unbeam.
	 *	Is there anything to beam?
	 */
	nBeamable = 0;
	if (!beforeFirst)						/* Selection doesn't start before 1st measure? */
		for (voice=1; voice<=MAXVOICES; voice++) {
			if (VOICE_MAYBE_USED(doc, voice)) {
				nBeamable = CountBeamable(doc, doc->selStartL, doc->selEndL, voice, TRUE);
				if (nBeamable > 1) break;
			}
		}
	nGRBeamable = 0;
	if (!beforeFirst)						/* Selection doesn't start before 1st measure? */
		for (voice=1; voice<=MAXVOICES; voice++) {
			if (VOICE_MAYBE_USED(doc, voice)) {
				nGRBeamable = CountGRBeamable(doc, doc->selStartL, doc->selEndL, voice, TRUE);
				if (nGRBeamable > 1) break;
			}
		}

	if (nBeamable>1 || nGRBeamable>1) {
		cmdIsBeam = TRUE;
		GetIndString(str, MENUCMD_STRS, 14);							/* "Beam Notes" */
		SetMenuItemText(groupsMenu, GM_Beam, str);
		EnableMenuItem(groupsMenu, GM_Beam);
		return;
	}	
}


/*
 *	Handle menu command Enable/Disable for Tuplets
 */

static void FixTupletCommands(Document *doc)
{
	Boolean hasTuplet; LINK pL, aNoteL, vStartL, vEndL;
	INT16 voice, tupleNum; Str255 str;
	
	hasTuplet = FALSE;
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteINTUPLET(aNoteL))
					{ hasTuplet = TRUE; goto knowTupled; }
					
knowTupled:
	if (hasTuplet) {
		cmdIsTuplet = FALSE;
		GetIndString(str, MENUCMD_STRS, 15);							/* "Remove Tuplet" */
		SetMenuItemText(groupsMenu, GM_Tuplet, str);
		EnableMenuItem(groupsMenu, GM_Tuplet);
		DisableMenuItem(groupsMenu, GM_FancyTuplet);
		return;
	}
	
	/*
	 *	No tupleted notes are selected, so nothing to unbeam. Is there anything to
	 * tuple? First, no tupleting is allowed for selections across measure boundaries.
	 */
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (MeasureTYPE(pL)) {
			DisableMenuItem(groupsMenu, GM_Tuplet);
			DisableMenuItem(groupsMenu, GM_FancyTuplet);
			return;
		}
	/*
	 *	For all voices, if there is a tuplable range (tupleNum>=2), enable the menu items.
	 */
	tupleNum = 0;
	if (!beforeFirst)
		for (voice=1; voice<=MAXVOICES; voice++) {
			if (VOICE_MAYBE_USED(doc, voice)) {
				GetNoteSelRange(doc,voice,&vStartL,&vEndL, NOTES_ONLY);
				tupleNum = (vStartL && vEndL) ? GetTupleNum(vStartL,vEndL,voice,TRUE) : 0;
				if (tupleNum>=2) break;
			}
		}

	if (tupleNum>=2) {
		cmdIsTuplet = TRUE;
		GetIndString(str, MENUCMD_STRS, 16);							/* "Create Tuplet" */
		SetMenuItemText(groupsMenu, GM_Tuplet, str);
		EnableMenuItem(groupsMenu, GM_Tuplet);
		EnableMenuItem(groupsMenu, GM_FancyTuplet);
	}
	else {
		DisableMenuItem(groupsMenu, GM_Tuplet);
		DisableMenuItem(groupsMenu, GM_FancyTuplet);
	}
}


/*
 *	Handle menu command Enable/Disable for octave signs.
 */

static void FixOctavaCommands(Document *doc, Boolean continSel)
{
	Boolean hasOctava; LINK pL, aNoteL, aGRNoteL, stfStartL, stfEndL;
	INT16 staff, octavaNum; Str255 str;

	if (!continSel) {
		DisableMenuItem(groupsMenu, GM_Octava);
		return;
	}
	
	hasOctava = FALSE;		
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteINOCTAVA(aNoteL))
					{ hasOctava = TRUE; goto knowOctavad; }
		if (LinkSEL(pL) && GRSyncTYPE(pL))
			for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
				if (GRNoteSEL(aGRNoteL) && GRNoteINOCTAVA(aGRNoteL))
					{ hasOctava = TRUE; goto knowOctavad; }				
	}
					
knowOctavad:
	if (hasOctava) {
		cmdIsOctava = FALSE;
		GetIndString(str, MENUCMD_STRS, 17);							/* "Remove Octave Sign" */
		SetMenuItemText(groupsMenu, GM_Octava, str);
		EnableMenuItem(groupsMenu, GM_Octava);
		return;
	}

	/*
	 *	No notes/grace notes in octave signs are selected, so we can't remove an octave
	 * sign. Is it possible to create one?
	 */

	octavaNum = 0;
		
	if (!beforeFirst) {
		for (staff=1; staff<=doc->nstaves; staff++) {
			GetStfSelRange(doc,staff,&stfStartL,&stfEndL);
			if (stfStartL && stfEndL)
				octavaNum = OctCountNotesInRange(staff, stfStartL, stfEndL, FALSE);
			else
				octavaNum = 0;
			if (octavaNum > 0) {
				if (!SelRangeChkOct(staff, stfStartL, stfEndL)) break;
				octavaNum = 0;
				}
		}
	}

	if (octavaNum>0) {
		cmdIsOctava = TRUE;
		GetIndString(str, MENUCMD_STRS, 18);							/* "Create Octave Sign" */
		SetMenuItemText(groupsMenu, GM_Octava, str);
		EnableMenuItem(groupsMenu, GM_Octava);
	}
	else
		DisableMenuItem(groupsMenu, GM_Octava);
}

/*
 *	Enable or disable all items in the Groups menu; disable entire menu if there is no
 *	score, if it's the clipboard, or if we're looking at the Master Page.
 */

static void FixGroupsMenu(Document *doc, Boolean continSel)
	{
		register Boolean noteSel;
		Boolean GRnoteSel, disableWholeMenu;

		disableWholeMenu = (doc==NULL || doc==clipboard || doc->masterView);
		UpdateMenu(groupsMenu, !disableWholeMenu);
		if (disableWholeMenu) return;
		
		noteSel = ObjTypeSel(doc, SYNCtype, 0)!=NILINK;
		GRnoteSel = ObjTypeSel(doc, GRSYNCtype, 0)!=NILINK;
		
		XableItem(groupsMenu, GM_AutomaticBeam, noteSel);

		/*
		 * If there are neither any notes nor grace notes selected, we just disable
		 *	all commands and we're done.
		 */
		if (!noteSel) {										/* No notes selected, so... */
			DisableMenuItem(groupsMenu, GM_Tuplet);			/* Can't remove or add any tuplets. */
			DisableMenuItem(groupsMenu, GM_FancyTuplet);
			if (!GRnoteSel) {
				DisableMenuItem(groupsMenu, GM_Beam); 			/* Can't remove or add any beams. */
				DisableMenuItem(groupsMenu, GM_BreakBeam);	 	/* Can't break any. */
				DisableMenuItem(groupsMenu, GM_FlipFractional); /* Can't flip any. */

				DisableMenuItem(groupsMenu, GM_Octava); 		/* Can't remove or add any octavas. */
				return;
				}
			}
		
		/* Finish handling menu Enable/Disable for Beams, Tuplets, Octavas. */

		FixBeamCommands(doc);
		FixTupletCommands(doc);
		FixOctavaCommands(doc, continSel);
	}

#endif

void AddWindowList()
{
	INT16 startItems, itemNum; Document *doc;
	
	/* Delete any window names from the end of the View menu. */
	
	startItems = CountMenuItems(viewMenu);
	for (itemNum = startItems; itemNum>VM_LastItem; itemNum--)
		DeleteMenuItem(viewMenu, itemNum);
	
	/* If any Documents are open, add divider and window names of all open ones. */
	
	if (TopDocument) {
		AppendMenu(viewMenu, "\p(-");

		/* Add each Document to the menu, starting after any special Documents: we
		 * don't want them to appear in the list.
		 */
		startItems = CountMenuItems(viewMenu);
		for (itemNum = startItems, doc = documentTable+SPEC_DOC_COUNT; doc<topTable; doc++)
			if (doc->inUse) {
				InsertMenuItem(viewMenu, "\ptemp", itemNum);
				GetWCTitle(doc->theWindow, strBuf);
				SetMenuItemCText(viewMenu, itemNum+1, strBuf);
				if (doc->theWindow==TopDocument) CheckMenuItem(viewMenu, itemNum+1, TRUE);
				itemNum++;
			}
	}
}

/* Fix all items in the View menu and add a list of open Documents (windows);
disable the entire menu if there's nothing to view. */

static void FixViewMenu(Document *doc)
	{
//		Boolean disableWholeMenu;
		
		AddWindowList();
#ifdef NOTYET
		disableWholeMenu = (doc==NULL);
		UpdateMenu(viewMenu, !disableWholeMenu);
		if (disableWholeMenu) return;
#else
		UpdateMenu(viewMenu, doc!=NULL);
		/* ??If doc is NULL, we should return at this point--continuing is dangerous! */
#endif
		
		XableItem(viewMenu,VM_GoTo,ENABLE_GOTO(doc));
		XableItem(viewMenu,VM_GotoSel,ENABLE_GOTO(doc));

		XableItem(viewMenu,VM_Reduce,ENABLE_REDUCE(doc));
		XableItem(viewMenu,VM_Enlarge,ENABLE_ENLARGE(doc));
		
		XableItem(viewMenu,VM_LookAtV,doc!=NULL && !doc->masterView && !doc->showFormat);
		XableItem(viewMenu,VM_LookAtAllV,doc!=NULL && !doc->masterView && !doc->showFormat);
#ifndef VIEWER_VERSION
		XableItem(viewMenu,VM_ShowDurProb,doc!=NULL && !doc->masterView);
		XableItem(viewMenu,VM_ShowSyncL,doc!=NULL && !doc->masterView);
		XableItem(viewMenu,VM_ShowInvis,doc!=NULL && doc!=clipboard && !doc->masterView);
		XableItem(viewMenu,VM_ShowSystems,doc!=NULL && doc!=clipboard);

		XableItem(viewMenu,VM_PianoRoll,doc!=NULL && !doc->masterView);
		
		XableItem(viewMenu, VM_ShowClipboard, doc!=NULL && doc!=clipboard);
		
		XableItem(viewMenu, VM_SymbolPalette, !IsWindowVisible(palettes[TOOL_PALETTE]));

#endif

		CheckMenuItem(viewMenu,VM_GoTo,doc!=NULL && doc->overview);
		CheckMenuItem(viewMenu, VM_ColorVoices, doc!=NULL && doc->colorVoices!=0);
#ifndef VIEWER_VERSION
		CheckMenuItem(viewMenu,VM_ShowDurProb,doc!=NULL && doc->showDurProb);
		CheckMenuItem(viewMenu,VM_ShowSyncL,doc!=NULL && doc->showSyncs);
		CheckMenuItem(viewMenu,VM_ShowInvis,doc!=NULL && doc->showInvis);
		CheckMenuItem(viewMenu, VM_ShowSystems, doc!=NULL && doc->frameSystems);
		CheckMenuItem(viewMenu,VM_PianoRoll,doc!=NULL && doc->pianoroll);
#endif
	}

/* Fix all items in Play/Record menu; disable entire menu if there's no score or
we're looking at the Master Page. */

static void FixPlayRecordMenu(Document *doc, INT16 nSel)
	{
		Boolean disableWholeMenu, noteSel, haveMIDI=(useWhichMIDI!=MIDIDR_NONE);

		disableWholeMenu = (doc==NULL || doc->masterView);

		if (!disableWholeMenu) {
			XableItem(playRecMenu, PL_PlayEntire, haveMIDI);
			XableItem(playRecMenu, PL_PlaySelection, doc!=clipboard && nSel!=0 && haveMIDI);
			XableItem(playRecMenu, PL_PlayFromSelection, doc!=clipboard && haveMIDI);
			XableItem(playRecMenu, PL_AllNotesOff, haveMIDI);
	
	#ifndef VIEWER_VERSION
			noteSel = ObjTypeSel(doc, SYNCtype, 0)!=NILINK;
			XableItem(playRecMenu, PL_Quantize, noteSel);
	
			XableItem(playRecMenu, PL_RecordInsert, doc!=clipboard && haveMIDI);
			XableItem(playRecMenu, PL_RecordMerge, doc!=clipboard && haveMIDI);
			XableItem(playRecMenu, PL_StepRecInsert, doc!=clipboard && haveMIDI);
			XableItem(playRecMenu, PL_StepRecMerge, doc!=clipboard && haveMIDI);
	#endif
	 		XableItem(playRecMenu, PL_MIDISetup, doc!=clipboard);
	 		XableItem(playRecMenu, PL_Metronome, haveMIDI);		/* Metro and thru are global options */
	 		XableItem(playRecMenu, PL_MIDIThru, useWhichMIDI==MIDIDR_FMS);
			XableItem(playRecMenu, PL_MIDIDynPrefs, doc!=clipboard);
			XableItem(playRecMenu, PL_MIDIModPrefs, doc!=clipboard);
			XableItem(playRecMenu, PL_PartMIDI, doc!=clipboard);
			XableItem(playRecMenu, PL_Transpose, doc!=clipboard);
			CheckMenuItem(playRecMenu, PL_Transpose, doc->transposed);
		}
		else {
			XableItem(playRecMenu, PL_PlayEntire, FALSE);
			XableItem(playRecMenu, PL_PlaySelection, FALSE);
			XableItem(playRecMenu, PL_PlayFromSelection, FALSE);
			XableItem(playRecMenu, PL_AllNotesOff, FALSE);
	
	#ifndef VIEWER_VERSION
			XableItem(playRecMenu, PL_Quantize, FALSE);
			XableItem(playRecMenu, PL_RecordInsert, FALSE);
			XableItem(playRecMenu, PL_RecordMerge, FALSE);
			XableItem(playRecMenu, PL_StepRecInsert, FALSE);
			XableItem(playRecMenu, PL_StepRecMerge, FALSE);
	#endif
			
	 		XableItem(playRecMenu, PL_MIDISetup, FALSE);
	 		XableItem(playRecMenu, PL_Metronome, FALSE);
	 		XableItem(playRecMenu, PL_MIDIThru, FALSE);
			XableItem(playRecMenu, PL_MIDIDynPrefs, FALSE);
			XableItem(playRecMenu, PL_MIDIModPrefs, FALSE);
			XableItem(playRecMenu, PL_PartMIDI, FALSE);
			XableItem(playRecMenu, PL_Transpose, FALSE);
			CheckMenuItem(playRecMenu, PL_Transpose, FALSE);
		}
	}

#ifdef VIEWER_VERSION

static void FixMasterPgMenu(Document *doc)
{
}

static void FixFormatMenu(Document *doc)
{
}

#else

/* Fix all items in Master Page menu. If no score or we're not in Master Page,
this menu shouldn't be installed, so we don't even need to disable it. */

static void FixMasterPgMenu(Document *doc)
{
	INT16 groupSel,nstavesMP, nparts; LINK staffL; Str255 str;

	if (doc==NULL || !doc->masterView) return;
	
	groupSel = GroupSel(doc);
	nstavesMP = doc->nstavesMP;

	staffL = LSSearch(doc->masterHeadL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
	GetIndString(str, MENUCMD_STRS,
		LinkSEL(staffL)? 19 : 20);							/* "Add Part/Staff Above/at Bottom" */
	SetMenuItemText(masterPgMenu, MP_AddPart, str);

	/* Disable all menu items except Add Part if score has no parts. */

	nparts = LinkNENTRIES(doc->masterHeadL);
	XableItem(masterPgMenu, MP_DeletePart, PartSel(doc));

	XableItem(masterPgMenu, MP_GroupParts, !groupSel && PartRangeSel(doc) && nstavesMP>0);
	XableItem(masterPgMenu, MP_UngroupParts, groupSel && nstavesMP>0);
	XableItem(masterPgMenu, MP_SplitPart, PartSel(doc));
	XableItem(masterPgMenu, MP_DistributeStaves, PartSel(doc));
	
	XableItem(masterPgMenu, MP_StaffSize, nstavesMP>0);
	XableItem(masterPgMenu, MP_StaffLines, nstavesMP>0);
	XableItem(masterPgMenu, MP_EditMargins, nstavesMP>0);

	XableItem(masterPgMenu, MP_Instrument, nstavesMP>0 && PartSel(doc));
	
//	XableItem(masterPgMenu, MP_CombineParts, nstavesMP>0 && PartRangeSel(doc));
}


/* Fix all items in Work on Format menu. If no score or we're not showing Format,
this menu shouldn't be installed, so we don't even need to disable it. */

static void FixFormatMenu(Document *doc)
{
	INT16 nVis, nInvis; LINK pL, aStaffL;

	if (doc==NULL || !doc->showFormat) return;
	
	nVis = nInvis = 0;
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (StaffTYPE(pL) && LinkSEL(pL)) {
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				if (StaffVIS(aStaffL)) nVis++;
				else						  nInvis++;
		}
	
	XableItem(formatMenu, FT_Invisify, nVis>=2);
	XableItem(formatMenu, FT_ShowInvis, nInvis>=1);
}

#endif
