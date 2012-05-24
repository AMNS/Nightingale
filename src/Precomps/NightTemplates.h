/* NightTemplates.h - assorted function prototypes for Nightingale */

/* AccModNRClick.c */

	Boolean DoOpenModNR(Document *, Point);
	Boolean DoAccModNRClick(Document *, Point);

/* ChooseCharDlog.c */

	Boolean	ChooseCharDlog(short, short, unsigned char *);

/* ChordSym.c */

	void		DrawChordSym(Document *, DDIST, DDIST, unsigned char [], INT16, PCONTEXT, Boolean, DRect *);
	Boolean	ChordSymDialog(Document *, unsigned char *, INT16 *);

/* CompactVoices.c */

	void		DoCompactVoices(Document *doc);
	void		SetDurCptV(Document *doc);
	Boolean	CompactVoices(Document *doc);


/* DelAddRedAccs.c */

	Boolean	DelNoteRedAcc(Document *, INT16, LINK, LINK, Boolean []);
	Boolean	DelGRNoteRedAcc(Document *, INT16, LINK, LINK, Boolean []);
	void		ArrangeSyncAccs(LINK, Boolean []);
	void		ArrangeGRSyncAccs(LINK, Boolean []);
	Boolean	DelRedundantAccs(Document *, INT16, INT16);
	Boolean	AddRedundantAccs(Document *, INT16, INT16, Boolean);
	Boolean	AddMIDIRedundantAccs(Document *doc, LINK endAddAccsL, Boolean
							addTiedLeft, Boolean addNaturals);

/* DelRedTimeSigs.c */

	Boolean	DelRedTimeSigs(Document *doc, Boolean respace, LINK *pFirstDelL,
						LINK *pLastDelL);

/* DialogsEditor.c */

	/* The following enums for dialog item numbers have to be available to other
		files because some of their item numbers are used as return values. */
 
	enum
	{						/* Set Dialog item numbers */
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
	
	enum
	{
		DELSOFT_REDUNDANTACCS_DI=3,
		DELALL_REDUNDANTACCS_DI=1
	};

	enum
	{
		NONAT_ADDREDUNDANTACCS_DI=1,
		ALL_ADDREDUNDANTACCS_DI=3
	};

	/* SetDurDialog (DialogsEditor.c) and NMSetDuration (Menu.c) need this. */
	enum
	{
		HALVE_DURS,
		DOUBLE_DURS,
		SET_DURS_TO
	};

	Boolean	LeftEndDialog(INT16 *, INT16 *, INT16 *, INT16 *);
	INT16		SpaceDialog(INT16, INT16, INT16);
	INT16		TremSlashesDialog(INT16);
	Boolean	EndingDialog(INT16, INT16 *, INT16, INT16 *);
	Boolean	MeasNumDialog(Document *);
	Boolean	PageNumDialog(Document *);
	Boolean	OctavaDialog(Document *, Byte *);
	INT16		LookAtDialog(Document *, INT16, LINK);
	INT16		GoToDialog(Document *, INT16 *, INT16 *, LINK *);
	Boolean	FTupletCheck(Document *, TupleParam *);
	Boolean	TupletDialog(Document *, TupleParam *, Boolean);
	Boolean	SetDurDialog(Document *, Boolean *, INT16 *, INT16 *, INT16 *, Boolean *, 
						INT16 *, Boolean *, Boolean *);
	Boolean	SetMBRestDialog(Document *, INT16 *);
	INT16		RastralDialog(Boolean, INT16, Boolean *, Boolean *);
	INT16		StaffLinesDialog(Boolean, INT16 *, Boolean *, Boolean *);
	Boolean	MarginsDialog(Document *, INT16 *,INT16 *,INT16 *,INT16 *);
	Boolean	KeySigDialog(INT16 *, INT16 *, Boolean *, Boolean);
	Boolean	SetKSDialogGuts(short, INT16 *, INT16 *, Boolean *);
	Boolean	TimeSigDialog(INT16 *, INT16 *, INT16 *, Boolean *, Boolean);
	Boolean	RehearsalMarkDialog(unsigned char *);
	Boolean 	PatchChangeDialog(unsigned char *);
	Boolean	PanSettingDialog(unsigned char *);
	Boolean	ChordFrameDialog(Document *, Boolean *, INT16 *, INT16 *, INT16 *,
						unsigned char *, unsigned char *);
	Boolean	TempoDialog(Boolean *, INT16 *, Boolean *, unsigned char *, unsigned char *);
	Boolean	SymbolIsBarline(void);
	Boolean	InsMeasUnkDurDialog(void);
	void		XLoadDialogsSeg(void);

/* Documents.c */

	void		ActivateDocument(Document *doc, INT16 activ);
	Boolean	DoCloseDocument(Document *doc);
	Boolean	DocumentSaved(Document *doc);
	Boolean	DoExtract(Document *doc);
	Boolean	DoCombineParts(Document *doc);
	Boolean	DoOpenDocument(unsigned char *fileName, short vrefnum, Boolean readOnly, FSSpec *pfsSpec);
	Boolean DoOpenDocument(unsigned char *fileName, short vRefNum, Boolean readOnly, FSSpec *pfsSpec, Document **pDoc);
	void		DoRevertDocument(Document *doc);
	Boolean	DoSaveDocument(Document *doc);
	Boolean	DoSaveAs(Document *doc);
	Document*	FirstFreeDocument(void);
	Document*	GetDocumentFromWindow(WindowPtr w);
	Boolean EqualFSSpec(FSSpec *fs1, FSSpec *fs2);
	Boolean	InitDocFields(Document *doc);
	Boolean	InitDocUndo(Document *doc);
	void		InstallDoc(Document *doc);
	void		InstallDocHeaps(Document *doc);
	void		InstallMagnify(Document *doc);
	void		InstallStrPool(Document *doc);
	void		ShowClipDocument(void);
	void		PositionWindow(WindowPtr,Document *);
	void 		ShowDocument(Document *doc);

/* Double.c */

	INT16		Double(Document *);
	
/* DragBeam.c */

	Boolean	FixStemLengths(LINK, DDIST, INT16);
	Boolean	FixGRStemLengths(LINK, DDIST, INT16);

	void		DoBeamEdit(Document *, LINK);

/* DragGraphic.c */

	void		DragGraphic(Document *, LINK);

/* DrawingEdit.c */

	void		DoDrawingEdit(Document *, LINK);

/* EnigmaConvert.c */

	Boolean	OpenETFFile(void);

/* Error.c */

	void		InstallArg(unsigned char *str, unsigned char *msg, INT16 argNum);

	void		BadInit(void);
	void		BadUpperLeft(long start, long last);
	void		CannotPrint(void);
	void		MissingValue(void);
	void		NoMoreMemory(void);
	void		NotOurs(unsigned char *name);
	void		OutOfMemory(long nBytes);
	void		PageTooLarge(void);
	void		PageTooSmall(void);
	void		TooManyColumns(INT16 limit);
	void		TooManyDocs(void);
	void		TooManyPages(INT16 limit);
	void		TooManyRows(INT16 limit);
	
	void		MayErrMsg(char *, ...);
	void		AlwaysErrMsg(char *, ...);
	Boolean	ReportIOError(INT16, INT16);
	Boolean	ReportResError(void);
	Boolean	ReportBadResource(Handle);
	void		MissingDialog(INT16);
	void		AppleEventError(char *, INT16);

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

	Boolean	CloseSetupFile(void);
	void		Finalize(void);

/* FlowInText.c */

	void		DoTextFlowIn(Document *doc);
	Boolean	SetFlowInText(Ptr pText);

/* HairpinEdit.c, DragDynamic.c */

	void		DoHairpinEdit(Document *, LINK);
	void		DragHairpin(Document *, LINK);

	void		DragDynamic(Document *, LINK);

/* HeaderFooterDialog.c */

	Boolean	GetHeaderFooterStrings(Document *, StringPtr, StringPtr, StringPtr,
										INT16, Boolean, Boolean);
	Boolean	HeaderFooterDialog(Document *);

/* HeapFileIO.c */

	INT16		WriteHeaps(Document *doc, INT16 refNum);
	INT16		ReadHeaps(Document *doc, INT16 refNum, long version, OSType fdType);

/* InfoDialog.c */

	void		InfoDialog(Document *);
	Boolean	ModNRDialog(Document *);
	void		XLoadInfoDialogSeg(void);

/* Initialize.c */

	Boolean	CreateSetupFile(void);
	Boolean	InitGlobals(void);
	void		Initialize(void);
	Boolean	OpenSetupFile(void);
	Boolean BuildEmptyDoc(Document *doc);

/* Magnify.c */

	short		D2PFunc(DDIST d);
	short		D2PXFunc(DDIST d);
	DDIST		P2DFunc(short p);
	
	INT16		UseMagnifiedSize(INT16 size, INT16 magnify);
	INT16		UseTextSize(INT16, INT16);
	INT16		UseRelMagnifiedSize(INT16 maxMagSize, INT16 magnify);
	INT16		UseMTextSize(INT16, INT16);

/* main.c */

	int		main(void);
	void		DoOpenApplication(Boolean askForFile);

/* MeasFill.c */

	Boolean	FillEmptyDialog(Document *, INT16 *, INT16 *);
	Boolean	FillEmptyMeas(Document *, LINK, LINK);
	Boolean	FillNonemptyMeas(Document *,LINK, LINK, INT16, INT16);

/* Menu.c */

	Boolean	DoMenu(long menuChoice);
	void		DoEditMenu(INT16 choice);
	Boolean	DoFileMenu(INT16 choice);
	void 		DoPlayRecMenu(INT16 choice);
	void		DoViewMenu(INT16 choice);
	void		FixMenus(void);
	void		InstallDocMenus(Document *);

/* MIDIDialogs.c */

	void		MIDIDialog(Document *);
	Boolean	MIDIThruDialog(void);
	
	Boolean	MetroDialog(SignedByte *viaMIDI, SignedByte *channel,
						SignedByte *note, SignedByte *velocity,
						short *duration);
	Boolean	FMSMetroDialog(SignedByte *viaMIDI, SignedByte *channel,
						SignedByte *note, SignedByte *velocity,
						short *duration, fmsUniqueID *device);
	Boolean	OMSMetroDialog(SignedByte *viaMIDI, SignedByte *channel,
						SignedByte *note, SignedByte *velocity,
						short *duration, OMSUniqueID *device);
	Boolean	CMMetroDialog(SignedByte *viaMIDI, SignedByte *channel,
						SignedByte *note, SignedByte *velocity,
						short *duration, MIDIUniqueID *device);

	Boolean	MIDIDynamDialog(Document *, Boolean *);
	Boolean	MIDIModifierDialog(Document *);
	Boolean	MIDIDriverDialog(short *pPortSetting, short *pInterfaceSpeed);

/* MIDI File-handling files */

	Boolean	GetTimingTrackInfo(INT16 *, INT16 *, INT16 *, long *);
	Boolean	GetTrackInfo(INT16 *, INT16 *, Boolean [], INT16 *, Boolean *, long *);
	Boolean	MFRespAndRfmt(Document *, INT16);
	Word		MF2MIDNight(Byte **);
	INT16		MIDNight2Night(Document *, TRACKINFO [], INT16, INT16, Boolean, Boolean, Boolean,
			INT16, long);
	LINK		NewRestSync(Document *, LINK, INT16, LINK *);
	Boolean	ReadMFHeader(Byte *, unsigned short *, unsigned short *);
	Word		ReadTrack(Byte **);
	Boolean	SetBracketsVis(Document *, LINK, LINK);
	
	Boolean	AddTies(Document *, INT16, LINKTIMEINFO [], INT16);
	Boolean	AddTuplets(Document *, INT16, LINKTIMEINFO [], INT16);
	LINK		BuildSyncs(Document *, INT16, LINKTIMEINFO [], INT16, NOTEAUX [], INT16, LINK);
	Boolean	ChooseAllTuplets(Document *, LINKTIMEINFO [], INT16, INT16 , INT16, INT16,
						MEASINFO [], INT16);
	Boolean	ChooseChords(Document *, INT16, LINKTIMEINFO [], INT16, INT16 *, NOTEAUX [],
						INT16);
	Boolean	ClarAllToMeas(Document *, LINKTIMEINFO [], INT16, INT16 *, INT16,
						MEASINFO [], INT16);
	Boolean	ClarAllToTuplets(Document *, INT16, LINKTIMEINFO [], INT16, INT16 *, INT16);
	Boolean	ClarAllBelowMeas(Document *, INT16, LINKTIMEINFO [], INT16, MEASINFO [], INT16,
						LINKTIMEINFO [], unsigned INT16, INT16 *);
	void		QuantizeAndClip(Document *, INT16, LINKTIMEINFO [], INT16 *, INT16);
	LINK		TranscribeVoice(Document *, INT16, LINKTIMEINFO [], INT16, INT16, NOTEAUX [],
						INT16, INT16, INT16, MEASINFO [], INT16, LINK,
						LINKTIMEINFO [], unsigned INT16, INT16 *, INT16 *);
	
	void		DPrintMeasTab(char *, MEASINFO [], INT16);
	void		DPrintLTIs(char *, LINKTIMEINFO [], INT16);

/* MidiMap.c */

	Boolean	OpenMidiMapFile(Document *doc, Str255 fileName, NSClientDataPtr pNSD);
	Boolean OpenMidiMapFile(Document *doc, Str255 fileName, FSSpec *fsSpec);
	Boolean OpenMidiMapFile(Document *doc, FSSpec *fsSpec);
	void InstallMidiMap(Document *doc, Handle fsSpecHdl);
	void InstallMidiMap(Document *doc, FSSpec *fsSpec);
	void ClearMidiMap(Document *doc);
	Boolean HasMidiMap(Document *doc);
	Boolean ChangedMidiMap(Document *doc, FSSpec *fsSpec);
	void GetMidiMap(Document *doc, FSSpec *pfsSpec);
	Boolean SaveMidiMap(Document *doc);
	
/* MIDIRecord.c */

	void		AvoidUnisonsInRange(Document *, LINK, LINK, INT16);
	Boolean	Record(Document *);
	Boolean	StepRecord(Document *, Boolean);
	
	Boolean	RTMRecord(Document *);

/* MoveUpDown.c */

	Boolean	MoveBarsUp(Document *, LINK, LINK);
	Boolean	MoveBarsDown(Document *, LINK, LINK);
	Boolean	MoveSystemUp(Document *, LINK);
	Boolean	MoveSystemDown(Document *, LINK);

/* NewSlur.c */

	void		NewSlur(Document *, INT16, INT16, Point);
	void		GetTiesCurveDir(Document *, INT16, LINK, Boolean []);
	void		GetSlurTieCurveDir(Document *, INT16, LINK, Boolean *);
	INT16		AddNoteTies(Document *, LINK, LINK, INT16, INT16);

/* NotelistConvert.c */

	Boolean	OpenNotelistFile(Str255 fileName, NSClientDataPtr pNSD);
	Boolean OpenNotelistFile(Str255 fileName, FSSpec *fsSpec);

/* Part.c */

	LINK		AddPart(Document *, INT16, INT16, INT16);
	INT16		Staff2Part(Document *, INT16);
	LINK 		Staff2PartLINK(Document *doc, INT16 nstaff);
	Boolean	DeletePart(Document *, INT16, INT16);

/* PitchCmds.c */

	void		CheckRange(Document *);
	Boolean	DTranspose(Document *, Boolean, INT16, INT16, Boolean);
	void		FlipSelDirection(Document *);
	Boolean	Respell(Document *);
	Boolean	Transpose(Document *, Boolean, INT16, INT16, INT16, Boolean);
	Boolean	TransposeKey(Document *, Boolean, INT16, INT16, INT16, Boolean [],
						Boolean, Boolean, Boolean);
						
/* Preferences.c */

	Boolean OpenTextSetupFile(void);
	Boolean GetTextConfig(void);
	char *GetPrefsValue(char *key);

/* PitchDialogs.c */

	Boolean	DTransposeDialog(Boolean *, INT16 *, INT16 *, Boolean *);
	Boolean	TransKeyDialog(Boolean [], Boolean *, INT16 *, INT16 *, INT16 *,
						Boolean *, Boolean *, Boolean *);
	Boolean	TransposeDialog(Boolean *, INT16 *, INT16 *, INT16 *, Boolean *);

/* RecTransMerge.c */

	Boolean	RecTransMerge(Document *doc);

/* Reformat.c and RfmtDlog.c */

	LINK		BreakSystem(Document *, LINK);
	Boolean	ReformatDialog(INT16, INT16, Boolean *, Boolean *, Boolean *, INT16 *,
						Boolean *, Boolean *, INT16 *, INT16 *);
	INT16		Reformat(Document *, LINK, LINK, Boolean, INT16, Boolean, INT16, INT16);
	Boolean	RespAndRfmtRaw(Document *, LINK, LINK, long);

/* ScoreInfo.c */

	void		ScoreInfo(void);

/* Sheet.c */

	enum {			/* GetSheetRect return values: */
		INARRAY_INRANGE,	/* Sheet in visible array and coords. in range: maybe visible */
		INARRAY_OVERFLOW,	/* Sheet in visible array and coords. overflow: not visible */
		NOT_INARRAY		/* Sheet not in visible array: not visible */
	};

	void		DoMasterSheet(Document *doc);
	void		DrawSheet(Document *doc, INT16 i, Rect *paper, Rect *vis);
	Boolean	FindSheet(Document *doc, Point pt, INT16 *sheet);
	void		GetAllSheets(Document *doc);
	short		GetSheetRect(Document *doc, INT16 nSheet, Rect *paper);
	void		GotoSheet(Document *doc, INT16 i, INT16 drawIt);
	void		InvalSheets(Document *doc, INT16 first, INT16 last);
	void		InvalSheetView(Document *doc);
	void		MagnifyPaper(Rect *paper, Rect *magPaper, INT16 magnify);
	void		UnmagnifyPaper(Rect *magPaper, Rect *paper, INT16 magnify);
	void		PickSheetMode(Document *doc, Boolean fromMenu, INT16 sheet);
	Boolean	ScreenPagesCanOverflow(Document *doc, INT16 magnify, INT16 numSheets);
	Boolean	ScreenPagesExceedView(Document *doc);
	void		SetCurrentSheet(Document *doc, INT16 sheet, Rect *paper);
	void		SheetMagnify(Document *doc, INT16 inc);
	void		WarnScreenPagesOverflow(Document *doc);

/* SheetSetup.c */

	void		DoSheetSetup(Document *doc);
	void		GetMaxRowCol(Document *doc, INT16 width, INT16 height, INT16 *maxRow,
					INT16 *maxCol);

/* StringToolbox.c */

	void		GetIndCString(char *pString, short strListID, short strIndex);
	void		CParamText(char cStr1[], char cStr2[], char cStr3[], char cStr4[]);
	void		DrawCString(char cStr[]);
	short		CStringWidth(char cStr[]);
	void		GetWCTitle(WindowPtr theWindow, char theCTitle[]);
	void		SetWCTitle(WindowRef theWindow, char theCTitle[]);
	void		SetDialogItemCText(Handle lHdl, char cStr[]);

//	void		SFPCGetFile(Point where, char cPrompt[], FileFilterUPP fileFilterUPP,
//					short numTypes, ConstSFTypeListPtr typeList, DlgHookUPP hookUPP,
//					SFReply *pReply, short dlogID, ModalFilterUPP filterUPP);
	void		SetMenuItemCText(MenuHandle theMenu, short item, char cItemString[]);
	void		AppendCMenu(MenuHandle theMenu, char cItemText[]);

/* StringUtils.c */

	char		*PToCString(unsigned char *str);
	unsigned char *CToPString(char *str);
	unsigned char *Pstrcpy(unsigned char *dst, const unsigned char *src);
	Boolean	streql(char *s, char *t);
	Boolean	Pstreql(unsigned char *s, unsigned char *t);
	Boolean	strneql(char *s, char *t, short len);

	void		PStrCat(StringPtr, ConstStringPtr);
	void		PStrCopy(ConstStringPtr, StringPtr);
	void		PStrnCopy(ConstStringPtr, StringPtr, short);
	Boolean	PStrCmp(ConstStringPtr, ConstStringPtr);
	Boolean	PStrnCmp(ConstStringPtr, ConstStringPtr, short);
	void		GoodStrncpy(char [256], char [256], unsigned long);

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
	
	Boolean	TextDialog(Document *, INT16 *, Boolean *, INT16 *, INT16 *, INT16 *,
					Boolean *, unsigned char *, unsigned char *, CONTEXT *);
	Boolean	DefineStyleDialog(Document *, unsigned char *, CONTEXT *);

/* ToolPalette.c */

	Boolean	TranslatePalChar(INT16 *, INT16, Boolean);
pascal void		DrawToolMenu(Rect *r);
pascal void		HiliteToolItem(Rect *r, short item);
pascal short	FindToolItem(Point pt);
	void		DoToolContent(Point pt, INT16 modifiers);
	void		HandleToolCursors(INT16 item);
	void		PalKey(char);
	char		GetPalChar(INT16 item);
	void		DoToolKeyDown(INT16 ch, INT16 key, INT16 modifiers);
	void		DoToolGrow(Point pt);
	void		ChangeToolSize(INT16 across, INT16 down, Boolean doingZoom);
	void		GetToolZoomBounds(void);
	Boolean	SaveToolPalette(Boolean inquire);

/* Transcribe.c (formerly GuessDurs.c) */

	Boolean	AnyOverfullMeasure(Document *);
	void		DoQuantize(Document *);

/* TrackVMove.c */

	Boolean	InsTrackPitch(Document *, Point, INT16 *, LINK, INT16, INT16 *, INT16 *,
						INT16);
	INT16		InsTrackUpDown(Document *, Point, INT16 *, LINK, INT16, INT16 *);
	void		SetCrossbar(INT16, INT16, PCONTEXT);
	void		InvertCrossbar(Document *);
	INT16		CalcAccidental(INT16, INT16);
	INT16		CalcNewDurIndex(INT16, INT16, INT16);
	INT16		CalcNewDynIndex(INT16, INT16, INT16);

/* VoiceNumbers.c */

	void		OffsetVoiceNums(Document *, INT16, INT16);
	void		BuildVoiceTable(Document *, Boolean);
	void		MapVoiceNums(Document *, INT16 []);
	void		UpdateVoiceTable(Document *, Boolean);
	INT16		User2IntVoice(Document *, INT16, LINK);
	Boolean	Int2UserVoice(Document *, INT16, INT16 *, LINK *);
	INT16		NewVoiceNum(Document *, LINK);

/* Windows.c */

	Boolean	ActiveWindow(WindowPtr w);
	void		AnalyzeWindows(void);
	INT16		DoCloseAllDocWindows(void);
	void		DoCloseWindow(WindowPtr w);
	void		DoDragWindow(WindowPtr w);
	void		DoSelectWindow(WindowPtr w);
	void		DoZoom(WindowPtr w, INT16 part);
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

	LINK		AddNoteToSync(Document *, MNOTE, LINK, SignedByte, INT16, INT16, SignedByte,
						Boolean, long);
	Boolean	AutoBeam(Document *);
	Boolean	BuildDocument(Document *doc, unsigned char *fileName, short vRefNum, FSSpec *pfsSpec, long *version, Boolean isNew);
	LINK		CreateSync(Document *, MNOTE, LINK *, SignedByte, INT16, INT16, SignedByte,
						Boolean, long);
	void		DoAboutBox(Boolean first);
	void		DoAutoBeam(Document *);
	void		DoClickErase(Document *doc, Point pt);
	void		DoClickFrame(Document *doc, Point pt);
	void		DoClickInsert(Document *doc, Point pt);
	Boolean	DoEditMaster(Document *doc, Point pt, INT16 modifiers, INT16 dblClick);
	Boolean	DoEditScore(Document *doc, Point pt, INT16 modifiers, INT16 dblClick);
	void		DoPageSetup(Document *doc);
	void		DoPrint(Document *doc);
	void		DoPrintScore(Document *doc);
	Boolean	DoPostScript(Document *doc);
	void		DrawDocumentControls(Document *doc);
	void		DrawDocumentView(Document *doc, Rect *updateRect);
	void		ExportDeskScrap(void);
	Boolean	ClipboardChanged(ScrapRef scrap);
	void		FillInWatershed128 (DoubleWord, Word, Byte, Byte, Boolean, Byte);
	void		FillMem(Byte, void *, DoubleWord);
	void		FixCrossSysObjects(Document *, LINK, LINK);
	void		ImportDeskScrap(void);
	Boolean	ImportMIDIFile(FSSpec *);
	void		MPDrawParams(Document *doc, LINK obj, LINK subObj, INT16 param, INT16 d);
	void		NSInvisify(Document *doc);
	void		OffsetSystem(LINK sysL, INT16 dh, INT16 dv);
	void		PrefsDialog(Document *, Boolean, INT16 *);
	unsigned INT16 ProcessScore(Document *, INT16, Boolean);
	void		QuickScroll(Document *doc, INT16 dx, INT16 dy, Boolean relCoords, Boolean doCopyBits);
	void		RecomputeView(Document *doc);
	void		SaveEPSF(void);
	long 		LastEndTime(Document *doc, LINK fromL, LINK toL);
	void		SaveMIDIFile(Document *);
	void		SaveNotelist(Document *, INT16, Boolean);
pascal void		ScrollDocument(ControlHandle control, INT16 part);
	void		SetBackground(Document *doc);
	DDIST		SetStfInvis(Document *doc, LINK pL, LINK aStaffL);
	void		UpdateSysMeasYs(Document *doc, LINK sysL, Boolean useLedg, Boolean masterPg);
	void		XLoadEditScoreSeg(void);
	void		XLoadSheetSetupSeg(void);
	Boolean	XStfObjOnStaff(LINK, INT16);
