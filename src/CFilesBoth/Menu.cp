/* Menu.c for Nightingale - general menu routines */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "CarbonPrinting.h"
#include "CarbonTemplates.h" // MAS
#include "MidiMap.h"
#ifdef SEARCH_CONTENT
	#include "SearchScore.h"
#endif

static void	MovePalette(WindowPtr whichPalette,Point position);

static Boolean IsSafeToQuit(void);
static Boolean SetTranspKeyStaves(Document *, Boolean []);
static void EditInstrMIDI(Document *doc);

static void	DoAppleMenu(short choice);
static void	DoTestMenu(short choice);
static void	DoScoreMenu(short choice);
static void	DoNotesMenu(short choice);
static void	DoGroupsMenu(short choice);

static void MPInstrument(Document *);

static void	DoMasterPgMenu(short choice);
static void	DoFormatMenu(short choice);
static void	DoMagnifyMenu(short choice);

static void	FMPreferences(Document *doc);

static void EMAddCautionaryTimeSigs(Document *doc);

static void	SMLeftEnd(Document *doc);
static void	SMTextStyle(Document *doc);
static void	SMMeasNum(Document *doc);
static void	SMRespace(Document *doc);
static void	SMRealign(Document *doc);
static void	SMReformat(Document *doc);

static void	NMSetDuration(Document *doc);
static void	NMInsertByPos(Document *doc);
static void	NMSetMBRest(Document *doc);
static void	NMFillEmptyMeas(Document *doc);
static void NMAddModifiers(Document *doc);
static void NMStripModifiers(Document *doc);
static short CountSelVoices(Document *);
static void NMMultiVoice(Document *doc);

static void	VMLookAt(void);
static void	VMLookAtAll(void);
static void	VMShowDurProblems(void);
static void	VMShowSyncs(void);
static void	VMShowInvisibles(void);
static void	VMColorVoices(void);
static void	VMGraphMode(void);
static void	VMShowToolPalette(void);
static void	VMActivate(short);

static void PLSetPlayDuration(Document *);
static Boolean OKToRecord(Document *doc);
static void	PLRecord(Document *doc, Boolean merge);
static void	PLStepRecord(Document *doc, Boolean merge);
static void	PLMIDIDynPrefs(Document *);
static void	PLMIDIModPrefs(Document *);
static void	PLTransposedScore(Document *);

static void InstallDebugMenuItems(Boolean installAll);

static void	FixFileMenu(Document *doc, short nSel);
static void	FixEditMenu(Document *doc, short nInSelRange, short nSel);
static void	FixTestMenu(Document *doc, short nSel);
static void	DisableSMMove(void);
static void	FixMoveMeasSys(Document *doc);
static void	FixScoreMenu(Document *doc, short nSel);
static LINK	MBRestSel(Document *);
static void	FixNotesMenu(Document *doc, Boolean continSel);
static Boolean SelRangeChkOct(short staff,LINK	staffStartL,LINK staffEndL);
static void	FixGroupsMenu(Document *doc, Boolean continSel);
static void	AddWindowList(void);
static void	FixViewMenu(Document *doc);
static void	FixPlayRecordMenu(Document *doc, short nSel);
static void	FixMasterPgMenu(Document *doc);
static void	FixFormatMenu(Document *doc);
				
static void DeleteObj(Document *, LINK);
static void DeleteSelObjs(Document *);

static Boolean cmdIsBeam, cmdIsTuplet, cmdIsOttava;

static Boolean	goUp=True;								/* For "Transpose" dialog */
static short	octaves=0, steps=0, semiChange=0;
static Boolean	slashes=False;

static Boolean	dGoUp=True;								/* For "Diatonic Transpose" dialog */
static short	dOctaves=0, dSteps=0;
static Boolean	dSlashes=False;

static Boolean	kGoUp=True;								/* For "Transpose Key" dialog */
static short	kOctaves=0, kSteps=0, kSemiChange=0;
static Boolean	kSlashes=False, kNotes=True, kChordSyms=True;

static Boolean	beforeFirst;

static Boolean showDbgWin = False;

static Boolean debugItemsNeedInstall = True;


Boolean DoMenu(long menuChoice)
	{
		short choice, menu;
		Boolean keepGoing = True;
		
		menu = HiWord(menuChoice); choice = LoWord(menuChoice);
		if (TopDocument) MEHideCaret(GetDocumentFromWindow(TopDocument));

		switch (menu) {
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
		
		if (keepGoing) HiliteMenu(0);
		return(keepGoing);
	}


/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the Apple Menu. */

static void DoAppleMenu(short choice)
{
	switch (choice) {
		case AM_About:
#ifdef PUBLIC_VERSION
			DoAboutBox(False);
#else
			if (ShiftKeyDown() && OptionKeyDown() && CmdKeyDown())
				Debugger();
			 else
				DoAboutBox(False);
#endif
			break;
		case AM_Help:
			Do_Help(appVRefNum, appDirID);
			UseResFile(appRFRefNum);				/* a precaution */
			break;
		default:
			break;
		}
}


/* -------------------------------------------------------------------------------------- */

Boolean IsSafeToQuit()
{
	Document *doc;  Boolean inMasterPage=False;
	
	for (doc=documentTable; doc<topTable; doc++)
		if (doc->inUse)
			if (doc->masterView)
				{ inMasterPage = True; break; }
	
	if (inMasterPage) {
		GetIndCString(strBuf, MENUCMDMSGS_STRS, 1);		/* "You can't quit now because at least one score is in Master Page." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
	else
		return True;
}


static Boolean GetNotelistFile(Str255 macfName, NSClientDataPtr pNSD);
static Boolean GetNotelistFile(Str255 macfName, NSClientDataPtr pNSD)
{
	OSStatus 	err = noErr;
	OSType 		inputType[MAXINPUTTYPE];
	
	inputType[0] = 'TEXT';
	err = OpenFileDialog(kNavGenericSignature, 1, inputType, pNSD);
	if (err != noErr || pNSD->nsOpCancel) return False;
	
	FSSpec fsSpec = pNSD->nsFSSpec;
	Pstrcpy(macfName, fsSpec.name);
	return True;
}


#ifdef SEARCH_CONTENT

static Boolean usePitch = True;
static Boolean useDuration = True;
static FASTFLOAT pitchWeight = 0.75;
static INT16 pitchSearchType = TYPE_PITCHMIDI_REL;
static INT16 durSearchType = TYPE_DURATION_REL;
static Boolean includeRests = True;
static INT16 maxTranspose = 127;
static INT16 pitchTolerance = 0;
static Boolean keepContour = True;
static INT16 chordNotes = TYPE_CHORD_OUTER;
static Boolean matchTiedDur = True;

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
		(void)DoSearchScore(doc, True, usePitch, useDuration, pitchSearchType,
							durSearchType, includeRests, maxTranspose, pitchTolerance,
							pitchWeight, keepContour, chordNotes, matchTiedDur);
		return;
	}

	if (SearchDialog(False, &findAll, &usePitch, &useDuration, &pitchSearchType,
						&durSearchType, &includeRests, &maxTranspose, &pitchTolerance,
						&keepContour, &chordNotes, &matchTiedDur)) {
		if (findAll)
			(void)DoIRSearchScore(doc, usePitch, useDuration, pitchSearchType,
								durSearchType, includeRests, maxTranspose, pitchTolerance,
								pitchWeight, keepContour, chordNotes, matchTiedDur);
		else
			(void)DoSearchScore(doc, False, usePitch, useDuration, pitchSearchType,
								durSearchType, includeRests, maxTranspose, pitchTolerance,
								pitchWeight, keepContour, chordNotes, matchTiedDur);
	}
}
#endif

/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the File Menu.  Return False if it's time to quit. */

Boolean DoFileMenu(short choice)
	{
		Boolean keepGoing = True, doSymbol;
		short vrefnum, returnCode;
		Document *doc=GetDocumentFromWindow(TopDocument);
		char str[256], tmpCStr[256];
		NSClientData nscd;  FSSpec fsSpec;
		
		switch (choice) {
			case FM_New:
				doSymbol = (TopDocument==NULL);
				DoOpenDocument(NULL, 0, False, NULL);
				LogPrintf(LOG_NOTICE, "Opened new score.  (DoFileMenu)\n");
				if (doSymbol && !IsWindowVisible(palettes[TOOL_PALETTE])) {
					AnalyzeWindows();
					DoViewMenu(VM_ToolPalette);
				}
				break;
			case FM_Open:
				if (!FirstFreeDocument()) TooManyDocs();
				 else {
					UseStandardType(documentType);
					GetIndCString(str, MENUCMDMSGS_STRS, 4);		/* "Which score do you want to open?" */
					returnCode = GetInputName(str, True, tmpStr, &vrefnum, &nscd);
					if (returnCode) {
						if (returnCode==OP_OpenFile) {
							fsSpec = nscd.nsFSSpec;
							vrefnum = nscd.nsFSSpec.vRefNum;
							Pstrcpy((unsigned char *)tmpCStr, tmpStr);
							PToCString((unsigned char *)tmpCStr);
							LogPrintf(LOG_NOTICE, "Opening file '%s'...  (DoFileMenu)\n",
								tmpCStr);
							if (DoOpenDocument(tmpStr, vrefnum, False, &fsSpec))
								LogPrintf(LOG_NOTICE, "Opened file '%s'.  (DoFileMenu)\n",
									tmpCStr);
						 }
						 else if (returnCode==OP_NewFile)
						 	keepGoing = DoFileMenu(FM_New);
						 else if (returnCode==OP_QuitInstead)
						 	keepGoing = DoFileMenu(FM_Quit);
						}
					}
				break;
			case FM_OpenReadOnly:
				if (!FirstFreeDocument()) TooManyDocs();
				 else {
					UseStandardType(documentType);
					GetIndCString(str, MENUCMDMSGS_STRS, 5);		/* "Which score do you want to open read-only?" */
					returnCode = GetInputName(str, False, tmpStr, &vrefnum, &nscd);
					if (returnCode)
						fsSpec = nscd.nsFSSpec;
						vrefnum = nscd.nsFSSpec.vRefNum;
						DoOpenDocument(tmpStr, vrefnum, True, &fsSpec);
						LogPrintf(LOG_NOTICE, "Opened read-only file '%s'.  (DoFileMenu)\n", PToCString(tmpStr));
				}
				break;
			case FM_Close:
				if (doc) DoCloseWindow(doc->theWindow);
				break;
			case FM_CloseAll:
				DoCloseAllDocWindows();
				break;
			case FM_Save:
				if (doc) {
					if (DoSaveDocument(doc)==NRV_SUCCESS)
						LogPrintf(LOG_NOTICE, "File saved.  (DoFileMenu)\n");
				}
				break;
			case FM_SaveAs:
				if (doc) {
					if (DoSaveAs(doc)==NRV_SUCCESS)
						LogPrintf(LOG_NOTICE, "File saved.  (DoFileMenu)\n");
				}
				break;
			case FM_Revert:
				if (doc) DoRevertDocument(doc);
				break;
			case FM_Import:
				UseStandardType('Midi');
				ClearStandardTypes();
				GetIndCString(str, MIDIFILE_STRS, 1);				/* "What MIDI file do you want to Import?" */
				returnCode = GetInputName(str, False, tmpStr, &vrefnum, &nscd);
				if (returnCode) {
					fsSpec = nscd.nsFSSpec;
					Pstrcpy((unsigned char *)tmpCStr, tmpStr);
					PToCString((unsigned char *)tmpCStr);
					LogPrintf(LOG_NOTICE, "Importing MIDI file '%s'...  (DoFileMenu)\n",
						tmpCStr);
					if (ImportMIDIFile(&fsSpec)) LogPrintf(LOG_NOTICE, "Imported MIDI file '%s'.  (DoFileMenu)\n",
						tmpCStr);
				}
				break;
			case FM_Export:
				if (doc) {
					if (SaveMIDIFile(doc))
						LogPrintf(LOG_NOTICE, "MIDI file exported.  (DoFileMenu)\n");
				}
				break;
			case FM_Extract:
				if (doc) DoExtract(doc);
				break;
			case FM_GetETF:
				/* ETF (Finale Enigma Transportable File) support removed, so do nothing. */
				
				break;
			case FM_GetNotelist:
				NSClientData nsData;
				Str255 filename;
				
				if (GetNotelistFile(filename, &nsData)) {
					Pstrcpy((unsigned char *)tmpCStr, filename);
					PToCString((unsigned char *)tmpCStr);
					LogPrintf(LOG_NOTICE, "Opening notelist file '%s'...  (DoFileMenu)\n",
						tmpCStr);
					if (OpenNotelistFile(filename, &nsData)) {
						LogPrintf(LOG_NOTICE, "Opened notelist file '%s'.  (DoFileMenu)\n",
									tmpCStr);
					}
				}
				break;
#ifdef USE_NL2XML
			case FM_Notelist2XML:
				NSClientData nsData;
				Str255 filename;
				
				if (GetNotelistFile(filename, &nsData))
					NL2XMLParseNotelistFile(fileName, &nsData);
				break;
#endif
			case FM_OpenMidiMap:
				MidiMapInfo();
				break;

			case FM_SheetSetup:
				if (doc) DoSheetSetup(doc);
				break;
			case FM_PageSetup:
				if (doc) DoPageSetup(doc);
				break;
			case FM_Print:
				if (doc) DoPrintScore(doc);
				break;
			case FM_SavePostScript:
				if (doc) DoPostScript(doc);
				break;
			case FM_SaveText:
				if (doc) SaveNotelist(doc, ANYONE, True);
				break;
			case FM_ScoreInfo:
				/* For users of public versions that don't have the Test menu, provide
				   a way to access our debugging and emergency-repair facilities. */
				   
				if (OptionKeyDown() && CmdKeyDown())
					InstallDebugMenuItems(ControlKeyDown());
				else ScoreInfo();
				break;
			case FM_Preferences:
				if (doc) FMPreferences(doc);
				break;
#ifdef TEST_SEARCHFILES_NG
			case FM_SearchInFiles:
				FMSearchFilesMelody();
				break;
#endif
			case FM_Quit:
				if (!IsSafeToQuit()) break;
				if (!SaveToolPalette(True)) break;
				ShowHidePalettes(False);
				quitting = True;
				break;
			}
		
		if (keepGoing) HiliteMenu(0);
		return(keepGoing);
	}


/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the Edit Menu. */

void DoEditMenu(short choice)
	{
		Document *doc=GetDocumentFromWindow(TopDocument);
		if (doc==NULL) return;
		
		switch (choice) {
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
			case EM_Merge:
				DoMerge(doc);
				break;
			case EM_Clear:
				DoClear(doc);
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
			case EM_Set:				/* Originally "Set"; now "QuickChange" */
				DoSet(doc);
				break;
			case EM_GetInfo:
				InfoDialog(doc);
				break;
			case EM_ModifierInfo:
				ModNRInfoDialog(doc);
				break;
#ifdef SEARCH_CONTENT
			case EM_SearchMelody:
				EMSearchMelody(doc, False);
				break;
			case EM_SearchAgain:
				EMSearchMelody(doc, True);
				break;
#endif
			case EM_AddTimeSigs:
				EMAddCautionaryTimeSigs(doc);
				break;

			/* These debug commands may be added to this menu: cf. InstallDebugMenuItems */
			
			case EM_Browser:
				if (OptionKeyDown())
					Browser(doc, doc->masterHeadL, doc->masterTailL);
				else if (ShiftKeyDown())
					Browser(doc, doc->undo.headL, doc->undo.tailL);
				else
					Browser(doc, doc->headL, doc->tailL);
				break;
			case EM_Debug:
				DoDebug("Menu");
				fflush(stdout);
				break;
			case EM_DeleteObjs:
				if (doc) DeleteSelObjs(doc);
				break;
		}
	}


/* -------------------------------------------------------------------------------------- */

//#ifndef PUBLIC_VERSION

/* Functions to delete selected objects from the score "stupidly", i.e., with almost
no attempt to maintain consistency. This is intended to allow wizards to repair
messed-up files and to help debug Nightingale, but it's obviously very dangerous:
ordinary users should never do it! */
 
static void DeleteObj(Document *doc, LINK pL)
{
	LogPrintf(LOG_NOTICE, "About to DeleteNode(%u) of type=%d...  (DeleteObj)", pL, ObjLType(pL));

	if (InObjectList(doc, pL, MAIN_DSTR)) {
		DeleteNode(doc, pL);
		LogPrintf(LOG_NOTICE, "done.\n");
		doc->changed = True;
	}
	else
		LogPrintf(LOG_WARNING, "OBJECT NOT IN MAIN OBJECT LIST. No action taken.  (DeleteObj)\n");
}

static void DeleteSelObjs(Document *doc)
{
	short nInRange, nSelFlag;
	LINK pL, nextL, firstMeasL;
	
	CountSelection(doc, &nInRange, &nSelFlag);
	LogPrintf(LOG_INFO, "%d objects in selection range, %d selected.  (DeleteSelObjs)\n", nInRange, nSelFlag);
	if (nSelFlag<=0) { SysBeep(1); return; }
	
	sprintf(strBuf, "%d", nSelFlag);
	CParamText(strBuf, "", "", "");
	if (CautionAdvise(STUPID_KILL_ALRT)==Cancel) return;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = nextL) {
		nextL = RightLINK(pL);
		if (LinkSEL(pL)) DeleteObj(doc, pL);
	}

	/* Our only attempt to preserve consistency is to make the selection range legal. */
	
	if (!InObjectList(doc, doc->selStartL, MAIN_DSTR)
	||  !InObjectList(doc, doc->selEndL, MAIN_DSTR)) {
		firstMeasL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, False);
		doc->selStartL = doc->selEndL = RightLINK(firstMeasL);
	}
}


static void SetMeasNumPos(Document *doc, LINK startL, LINK endL, short xOffset, short yOffset);
static void ResetAllMeasNumPos(Document *doc);

static void SetMeasNumPos(Document *doc, LINK startL, LINK endL, short xOffset, short yOffset)
{
	PAMEASURE	aMeasure;
	LINK		pL, aMeasureL;

	for (pL = startL; pL!=endL; pL=RightLINK(pL)) {
		if (MeasureTYPE(pL)) {
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
				aMeasure = GetPAMEASURE(aMeasureL);
				aMeasure->xMNStdOffset = xOffset;
				aMeasure->yMNStdOffset = yOffset;
			}
			doc->changed = True;
		}
	}
}


static void ResetAllMeasNumPos(Document *doc)
{
	SetMeasNumPos(doc, doc->headL, doc->tailL, 0, 0);
	InvalWindow(doc);
}

/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the Test Menu. */

static void DoTestMenu(short choice)
	{
#ifndef PUBLIC_VERSION
		Document *doc = GetDocumentFromWindow(TopDocument);			

		switch (choice) {
			case TS_Browser:
				if (doc) {
					if (OptionKeyDown())
						Browser(doc, doc->masterHeadL, doc->masterTailL);
					else if (ShiftKeyDown())
						Browser(doc, doc->undo.headL, doc->undo.tailL);
					else
						Browser(doc, doc->headL, doc->tailL);
				}
				break;
			case TS_HeapBrowser:
				HeapBrowser(SYNCtype);
				break;
			case TS_Context:
				if (doc) ShowContext(doc);
				break;
			case TS_ClickErase:
				clickMode = ClickErase;
				LogPrintf(LOG_INFO, "ClickMode set to ClickErase\n");
				break;
			case TS_ClickSelect:
				clickMode = ClickSelect;
				LogPrintf(LOG_INFO, "ClickMode set to ClickSelect\n");
				break;
			case TS_ClickFrame:
				clickMode = ClickFrame;
				LogPrintf(LOG_INFO, "ClickMode set to ClickFrame\n");
				break;
			case TS_Debug:
				DoDebug("Menu");
				break;
			case TS_ShowDebug:
//				static Boolean showDbgWin = False;
				
				showDbgWin = !showDbgWin;
				if (showDbgWin) {
//					ShowHideDebugWindow(True);
					LogPrintf(LOG_WARNING, "Show Debug Window isn't implemented.  (DoTestMenu)\n");					
				}
				else {				
//					ShowHideDebugWindow(False);
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


/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the Score Menu. */

static void DoScoreMenu(short choice)
	{
		Document *doc=GetDocumentFromWindow(TopDocument);
		LINK insertL;  short where;
		
		if (doc==NULL) return;
		
		switch (choice) {
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


/* -------------------------------------------------------------------------------------- */

/* Collect information for the Transpose Key command: fill the <trStaff> array with
flags indicating which staff nos. should be transposed (because they have something
selected in the reserved area at the start of the selection). To keep users from
getting confused with the Transpose Chromatic and Transpose Diatonic commands (which
work very differently), if anything after the reserved area is selected, return False
to indicate Transpose Key shouldn't be allowed. */

static Boolean SetTranspKeyStaves(Document *doc, Boolean trStaff[])
{
	short s;
	LINK pL, aClefL, aKeySigL, aTimeSigL, measL;
	
	/* If anything is selected after the initial reserved area, don't allow Transpose Key. */
	
	measL = LSSearch(LeftLINK(doc->selEndL), MEASUREtype, ANYONE, GO_LEFT, False);
	if (measL) return False;

	/* Decide which staves will be affected. */
	
	for (s = 1; s<=MAXSTAVES; s++)
		trStaff[s] = False;
		
	for (pL = doc->selStartL; pL && !MeasureTYPE(pL); pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case CLEFtype:
				aClefL = FirstSubLINK(pL);
				for ( ; aClefL; aClefL = NextCLEFL(aClefL))
					if (ClefSEL(aClefL)) trStaff[ClefSTAFF(aClefL)] = True;
				break;
			case KEYSIGtype:
				aKeySigL = FirstSubLINK(pL);
				for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
					if (KeySigSEL(aKeySigL)) trStaff[KeySigSTAFF(aKeySigL)] = True;
				break;
			case TIMESIGtype:
				aTimeSigL = FirstSubLINK(pL);
				for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
					if (TimeSigSEL(aTimeSigL)) trStaff[TimeSigSTAFF(aTimeSigL)] = True;
				break;
			default:
				;
		}

	return True;
}

/*	Handle a choice from the Notes Menu. */

static void DoNotesMenu(short choice)
	{
		Document *doc=GetDocumentFromWindow(TopDocument);
		short delAccCode;
		Boolean canTranspKey, trStaff[MAXSTAVES+1];
		short addAccCode;
		
		if (doc==NULL) return;
		
		switch (choice) {
			case NM_SetDuration:
				NMSetDuration(doc);
				break;
			case NM_CompactV:
				DoCompactVoices(doc);
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
					PrepareUndo(doc, doc->selStartL, U_AddRedundAcc, 42);	/* "Add Redundant Accidentals" */
					if (AddRedundantAccs(doc, ANYONE, addAccCode, True))
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


/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the Groups Menu. */

void DoGroupsMenu(short choice)
	{
		Document *doc=GetDocumentFromWindow(TopDocument);
		TupleParam tParam;  Boolean okay;
		
		if (doc==NULL) return;
		
		switch (choice) {
			case GM_AutomaticBeam:
				DoAutoBeam(doc);
				break;
			case GM_Beam:
				if (cmdIsBeam)	DoBeam(doc);
				else			DoUnbeam(doc);
				break;
			case GM_BreakBeam:
				DoBreakBeam(doc);
				break;
			case GM_FlipFractional:
				DoFlipFractionalBeam(doc);
				break;
			case GM_Tuplet:
				if (cmdIsTuplet) {
					tParam.isFancy = False;
					DoTuple(doc, &tParam);
					}
				else {
					DoUntuple(doc);
					}
				break;
			case GM_FancyTuplet:
				if (FTupletCheck(doc, &tParam)) {
					tParam.isFancy = True;
					okay = TupletDialog(doc, &tParam, True);
					if (okay) DoTuple(doc, &tParam);
					}
				break;
			case GM_Ottava:
				if (cmdIsOttava)	DoOttava(doc);
				else				DoRemoveOttava(doc);
				break;
			default:
				break;
			}
	}


/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the View Menu. */

void DoViewMenu(short choice)
	{
		Document *doc=GetDocumentFromWindow(TopDocument);
		
		switch (choice) {
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
			case VM_ColorVoices:
				VMColorVoices();
				break;
			case VM_RedrawScr:
				RefreshScreen();
				break;
			case VM_GraphMode:
				VMGraphMode();
				break;
			case VM_ShowClipboard:
				ShowClipDocument();
				break;
			case VM_ToolPalette:
				VMShowToolPalette();
				break;
			case VM_ShowSearchPattern:
#ifdef SEARCH_CONTENT
				ShowSearchPatDocument();
#endif
				break;
			default:
				VMActivate(choice);
				break;
			}
	}


/* -------------------------------------------------------------------------------------- */

/* Handle the "MIDI Dynamics Preferences" command. */

static void PLMIDIDynPrefs(Document *doc)
{
	static Boolean apply=False;
	Boolean okay;
	
	okay = MIDIDynamDialog(doc, &apply);
	if (okay)
		DisableUndo(doc, False);
	if (okay && apply) {
		SetVelFromContext(doc, False);
		doc->changed = True;
	}
}


/* Handle the "MIDI Modifier Preferences" command. */

static void PLMIDIModPrefs(Document *doc)
{
	Boolean okay;
	
	okay = MIDIModifierDialog(doc);
}


/* Toggle the document's "transposed score" flag. */

static void PLTransposedScore(Document *doc)
{
	static Boolean warnedTransp = False;
	short partn, nparts;
	LINK partL;
	PPARTINFO pPart;

	if (!warnedTransp) {
		nparts = LinkNENTRIES(doc->headL)-1;
		partL = NextPARTINFOL(FirstSubLINK(doc->headL));
		for (partn = 1; partn<=nparts; partn++, partL = NextPARTINFOL(partL)) {
			pPart = GetPPARTINFO(partL);
			if (pPart->transpose!=0) {
				NoteInform(TRANSP_ALRT);
				warnedTransp = True;
				break;
			}
		}
	}
	doc->transposed = !doc->transposed;
	doc->changed = True;
}


/* Handle the "Instrument MIDI Settings" dialog. */

static void EditInstrMIDI(Document *doc)
{
	LINK partL, pMPartL;
	PPARTINFO pPart, pMPart;
	PARTINFO partInfo;  short firstStaff;
	short partn;
	Boolean allParts = False;

	partL = GetSelPart(doc);
	firstStaff = PartFirstSTAFF(partL);
	pPart = GetPPARTINFO(partL);
	partInfo = *pPart;
	if (useWhichMIDI==MIDIDR_CM) {
		MIDIUniqueID device;
		partn = (short)PartL2Partn(doc, partL);
		device = GetCMDeviceForPartn(doc, partn);
		if (CMPartMIDIDialog(doc, &partInfo, &device, &allParts)) {
			pPart = GetPPARTINFO(partL);
			*pPart = partInfo;
			pMPartL = Staff2PartL(doc, doc->masterHeadL, firstStaff);
			pMPart = GetPPARTINFO(pMPartL);
			*pMPart = partInfo;
			SetCMDeviceForPartn(doc, partn, device);
			doc->changed = True;
			
			if (allParts) {
				partL = FirstSubLINK(doc->headL);
				for (short partNum=0; partNum<=LinkNENTRIES(doc->headL)-1; partNum++,
						partL = NextPARTINFOL(partL)) {
					if (partNum==0) continue;					/* skip dummy partn = 0 */
										
				if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "EditInstrMIDI: 1.doc->cmPartDeviceList[%d]=%d\n", partNum, doc->cmPartDeviceList[partNum]);
					SetCMDeviceForPartn(doc, partNum, device);
				if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "EditInstrMIDI: 2.doc->cmPartDeviceList[%d]=%d\n", partNum, doc->cmPartDeviceList[partNum]);
				}
			}
		}
	}
	else if (PartMIDIDialog(doc, &partInfo, &allParts)) {
	
		/* Update the PARTINFO in both the main object list and Master Page. */
		
		pPart = GetPPARTINFO(partL);
		*pPart = partInfo;
		pMPartL = Staff2PartL(doc, doc->masterHeadL, firstStaff);
		pMPart = GetPPARTINFO(pMPartL);
		*pMPart = partInfo;
		doc->changed = True;
	}
}

/* -------------------------------------------------------------------------------------- */
/* Handle a choice from the Play/Record Menu */

void DoPlayRecMenu(short choice)
	{		
		Document *doc=GetDocumentFromWindow(TopDocument);
//		short oldMIDIThru;

		switch (choice) {
			case PL_PlayEntire:
				MEHideCaret(doc);
				if (doc) PlayEntire(doc);
				break;
			case PL_PlayFromSelection:
				MEHideCaret(doc);
				if (doc) PlaySequence(doc, doc->selStartL, doc->tailL, True, False);
				break;
			case PL_PlaySelection:
				MEHideCaret(doc);
				if (doc) PlaySelection(doc);
				break;
			case PL_MutePart:
				MEHideCaret(doc);
				if (doc) MutePartDialog(doc);
				break;
			case PL_PlayVarSpeed:
				MEHideCaret(doc);
				if (doc) SetPlaySpeedDialog();
				break;
			case PL_SetPlayDuration:
				MEHideCaret(doc);
				if (doc) PLSetPlayDuration(doc);
				break;			
			case PL_AllNotesOff:
				if (useWhichMIDI==MIDIDR_CM)
					CMAllNotesOff();
				break;
			case PL_RecordInsert:
				if (doc) PLRecord(doc, False);
				break;
			case PL_Quantize:
				if (doc) DoQuantize(doc);
				break;
			case PL_RecordMerge:
				if (doc) PLRecord(doc, True);
				break;
			case PL_StepRecInsert:
				if (doc) PLStepRecord(doc, False);
				break;
			case PL_StepRecMerge:
				if (doc) PLStepRecord(doc, True);
				break;
			case PL_MIDISetup:
				MEHideCaret(doc);
//				oldMIDIThru = config.midiThru;
				if (doc) MIDIPrefsDialog(doc);
				break;
			case PL_Metronome:
				if (useWhichMIDI==MIDIDR_CM)
					CMMetroDialog(&config.metroViaMIDI, &config.metroChannel, &config.metroNote,
									&config.metroVelo, &config.metroDur, &config.cmMetroDev);
				else
					MetroDialog(&config.metroViaMIDI, &config.metroChannel, &config.metroNote,
									&config.metroVelo, &config.metroDur);
				break;
			case PL_MIDIThru:
				break;
			case PL_MIDIDynPrefs:
				if (doc) PLMIDIDynPrefs(doc);
				break;
			case PL_MIDIModPrefs:
				if (doc) PLMIDIModPrefs(doc);
				break;
			case PL_InstrMIDI:
				if (doc) EditInstrMIDI(doc);
				break;
			case PL_Transpose:
				if (doc) PLTransposedScore(doc);
				break;

			default:
				break;
			}
	}


/* -------------------------------------------------------------------------------------- */

void MPInstrument(Document *doc)
{
	LINK staffL, aStaffL;
	short partStaffn;
	
	partStaffn = -1;
	staffL = LSSearch(doc->masterHeadL, STAFFtype, ANYONE, GO_RIGHT, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) {
			partStaffn = StaffSTAFF(aStaffL);
			break;
		}

	if (partStaffn<0) return;
	
	if (SDInstrDialog(doc, partStaffn)) {
		doc->masterChanged = True;
		InvalWindow(doc);
	}
}
	
#ifdef NOTYET
/* For now, we have a Combine Parts command accessible in "normal" mode; it'd
be better to have combining parts in Master Page, but there are unsolved problems
with MPCombineParts(). */

void MPCombineParts(Document *doc)
{
	LINK firstPartL, lastPartL;
	
	if (!GetPartSelRange(doc, &firstPartL, &lastPartL)) return;
	
	if (MPCombineParts(doc, firstPartL, lastPartL)) {
		doc->masterChanged = True;
		InvalWindow(doc);
	}
}
#endif

/* -------------------------------------------------------------------------------------- */
/* Handle a choice from the MasterPage Menu */

static void DoMasterPgMenu(short choice)
{		
	Document *doc=GetDocumentFromWindow(TopDocument);
	if (doc==NULL) return;
	
	switch (choice) {
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
#ifdef NOTYET
		case MP_CombineParts:
			MPCombineParts(doc);
#endif
		default:
			break;
	}
}


/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the Show Format Menu */

static void DoFormatMenu(short choice)
{		
	Document *doc=GetDocumentFromWindow(TopDocument);
	if (doc==NULL) return;
	
	switch (choice) {
		case FT_Invisify:
			SFInvisify(doc);
			break;
		case FT_ShowInvis:
			SFVisify(doc);
			break;
		default:
			break;
	}
}


/* -------------------------------------------------------------------------------------- */
/*	Handle a choice from the Reduce/Enlarge To (Magnify) Menu */

static void DoMagnifyMenu(short choice)
{		
	/* Assume the menu simply contains a list of all possible <doc->magnify> values,
	   starting with the smallest (MIN_MAGNIFY). */
	   
	Document *doc=GetDocumentFromWindow(TopDocument);
	short newMagnify, magnifyDiff;

	if (doc) {
		newMagnify = MIN_MAGNIFY+(choice-1);
		magnifyDiff = newMagnify-(doc->magnify);
		SheetMagnify(doc, magnifyDiff);
	}
}


/* ===================================================================== Miscellaneous == */

static void MovePalette(WindowPtr whichPalette, Point position)
{
	PaletteGlobals *pg;  GrafPtr oldPort;  Rect portRect;
	
	if (GetWRefCon(whichPalette)==TOOL_PALETTE) {
		ShowHide(whichPalette,False);
		MoveWindow(whichPalette, position.h, position.v, False);
		pg = *paletteGlobals[TOOL_PALETTE];
		ChangeToolSize(pg->firstAcross,pg->firstDown,True);
	}
	 else {
		MoveWindow(whichPalette, position.h, position.v, False);
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
		/* The TopWindow is an application window or is NULL */
		BringToFront(whichPalette);
		if (TopWindow==NULL)
			/* Make the palette visible and generate an activate event. */
			ShowWindow(whichPalette);
		 else {
			/* Make the palette visible but don't generate an activate event. */
			ShowHide(whichPalette, True);
			HiliteWindow(whichPalette, True);
		}
	}
}


/*
 *	Get user preferences from dialog.
 */

static void FMPreferences(Document *doc)
{
	static short section=1;			/* Initially show General preferences */
	
	PrefsDialog(doc, doc->used, &section);
	FillSpaceMap(doc, doc->spaceTable);
}


/* Add "cautionary" time signatures: if any system begins with a time-signature change,
if the previous system ends with a barline, add the same time signature after the
barline on all staves. NB: We assume that every time-signature change is the same on
all staves! Nightingale requires all barlines to extend across all staves -- i.e.,
Measure objects must have subobjects on every Staff). That makes our assumption safer,
but not really safe; it'd be better to check here.  */

static void EMAddCautionaryTimeSigs(Document *doc)
{
	short numAdded;
	char fmtStr[256];
	
	numAdded = AddCautionaryTimeSigs(doc);
	if (numAdded==0) {
		LogPrintf(LOG_INFO, "No new time signatures needed.  (EMAddCautionaryTimeSigs)\n");
		GetIndCString(strBuf, MENUCMDMSGS_STRS, 10);	/* "No new cautionary time signatures needed." */
		CParamText(strBuf, "", "", "");
		NoteInform(GENERIC_ALRT);
	}
	else {
		LogPrintf(LOG_INFO, "Added %d time signature set(s).  (EMAddCautionaryTimeSigs)\n",
					numAdded);
		GetIndCString(fmtStr, MENUCMDMSGS_STRS, 9);	/* "Added %d cautionary time signature set(s)." */
		sprintf(strBuf, fmtStr, numAdded);
	}
	if (numAdded!=0) InvalWindow(doc);
}


/* Change indents and showing/not showing part names at left end of systems. */
 
static void SMLeftEnd(Document *doc)
{
	short	deltaFirstIndent, deltaOtherIndent;
	Boolean	changedNameVis;
	short	firstNames, firstDist, otherNames, otherDist;
	
	changedNameVis = False;
	firstNames = doc->firstNames;
	firstDist = d2pt(doc->dIndentFirst);
	otherNames = doc->otherNames;
	otherDist = d2pt(doc->dIndentOther);
	
	if (LeftEndDialog(&firstNames, &firstDist, &otherNames, &otherDist)) {
		if (firstNames!=doc->firstNames) {
			doc->firstNames = firstNames;
			changedNameVis = True;
			doc->changed = True;
		}
		deltaFirstIndent = pt2d(firstDist)-doc->dIndentFirst;
		if (deltaFirstIndent!=0) {
			doc->dIndentFirst = pt2d(firstDist);
			IndentSystems(doc, deltaFirstIndent, True);
		}

		if (otherNames!=doc->otherNames) {
			doc->otherNames = otherNames;
			changedNameVis = True;
			doc->changed = True;
		}
		deltaOtherIndent = pt2d(otherDist)-doc->dIndentOther;
		if (deltaOtherIndent!=0) {
			doc->dIndentOther = pt2d(otherDist);
			IndentSystems(doc, deltaOtherIndent, False);
		}

		if (deltaFirstIndent!=0 || deltaOtherIndent!=0 || changedNameVis)
			InvalWindow(doc);
	}
}


/*	Set global font characteristics. */
 
static void SMTextStyle(Document *doc)
{
	static Boolean firstCall=True;
	static Str255 string;
	Boolean okay;
	CONTEXT context;
	LINK firstMeas;

	firstMeas = LSSearch(doc->headL, MEASUREtype, ANYONE, False, False);
	GetContext(doc, firstMeas, 1, &context);

	if (firstCall) {
		GetIndString(tmpStr, MiscStringsID, 6);		/* Get default sample string */
		Pstrcpy(string,tmpStr);
		firstCall = False;
	}
	okay = DefineStyleDialog(doc, string, &context);
}


/* Get measure-numbering parameters for <doc> from user. */
	
static void SMMeasNum(Document *doc)
{
	if (MeasNumDialog(doc)) {
		EraseAndInvalMessageBox(doc);
		InvalWindow(doc);
	}
}


/* Respace selected measures to tightness set by user from dialog; or, if selection is
a single object (a clef or key signature) before the first measure of its system, set
the width of space before the first measure to a given amount. The latter case was
originally intended as a workaround for a bug that's been cropping up every now and then
for years, but it could also be useful to a user who simply wants more or less than the
standard amount of space at the beginning of a system. --DAB, July 2020 */

void RespaceBefFirstMeas(Document *doc, LINK pL);
void RespaceBefFirstMeas(Document *doc, LINK pL)
{
	static DDIST dSpace=999;
	LINK measL;

//LogPrintf(LOG_DEBUG, "RespaceBefFirstMeas: pL=%u\n", pL);
	dSpace = BefMeasSpaceDialog(doc, dSpace);
 	measL = SSearch(pL, MEASUREtype, GO_RIGHT);
	LogPrintf(LOG_INFO, "Changing space before first measure of system from %d to %d.  (RespaceBefFirstMeas)\n",
				LinkXD(measL), dSpace);
	LinkXD(measL) = dSpace;
	InvalSystem(measL);
	doc->changed = True;
}


static void SMRespace(Document *doc)
{
	short spMin, spMax, dval;

	/* Set the width of space before the first measure iff selection is a single object
	   and is before 1st measure of system. */
	
	if (doc->selEndL==RightLINK(doc->selStartL) && LinkBefFirstMeas(doc->selStartL)) {
		RespaceBefFirstMeas(doc, doc->selStartL);
		return;
	}
	
	GetMSpaceRange(doc, doc->selStartL, doc->selEndL, &spMin, &spMax);
	dval = SpaceDialog(RESPACE_DLOG, spMin, spMax);
	if (dval>0) {				
		WaitCursor();
		InitAntikink(doc, doc->selStartL, doc->selEndL);
		if (RespaceBars(doc, doc->selStartL, doc->selEndL, RESFACTOR*(long)dval, 
														True, False))
			doc->spacePercent = dval;
			
		/* FIXME: It would be much better not to Antikink if RespaceBars failed, but we
		   have no other way to free its data structures! Why can't RespaceBars just
		   free its data structures itself? */
			
		Antikink();
	}
}


/* For each object in the selection range with subobjects, align all of its subobjects
with its first selected subobject. */

static void SMRealign(Document *doc)
{
	LINK pL; Boolean didSomething=False;

	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (RealignObj(pL, True)) didSomething = True;
		
	if (didSomething) doc->changed = True;
	InvalSelRange(doc);
}


/* Reformat first System of selection thru last System of selection in accordance
with parameters set by user in dialog. */

static void SMReformat(Document *doc)
{
	static Boolean firstCall=True, changeSBreaks, careMeasPerSys, exactMPS,
						careSysPerPage, justify;
	short useMeasPerSys;
	static short measPerSys, sysPerPage, titleMargin;
	short endSysNum, status;
	LINK startSysL, endSysL;
	
	if (firstCall) {
		changeSBreaks = True;
		careMeasPerSys = exactMPS = False;
		measPerSys = 4;
		
		careSysPerPage = False;
		sysPerPage = 6;
		justify = False;
		titleMargin = config.titleMargin;
		
		firstCall = False;
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
	}
}


/* Invoke Add Modifiers dialog to add modifiers to all selected notes (for chords, really
to their main notes) or rests (fermata only). (JGG, 4/21/01) */
	
static void NMAddModifiers(Document *doc)
{
	LINK		objL, aNoteL, mainNoteL, aModNRL;
	short		voice;
	char		data = 0;
	static Byte	modCode = MOD_STACCATO;
	
	if (ModNRDialog(ADDMOD_DLOG, &modCode)) {
		WaitCursor();
		PrepareUndo(doc, doc->selStartL, U_AddMods, 49);    		/* "Add Modifiers" */
		for (objL=doc->selStartL; objL!=doc->selEndL; objL=RightLINK(objL))
			if (SyncTYPE(objL))
				for (aNoteL=FirstSubLINK(objL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSEL(aNoteL)) {
						if (NoteREST(aNoteL) && modCode!=MOD_FERMATA) continue;
							
						/* If part of a chord and not the main note, and the main
							note is selected, skip it. */
							
						voice = NoteVOICE(aNoteL);
						mainNoteL = FindMainNote(objL, voice);
						if (aNoteL!=mainNoteL && NoteSEL(mainNoteL)) continue;
						aModNRL = AutoNewModNR(doc, modCode, data, objL, aNoteL);
						if (aModNRL==NILINK) {
							MayErrMsg("NMAddModifiers: AutoNewModNR failed.");
							goto stop;
						}
						doc->changed = True;
					}
stop:
		InvalSelRange(doc);
		ArrowCursor();
	}
}


/* Strip all modifiers from all selected notes. */
	
static void NMStripModifiers(Document *doc)
{
	LINK objL, aNoteL;  PANOTE aNote;
	
	PrepareUndo(doc, doc->selStartL, U_StripMods, 21);    	/* "Remove Modifiers" */
	for (objL=doc->selStartL; objL!=doc->selEndL; objL=RightLINK(objL))
		if (SyncTYPE(objL))
			for (aNoteL=FirstSubLINK(objL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->selected && aNote->firstMod) {
					DelModsForSync(objL, aNoteL);
					aNote->firstMod = NILINK;
					doc->changed = True;
				}
			}
	InvalSelRange(doc);
}


static short CountSelVoices(Document *doc)
{
	Boolean voiceUsed[MAXVOICES+1];
	short v, nSelVoices;
	LINK pL, aNoteL, aGRNoteL;
	
	for (v = 1; v<=MAXVOICES; v++)
		voiceUsed[v] = False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					if (NoteSEL(aNoteL))
						voiceUsed[NoteVOICE(aNoteL)] = True;
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					if (GRNoteSEL(aGRNoteL))
						voiceUsed[GRNoteVOICE(aGRNoteL)] = True;
				}
				break;
			default:
				;
		}
		
	for (nSelVoices = 0, v = 1; v<=MAXVOICES; v++)
		if (voiceUsed[v]) nSelVoices++;
	return nSelVoices;
}

	
/* Apply multivoice notation rules as specified by user in dialog. */
	
static void NMMultiVoice(Document *doc)
{
	static short nSelVoices, voiceRole;
	static Boolean measMulti, assume;

	nSelVoices = CountSelVoices(doc);
	if (MultivoiceDialog(nSelVoices, &voiceRole, &measMulti, &assume))
		DoMultivoiceRules(doc, voiceRole, measMulti, assume);
}
	

/* Set logical and/or physical duration of selected notes/rests as specified by user. */
	
static void NMSetDuration(Document *doc)
{
	static short lDur=QTR_L_DUR, nDots=0, pDurPct=-1, lDurAction=SET_DURS_TO;
	static Boolean setLDur=True, setPDur=True, cptV=True;
	Boolean unbeam, didAnything=False;
	
	if (pDurPct<0) pDurPct = config.legatoPct;
	
	if (SetDurDialog(doc,&setLDur,&lDurAction,&lDur,&nDots,&setPDur,&pDurPct,&cptV,&unbeam)) {
		if (unbeam) Unbeam(doc);
		PrepareUndo(doc, doc->selStartL, U_SetDuration, 22);    	/* "Set Duration" */

		if (setLDur) {
			switch (lDurAction) {
				case HALVE_DURS:
					didAnything = SetDoubleHalveSelNoteDur(doc, False, False);
					break;
				case DOUBLE_DURS:
					didAnything = SetDoubleHalveSelNoteDur(doc, True, False);
					break;
				case SET_DURS_TO:
					didAnything = SetSelNoteDur(doc, lDur, nDots, False);
					break;
				default:
					break;
			}
			if (didAnything) {
				if (cptV) SetDurCptV(doc);		/* FIXME: Bug: this changes sel range, screwing up FixTimeStamps! */
				FixTimeStamps(doc, doc->selStartL, doc->selEndL);
				if (doc->autoRespace)
					RespaceBars(doc, doc->selStartL, doc->selEndL,
								RESFACTOR*(long)doc->spacePercent, False, False);
				else
					InvalMeasures(doc->selStartL, LeftLINK(doc->selEndL), ANYONE);	/* Force redrawing */
			}
		}
		
		if (setPDur)
			SetPDur(doc, pDurPct, True);
	}
}

	
/* Choose whether notes should be inserted graphically (by horizontal position) or
temporally (based on elapsed time in the measure for the voice). */

static void NMInsertByPos(Document *doc)
{
	doc->insertMode = !doc->insertMode;
}


/* Set multibar rests to the number of measures specified by user. */
	
static void NMSetMBRest(Document *doc)
{
	static short newNMeas=4;

	if (SetMBRestDialog(doc, &newNMeas)) {
		PrepareUndo(doc, doc->selStartL, U_SetDuration, 23);    	/* "Set Multibar Rest" */
		if (SetSelMBRest(doc, newNMeas)) {
			FixTimeStamps(doc, doc->selStartL, doc->selEndL);
			UpdateMeasNums(doc, NILINK);
			if (doc->autoRespace)
				RespaceBars(doc, doc->selStartL, doc->selEndL,
							RESFACTOR*(long)doc->spacePercent, False, False);
			else
				InvalMeasures(doc->selStartL, LeftLINK(doc->selEndL), ANYONE);	/* Force redrawing */
		}
	}
}


/* Fill empty measures in user-specified range with whole-measure rests. */
	
static void NMFillEmptyMeas(Document *doc)
{
	short startMN, endMN, nFilled;
	LINK startMeasL, endMeasL, aMeasureL, startL, endL;
	
	/* Get default start and end measures from the selection. */
	
	startMeasL = SSearch(doc->selStartL, MEASUREtype, GO_LEFT);		/* starting measure */
	if (startMeasL==NILINK) SSearch(doc->selStartL, MEASUREtype, GO_RIGHT);
	aMeasureL = FirstSubLINK(startMeasL);
	startMN = MeasMEASURENUM(aMeasureL)+doc->firstMNNumber;
	
	endMeasL = SSearch(doc->selEndL, MEASUREtype, GO_LEFT);			/* ending measure */
	aMeasureL = FirstSubLINK(endMeasL);
	endMN = MeasMEASURENUM(aMeasureL)+doc->firstMNNumber;

	if (FillEmptyDialog(doc, &startMN, &endMN)) {
		startL = MNSearch(doc, doc->headL, startMN-doc->firstMNNumber, GO_RIGHT, True);
		endL = MNSearch(doc, doc->headL, endMN-doc->firstMNNumber, GO_RIGHT, True);
		if (startL && endL) {
			/* We have to change the selection range temporarily so PrepareUndo() knows
				what to do. */
				
			LINK saveSelStartL, saveSelEndL;
			saveSelStartL = doc->selStartL; saveSelEndL = doc->selEndL;
			doc->selStartL = startL; doc->selEndL = endL;
			PrepareUndo(doc, doc->selStartL, U_FillEmptyMeas, 24);	/* "Fill Empty Measures" */
			doc->selStartL = saveSelStartL; doc->selEndL = saveSelEndL;

			WaitCursor();
			nFilled = FillEmptyMeas(doc, startL, endL);
			if (nFilled>0) {
				LogPrintf(LOG_INFO, "Filled %d empty staff-measure(s). startMN=%d endMN=%d  (NMFillEmptyMeas)\n",
							nFilled, startMN, endMN);
				FixTimeStamps(doc, startL, endL);
				UpdateMeasNums(doc, NILINK);
			}
			else
				LogPrintf(LOG_INFO, "No empty staff-measure(s) to fill. startMN=%d endMN=%d  (NMFillEmptyMeas)\n",
							startMN, endMN);
		}
	}
}


/* --------------------------------------------------------------- View menu helpers -- */

/* Get voice to look at from dialog, initialized with voice and part of first selected
object or, if nothing is selected, default voice and part of the insertion point's
staff. In either case, add the user-specified voice to the voice mapping table and
set document's <lookVoice> to it. */
 
static void VMLookAt()
{
	short		dval, newLookV, userVoice, saveStaff;
	Document	*doc=GetDocumentFromWindow(TopDocument);
	LINK		partL;
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

	
/* Look at all voices. */

static void VMLookAtAll()
{
	Document *doc=GetDocumentFromWindow(TopDocument);

	if (doc) {
		doc->lookVoice = ANYONE;
		InvalWindow(doc);
		EraseAndInvalMessageBox(doc);
		}
}


/* Toggle showing/hiding measure duration problems. */

static void VMShowDurProblems()
{
	Document *doc=GetDocumentFromWindow(TopDocument);

	if (doc) {
		doc->showDurProb = !doc->showDurProb;
		CheckMenuItem(viewMenu, VM_ShowDurProb, doc->showDurProb);
		InvalWindow(doc);
	}
}


/* Toggle showing/hiding Sync lines. */

static void VMShowSyncs()
{
	Document *doc=GetDocumentFromWindow(TopDocument);

	if (doc) {
		doc->showSyncs = !doc->showSyncs;
		CheckMenuItem(viewMenu, VM_ShowSyncL, doc->showSyncs);
		InvalWindow(doc);
	}
}

	
/* Toggle showing/hiding invisible symbols. */

static void VMShowInvisibles()
{
	Document *doc=GetDocumentFromWindow(TopDocument);

	if (doc) {
		doc->showInvis = !doc->showInvis;
		CheckMenuItem(viewMenu, VM_ShowInvis, doc->showInvis);
		InvalWindow(doc);
	}
}


/*	Set coloring of voices: all voices in color, non-default voices in color, or
everything in black. The UI is kludgly, and it makes setting COLORVOICES_NONDEFLT
difficult -- but that mode is rarely used. */

static void VMColorVoices()
{
	Document *doc=GetDocumentFromWindow(TopDocument);

	if (doc) {
		if (doc->colorVoices==COLORVOICES_NONE)
			doc->colorVoices = ((ShiftKeyDown() && OptionKeyDown())?
									COLORVOICES_NONDEFLT : COLORVOICES_ALL);
		else
			doc->colorVoices = COLORVOICES_NONE;
		CheckMenuItem(viewMenu, VM_ColorVoices, doc->colorVoices);
		InvalWindow(doc);
	}
}


/* Set "graph mode": show score in "normal" pure CWMN, CWMN but with noteheads replaced
by tiny graphs, or pianoroll. */
 
#define ASSUME_NHGRAPHS	0


enum {
	BUT1_OK=1,
	NHGRAPH_DI=3,
	PIANOROLL_DI,
	ASSUME_DI=6
};

/* Ask user whether they want notehead graphs or pianoroll. Returns True for notehead
graphs, False for pianoroll. */

static short group1;

static Boolean ChooseNHGraphMode();
static Boolean ChooseNHGraphMode()
{
	short itemHit;
	Boolean value;
	static short assumeNHGraphPR=0, oldChoice=0;
	DialogPtr dlog;  GrafPtr oldPort;
	Boolean keepGoing=True;
	static Boolean firstCall=True;

	if (firstCall) {
		if (ASSUME_NHGRAPHS==1)			assumeNHGraphPR = NHGRAPH_DI;
		else if (ASSUME_NHGRAPHS==2)	assumeNHGraphPR = PIANOROLL_DI;
		else							assumeNHGraphPR = 0;
		firstCall = False;
	}
	
	if (assumeNHGraphPR)
		value = (assumeNHGraphPR==NHGRAPH_DI);
	else {
	
		/* Build dialog window and install its item values */
		
		ModalFilterUPP	filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP==NULL) {
			MissingDialog(GRAPHMODE_DLOG);
			return False;
		}
		GetPort(&oldPort);
		dlog = GetNewDialog(GRAPHMODE_DLOG, NULL, BRING_TO_FRONT);
		if (dlog==NULL) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(GRAPHMODE_DLOG);
			return False;
		}
		SetPort(GetDialogWindowPort(dlog));
	
		/* Fill in dialog's values here */
	
		group1 = (oldChoice? oldChoice : NHGRAPH_DI);
		PutDlgChkRadio(dlog, NHGRAPH_DI, (group1==NHGRAPH_DI));
		PutDlgChkRadio(dlog, PIANOROLL_DI, (group1==PIANOROLL_DI));
	
		PlaceWindow(GetDialogWindow(dlog), (WindowPtr)NULL, 0, 80);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		/* Entertain filtered user events until dialog is dismissed */
		
		while (keepGoing) {
			ModalDialog(filterUPP, &itemHit);
			switch(itemHit) {
				case BUT1_OK:
					keepGoing = False;
					value = (group1==NHGRAPH_DI);
					if (GetDlgChkRadio(dlog, ASSUME_DI)) assumeNHGraphPR = group1;
					oldChoice = group1;
					break;
				case NHGRAPH_DI:
				case PIANOROLL_DI:
					if (itemHit!=group1) SwitchRadio(dlog, &group1, itemHit);
					break;
				case ASSUME_DI:
					PutDlgChkRadio(dlog, ASSUME_DI, !GetDlgChkRadio(dlog, ASSUME_DI));
					break;
				default:
					;
			}
		}
		
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
	}
	
	return value;
}

static void VMGraphMode()
{
	Document *doc=GetDocumentFromWindow(TopDocument);

	if (doc) {
		if (doc->graphMode==GRAPHMODE_NORMAL) {
			doc->graphMode = (ChooseNHGraphMode()?
									GRAPHMODE_NHGRAPHS : GRAPHMODE_PIANOROLL);
		}
		else
			doc->graphMode = GRAPHMODE_NORMAL;
		CheckMenuItem(viewMenu, VM_GraphMode, doc->graphMode);
		InvalRangeContent(doc->headL, doc->tailL);
		InvalWindow(doc);
	}
}


static void VMShowToolPalette()
{
	short palIndex;
	Rect screen, pal, docRect;

	palIndex = TOOL_PALETTE;
	GetWindowPortBounds(palettes[palIndex], &pal);
	OffsetRect(&pal, -pal.left, -pal.right);
	if (TopDocument) {
		/* Initial position is the offset in config.toolsPosition from the upper
		   left corner of the document's window, but not offscreen. */
							 
		GetWindowRgnBounds(TopDocument, kWindowContentRgn, &docRect);					
		docRect.right = docRect.left + pal.right;
		docRect.bottom = docRect.top + pal.bottom;
		GetMyScreen(&docRect, &screen);
		OffsetRect(&docRect, config.toolsPosition.h, config.toolsPosition.v);
		PullInsideRect(&docRect, &screen, 2);
//LogPrintf(LOG_DEBUG, "VMShowToolPalette: docRect.top=%d\n", docRect.top);  // FIX ISSUE 67
	}
	else {
		/* Initial position is upper left corner of main screen */
		
		docRect = GetQDScreenBitsBounds();
		InsetRect(&docRect, 10, GetMBarHeight()+16);
//LogPrintf(LOG_DEBUG, "VMShowToolPalette: docRect.top=%d GetMBarHeight()=%d\n", docRect.top,
//GetMBarHeight());														// FIX ISSUE 67
	}
	(*paletteGlobals[palIndex])->position.h = docRect.left;
	(*paletteGlobals[palIndex])->position.v = docRect.top;
	MovePalette(palettes[palIndex],(*paletteGlobals[palIndex])->position);
	palettesVisible[TOOL_PALETTE] = True;
}


#define SPEC_DOC_COUNT 1	/* No. of special Documents (e.g., clipboard) at start of <documentTable> */

static void VMActivate(short choice)
{
	short itemNum;  Document *doc;

	/* The items at the end of the View menu (after the dividing line after VM_LastItem)
	   correspond to Documents in documentTable. Search for the Document we want,
	   starting after any special Documents, which do not appear in the list. */
	
	for (itemNum = VM_LastItem+2, doc = documentTable+SPEC_DOC_COUNT; doc<topTable; doc++)
		if (doc->inUse) {
			if (itemNum==choice)	{ SelectWindow(doc->theWindow); break; }
			else					itemNum++;
		}
}


/* --------------------------------------------------------------------- Other helpers -- */


static void PLSetPlayDuration(Document *doc)
{
	static short plDurPct=-1;
	
	if (plDurPct<0) plDurPct = config.legatoPct;
	
	if (SetPlayDurDialog(doc, &plDurPct)) {
		//PrepareUndo(doc, doc->selStartL, U_SetPlayDuration, ??);    	/* "Set Play Duration" */
		
LogPrintf(LOG_DEBUG, "PLSetPlayDuration: plDurPct=%d\n", plDurPct);

		SetPDur(doc, plDurPct, True);
	}
}


static Boolean OKToRecord(Document *doc)
{
	short anInt;  LINK lookPartL, selPartL;
	char str[256];
	
	if (HasSmthgAcross(doc, doc->selStartL, str)) {
		CParamText(str, "", "", "");
		CautionAlert(EXTOBJ_ALRT, NULL);
		return False;
	}
	
	if (doc->lookVoice>=0) {
		Int2UserVoice(doc, doc->lookVoice, &anInt, &lookPartL);
		Int2UserVoice(doc, doc->selStaff, &anInt, &selPartL);		/* since staff no.=default voice no. */
		if (lookPartL!=selPartL) {
			GetIndCString(strBuf, MENUCMDMSGS_STRS, 3);				/* "You can't Record onto a staff in one part while Looking..." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return False;
		}
	}
	
	return True;
}


/* Record MIDI data in real time. */

static void PLRecord(Document *doc, Boolean merge)
{
	if (!OKToRecord(doc)) return;

	LogPrintf(LOG_INFO, "0. Starting to record.  (PLRecord)\n");
	
	if (merge) {
		PrepareUndo(doc, doc->selStartL, U_RecordMerge, 25);		/* "Record Merge" */
		RecTransMerge(doc);
	}
	else {
		PrepareUndo(doc, doc->selStartL, U_Record, 26);				/* "Record" */
		
		LogPrintf(LOG_INFO, "0.1. Ready to record.  (PLRecord)\n");
		Record(doc);
	}
}


/* Step-record MIDI data. */

static void PLStepRecord(Document *doc, Boolean merge)
{
	short spaceProp;
	
	if (!OKToRecord(doc)) return;

	PrepareUndo(doc, doc->selStartL, U_Record, 27);					/* "Step Record" */
	if (StepRecord(doc, merge)) {
		spaceProp = MeasSpaceProp(doc->selStartL);
		
		/* Reformatting (even if done by RespaceBars calling Reformat) may destroy
		   selection status, so we use notes' tempFlags to save and restore it. */
		 
		NotesSel2TempFlags(doc);
		if (merge) {
			if (RespaceBars(doc, doc->selStartL, doc->selEndL, spaceProp, False, True))
				TempFlags2NotesSel(doc);
		}
		else {
			if (RespAndRfmtRaw(doc, doc->selStartL, doc->selEndL, spaceProp))
				TempFlags2NotesSel(doc);
		}
	}
}

/* When activating a document, ensure that the correct menu is installed for the current
mode of viewing that document: Play/Record, Master Page, or Show Format menu. */

void InstallDocMenus(Document *doc)
{
	short mid;  MenuHandle curMh, mh;

	if ((curMh = GetMenuHandle(playRecID))) DeleteMenu(playRecID);
	 else if ((curMh = GetMenuHandle(masterPgID))) DeleteMenu(masterPgID);
	 else if ((curMh = GetMenuHandle(formatID))) DeleteMenu(formatID);

	/* GetMenuHandle may give NULL for the new menu (if that menu was never installed?
	   I dunno), so instead pass one of our stored menu handles to InsertMenu. */
	 
	if (doc->masterView)		{ mid = masterPgID; mh = masterPgMenu; }
	else if (doc->showFormat)	{ mid = formatID; mh = formatMenu; }
	else						{ mid = playRecID; mh = playRecMenu; }
	
	InsertMenu(mh, 0);
	DrawMenuBar();

	if (curMh) {
		/* Flash to call user's attention to the fact that there's a new menu. */
		
		HiliteMenu(mid);
		SleepTicks(1);
		HiliteMenu(0);
	}
}


static void InstallDebugMenuItems(Boolean installAll)
{
	if (debugItemsNeedInstall) {
		LogPrintf(LOG_INFO, "Installing debug menu items...  (InstallDebugMenuItems)\n");
		AppendMenu(editMenu, "\p(-");
		AppendMenu(editMenu, "\pBrowser");
		if (installAll) {
			AppendMenu(editMenu, "\pDebug...");
			AppendMenu(editMenu, "\pDelete Objects");
		}
	}
	else {
		LogPrintf(LOG_INFO, "Removing debug menu items...  (InstallDebugMenuItems)\n");
		DeleteMenuItem(editMenu, EM_DeleteObjs);			
		DeleteMenuItem(editMenu, EM_Debug);
		DeleteMenuItem(editMenu, EM_Browser);
		DeleteMenuItem(editMenu, EM_Browser-1);
	}
	
	debugItemsNeedInstall = !debugItemsNeedInstall;
}


/* ============================================================ MENU-UPDATING ROUTINES == */

/* Force all menus to reflect internal state. This should be called when we're about to
let user peruse them. */

void FixMenus()
	{
		WindowPtr w;
		Document *theDoc;
		short nInRange=0, nSel=0;
		LINK firstMeasL;
		Boolean continSel=False;
		
		w = TopWindow;
		if (w==NULL)	theDoc = NULL;
		 else			theDoc = GetDocumentFromWindow(TopDocument);
		
		/* Precompute information about the current score and the clipboard for the
		   benefit of the various menu fixing routines about to be called. We have
		   to install theDoc first because, if the active window has changed, the
		   resulting Activate event (which will install it) may not have been handled
		   yet. */
		
		if (theDoc) {
			if (theDoc!=clipboard ) {
				InstallDoc(theDoc);
				CountSelection(theDoc, &nInRange, &nSel);
				beforeFirst = BeforeFirstMeas(theDoc->selStartL);

				continSel = ContinSelection(theDoc, config.strictContin!=0);
				theDoc->canCutCopy = continSel;
				}
			 else {								/* Clipboard is top document */
			 	nInRange = nSel = 0;
			 	beforeFirst = False;
			 }
			InstallDoc(clipboard);
			firstMeasL = SSearch(clipboard->headL, MEASUREtype, False);
			clipboard->canCutCopy = RightLINK(firstMeasL)!=clipboard->tailL;
			InstallDoc(theDoc);
			}
		
		FixFileMenu(theDoc, nSel);
		FixEditMenu(theDoc, nInRange, nSel);
		FixTestMenu(theDoc, nSel);
		FixScoreMenu(theDoc, nSel);
		FixNotesMenu(theDoc, continSel);
		FixGroupsMenu(theDoc, continSel);
		FixViewMenu(theDoc);
		FixPlayRecordMenu(theDoc, nSel);
		FixMasterPgMenu(theDoc);
		FixFormatMenu(theDoc);
		
		UpdateMenuBar();		/* Will do anything only if UpdateMenu was called by above */
	}

/* Enable or disable all items in the File menu */

static void FixFileMenu(Document *doc, short nSel)
	{
		//always disable Finale ETF import until it has been removed from menu (rsrc) entirely
		XableItem(fileMenu,FM_GetETF,False);        
        
		XableItem(fileMenu,FM_Close,doc!=NULL);
		XableItem(fileMenu,FM_CloseAll,doc!=NULL);

		XableItem(fileMenu,FM_Save,doc!=NULL && !doc->readOnly && doc->changed
										&& doc!=clipboard && !doc->masterView);
		XableItem(fileMenu,FM_SaveAs,doc!=NULL && doc!=clipboard && !doc->masterView);
		XableItem(fileMenu,FM_Extract,doc!=NULL && doc!=clipboard);

		// always disable NoteScan import until it has been removed from menu (rsrc) entirely
		XableItem(fileMenu,FM_GetScan,False);
		XableItem(fileMenu,FM_SavePostScript,doc!=NULL && doc!=clipboard
										&& !doc->masterView && !doc->showFormat);
		XableItem(fileMenu,FM_SaveText,doc!=NULL && doc!=clipboard
										&& !doc->masterView && !doc->showFormat && nSel>0);
		XableItem(fileMenu,FM_Revert,doc!=NULL && doc->changed && !doc->readOnly &&
										doc!=clipboard && doc->named &&
										!doc->converted);
		XableItem(fileMenu,FM_Export,doc!=NULL && doc!=clipboard && !doc->masterView);
		XableItem(fileMenu,FM_OpenMidiMap,doc!=NULL && doc!=clipboard);
		XableItem(fileMenu,FM_SheetSetup,doc!=NULL && doc!=clipboard && !doc->overview);
		XableItem(fileMenu,FM_PageSetup,doc!=NULL && doc!=clipboard);
		XableItem(fileMenu,FM_Print,doc!=NULL && doc!=clipboard && !doc->showFormat);

		XableItem(fileMenu,FM_ScoreInfo,doc!=NULL);
		XableItem(fileMenu,FM_Preferences,doc!=NULL && doc!=clipboard);
#ifdef TEST_SEARCHFILES_NG
		XableItem(fileMenu, FM_SearchInFiles, ??);        
#else
		XableItem(fileMenu, FM_SearchInFiles, False);        
#endif
	}


static short CountSelPages(Document *);
static short CountSelPages(Document *doc)
	{
		Boolean selAcrossFirst;  short nPages;  LINK pL;
		
		if (!doc) return 0;
		
		/* Count one Page that's not in the selection range for the Page the selection
		   starts on, unless the selection starts on or before the first Page and ends
		   after the first Page of the score. */
		   
		selAcrossFirst = False;	
		for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		
			/* selStartL is in the selection, so test it before considering breaking */
			
			if (pL==doc->selStartL) selAcrossFirst = True;
			if (PageTYPE(pL)) break;
			if (pL==doc->selEndL) selAcrossFirst = False;
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

/* Enable or disable all items in the Edit menu; disable entire menu if we're looking
at the Master Page or Showing Format. */

static void FixEditMenu(Document *doc, short /*nInRange*/, short nSel)
	{
		Boolean mergeable,enablePaste,enableClearSys,enableClearPage;
		char str[256], undoMenuItem[256], fmtStr[256];
		short nPages;

		UpdateMenu(editMenu, doc!=NULL && !doc->masterView && !doc->showFormat);

		/* Unlike most other menus, we can't just disable the whole menu if there's
		   no score open, so the code below has to work even if doc==NULL!
		
		   Handle clipboard commands if either doc==NULL, or it exists and is in the
		   normal view (not Master Page or Work on Format). */
		
		if (doc==NULL || (!doc->masterView && !doc->showFormat)) {
			XableItem(editMenu, EM_Undo, False || 
				(doc!=NULL && doc->undo.lastCommand!=U_NoOp && doc->undo.canUndo));
			if (doc) {
				GetUndoString(doc, undoMenuItem);
				SetMenuItemCText(editMenu, EM_Undo, undoMenuItem);
			}
	
			XableItem(editMenu,EM_Cut,False ||
				(doc!=NULL && doc!=clipboard && doc->canCutCopy && !beforeFirst));
			XableItem(editMenu,EM_Copy,False ||
				(doc!=NULL && doc!=clipboard && doc->canCutCopy && !beforeFirst));
			XableItem(editMenu,EM_Clear,False ||
				(doc!=NULL && doc!=clipboard && doc->canCutCopy
						&& BFSelClearable(doc, beforeFirst)));
					
			nPages = CountSelPages(doc);
			if (nPages>1) {
				GetIndCString(fmtStr, MENUCMD_STRS, 1);					/* "Copy %d Pages" */
				sprintf(str, fmtStr, nPages);
			}
			else
				GetIndCString(str, MENUCMD_STRS, 2);					/* "Copy Page" */
			SetMenuItemCText(editMenu, EM_CopyPage, str);
			if (nPages>1) {
				GetIndCString(fmtStr, MENUCMD_STRS, 21);				/* "Clear %d Pages" */
				sprintf(str, fmtStr, nPages);
			}
			else
				GetIndCString(str, MENUCMD_STRS, 22);					/* "Clear Page" */
			SetMenuItemCText(editMenu, EM_ClearPage, str);

			switch (lastCopy) {
				case COPYTYPE_CONTENT:
					GetIndCString(str, MENUCMD_STRS, 3);				/* "Paste Insert" */
					SetMenuItemCText(editMenu, EM_Paste, str);
					enablePaste = (doc!=NULL && doc!=clipboard && clipboard->canCutCopy
									&& !beforeFirst);
					XableItem(editMenu, EM_Paste, enablePaste);
					break;
				case COPYTYPE_SYSTEM:
					GetIndCString(str, MENUCMD_STRS, 4);				/* "Paste System" */
					SetMenuItemCText(editMenu, EM_Paste, str);
					enablePaste = (doc!=NULL && doc!=clipboard && !beforeFirst);
					XableItem(editMenu, EM_Paste, enablePaste);
					break;
				case COPYTYPE_PAGE:
					nPages = clipboard->numSheets;
					if (nPages>1) {
						GetIndCString(fmtStr, MENUCMD_STRS, 5);			/* "Paste %d Pages" */
						sprintf(str, fmtStr, nPages);
					}
					else
						GetIndCString(str, MENUCMD_STRS, 6);			/* "Paste Page" */
					SetMenuItemCText(editMenu, EM_Paste, str);
					enablePaste = (doc!=NULL && doc!=clipboard && !beforeFirst);
					XableItem(editMenu, EM_Paste, enablePaste);
					break;
			}
													
			mergeable = (lastCopy==COPYTYPE_CONTENT);
			XableItem(editMenu, EM_Merge, (doc!=NULL && doc!=clipboard && mergeable &&
												clipboard->canCutCopy && !beforeFirst));
			XableItem(editMenu, EM_Double,
				(doc!=NULL && doc!=clipboard && !beforeFirst && nSel>0));

			enableClearSys = False;
			
			if (doc) {
				LINK firstMeasL;

				firstMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);
				if (IsAfter(doc->selStartL,firstMeasL))
					enableClearSys = False;
				else {
					enableClearSys = (doc!=NULL && doc!=clipboard);
				}
			}

			XableItem(editMenu, EM_ClearSystem, enableClearSys);
			XableItem(editMenu, EM_CopySystem, doc!=NULL && doc!=clipboard);
			
			enableClearPage = False;
			
			if (doc) {
				LINK firstMeasL;

				firstMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);
				if (IsAfter(doc->selStartL,firstMeasL))
					enableClearPage = False;
				else {
					enableClearPage = (doc!=NULL && doc!=clipboard);
				}
			}

			XableItem(editMenu, EM_ClearPage, enableClearPage);
			XableItem(editMenu, EM_CopyPage, doc!=NULL && doc!=clipboard);

			XableItem(editMenu, EM_SelAll,
				(doc!=NULL && doc!=clipboard && (RightLINK(doc->headL)!=doc->tailL)));
			XableItem(editMenu, EM_ExtendSel,
				(doc!=NULL && doc!=clipboard && nSel>0));

			XableItem(editMenu, EM_Set, doc!=clipboard && IsSetEnabled(doc));

			XableItem(editMenu, EM_GetInfo, doc!=NULL && nSel==1);

			if (doc==NULL || nSel!=1)
				XableItem(editMenu, EM_ModifierInfo, False);
			else
				XableItem(editMenu, EM_ModifierInfo, SyncTYPE(doc->selStartL));

#ifdef SEARCH_CONTENT
		XableItem(editMenu, EM_SearchMelody, searchPatDoc!=NULL);
		XableItem(editMenu, EM_SearchAgain, searchPatDoc!=NULL);        
#else
		XableItem(editMenu, EM_SearchMelody, False);        
		XableItem(editMenu, EM_SearchAgain, False);        
#endif
		}
	}

static void FixTestMenu(Document *doc, short nSel)
	{
#ifndef PUBLIC_VERSION
		if (clickMode==ClickErase)
			SetItemMark(testMenu, TS_ClickErase, '*');
		else
			SetItemMark(testMenu, TS_ClickErase, noMark);
		if (clickMode==ClickSelect)
			SetItemMark(testMenu, TS_ClickSelect, '*');
		else
			SetItemMark(testMenu, TS_ClickSelect, noMark);
		if (clickMode==ClickFrame)
			SetItemMark(testMenu, TS_ClickFrame, '*');
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
		LINK pageL, sysL, startBarL, endBarL, nextBarL, measAfterSelStart;
		Boolean measSysEnd, moveMeasUp;

		if (doc->masterView) {
			DisableSMMove();
			return;
		}

		/* Enable "Move Measures Up" iff selStartL and LeftLINK(selEndL) are in the same
		   System; that System is not the first; selStartL is in the first Measure of
		   that	System; and there's at least one Measure in the System after selStartL.
		   Enable "Move Measures Down" iff selStartL and LeftLINK(selEndL) are in the
		   same System, and LeftLINK(selEndL) is in the last Measure of that System. */

		if (!SameSystem(doc->selStartL, LeftLINK(doc->selEndL))) {
			DisableSMMove();
			return;
		}
		
		sysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_LEFT, False);

		if (!sysL) {
			DisableSMMove();
			return;
		}

		startBarL = NILINK;
		if (LinkLSYS(sysL)!=NILINK)
			startBarL = LSSearch(LeftLINK(doc->selStartL), MEASUREtype, ANYONE, GO_LEFT, False);

		if (startBarL && LinkLSYS(sysL)!=NILINK) {
			nextBarL = LinkRMEAS(startBarL);
			measAfterSelStart = nextBarL ? (MeasSYSL(nextBarL)==sysL) : False;
			moveMeasUp = ( !beforeFirst && FirstMeasInSys(startBarL) &&
					 			IsAfter(sysL,startBarL) && measAfterSelStart);
			XableItem(scoreMenu, SM_MoveMeasUp, moveMeasUp);
		}
		else
			DisableMenuItem(scoreMenu, SM_MoveMeasUp);

		endBarL = LSISearch(LeftLINK(doc->selEndL), MEASUREtype, ANYONE, GO_LEFT, False);
		measSysEnd = False;
		if (endBarL)
			measSysEnd = IsLastUsedMeasInSys(doc, endBarL);
		XableItem(scoreMenu, SM_MoveMeasDown, !beforeFirst && endBarL && measSysEnd);

		/* Enable "Move System Up" iff selStartL and selEndL are in the same System, that
		   System is the first in its Page, and that Page is not the first in the score.

		   Enable "Move System Down" iff selStartL and selEndL are in the same System, that
		   System is the last in its Page, and that Page is not the last in the score. */

		pageL = SysPAGE(sysL);
		XableItem(scoreMenu, SM_MoveSystemUp,
			(FirstSysInPage(sysL) && LinkLPAGE(pageL)!=NILINK));
		XableItem(scoreMenu, SM_MoveSystemDown, 
			(IsLastSysInPage(sysL) && LinkRPAGE(pageL)!=NILINK));
	}

/* Fix all items in the Score menu; disable it entirely if there is no score or <doc> is
the clipboard. */

static void FixScoreMenu(Document *doc, short nSel)
	{
		LINK insertL; 
		short where;
		Str255 str;
	
		UpdateMenu(scoreMenu, doc!=NULL && doc!=clipboard);
		
		if (doc==NULL || doc==clipboard) return;

		/* Context switching directly between showFormat and masterView presents as-yet
		   unanalyzed problems, so for now, prevent user from going directly from
		   either one to the other. */

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
				IsAfter(insertL, doc->selStartL)? 10 : 11);			/* "Add Page Before/After" */
			SetMenuItemText(scoreMenu, SM_AddPage, str);
			EnableMenuItem(scoreMenu, SM_AddPage);
			}
		else {
			GetIndString(str, MENUCMD_STRS, 12);					/* "Add Page" */
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

static LINK MBRestSel(Document *doc)
{
	LINK	pL, aNoteL;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteREST(aNoteL))
					if (NoteType(aNoteL)<=WHOLE_L_DUR) return pL;
			
	return NILINK;
}

/* Enable or disable all items in the Notes menu; disable entire menu if there is no
score, if it's the clipboard, or if we're looking at the Master Page. */

static void FixNotesMenu(Document *doc, Boolean /*continSel*/)
	{
		Boolean noteSel, GRnoteSel, keySigSel, chordSymSel, slurSel, disableWholeMenu;

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
		if (!noteSel)	DisableMenuItem(notesMenu, NM_SetMBRest);
		else			XableItem(notesMenu, NM_SetMBRest, (MBRestSel(doc)!=NILINK));

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


static Boolean SelRangeChkOct(short staff, LINK staffStartL, LINK staffEndL)
{
	LINK pL, aNoteL, aGRNoteL;
	PANOTE aNote;  PAGRNOTE aGRNote;
	
	for (pL = staffStartL; pL!=staffEndL; pL = RightLINK(pL))
		if (LinkSEL(pL))
			if (SyncTYPE(pL)) {
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staff) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->inOttava) return True;
						else break;
					}
				}
			else if (GRSyncTYPE(pL)) {
				for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					if (GRNoteSTAFF(aGRNoteL)==staff) {
						aGRNote = GetPAGRNOTE(aGRNoteL);
						if (aGRNote->inOttava) return True;
						else break;
					}
			}

	return False;
}

static void FixBeamCommands(Document *);
static void FixTupletCommands(Document *);
static void FixOttavaCommands(Document *, Boolean);

/* Handle menu command Enable/Disable for Beams */

static void FixBeamCommands(Document *doc)
{
	Boolean hasBeam, hasGRBeam; LINK pL, aNoteL, aGRNoteL;
	short voice, nBeamable, nGRBeamable; Str255 str;
	
	hasBeam = False;			
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteBEAMED(aNoteL))
					{ hasBeam = True; goto knowBeamed; }
		
knowBeamed:
	/*
	 * Handle menu Enable/Disable for grace note beams: if range is
	 * either unBeam-able or unGRBeam-able, Enable GM_Unbeam; if range
	 * is beamable/not beamable, Xable accordingly; then check for
	 * GRbeam-able; if it is, enable it.
	 */
	hasGRBeam = False;			
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && GRSyncTYPE(pL))
			for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
				if (GRNoteSEL(aGRNoteL) && GRNoteBEAMED(aGRNoteL))
					{ hasGRBeam = True; goto knowGRBeamed; }

knowGRBeamed:
	/* Enable/disable the Break Beam cmd (easy) and the Beam/Unbeam cmd (not easy). */
	
	XableItem(groupsMenu, GM_BreakBeam, hasBeam);
	XableItem(groupsMenu, GM_FlipFractional, hasBeam);
	
	DisableMenuItem(groupsMenu, GM_Beam);
	
	if (hasBeam || hasGRBeam) {
		cmdIsBeam = False;
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
				nBeamable = CountBeamable(doc, doc->selStartL, doc->selEndL, voice, True);
				if (nBeamable > 1) break;
			}
		}
	nGRBeamable = 0;
	if (!beforeFirst)						/* Selection doesn't start before 1st measure? */
		for (voice=1; voice<=MAXVOICES; voice++) {
			if (VOICE_MAYBE_USED(doc, voice)) {
				nGRBeamable = CountGRBeamable(doc, doc->selStartL, doc->selEndL, voice, True);
				if (nGRBeamable > 1) break;
			}
		}

	if (nBeamable>1 || nGRBeamable>1) {
		cmdIsBeam = True;
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
	short voice, tupleNum; Str255 str;
	
	hasTuplet = False;
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteINTUPLET(aNoteL))
					{ hasTuplet = True; goto knowTupled; }
					
knowTupled:
	if (hasTuplet) {
		cmdIsTuplet = False;
		GetIndString(str, MENUCMD_STRS, 15);							/* "Remove Tuplet" */
		SetMenuItemText(groupsMenu, GM_Tuplet, str);
		EnableMenuItem(groupsMenu, GM_Tuplet);
		DisableMenuItem(groupsMenu, GM_FancyTuplet);
		return;
	}
	
	/* No tupleted notes are selected, so nothing to unbeam. Is there anything to
	   tuple? First, no tupleting is allowed for selections across measure boundaries. */
	   
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (MeasureTYPE(pL)) {
			DisableMenuItem(groupsMenu, GM_Tuplet);
			DisableMenuItem(groupsMenu, GM_FancyTuplet);
			return;
		}
		
	/* For all voices, if there is a tuplable range (tupleNum>=2), enable the menu items. */
	tupleNum = 0;
	if (!beforeFirst)
		for (voice=1; voice<=MAXVOICES; voice++) {
			if (VOICE_MAYBE_USED(doc, voice)) {
				GetNoteSelRange(doc,voice,&vStartL,&vEndL, NOTES_ONLY);
				tupleNum = (vStartL && vEndL) ? GetTupleNum(vStartL,vEndL,voice,True) : 0;
				if (tupleNum>=2) break;
			}
		}

	if (tupleNum>=2) {
		cmdIsTuplet = True;
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

static void FixOttavaCommands(Document *doc, Boolean continSel)
{
	Boolean hasOttava; LINK pL, aNoteL, aGRNoteL, stfStartL, stfEndL;
	short staff, ottavaNum; Str255 str;

	if (!continSel) {
		DisableMenuItem(groupsMenu, GM_Ottava);
		return;
	}
	
	hasOttava = False;		
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteINOTTAVA(aNoteL))
					{ hasOttava = True; goto knowOttavad; }
		if (LinkSEL(pL) && GRSyncTYPE(pL))
			for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
				if (GRNoteSEL(aGRNoteL) && GRNoteINOTTAVA(aGRNoteL))
					{ hasOttava = True; goto knowOttavad; }				
	}
					
knowOttavad:
	if (hasOttava) {
		cmdIsOttava = False;
		GetIndString(str, MENUCMD_STRS, 17);							/* "Remove Octave Sign" */
		SetMenuItemText(groupsMenu, GM_Ottava, str);
		EnableMenuItem(groupsMenu, GM_Ottava);
		return;
	}

	/* No notes/grace notes in octave signs are selected, so we can't remove an octave
	   sign. Is it possible to create one? */

	ottavaNum = 0;
		
	if (!beforeFirst) {
		for (staff=1; staff<=doc->nstaves; staff++) {
			GetStfSelRange(doc,staff,&stfStartL,&stfEndL);
			if (stfStartL && stfEndL)
				ottavaNum = OctCountNotesInRange(staff, stfStartL, stfEndL, False);
			else
				ottavaNum = 0;
			if (ottavaNum > 0) {
				if (!SelRangeChkOct(staff, stfStartL, stfEndL)) break;
				ottavaNum = 0;
			}
		}
	}

	if (ottavaNum>0) {
		cmdIsOttava = True;
		GetIndString(str, MENUCMD_STRS, 18);							/* "Create Octave Sign" */
		SetMenuItemText(groupsMenu, GM_Ottava, str);
		EnableMenuItem(groupsMenu, GM_Ottava);
	}
	else
		DisableMenuItem(groupsMenu, GM_Ottava);
}

/* Enable or disable all items in the Groups menu; disable entire menu if there is no
score, if it's the clipboard, or if we're looking at the Master Page. */

static void FixGroupsMenu(Document *doc, Boolean continSel)
{
	Boolean noteSel, GRnoteSel, disableWholeMenu;

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
	if (!noteSel) {											/* No notes selected, so... */
		DisableMenuItem(groupsMenu, GM_Tuplet);				/* Can't remove or add any tuplets. */
		DisableMenuItem(groupsMenu, GM_FancyTuplet);
		if (!GRnoteSel) {
			DisableMenuItem(groupsMenu, GM_Beam); 			/* Can't remove or add any beams. */
			DisableMenuItem(groupsMenu, GM_BreakBeam);	 	/* Can't break any. */
			DisableMenuItem(groupsMenu, GM_FlipFractional); /* Can't flip any. */

			DisableMenuItem(groupsMenu, GM_Ottava); 		/* Can't remove or add any ottavas. */
			return;
		}
	}
	
	/* Finish handling menu Enable/Disable for Beams, Tuplets, Ottavas. */

	FixBeamCommands(doc);
	FixTupletCommands(doc);
	FixOttavaCommands(doc, continSel);
}

void AddWindowList()
{
	short startItems, itemNum;  Document *doc;
	
	/* Delete any window names from the end of the View menu. */
	
	startItems = CountMenuItems(viewMenu);
	for (itemNum = startItems; itemNum>VM_LastItem; itemNum--)
		DeleteMenuItem(viewMenu, itemNum);
	
	/* If any Documents are open, add divider and window names of all open ones. */
	
	if (TopDocument) {
		AppendMenu(viewMenu, "\p(-");

		/* Add each Document to the menu, starting after any special Documents: we
		   don't want them to appear in the list. */
		   
		startItems = CountMenuItems(viewMenu);
		for (itemNum = startItems, doc = documentTable+SPEC_DOC_COUNT; doc<topTable; doc++)
			if (doc->inUse) {
				InsertMenuItem(viewMenu, "\ptemp", itemNum);
				GetWCTitle(doc->theWindow, strBuf);
				SetMenuItemCText(viewMenu, itemNum+1, strBuf);
				if (doc->theWindow==TopDocument) CheckMenuItem(viewMenu, itemNum+1, True);
				itemNum++;
			}
	}
}

/* Fix all items in the View menu and add a list of open Documents (windows). Even if
there's not a regular score Document open, let the user show or hide the clipboard, etc. */

static void FixViewMenu(Document *doc)
{
	AddWindowList();
	UpdateMenu(viewMenu, doc!=NULL);
	
	/* Be careful not to assume doc exists. */
	
	XableItem(viewMenu, VM_GoTo, ENABLE_GOTO(doc));
	XableItem(viewMenu, VM_GotoSel, ENABLE_GOTO(doc));

	XableItem(viewMenu, VM_Reduce, ENABLE_REDUCE(doc));
	XableItem(viewMenu, VM_Enlarge, ENABLE_ENLARGE(doc));
	
	XableItem(viewMenu, VM_LookAtV, doc!=NULL && !doc->masterView && !doc->showFormat);
	XableItem(viewMenu, VM_LookAtAllV, doc!=NULL && !doc->masterView && !doc->showFormat);
	XableItem(viewMenu, VM_ShowDurProb, doc!=NULL && !doc->masterView);
	XableItem(viewMenu, VM_ShowSyncL, doc!=NULL && !doc->masterView);
	XableItem(viewMenu, VM_ShowInvis, doc!=NULL && doc!=clipboard && !doc->masterView);
	XableItem(viewMenu, VM_ShowSystems, doc!=NULL && doc!=clipboard);

	XableItem(viewMenu, VM_GraphMode, doc!=NULL && !doc->masterView);
	
	XableItem(viewMenu, VM_ShowClipboard, doc!=NULL && doc!=clipboard);
	XableItem(viewMenu, VM_ToolPalette, !IsWindowVisible(palettes[TOOL_PALETTE]));
#ifdef SEARCH_CONTENT
	XableItem(viewMenu, VM_ShowSearchPattern, doc!=searchPatDoc);
#else
	XableItem(viewMenu, VM_ShowSearchPattern, False);
#endif

	CheckMenuItem(viewMenu, VM_GoTo, doc!=NULL && doc->overview);
	CheckMenuItem(viewMenu, VM_ColorVoices, doc!=NULL && doc->colorVoices!=COLORVOICES_NONE);
	CheckMenuItem(viewMenu, VM_ShowDurProb, doc!=NULL && doc->showDurProb);
	CheckMenuItem(viewMenu, VM_ShowSyncL, doc!=NULL && doc->showSyncs);
	CheckMenuItem(viewMenu, VM_ShowInvis, doc!=NULL && doc->showInvis);
	CheckMenuItem(viewMenu, VM_ShowSystems, doc!=NULL && doc->frameSystems);
	CheckMenuItem(viewMenu, VM_GraphMode, doc!=NULL && doc->graphMode);
}

/* Fix all items in Play/Record menu; disable entire menu if there's no score or we're
looking at the Master Page. */

static void FixPlayRecordMenu(Document *doc, short nSel)
{
	Boolean disableWholeMenu, noteSel, haveMIDI=(useWhichMIDI!=MIDIDR_NONE);

	disableWholeMenu = (doc==NULL || doc->masterView);

	if (!disableWholeMenu) {
		XableItem(playRecMenu, PL_PlayEntire, haveMIDI);
		XableItem(playRecMenu, PL_PlaySelection, doc!=clipboard && nSel!=0 && haveMIDI);
		XableItem(playRecMenu, PL_PlayFromSelection, doc!=clipboard && haveMIDI);
		XableItem(playRecMenu, PL_MutePart, haveMIDI);
		XableItem(playRecMenu, PL_PlayVarSpeed, haveMIDI);
		XableItem(playRecMenu, PL_SetPlayDuration, doc!=clipboard && nSel!=0 && haveMIDI);
		XableItem(playRecMenu, PL_AllNotesOff, haveMIDI);

		noteSel = ObjTypeSel(doc, SYNCtype, 0)!=NILINK;
		XableItem(playRecMenu, PL_Quantize, noteSel);

		XableItem(playRecMenu, PL_RecordInsert, doc!=clipboard && haveMIDI);
		XableItem(playRecMenu, PL_RecordMerge, doc!=clipboard && haveMIDI);
		XableItem(playRecMenu, PL_StepRecInsert, doc!=clipboard && haveMIDI);
		XableItem(playRecMenu, PL_StepRecMerge, doc!=clipboard && haveMIDI);
		XableItem(playRecMenu, PL_MIDISetup, doc!=clipboard);
		XableItem(playRecMenu, PL_Metronome, haveMIDI);		/* Metro and thru are global options */
		XableItem(playRecMenu, PL_MIDIDynPrefs, doc!=clipboard);
		XableItem(playRecMenu, PL_MIDIModPrefs, doc!=clipboard);
		XableItem(playRecMenu, PL_InstrMIDI, doc!=clipboard);
		XableItem(playRecMenu, PL_Transpose, doc!=clipboard);
		CheckMenuItem(playRecMenu, PL_Transpose, doc->transposed);
	}
	else {
		XableItem(playRecMenu, PL_PlayEntire, False);
		XableItem(playRecMenu, PL_PlaySelection, False);
		XableItem(playRecMenu, PL_PlayFromSelection, False);
		XableItem(playRecMenu, PL_MutePart, False);
		XableItem(playRecMenu, PL_PlayVarSpeed, False);
		XableItem(playRecMenu, PL_SetPlayDuration, False);
		XableItem(playRecMenu, PL_AllNotesOff, False);	
		XableItem(playRecMenu, PL_Quantize, False);
		XableItem(playRecMenu, PL_RecordInsert, False);
		XableItem(playRecMenu, PL_RecordMerge, False);
		XableItem(playRecMenu, PL_StepRecInsert, False);
		XableItem(playRecMenu, PL_StepRecMerge, False);
		XableItem(playRecMenu, PL_MIDISetup, False);
		XableItem(playRecMenu, PL_Metronome, False);
		XableItem(playRecMenu, PL_MIDIThru, False);
		XableItem(playRecMenu, PL_MIDIDynPrefs, False);
		XableItem(playRecMenu, PL_MIDIModPrefs, False);
		XableItem(playRecMenu, PL_InstrMIDI, False);
		XableItem(playRecMenu, PL_Transpose, False);
		CheckMenuItem(playRecMenu, PL_Transpose, False);
	}
}

/* Fix all items in Master Page menu. If no score or we're not in Master Page, this
menu shouldn't be installed, so we don't need to disable it. */

static void FixMasterPgMenu(Document *doc)
{
	short groupIsSel, nstavesMP, nparts;  LINK staffL;  Str255 str;

	if (doc==NULL || !doc->masterView) return;
	
	groupIsSel = GroupIsSel(doc);
	nstavesMP = doc->nstavesMP;

	staffL = LSSearch(doc->masterHeadL, STAFFtype, ANYONE, GO_RIGHT, False);
	GetIndString(str, MENUCMD_STRS,
		LinkSEL(staffL)? 19 : 20);							/* "Add Parts Above/at Bottom" */
	SetMenuItemText(masterPgMenu, MP_AddPart, str);

	/* Disable all menu items except Add Parts if score has no parts. */

	nparts = LinkNENTRIES(doc->masterHeadL);
	XableItem(masterPgMenu, MP_DeletePart, PartIsSel(doc));

	XableItem(masterPgMenu, MP_GroupParts, !groupIsSel && PartRangeIsSel(doc) && nstavesMP>0);
	XableItem(masterPgMenu, MP_UngroupParts, groupIsSel && nstavesMP>0);
	XableItem(masterPgMenu, MP_SplitPart, PartIsSel(doc));
	XableItem(masterPgMenu, MP_DistributeStaves, PartIsSel(doc));
	
	XableItem(masterPgMenu, MP_StaffSize, nstavesMP>0);
	XableItem(masterPgMenu, MP_StaffLines, nstavesMP>0);
	XableItem(masterPgMenu, MP_EditMargins, nstavesMP>0);

	XableItem(masterPgMenu, MP_Instrument, nstavesMP>0 && PartIsSel(doc));
	
//	XableItem(masterPgMenu, MP_CombineParts, nstavesMP>0 && PartRangeIsSel(doc));
}


/* Fix all items in Work on Format menu. If no score or we're not showing Format,
this menu shouldn't be installed, so we don't even need to disable it. */

static void FixFormatMenu(Document *doc)
{
	short nVis, nInvis;  LINK pL, aStaffL;

	if (doc==NULL || !doc->showFormat) return;
	
	nVis = nInvis = 0;
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (StaffTYPE(pL) && LinkSEL(pL)) {
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				if (StaffVIS(aStaffL))	nVis++;
				else					nInvis++;
		}
	
	XableItem(formatMenu, FT_Invisify, nVis>=2);
	XableItem(formatMenu, FT_ShowInvis, nInvis>=1);
}
