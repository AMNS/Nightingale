/* NightTemplates.h - assorted function prototypes for Nightingale */

/* AccModNRClick.c */

	Boolean		DoOpenModNR(Document *, Point);
	Boolean		DoAccModNRClick(Document *, Point);

/* ChooseCharDlog.c */

	Boolean		ChooseCharDlog(short, short, unsigned char *);

/* ChordSym.c */

	void		DrawChordSym(Document *, DDIST, DDIST, StringPtr, short, PCONTEXT, Boolean, DRect *);
	Boolean		ChordSymDialog(Document *, StringPtr, short *);

/* CompactVoices.c */

	void		DoCompactVoices(Document *doc);
	void		SetDurCptV(Document *doc);
	Boolean		CompactVoices(Document *doc);


/* DelAddRedAccs.c */

	Boolean		DelNoteRedAcc(Document *, short, LINK, LINK, Boolean []);
	Boolean		DelGRNoteRedAcc(Document *, short, LINK, LINK, Boolean []);
	void		ArrangeSyncAccs(LINK, Boolean []);
	void		ArrangeGRSyncAccs(LINK, Boolean []);
	Boolean		DelRedundantAccs(Document *, short, short);
	Boolean		AddRedundantAccs(Document *, short, short, Boolean);
	Boolean		AddMIDIRedundantAccs(Document *doc, LINK endAddAccsL, Boolean
							addTiedLeft, Boolean addNaturals);

/* DelAddRedTimeSigs.c */

	Boolean		DelRedTimeSigs(Document *doc, Boolean respace, LINK *pFirstDelL,
						LINK *pLastDelL);
	short		AddCautionaryTimeSigs(Document *doc);

/* DialogsEditor.c */

	/* The following enums for dialog item numbers have to be available to other
	   files because some of their item numbers are used as return values. */
 
	enum {						/* Set Dialog item numbers */
		VOICE_DI=9,
		STAFF_DI,
		NRSHAPE_DI,
		NRSMALL_DI,
		STEMLEN_DI,
		GRAPHICY_DI,
		SET_DI1,
		INVIS_DI,
		SLURSHAPE_DI
	};
	
	enum {
		DELSOFT_REDUNDANTACCS_DI=3,
		DELALL_REDUNDANTACCS_DI=1
	};

	enum {
		NONAT_ADDREDUNDANTACCS_DI=1,
		ALL_ADDREDUNDANTACCS_DI=3
	};

	/* SetDurationDialog (DurationEdit.c) and NMSetDuration (Menu.c) need this. */
	
	enum {
		HALVE_DURS,
		DOUBLE_DURS,
		SET_DURS_TO
	};

	Boolean		LeftEndDialog(short *, short *, short *, short *);
	short		SpaceDialog(short, short, short);
	short		BefMeasSpaceDialog(Document *, short);
	short		TremSlashesDialog(short);
	Boolean		EndingDialog(short, short *, short, short *);
	Boolean		MeasNumDialog(Document *);
	Boolean		PageNumDialog(Document *);
	Boolean		OttavaDialog(Document *, Byte *);
	short		LookAtDialog(Document *, short, LINK);
	short		GoToDialog(Document *, short *, short *, LINK *);
	Boolean		TupletDialog(Document *, TupleParam *, Boolean);
	Boolean		SetMBRestDialog(Document *, short *);
	short		RastralDialog(Boolean, short, Boolean *, Boolean *);
	short		StaffLinesDialog(Boolean, short *, Boolean *, Boolean *);
	Boolean		MarginsDialog(Document *, short *,short *,short *,short *);
	Boolean		KeySigDialog(short *, short *, Boolean *, Boolean);
	Boolean		SetKSDialogGuts(short, short *, short *, Boolean *);
	Boolean		TimeSigDialog(short *, short *, short *, Boolean *, Boolean);
	Boolean		RehearsalMarkDialog(unsigned char *);
	Boolean 	PatchChangeDialog(unsigned char *);
	Boolean		PanSettingDialog(unsigned char *);
	Boolean		ChordFrameDialog(Document *, Boolean *, short *, short *, short *,
						unsigned char *, unsigned char *);
	Boolean		TempoDialog(Boolean *, Boolean *, short *, Boolean *, Boolean *, unsigned char *, unsigned char *);
	Boolean		SymbolIsBarline(void);
	Boolean		InsMeasUnkDurDialog(void);
	void		XLoadDialogsSeg(void);

/* Documents.c */

	void		ActivateDocument(Document *doc, short activ);
	Boolean		DoCloseDocument(Document *doc);
	Boolean		DocumentSaved(Document *doc);
	Boolean		DoExtract(Document *doc);
	Boolean		DoCombineParts(Document *doc);
	Boolean		DoOpenDocument(StringPtr fileName, short vrefnum, Boolean readOnly, FSSpec *pfsSpec);
	Boolean		DoOpenDocumentX(StringPtr fileName, short vRefNum, Boolean readOnly, FSSpec *pfsSpec,
						Document **pDoc);
	void		DoRevertDocument(Document *doc);
	short		DoSaveDocument(Document *doc);
	short		DoSaveAs(Document *doc);
	Document	*FirstFreeDocument(void);
	Document	*	GetDocumentFromWindow(WindowPtr w);
	Boolean		EqualFSSpec(FSSpec *fs1, FSSpec *fs2);
	Boolean		InitDocFields(Document *doc);
	Boolean		InitDocUndo(Document *doc);
	void		InstallDoc(Document *doc);
	void		InstallDocHeaps(Document *doc);
	void		InstallMagnify(Document *doc);
	void		InstallStrPool(Document *doc);
	void		ShowClipDocument(void);
	void		PositionWindow(WindowPtr, Document *);
	void 		ShowDocument(Document *doc);

/* Double.c */

	short		Double(Document *);
	
/* DragBeam.c */

	Boolean		FixStemLengths(LINK, DDIST, short);
	Boolean		FixGRStemLengths(LINK, DDIST, short);

	void		DoBeamEdit(Document *, LINK);

/* DragGraphic.c */

	void		DragGraphic(Document *, LINK);

/* DrawingEdit.c */

	void		DoDrawingEdit(Document *, LINK);

/* EnigmaConvert.c */

	Boolean	OpenETFFile(void);

/* Error.c */

	void		InstallArg(unsigned char *str, unsigned char *msg, short argNum);

	void		BadInit(void);
	void		BadUpperLeft(long start, long last);
	void		CannotPrint(void);
	void		MissingValue(void);
	void		NoMoreMemory(void);
	void		NotOurs(unsigned char *name);
	void		OutOfMemory(long nBytes);
	void		PageTooLarge(void);
	void		PageTooSmall(void);
	void		TooManyColumns(short limit);
	void		TooManyDocs(void);
	void		TooManyPages(short limit);
	void		TooManyRows(short limit);
	
	void		MayErrMsg(char *, ...);
	void		AlwaysErrMsg(char *, ...);
	Boolean		ReportIOError(short, short);
	Boolean		ReportResError(void);
	Boolean		ReportBadResource(Handle);
	void		MissingDialog(short);
	void		AppleEventError(char *, short);

/* Event.c */

	Boolean	DoEvent(void);
	void		DoUpdate(WindowPtr w);
	void		UpdateAllWindows(void);
	void		DoActivate(EventRecord *evt, Boolean activ, Boolean isJuggle);
	void		DoSuspendResume(EventRecord *evt);
	void		AutoScroll(void);
pascal OSErr	HandleOAPP(const AppleEvent *appleEvent, AppleEvent *reply, /*unsigned*/ long refcon);
pascal OSErr	HandleODOC(const AppleEvent *appleEvent, AppleEvent *reply, /*unsigned*/ long refcon);
pascal OSErr	HandlePDOC(const AppleEvent *appleEvent, AppleEvent *reply, /*unsigned*/ long refcon);
pascal OSErr	HandleQUIT(const AppleEvent *appleEvent, AppleEvent *reply, /*unsigned*/ long refcon);
	Document *FSpecOpenDocument(FSSpec *theFile);

/* Extract.c */

	void		ExFixMeasAndSysRects(Document *newDoc);
	Document	*CreatePartDoc(Document *doc, unsigned char *fileName, short vRefNum, FSSpec *pfsSpec,
						LINK part);

/* Finalize.c */

	Boolean		ClosePrefsFile(void);
	void		Finalize(void);

/* FlowInText.c */

	void		DoTextFlowIn(Document *doc);
	Boolean		SetFlowInText(Ptr pText);

/* FontUtils.c */

	void		DisplayAvailableFonts(void);
	void		EnumerateDocumentFonts(Document *doc);
	void		GetNFontInfo(short, short, short, FontInfo *);
	Boolean		GetFontNumber(const Str255, short *);
	short		FontName2Index(Document *, StringPtr);
	Boolean		FontID2Name(Document *doc, short fontID, StringPtr fontName);
	short		UI2InternalStyle(Document *, short);
	short		Internal2UIStyle(short);

/* HairpinEdit.c, DragDynamic.c */

	void		DoHairpinEdit(Document *, LINK);
	void		DragHairpin(Document *, LINK);

	void		DragDynamic(Document *, LINK);

/* HeaderFooterDialog.c */

	Boolean		GetHeaderFooterStrings(Document *, StringPtr, StringPtr, StringPtr,
										short, Boolean, Boolean);
	Boolean		HeaderFooterDialog(Document *);

/* HeapFileIO.c */

	short		WriteHeaps(Document *doc, short refNum);
	short		ReadHeaps(Document *doc, short refNum, long version, OSType fdType);

/* InfoDialog.c */

	void		InfoDialog(Document *);
	Boolean		ModNRInfoDialog(Document *);
	void		XLoadInfoDialogSeg(void);

/* Initialize.c */

	Boolean		CreatePrefsFile(void);
	Boolean		InitGlobals(void);
	void		Initialize(void);
	Boolean		OpenPrefsFile(void);
	Boolean		BuildEmptyDoc(Document *doc);

/* InitNightingale.c */

	void		InitNightingale(void);
	Boolean		NInitPaletteWindows(void);
	Boolean		InitDynamicPalette(void);
	Boolean		InitModNRPalette(void);
	Boolean		InitDurationPalette(void);

/* Magnify.c */

	short		D2PFunc(DDIST d);
	short		D2PXFunc(DDIST d);
	DDIST		P2DFunc(short p);
	
	short		UseMagnifiedSize(short size, short magnify);
	short		UseTextSize(short, short);
	short		UseRelMagnifiedSize(short maxMagSize, short magnify);
	short		UseMTextSize(short, short);

/* main.c */

	int			main(int argc, const char * argv[]);
	void		DoOpenApplication(Boolean askForFile);

/* MeasFill.c */

	Boolean		FillEmptyDialog(Document *, short *, short *);
	Boolean		IsRangeEmpty(LINK, LINK, short, Boolean *);
	short		FillEmptyMeas(Document *, LINK, LINK);
	Boolean		FillNonemptyMeas(Document *, LINK, LINK, short, short);

/* Menu.c */

	Boolean		DoMenu(long menuChoice);
	void		DoEditMenu(short choice);
	Boolean		DoFileMenu(short choice);
	void 		DoPlayRecMenu(short choice);
	void		DoViewMenu(short choice);
	void		FixMenus(void);
	void		InstallDocMenus(Document *);

/* MIDIDialogs.c */

	void		MIDIPrefsDialog(Document *);
	Boolean		MIDIThruDialog(void);
	
	Boolean		MetroDialog(SignedByte *viaMIDI, SignedByte *channel, SignedByte *note,
						SignedByte *velocity, short *duration);
	Boolean		CMMetroDialog(SignedByte *viaMIDI, SignedByte *channel, SignedByte *note,
						SignedByte *velocity, short *duration, MIDIUniqueID *device);

	Boolean		MIDIDynamDialog(Document *, Boolean *);
	Boolean		MIDIModifierDialog(Document *);
	Boolean		MIDIDriverDialog(short *pPortSetting, short *pInterfaceSpeed);
	Boolean		MutePartDialog(Document *);
	Boolean		SetPlaySpeedDialog(void);

/* MIDI File-handling files */

	Boolean	GetTimingTrackInfo(short *, short *, short *, long *);
	Boolean	GetTrackInfo(short *, short *, Boolean [], short *, Boolean *, long *);
	Boolean	MFRespAndRfmt(Document *, short);
	Word	MF2MIDNight(Byte **);
	short	MIDNight2Night(Document *, TRACKINFO [], short, short, Boolean, Boolean, Boolean,
			short, long);
	LINK	NewRestSync(Document *, LINK, short, LINK *);
	Boolean	ReadMFHeader(Byte *, unsigned short *, unsigned short *);
	Word	ReadTrack(Byte **);
	Boolean	SetBracketsVis(Document *, LINK, LINK);
	
	Boolean	AddTies(Document *, short, LINKTIMEINFO [], short);
	Boolean	AddTuplets(Document *, short, LINKTIMEINFO [], short);
	LINK	BuildSyncs(Document *, short, LINKTIMEINFO [], short, NOTEAUX [], short, LINK);
	Boolean	ChooseAllTuplets(Document *, LINKTIMEINFO [], short, short , short, short,
						MEASINFO [], short);
	Boolean	ChooseChords(Document *, short, LINKTIMEINFO [], short, short *, NOTEAUX [],
						short);
	Boolean	ClarAllToMeas(Document *, LINKTIMEINFO [], short, short *, short,
						MEASINFO [], short);
	Boolean	ClarAllToTuplets(Document *, short, LINKTIMEINFO [], short, short *, short);
	Boolean	ClarAllBelowMeas(Document *, short, LINKTIMEINFO [], short, MEASINFO [], short,
						LINKTIMEINFO [], unsigned short, short *);
	void	QuantizeAndClip(Document *, short, LINKTIMEINFO [], short *, short);
	LINK	TranscribeVoice(Document *, short, LINKTIMEINFO [], short, short, NOTEAUX [],
						short, short, short, MEASINFO [], short, LINK,
						LINKTIMEINFO [], unsigned short, short *, short *);
	
	void	DPrintMeasTab(char *, MEASINFO [], short);
	void	DPrintLTIs(char *, LINKTIMEINFO [], short);

/* MidiMap.c */

	Boolean	OpenMidiMapFileNSD(Document *doc, Str255 fileName, NSClientDataPtr pNSD);
	Boolean OpenMidiMapFileFNFS(Document *doc, Str255 fileName, FSSpec *fsSpec);
	Boolean OpenMidiMapFile(Document *doc, FSSpec *fsSpec);
	void InstallMidiMap(Document *doc, FSSpec *fsSpec);
	void ClearMidiMap(Document *doc);
	Boolean HasMidiMap(Document *doc);
	Boolean ChangedMidiMap(Document *doc, FSSpec *fsSpec);
	void GetMidiMap(Document *doc, FSSpec *pfsSpec);
	Boolean SaveMidiMap(Document *doc);
	
/* MIDIRecord.c */

	Boolean	Record(Document *);
	Boolean	StepRecord(Document *, Boolean);
	
	Boolean	RTMRecord(Document *);

/* ModNREdit.c */

	Boolean	ModNRDialog(unsigned short dlogID, Byte *modCode);

/* MoveUpDown.c */

	Boolean	MoveBarsUp(Document *, LINK, LINK);
	Boolean	MoveBarsDown(Document *, LINK, LINK);
	Boolean	MoveSystemUp(Document *, LINK);
	Boolean	MoveSystemDown(Document *, LINK);

/* NewSlur.c */

	void		NewSlur(Document *, short, short, Point);
	void		GetTiesCurveDir(Document *, short, LINK, Boolean []);
	void		GetSlurTieCurveDir(Document *, short, LINK, Boolean *);
	short		AddNoteTies(Document *, LINK, LINK, short, short);

/* NotelistConvert.c */

	Boolean FSOpenNotelistFile(Str255 fileName, FSSpec *fsSpec);
	Boolean	OpenNotelistFile(Str255 fileName, NSClientDataPtr pNSD);

/* Part.c */

	LINK		AddPart(Document *, short, short, short);
	short		Staff2Part(Document *, short);
	LINK 		Staff2PartLINK(Document *doc, short nstaff);
	LINK		Sel2Part(Document *doc);
	Boolean		DeletePart(Document *, short, short);

/* PitchCmds.c */

	void		CheckRange(Document *);
	Boolean		DTranspose(Document *, Boolean, short, short, Boolean);
	void		FlipSelDirection(Document *);
	Boolean		Respell(Document *);
	Boolean		Transpose(Document *, Boolean, short, short, short, Boolean);
	Boolean		TransposeKey(Document *, Boolean, short, short, short, Boolean [],
						Boolean, Boolean, Boolean);
						
/* Preferences.c */

	Boolean OpenTextSetupFile(void);
	Boolean GetTextConfig(void);
	char *GetPrefsValue(char *key);

/* PitchDialogs.c */

	Boolean	DTransposeDialog(Boolean *, short *, short *, Boolean *);
	Boolean	TransKeyDialog(Boolean [], Boolean *, short *, short *, short *,
						Boolean *, Boolean *, Boolean *);
	Boolean	TransposeDialog(Boolean *, short *, short *, short *, Boolean *);

/* RecTransMerge.c */

	Boolean	RecTransMerge(Document *doc);

/* Reformat.c and RfmtDlog.c */

	LINK		BreakSystem(Document *, LINK);
	Boolean		ReformatDialog(short, short, Boolean *, Boolean *, Boolean *, short *,
						Boolean *, Boolean *, short *, short *);
	short		Reformat(Document *, LINK, LINK, Boolean, short, Boolean, short, short);
	Boolean		RespAndRfmtRaw(Document *, LINK, LINK, long);

/* ScoreInfo.c */

	void		ScoreInfo(void);

/* Sheet.c */

	enum {			/* GetSheetRect return values: */
		INARRAY_INRANGE,	/* Sheet in visible array and coords. in range: maybe visible */
		INARRAY_OVERFLOW,	/* Sheet in visible array and coords. overflow: not visible */
		NOT_INARRAY		/* Sheet not in visible array: not visible */
	};

	void		DoMasterSheet(Document *doc);
	void		DrawSheet(Document *doc, short i, Rect *paper, Rect *vis);
	Boolean		FindSheet(Document *doc, Point pt, short *sheet);
	void		GetAllSheets(Document *doc);
	short		GetSheetRect(Document *doc, short nSheet, Rect *paper);
	void		GotoSheet(Document *doc, short i, short drawIt);
	void		InvalSheets(Document *doc, short first, short last);
	void		InvalSheetView(Document *doc);
	void		MagnifyPaper(Rect *paper, Rect *magPaper, short magnify);
	void		UnmagnifyPaper(Rect *magPaper, Rect *paper, short magnify);
	void		PickSheetMode(Document *doc, Boolean fromMenu, short sheet);
	Boolean		ScreenPagesCanOverflow(Document *doc, short magnify, short numSheets);
	Boolean		ScreenPagesExceedView(Document *doc);
	void		SetCurrentSheet(Document *doc, short sheet, Rect *paper);
	void		SheetMagnify(Document *doc, short inc);
	void		WarnScreenPagesOverflow(Document *doc);

/* SheetSetup.c */

	void		DoSheetSetup(Document *doc);
	void		GetMaxRowCol(Document *doc, short width, short height, short *maxRow,
					short *maxCol);

/* StringToolbox.c */

	void		GetIndCString(char *pString, short strListID, short strIndex);
	void		CParamText(char cStr1[], char cStr2[], char cStr3[], char cStr4[]);
	void		DrawCString(char cStr[]);
	short		CStringWidth(char cStr[]);
	void		GetWCTitle(WindowPtr theWindow, char theCTitle[]);
	void		SetWCTitle(WindowRef theWindow, char theCTitle[]);
	void		SetDialogItemCText(Handle lHdl, char cStr[]);
	void		SetMenuItemCText(MenuHandle theMenu, short item, char cItemString[]);
	void		AppendCMenu(MenuHandle theMenu, char cItemText[]);

/* StringUtils.c */

	char		*PToCString(StringPtr str);
	StringPtr	CToPString(char *str);
	StringPtr	CtoPstr(StringPtr str);
	StringPtr	PtoCstr(StringPtr str);

	StringPtr	Pstrcpy(StringPtr dst, ConstStringPtr src);
	void		PStrncpy(StringPtr, ConstStringPtr, short);
	Boolean		streql(char *s, char *t);
	Boolean		strneql(char *s, char *t, short len);
	Boolean		Pstreql(StringPtr s1, StringPtr s2);
	Boolean		Pstrneql(StringPtr s1, StringPtr s2, short n);
	short		Pstrlen(ConstStringPtr str);
	void		PStrCat(StringPtr, ConstStringPtr);
	void		GoodStrncpy(char *, char *, unsigned long);
	Boolean		ExpandPString(unsigned char *dstStr, unsigned char *srcStr, bool wider);
	Boolean		GetFinalSubstring(char *str, char *substr, char delimChar);
	Boolean		GetInitialSubstring(char *str, char *substr, short len);
	void		MacTypeToString(OSType aMacType, char versionCString[]);

/* SysPageEdit.c */

	void		CutSystem(Document *);
	void		CopySystem(Document *);
	void		PasteSystem(Document *);
	void		ClearSystem(Document *);
	
	void		CutPages(Document *);
	void		CopyPages(Document *);
	void		PastePages(Document *);
	void		ClearPages(Document *);

/* TextDialog.c */

	enum {					/* In the order of the Text Styles pop-up (ID 35) */
		TSNoSTYLE = 0,
		TSRegular1STYLE,
		TSRegular2STYLE,
		TSRegular3STYLE,
		TSRegular4STYLE,
		TSRegular5STYLE,
		TSRegular6STYLE,
		TSRegular7STYLE,
		TSRegular8STYLE,
		TSRegular9STYLE,
		____________,
		TSThisItemOnlySTYLE
	};
	
	void		XLoadTextDialogSeg(void);
	
	Boolean		TextDialog(Document *, short *, Boolean *, short *, short *, short *,
						Boolean *, Boolean *, unsigned char *, unsigned char *, CONTEXT *);
	Boolean		DefineStyleDialog(Document *, unsigned char *, CONTEXT *);

/* ToolPalette.c */

	Boolean		TranslatePalChar(short *, short, Boolean);
pascal void		DrawToolPalette(Rect *r);
pascal void		HiliteToolItem(Rect *r, short item);
pascal short	FindToolItem(Point pt);
	void		DoToolContent(Point pt, short modifiers);
	void		HandleToolCursors(short item);
	void		PalKey(char);
	char		GetPalChar(short item);
	void		DoToolKeyDown(short ch, short key, short modifiers);
	void		DoToolGrow(Point pt);
	void		ChangeToolSize(short across, short down, Boolean doingZoom);
	void		GetToolZoomBounds(void);
	Boolean		SaveToolPalette(Boolean inquire);

/* Transcribe.c (formerly GuessDurs.c) */

	Boolean		AnyOverfullMeasure(Document *);
	void		DoQuantize(Document *);

/* TrackVMove.c */

	Boolean		InsTrackPitch(Document *, Point, short *, LINK, short, short *, short *,
								short);
	short		InsTrackUpDown(Document *, Point, short *, LINK, short, short *);
	void		SetCrossbar(short, short, PCONTEXT);
	void		InvertCrossbar(Document *);
	short		CalcAccidental(short, short);
	short		CalcNewDurIndex(short, short, short);
	short		CalcNewDynIndex(short, short, short);

/* VoiceNumbers.c */

	void		OffsetVoiceNums(Document *, short, short);
	void		BuildVoiceTable(Document *, Boolean);
	void		MapVoiceNums(Document *, short []);
	void		UpdateVoiceTable(Document *, Boolean);
	short		User2IntVoice(Document *, short, LINK);
	Boolean		Int2UserVoice(Document *, short, short *, LINK *);
	short		NewVoiceNum(Document *, LINK);
	short		NCountVoices(Document *doc);
	void		GetVoiceTableLine(Document *doc, short vNum, char *str);

/* Windows.c */

	Boolean		ActiveWindow(WindowPtr w);
	void		AnalyzeWindows(void);
	short		DoCloseAllDocWindows(void);
	void		DoCloseWindow(WindowPtr w);
	void		DoDragWindow(WindowPtr w);
	void		DoSelectWindow(WindowPtr w);
	void		DoZoom(WindowPtr w, short part);
	void		DrawMessageBox(Document *doc, Boolean reallyDraw);
	void		DrawMessageString(Document *doc, char *msgStr);
	void		EraseAndInvalMessageBox(Document *);
	void		FinishMessageDraw(Document *doc);
	void		HiliteUserWindows(void);
	void		InvalWindow(Document *doc);
	void		OurDragWindow(WindowPtr w);
	void		PrepareMessageDraw(Document *doc, Rect *messageRect, Boolean boundsOnly);
	void		RecomputeWindow(WindowPtr w);
	void		SendToBack(WindowPtr w);
	void		SetZoomState(WindowPtr w);
	void		ShowHidePalettes(Boolean show);
	void		UpdatePalette(WindowPtr w);

/* Miscellaneous */

	LINK		NUIAddNoteToSync(Document *, MNOTE, LINK, SignedByte, short, short, SignedByte,
						Boolean, long);
	Boolean		AutoBeam(Document *);
	Boolean		BuildDocument(Document *doc, unsigned char *fileName, short vRefNum,
					FSSpec *pfsSpec, long *version, Boolean isNew);
	LINK		NUICreateSync(Document *, MNOTE, LINK *, SignedByte, short, short, SignedByte,
						Boolean, long);
	void		DoAboutBox(Boolean first);
	void		DoAutoBeam(Document *);
	void		DoClickErase(Document *doc, Point pt);
	void		DoClickFrame(Document *doc, Point pt);
	void		DoClickInsert(Document *doc, Point pt);
	Boolean		DoEditMaster(Document *doc, Point pt, short modifiers, short dblClick);
	Boolean		DoEditScore(Document *doc, Point pt, short modifiers, short dblClick);
	void		DoPrint(Document *doc);
	void		DrawDocumentControls(Document *doc);
	void		DrawDocumentView(Document *doc, Rect *updateRect);
	void		ExportDeskScrap(void);
	Boolean		ClipboardChanged(ScrapRef scrap);
	void		FixCrossSysObjects(Document *, LINK, LINK);
	void		ImportDeskScrap(void);
	Boolean		ImportMIDIFile(FSSpec *);
	void		MPDrawParams(Document *doc, LINK obj, LINK subObj, short param, short d);
	void		NSInvisify(Document *doc);
	void		OffsetSystem(LINK sysL, short dh, short dv);
	void		PrefsDialog(Document *, Boolean, short *);
	void		QuickScroll(Document *doc, short dx, short dy, Boolean relCoords, Boolean doCopyBits);
	void		RecomputeView(Document *doc);
	void		SaveEPSF(void);
	long 		LastEndTime(Document *doc, LINK fromL, LINK toL);
	Boolean		SaveMIDIFile(Document *);
	void		SaveNotelist(Document *, short, Boolean);
pascal void		ScrollDocument(ControlHandle control, short part);
	void		SetBackground(Document *doc);
	DDIST		SetStfInvis(Document *doc, LINK pL, LINK aStaffL);
	void		UpdateSysMeasYs(Document *doc, LINK sysL, Boolean useLedg, Boolean masterPg);
	void		XLoadEditScoreSeg(void);
	void		XLoadSheetSetupSeg(void);
	Boolean		XStfObjOnStaff(LINK, short);
