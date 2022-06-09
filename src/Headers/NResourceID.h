/* NResourceID.h for Nightingale - Symbolic definitions for resource ID's */

/* Alerts and Dialogs */

#define BIMIDI_PRINTERPORT_ALRT 170
#define NOTJUST_ALRT 180
#define HIDDENSTAVES_ALRT 184
#define STFSIZE_ALRT 190				/* Staves aren't the same size as the standard size */
#define MANYPAGES_ALRT 200				/* More pages than can be viewed w/just scroll bar */
#define NO_CM_DEVS_ALRT 201				/* With Core MIDI (& formerly OMS), no devices avail. for playback */
#define EXPORT_ALRT 202					/* Export master page changes to score? */
#define EXPORTFMT_ALRT 203				/* Export master page changes and lose Work on Format changes? */
#define DANGLING_ALRT 204				/* Export master page results in dangling systems */
#define NOPARTS_ALRT 205				/* Can't leave master page with no parts */
#define DANGL_CONTENTS_ALRT 206			/* Export master page results in dangling contents */
#define LOSESELECT_ALRT 210				/* Deleting part will forget selection */
#define DELPART_ALRT 213				/* Can't delete more than one part at a time. */
#define DELPART_ANYWAY_ALRT 214			/* Delete Staff/Part, even if it has content? */
#define TRANSKEY_ALRT 220
#define SMALL_GENERIC_ALRT 230
#define SETSTAFF_CHORD_ALRT 240 
#define SETVOICE_CHORD_ALRT 241
#define SETSTAFF_ALRT 245
#define CNFG_ALRT 250
#define CNFGSIZE_ALRT 252
#define RFMT_EXACT_ALRT 258
#define RFMT_XSYS1_ALRT 260
#define RFMT_XSYS2_ALRT 261
#define SAVEMF_TIMESIG_ALRT 265
#define OPENMF_TIMESIG_ALRT1 266
#define OPENMF_TIMESIG_ALRT2 267
#define CLEAR_BARS_ALRT 270
#define STUPID_KILL_ALRT 280
#define OPENNOTELIST_ALRT 284
#define COARSE_QUANTIZE_ALRT 290
#define TOO_COARSE_QUANT_ALRT 294
#define TOO_FINE_QUANT_ALRT 296
#define SDBEAM_ALRT 312					/* Set Duration must unbeam selection */
#define BBBEAM_ALRT 314					/* Beam by Beat must unbeam selection */
#define RFMT_HIDDENSTAVES_ALRT 316
#define RFMT_QDOVERFLOW_ALRT 318
#define SFVIS_ALL_ALRT 320				/* Cannot visify all staves: reformat? */
#define SFVIS_ALL_ALRT1 321				/* Cannot visify all staves */
#define RTM_TIMESIG_ALRT 324
#define SAVECNFG_ALRT 330
#define USEMM_ALRT 335

#define SAVEMM_ALRT 340					/* Save MIDI Manager port hookup? */
#define OPENDOCS_ALRT 344
#define ADDBAR_ALRT 350
#define RSP_OVERFLOW_ALRT 358			/* At least one system will overflow; may need to reformat */
#define MORE_CLIPSTAVES_ALRT 360		/* More staves in clipboard than document */
#define ACCIDENTAL_TIE_ALRT 364
#define BREAKBEAM_ALRT 365
#define FLIPFRACBEAM_ALRT 366
#define MUSFONT_ALRT 368
#define READ_ALRT 370
#define READMIDI_ALRT 371
#define SAVE_ALRT 372
#define SAVEMF_ALRT 373
#define SAVENOTELIST_ALRT 374
#define TRANSCRIBE_ALRT 375
#define READNOTESCAN_ALRT 376
#define OPENTEXTFILE_ALRT	378
#define WRITETEXTFILE_ALRT	379
#define SAFESAVE_ALRT 380
#define SAFESAVE1_ALRT 381
#define SAFESAVEINFO_ALRT 385
#define READ_PROBLEM_ALRT 387
#define JUSTSPACE_ALRT 390				/* Justify may not give good spacing */
#define OPENPREFS_ALRT 393
#define MODNRINFO_ALRT 399				/* Bad value in Modifier Info */
#define EXTOBJ_ALRT 410
#define DEXTOBJ_ALRT 412
#define DOUBLE_ALRT 414
#define NOSHOW_LEFTEND_ALRT 420			/* Changing first system left end params... */
#define ACCIDENTAL_ALRT 430				/* Delete redundant accidentals? */
#define ADDACCIDENTAL_ALRT 432			/* Add redundant accidentals? */
#define TRANSP_ALRT 440					/* "Transposed Score" doesn't affect existing notes */
#define MANYFONTS_ALRT 444				/* Too many fonts */
#define MIDIBADVALUE_ALRT 462			/* MIDI setup value out of range */
#define GINFO_ALRT 463					/* Bad value in Get Info */
#define GENERIC_ALRT 464				/* Generic alert with one param and OK */
#define GENERIC2_ALRT 465				/* Generic alert with two params and OK */
#define GENERIC3_ALRT 467				/* Generic alert with one param, OK, and Cancel */
#define SAVEPALETTE_ALRT 469			/* Save changes to tools palette? */
#define SAVEETFLYRICS_ALRT 470
#define BIG_GENERIC_ALRT 474
#define SPACE_ALRT 479					/* Illegal spacing percent */
#define STAFFLEN_ALRT 480				/* Illegal staff length */
#define ILLNSP_ALRT 481					/* Illegal no. of staves per part */
#define ILLNS_ALRT 482					/* Illegal no. of staves */
#define FITSTAFF_ALRT 483				/* Can't fit stave(s) on score page */
#define RASTRAL_ALRT 484				/* Rastral must be 0 to whatever */
#define BADMARGIN_ALRT 486				/* Illegal margin */
#define DEBUGERR_ALRT 550				/* Lots of errors found by Debug */
#define BMP_ALRT 552					/* Can’t open BMP file */
#define PASTE_STFCLIP_ALRT 660			/* More staves in clipboard than below ins. pt. */

#define ABOUT_DLOG 451					/* "About Nightingale..." */
#define STAFFLINES_DLOG 454				/* Staff/Ledger lines dialog */
#define STAFFSIZE_DLOG 455				/* Set staff size dialog */
#define ADDPART_DLOG 456				/* Add part dialog */
#define MARGINS_DLOG 460
#define USEMIDIDR_DLOG 485				/* This replaces the USE...MIDI driver ALRTs (formerly 332-337) */
#define MIDISETUP_DLOG 489				/* Old MIDI setup dialog */
#define CM_MIDISETUP_DLOG 490			/* MIDI setup dialog for Core MIDI (and formerly OMS) */

#define METRO_DLOG 491 					/* Old metronome setup dialog */
#define CM_METRO_DLOG 492				/* Metronome setup dialog for Core MIDI (and formerly OMS) */
#define MIDITHRU_DLOG 493				/* MIDI Thru Preferences */
#define	KEYSIG_DLOG 500					/* Key Signature dialog */
#define	TIMESIG_DLOG 510				/* Time Signature dialog */

#define GROUPPARTS_DLOG 570
#define OWNER_DLOG 580
#define INSMEASUNKDUR_DLOG 590

#define SYNCINFO_DLOG 596				/* Get Info dialog for SYNCs & GRSYNCs */
#define EXTINFO_DLOG 597				/* Get Info dialog for extended objects */
#define GENERALINFO_DLOG 598			/* General Get Info dialog */
#define MODNRINFO_DLOG 599				/* Note Modifier Info dialog */

#define PREFS_DLOG 601					/* Preferences */
#define SCOREINFO_DLOG 640
#define REFORMAT_DLOG 650
#define RESPACE_DLOG 653
#define CHKMERGE_DLOG 670
#define EXTRACT_DLOG 685				/* Whether to save or open parts, etc. */
#define MBREST_DLOG 690

#define INSTRUMENT_DLOG 701
#define BALANCE_DLOG 702				/* Balance (part velocity) dialog */
#define PARTMIDI_DLOG 703
#define OMS_PARTMIDI_DLOG 704
#define GOTO_DLOG 705
#define CM_INSTRUMENT_DLOG 708
#define CM_PARTMIDI_DLOG 709
#define SET_DLOG 711					/* "QuickChange" (formerly Set) */
#define NEW_TEXT_DLOG 713
#define CHOOSECHAR_DLOG 714
#define TEXTSTYLE_DLOG 715
#define FLOWIN_DLOG 718
#define LOOK_DLOG 720					/* "Look at voice..." */
#define MEASNUM_DLOG 730
#define OPENFILE_DLOG 734
#define PAGENUM_DLOG 735
#define HEADERFOOTER_DLOG 736
#define FANCYTUPLET_DLOG 741
#define OTTAVA_DLOG 745
#define MULTIVOICE_DLOG 750
#define SETDUR_DLOG 761
#define SETDYN_DLOG 762
#define SETMOD_DLOG 763
#define ADDMOD_DLOG 764
#define QUANTIZE_DLOG 765
#define RECORD_DLOG 770
#define TRANSMIDI_DLOG 778
#define LEFTEND_DLOG 780
#define TRANSPOSE_DLOG 790				/* Chromatic transpose */
#define DTRANSPOSE_DLOG 792				/* Diatonic transpose */
#define TRANSKEY_DLOG 794				/* Transpose Key */
#define SYSOVERFLOW_DLOG 800
#define SLURTIE_DLOG 804
#define SCREENOVERFLOW_DLOG 806
#define DBLBAR_IS_BARLINE_DLOG 808		/* Should a new double bar or repeat bar be a barline? */
#define PSTYPE_DLOG 810
#define PASTE_DELREDACCS_DLOG 814
#define TREMSLASHES_DLOG 820
#define ENDING_DLOG 825
#define REHEARSALMK_DLOG 830
#define TEMPO_DLOG 832
#define CHORDSYM_DLOG 834
#define PATCHCHANGE1_DLOG 836
#define PNSETTING_DLOG 837
#define DOUBLE_DLOG 840
#define SETKEYSIG_DLOG 849				/* Set Key Sig, for Open NoteScan File */
#define SETCLEF_DLOG 850				/* Set Clef, for Open NoteScan File */
#define NSINFO_DLOG 851					/* Open NoteScan info */
#define PATCHCHANGE_DLOG 853
#define MIDIVELTABLE_DLOG 860
#define MIDIMODTABLES_DLOG 862
#define FILLEMPTY_DLOG 870				/* Fill empty measures */
#define RESPACE_BEF_MEAS_DLOG 874		/* Respace area before system's 1st measure */
#define MISSINGFONTS_DLOG 880
#define MFINFO_DLOG 890					/* MIDI File Info */
#define DURCHOICE_NODOTS_DLOG 884
#define DURCHOICE_1DOT_DLOG 886
#define MESSAGE_DLOG 900				/* Generic message with no controls */
#define MIDI_DRIVER_DLOG 903			/* MIDI driver low-level control dialog */
#define RECORDING_DLOG 910
#define ENABLENS_DLOG 914
#define NSERRLIST_DLOG 920
#define MIDIMAP_DLOG 940
#define COMBINEPARTS_DLOG 985				/* Whether to save or open parts, etc. */
#define PLAYSPEED_DLOG 1010
#define MUTEPART_DLOG 1012
#define DEBUG_DLOG 1024

/* Menus: all pulldowns, and some popups (more popups appear in #defines below) */

enum {
	appleID = 1,
	fileID,
	editID,
	testID,
	scoreID,
	notesID,
	viewID,
	playRecID,
	masterPgID,
	groupsID = 11,
	formatID = 13,

	magnifyID = 25,
	transposePopID = 30,
	gotoPopID,
	dTransposePopID,
	octavePopID
};

/* Menu item codes */

enum {
	AM_About = 1,				/* Apple menu */
	AM_Help
};
	
enum {							/* File menu */
	FM_New = 1,
	FM_Open,
	FM_OpenReadOnly,
	FM_Close,
	FM_CloseAll,
	FM_____________1,
	FM_Save,
	FM_SaveAs,
	FM_Revert,
	FM_Extract,
	FM_Import,
	FM_Export,
	FM_GetScan,
	FM_GetETF,
	FM_GetNotelist,
#ifdef USE_NL2XML
	FM_Notelist2XML,
#endif
	FM_OpenMidiMap,
	FM_____________2,
	FM_SheetSetup,
	FM_PageSetup,
	FM_Print,
	FM_SavePostScript,
	FM_SaveText,
	FM_____________3,
	FM_ScoreInfo,
	FM_Preferences,
	FM_SearchInFiles,
	FM_Quit
};

/* If you change the Edit Menu items, make sure EM_LastItem still equals the last menu
item.  --CER 5/17/2004 */

enum {							/* Edit menu */
	EM_Undo = 1,
	EM_____________1,
	EM_Cut,
	EM_Copy,
	EM_Paste,
	EM_Merge,
	EM_Clear,
	EM_____________2,
	EM_Double,
	EM_____________3,
	EM_ClearSystem,
	EM_CopySystem,
	EM_ClearPage,
	EM_CopyPage,
	EM_____________4,
	EM_SelAll,
	EM_ExtendSel,
	EM_____________5,
	EM_Set,
	EM_GetInfo,
	EM_ModifierInfo,
	EM_____________6,
	EM_SearchMelody,
	EM_SearchAgain,
	EM_____________7,
	EM_AddTimeSigs,
	EM_LastItem = EM_AddTimeSigs,
	EM_Browser = EM_LastItem+2,
	EM_Debug,
	EM_ShowDebug,
	EM_DeleteObjs
};

enum {							/* Test menu */
	TS_Browser = 1,
	TS_HeapBrowser,
	TS_Context,
	TS_____________1,
	TS_ClickErase,
	TS_ClickSelect,
	TS_ClickFrame,
	TS_____________2,
	TS_Debug,
	TS_ShowDebug,
	TS_____________3,
	TS_DeleteObjs,
	TS_ResetMeasNumPos,
	TS_PrintMusFontTables
};

enum {							/* Score menu */
	SM_MasterSheet = 1,
	SM_ShowFormat,
	SM_CombineParts,
	SM_____________1,
	SM_LeftEnd,
    SM_TextStyle,
	SM_MeasNum,
	SM_PageNum,
	SM_____________2,
    SM_AddSystem,
    SM_AddPage,
    SM_____________3,
    SM_FlowIn,
    SM_____________4,
    SM_Respace,
    SM_Reformat,
    SM_Justify,
    SM_Realign,
    SM_AutoRespace,
    SM_____________5,
    SM_MoveMeasUp,
    SM_MoveMeasDown,
    SM_MoveSystemUp,
    SM_MoveSystemDown
};

enum {							/* Notes menu */
    NM_SetDuration = 1,
	NM_CompactV,
	NM_InsertByPos,
    NM_ClarifyRhythm,
	NM_____________1,
    NM_SetMBRest,
	NM_FillEmptyMeas,
	NM_____________2,
	NM_Transpose,
	NM_DiatonicTranspose,
	NM_TransposeKey,
	NM_Respell,
	NM_Accident,
	NM_AddAccident,
	NM_CheckRange,
	NM_____________3,
    NM_AddModifiers,
    NM_StripModifiers,
	NM_____________4,
	NM_Multivoice,
    NM_Flip
};

enum {							/* Groups menu */
	GM_AutomaticBeam = 1,
    GM_Beam,
    GM_BreakBeam,
	GM_FlipFractional,
	GM_______________1,
    GM_Tuplet,
    GM_FancyTuplet,
    GM_______________2,
    GM_Ottava
};

enum {							/* View menu */
	VM_GoTo = 1,
	VM_GotoSel,
	VM_____________1,
	VM_Reduce,
	VM_Enlarge,
	VM_ReduceEnlargeTo,
	VM_____________2,
	VM_LookAtV,
	VM_LookAtAllV,
	VM_ShowDurProb,
	VM_ShowSyncL,
	VM_ShowInvis,
	VM_ShowSystems,
	VM_ColorVoices,
	VM_PianoRoll,
	VM_____________3,
	VM_RedrawScr,
	VM_____________4,
	VM_ShowClipboard,
	VM_ToolPalette,
	VM_ShowSearchPattern,
	VM_LastItem=VM_ShowSearchPattern
};

enum {							/* Play/Record menu */
    PL_PlayEntire = 1,
	PL_PlayFromSelection,
    PL_PlaySelection,
	PL_MutePart,
	PL_PlayVarSpeed,
	PL_AllNotesOff,
	PL_____________1,
	PL_RecordInsert,
	PL_Quantize,
	PL_RecordMerge,
	PL_StepRecInsert,
	PL_StepRecMerge,
	PL_____________2,
    PL_MIDISetup,
    PL_Metronome,
    PL_MIDIThru,
	PL_MIDIDynPrefs,
	PL_MIDIModPrefs,
    PL_____________3,
    PL_InstrMIDI,
	PL_Transpose
};
	
enum {							/* Master Page menu */
	MP_AddPart = 1,
	MP_DeletePart,
	MP_____________1,
	MP_GroupParts,
	MP_UngroupParts,
	MP_SplitPart,
	MP_DistributeStaves,
	MP_____________2,
	MP_StaffSize,
	MP_StaffLines,
	MP_EditMargins,
	MP_____________3,
	MP_Instrument
};

enum {							/* Format menu */
	FT_Invisify = 1,
	FT_ShowInvis
};

/* Miscellaneous temporarily-installed popup menus */

#define FONT_POPUP				20
#define SIZE_POPUP				21
#define STAFFREL_POPUP			22
#define TEXTSTYLE_POPUP			35
#define STYLECHOICE_POPUP		36
#define FLOWTEXTSTYLE_POPUP		37

/* Popup menus for the QuickChange (formerly Set) command */

#define LYRICSTYLE_POPUP FLOWTEXTSTYLE_POPUP	/* OK if they're never used together! */
#define	MAINSET_POPUP			49
#define	NOTEPROPERTY_POPUP		50
#define	TEXTPROPERTY_POPUP		54
#define TUPLETPROPERTY_POPUP	55
#define	HEADSHAPE_POPUP			51
#define	NOTESIZE_POPUP			52
#define	VISIBILITY_POPUP		53
#define ACCNUMVIS_POPUP			56
#define CHORDSYMPROPERTY_POPUP	57
#define NOTE_ACC_PARENS_POPUP	58
#define SLURPROPERTY_POPUP		59
#define SLUR_APPEARANCE_POPUP	60
#define TEMPOPROPERTY_POPUP		62
#define BARLINE_PROPERTY_POPUP	70
#define BARLINE_TYPE_POPUP	71
#define DYNAMIC_PROPERTY_POPUP	72
#define DYNAMIC_SIZE_POPUP		73
#define BEAMSET_PROPERTY_POPUP	74
#define BEAMSET_THICKNESS_POPUP	75

#define ENDING_MENU 80
#define PARTS_MENU 101

#define	CARD_MENU 133					/* Popup menus for the Preferences dialog */
#define	SPACETBL_MENU 134
#define	MUSFONT_MENU 135

#define NODOTDUR_MENU 139
#define ONEDOTDUR_MENU 140
#define TWODOTDUR_MENU 141
#define DYNAMIC_MENU 142
#define MODIFIER_MENU 143

#define STAFFLINES_MENU 150				/* For Staff/Ledger lines dialog */

#define INSTR_STRS 130					/* Instrument definition strings */
#define INFOUNITS_STRS 140				/* Units for distances in Get Info dialog */
#define HEADERFOOTER_STRS 145			/* Strings for Header/footer dialog */
#define MENUCMD_STRS 150				/* Menu commands whose wording is variable */
#define UNDO_STRS 160					/* Menu command strings for use in Undo */
#define UNDOWORD_STRS 161				/* "Undo" and "Redo" words/strings */
#define FONT_STRS 170					/* Font name strings */
#define SAVEPREFS_STRS 220				/* Strings warning about saving various Preferences */

#define INITERRS_STRS 229				/* Initialization error messages */
#define MPERRS_STRS 230					/* Master Page error messages */
#define MIDIERRS_STRS 231				/* MIDI error messages */
#define DOUBLE_STRS 232					/* Double error messages */
#define MISCERRS_STRS 233				/* Miscellaneous error messages */
#define SYSPE_STRS 234					/* Clear/Copy/Pages System/Page error messages */
#define MIDIPLAYERRS_STRS 235			/* MIDI playback error messages */
#define NOTESCANERRS_STRS 236			/* Open NoteScan file error messages */
#define DIALOGERRS_STRS 240				/* Miscellaneous dialog error messages */
#define FILEIO_STRS 241					/* File I/O error messages */
#define INFOERRS_STRS 250				/* Get Info error messages */
#define MIDIFILE_STRS 260				/* MIDI File Open/Save strings */
#define BEAMERRS_STRS 261				/* Beaming error strings */
#define CHECKERRS_STRS 262				/* Object-checking (Check.c, etc.) error strings */
#define CLIPERRS_STRS 263				/* Clipboard error strings, incl. Paste Merge */
#define INSTRERRS_STRS 265				/* Instrument dialog error strings */
#define INSERTERRS_STRS 266				/* Insert error strings */
#define SLURERRS_STRS 267				/* Slur error strings */
#define OCTAVEERRS_STRS 268				/* Octave sign error strings */
#define PITCHERRS_STRS 269				/* Pitch error strings */
#define PREFSERRS_STRS 270				/* Preferences dialog error strings */
#define PRINTERRS_STRS 271				/* Printing and PostScript error strings */
#define RFMTDLOG_STRS 272				/* Reformat dialog strings */
#define RHYTHMERRS_STRS 273				/* Rhythm-handling error strings */
#define SETERRS_STRS 274				/* QuickChange (formerly Set) error strings */
#define TEXTERRS_STRS 275				/* Text-edit dialog error strings */
#define TUPLETERRS_STRS 276				/* Tuplet error strings */
#define MEASFILLERRS_STRS 278			/* Fill Empty Measures error strings */
#define MENUCMDMSGS_STRS 279			/* Misc. messages for menu commands */
#define SET_STRS 280					/* QuickChange (formerly Set) non-error strings */
#define ENIGMA_STRS 285					/* Enigma converter strings (no longer used) */
#define NOTELIST_STRS 286				/* Open Notelist strings */
#define MPGENERAL_STRS 290				/* Master Page general (non-error) strings */
#define DIALOG_STRS 300					/* Misc. dialog non-error and feedback strings */
#define DURATION_STRS 310				/* Duration units (plural) */
#define FLOWIN_STRS 318					/* Flow-in text strings */
#define MIDIPLAY_STRS 320				/* MIDI Play non-error strings */
#define MIDIREC_STRS 322				/* MIDI Record non-error strings */
#define COPYPROTECT_STRS 330			/* Messages related to copy protection */
#define EXTOBJ_STRS 334					/* Strings naming truncated extended objects */
#define SCOREINFO_STRS 340				/* Strings for the Score Info command */
#define MESSAGEBOX_STRS 350				/* Strings that appear in the message box */
#define ENDING_STRS 360					/* Ending labels */
#define MIDIMAPINFO_STRS 370			/* Strings for the Midi Map Info dialog */
#define TEXTPREFS_STRS 380				/* Text prefs strings */


#define MESSAGE_STRS 400				/* Progress messages */

/* Progress message strings. If you add any, also update MAX_MESSAGE_STR elsewhere. */

#define ARRANGEMEAS_PMSTR 1
#define ARRANGESYS_PMSTR 2
#define MIDIDR_MM_PMSTR 3
#define MIDIDR_BI_PMSTR 4
#define TIMINGTRACK_PMSTR 5
#define OTHERTRACK_PMSTR 6
#define RESPACE_PMSTR 7
#define SKIPTRACKS_PMSTR 8
#define SKIPPARTS_PMSTR 9
#define CREATE_PREFS_PMSTR 10
#define MIDIDR_NONE_PMSTR 11
#define VOICE_PMSTR 12
#define SKIPVOICE_PMSTR 13
#define CONVERTETF_PMSTR 14
#define MIDIDR_OMS_PMSTR 15
#define CONVERTNOTELIST_PMSTR 16
#define CONTINUENOTELIST_PMSTR 17
#define CHANGE_TOPMARGIN_PMSTR 18

/* Cursors */

#define RHAND_CURS 128					/* Right hand cursor */
#define LHAND_CURS 129					/* Left hand cursor */
#define FLOWIN_CURS 280					/* Graphic-insertion cursor */
#define THREAD_CURS 2004				/* 'Threader' cursor */
#define ARROW_CURS 2005					/* Arrow cursor */
#define DRAG_CURS 2006					/* Cursor for beam & slur dragging */
#define GENLDRAG_CURS 2008				/* Cursor for normal dragging */
#define MIDIPLAY_CURS 3000				/* Cursor displayed while playing MIDI */

/* Miscellaneous */

#define PREFS_MIDI 128					/* MIDI preferences (dynamics->velocity) resource */

#define PREFS_MIDI_MODNR 128			/* MIDI modifier preferences resource */

#define THE_CNFG 128					/* General configuration resource */

#define THE_PLMP 128					/* Palette remapping resource */

#define THE_BIMI 128					/* Built-in MIDI port and interface speed (no longer used) */

#define NGALE_ICON 128

#define CHOOSECHAR_LDEF 128				/* 0=standard LDEF, 128 our special LDEF */

#define	ABOUT_TEXT	ABOUT_DLOG

/* MIDI Manager time, input, and output Port Info Records */

#define portResType		'port'
#define time_port		128		
#define input_port		129	
#define output_port		130	
