/* SearchScorePrivate.h for Nightingale, from the OMRAS version of ca. 2007. Only Search
Score files should include this. */

#ifdef SEARCH_SCORE_MAIN
	#define SS_GLOBAL
#else
	#define SS_GLOBAL extern
#endif


/* Macros and functions for accessing the data structure of scores being searched. We
use these to isolate the search data structure from Nightingale's object list and thereby
facilitate searching of a different data structure. FIXME: Since "regular" Nightingale
doesn't have the OMRAS' version's pseudo-database feature (which never worked very well
anyway), the below definitions could be greatly simplified; likewise, references to
SEARCH_DBFORMAT_MEF in other search-related code could be removed.

NB: These are not used for the query data structure. We assume the query will always
begin as a Nightingale object list and will be converted to our SEARCHPAT form before
we start searching. */


/* Data-structure access macros/routines */

#ifdef SEARCH_DBFORMAT_MEF

#define MEF_LINK							long
#define MEF_NILINK							0L
#define MEF_HEADL							1L
#define MEF_LeftLINK(oLink)					((oLink)-1>=MEF_HEADL? (oLink)-1 : DB_NILINK)
#define MEF_RightLINK(oLink)				((oLink)+1<objCount? (oLink)+1 : DB_NILINK)
#define MEF_FirstSubLINK(oLink)				obj[oLink].firstSO
#define MEF_NoteNUM(aNoteL)					subobj[aNoteL].su.nw.noteNum
#define MEF_NoteREST(aNoteL)				(subobj[aNoteL].su.nw.noteNum==0)
#define MEF_NoteTIEDL(aNoteL)				(subobj[aNoteL].su.nw.tiedL)
#define MEF_NoteTIEDR(aNoteL)				(subobj[aNoteL].su.nw.tiedR)
#define MEF_SyncTYPE(oLink)					(obj[oLink].opcode==MEFTYPE_SYNC)
#define MEF_StringTYPE(oLink)				(obj[oLink].opcode==MEFTYPE_LABEL)
#define MEF_VOICE_MAYBE_USED(doc, v)		(voiceUsed[v])
#define MEF_StringMaybeSectionID(graphicL)	TRUE

typedef struct {
	char		name[256];				/* Filename (C string) (unused) */
	MEF_LINK 	headL,					/* links to header and tail objects */
				tailL;
	short		lookVoice;				/* Voice to look at, or ANYONE for all voices */
} MEF_Document;

#define DB_Document								MEF_Document

#define DB_LINK									MEF_LINK
#define DB_NILINK								MEF_NILINK

#define DB_LeftLINK(oLink)						MEF_LeftLINK(oLink)
#define DB_RightLINK(oLink)						MEF_RightLINK(oLink)
#define DB_FirstSubLINK(oLink)					MEF_FirstSubLINK(oLink)
#define DB_IsAfter(obj1, obj2)					MEF_IsAfter(obj1, obj2)

#define DB_NextNOTEL(syncL, aNoteL)				MEF_NextNOTEL(syncL, aNoteL)

#define DB_NoteREST(aNoteL)						MEF_NoteREST(aNoteL)
#define DB_NoteTIEDL(aNoteL)					MEF_NoteTIEDL(aNoteL)
#define DB_NoteTIEDR(aNoteL)					MEF_NoteTIEDR(aNoteL)
#define DB_NoteNUM(aNoteL)						MEF_NoteNUM(aNoteL)
#define DB_NoteVOICE(syncL, aNoteL)				MEF_NoteVOICE(syncL, aNoteL)
#define DB_SimpleLDur(aNoteL)					MEF_SimpleLDur(aNoteL)
#define DB_SimpleLDurOrCode(aNoteL)				MEF_SimpleLDur(aNoteL)
#define DB_NoteType(aNoteL)						MEF_NoteType(aNoteL)

#define DB_SyncTYPE(oLink)						MEF_SyncTYPE(oLink)
#define DB_StringTYPE(oLink)					MEF_StringTYPE(oLink)

#define DB_FindFirstNote(syncL, voice)			MEF_FindFirstNote(syncL, voice)
#define DB_ChordNextNR(syncL, theNoteL)			MEF_ChordNextNR(theNoteL)
#define DB_GetExtremeNotes(syncL, voice, pLowNoteL, pHiNoteL) \
												MEF_GetExtremeNotes(syncL, voice, pLowNoteL, pHiNoteL)
#define DB_NoteNum2Note(syncL, voice, noteNum) \
												MEF_NoteNum2Note(syncL, voice, noteNum)
#define DB_SyncAbsTime(syncL)					obj[syncL].time

#define DB_VCountNotes(voice, startL, endL, chords) \
												MEF_VCountNotes(voice, startL, endL, chords)

#define DB_StringSearch(startL, goLeft)			MEF_StringSearch(startL, goLeft)
#define DB_FilenameSearch(startL, goLeft)		MEF_FilenameSearch(startL, goLeft)
#define DB_LVSSearch(startL, voice, goLeft)		MEF_LVSSearch(startL, voice, goLeft)
														
#define DB_StringMaybeSectionID(graphicL)		MEF_StringMaybeSectionID(graphicL)

#define DB_GetNString(graphicL, isFilename, theStr)	MEF_GetNString(graphicL, isFilename, theStr)

#define DB_VOICE_MAYBE_USED(doc, v)				MEF_VOICE_MAYBE_USED(doc, v)

#define DB_UserNextVoice(doc, voice)			MEF_UserNextVoice(doc, voice)

#define DEBUGPR FALSE
#define MINIMAL_PR FALSE

#define MAX_STRINGLEN 200						/* Maximum no. of chars. in a string */
#define MAX_SUBOBJS MAX_STRINGLEN				/* Max. no. of subobjects any object can have */

#define HEADER_STRING "$ MEF-V1"				/* Required header, including file-format version */


/*  Object and subobject structs */

typedef struct {
	long			time;
	unsigned char	opcode;
	unsigned char	subobjCount;
	long			firstSO;
} MEF_OBJ;

typedef struct {
	unsigned char	last:1;					/* TRUE = last note of chord, or only note or rest */
	unsigned char	noteNum:7;				/* MIDI note number */
	unsigned char	tiedL:1;				/* TRUE if note is tied to previous note */
	unsigned char	tiedR:1;				/* TRUE if note is tied to next note */
	unsigned char	slurredL:1;				/* TRUE if a slur extends from this note to the left */
	unsigned char	slurredR:1;				/* TRUE if a slur extends from this note to the right */	
	unsigned char	filler:4;				/* unused */	
} MEF_NWORD;

typedef struct {
	union {
		unsigned char	data;
		MEF_NWORD		nw;
	} su;
} MEF_SUBOBJ;


SS_GLOBAL long objArrayCount, subobjArrayCount;	/* Max. numbers of objects and of subobjects */


/* Object types ("opcodes") */

#define MEFTYPE_NONE 0
#define MEFTYPE_SYNC 1
#define MEFTYPE_NRC 2
#define MEFTYPE_FILENAME 3
#define MEFTYPE_LABEL 4
#define MEFTYPE_MEASURE 5
#define MEFTYPE_HEADER 6
#define MEFTYPE_TAIL 7
#define COUNT_MEFTYPES (MEFTYPE_TAIL+1)

/* The object and subobject lists */

SS_GLOBAL MEF_OBJ *obj;
SS_GLOBAL MEF_SUBOBJ *subobj;
SS_GLOBAL long objArrayLen, subobjArrayLen;				/* Amount available */
SS_GLOBAL long objCount, subobjCountTotal;				/* Amount in use */
SS_GLOBAL Boolean voiceUsed[MAXVOICES+1];

/* MEF-specific prototypes */

Boolean MEF_IsAfter(MEF_LINK obj1, MEF_LINK obj2);
MEF_LINK MEF_ChordNextNR(MEF_LINK theNoteL);
MEF_LINK MEF_FindFirstNote(MEF_LINK syncL, short voice);
void MEF_GetExtremeNotes(MEF_LINK syncL, INT16 voice, MEF_LINK *pLowNoteL, MEF_LINK *pHiNoteL);
MEF_LINK MEF_NextNOTEL(MEF_LINK syncL, MEF_LINK aNoteL);
MEF_LINK MEF_NoteNum2Note(MEF_LINK syncL, INT16 voice, INT16 noteNum);
short MEF_NoteType(MEF_LINK aNoteL);
short MEF_NoteVOICE(MEF_LINK syncL, MEF_LINK aNoteL);
long MEF_SimpleLDur(MEF_LINK aNoteL);
unsigned INT16 MEF_VCountNotes(INT16 v, MEF_LINK startL, MEF_LINK endL, Boolean chords);

#else	// SEARCH_DBFORMAT_MEF

#define StringMaybeSectionID(graphicL)		(GraphicINFO(graphicL)==SECTION_ID_NSTYLE)

#define DB_Document							Document

#define DB_LINK								LINK
#define DB_NILINK							NILINK

#define DB_RightLINK(oLink)					RightLINK(oLink)
#define DB_FirstSubLINK(oLink)				FirstSubLINK(oLink)
#define DB_IsAfter(obj1, obj2)				IsAfter(obj1, obj2)

#define DB_NextNOTEL(syncL, aNoteL)			NextNOTEL(aNoteL)

#define DB_NoteREST(aNoteL)					NoteREST(aNoteL)
#define DB_NoteTIEDL(aNoteL)				NoteTIEDL(aNoteL)
#define DB_NoteTIEDR(aNoteL)				NoteTIEDR(aNoteL)
#define DB_NoteNUM(aNoteL)					NoteNUM(aNoteL)
#define DB_NoteVOICE(syncL, aNoteL)			NoteVOICE(aNoteL)
#define DB_SimpleLDur(aNoteL)				SimpleLDur(aNoteL)
#define DB_NoteType(aNoteL)					NoteType(aNoteL)

#define DB_SyncTYPE(oLink)					SyncTYPE(oLink)

#define DB_FindFirstNote(syncL, voice)		FindFirstNote(syncL, voice)
#define DB_ChordNextNR(syncL, theNoteL)	ChordNextNR(syncL, theNoteL)
#define DB_SimpleLDurOrCode(aNoteL)			N_SimpleLDurOrCode(aNoteL)
#define DB_GetExtremeNotes(syncL, voice, pLowNoteL, pHiNoteL) \
											GetExtremeNotes(syncL, voice, pLowNoteL, pHiNoteL)
#define DB_NoteNum2Note(syncL, voice, noteNum) \
											NoteNum2Note(syncL, voice, noteNum)
#define DB_SyncAbsTime(syncL)				SyncAbsTime(syncL)

#define DB_VCountNotes(voice, startL, endL, chords) \
											VCountNotes(voice, startL, endL, chords)

#define DB_StringSearch(startL, goLeft) \
											StringSearch(startL, goLeft)
#define DB_LVSSearch(startL, voice, goLeft) \
											LVSSearch(startL, voice, goLeft)

#define DB_StringMaybeSectionID(graphicL)	StringMaybeSectionID(graphicL)

#define DB_GetNString(graphicL, isFilename, theStr)	GetNString(graphicL, theStr)

#define DB_VOICE_MAYBE_USED(doc, v)			VOICE_MAYBE_USED(doc, v)

#define DB_Sel2UserNextVoice(doc)			Sel2UserNextVoice(doc)
#define DB_UserNextVoice(doc, voice)		UserNextVoice(doc, voice)

/* Native-format-specific prototypes */

LINK FindFirstNote(LINK pL, INT16 voice);

#endif /* SEARCH_DBFORMAT_MEF */


/* Search-parameter block */

typedef struct {
	Boolean usePitch;
	Boolean useDuration;
	FASTFLOAT pitchWeight;				/* 0.0 to 1.0; duration weight = 1.0 - this */
	INT16 pitchSearchType;
	INT16 durSearchType;
	Boolean includeRests;
	INT16 maxTranspose;
	INT16 pitchTolerance;
	Boolean pitchKeepContour;
	INT16 chordNotes;
	Boolean matchTiedDur;
} SEARCHPARAMS;


/* Search-pattern struct and associated constants */

#define MAX_PATLEN 32		/* Maximum pattern length. Caveat: see note in SearchScore(). */

typedef struct {
	INT16 patLen;
	Boolean noteRest[MAX_PATLEN];
	Boolean tiedL[MAX_PATLEN];
	INT16 noteNum[MAX_PATLEN];
	INT16 prevNoteNum[MAX_PATLEN];	/* of previous note, if any--not rest */
	INT16 lDur[MAX_PATLEN];			/* <= 0 = _DUR code, else value from SimpleLDur */
	INT16 durPlay[MAX_PATLEN];		// ??change to <pDur>?
} SEARCHPAT;


typedef struct {
	INT16 pitchErr;			/* Pitch error (semitones; 0 if we don't care) */
	INT16 durationErr;		/* Duration error (0 if expected value or don't care, 1 otherwise) */
} ERRINFO;


#define MAX_HITS 250

typedef struct {
	INT16		docNum;				/* Index into docTable */
	DB_LINK	foundL;
	char		scoreName[32];
	INT16		measNum;
	char		locStr[32];
	INT16		foundVoice;
	char		vInfoStr[32];
	ERRINFO	totalError;
	INT16		relEst;				/* Relevance estimate (percent) */
} MATCHINFO;

typedef struct {
	long nDocsMissingFonts;
	long nDocsNonstandardSize;
	long nDocsTooManyPages;
	long nDocsPageSetupPblm;
	long nDocsOther;	
} InfoIOWarning;

/* Implementation necessitated defines */

#define RESLISTKIND 		103

#define resListDocWindowID	2001

SS_GLOBAL Document *gResListDocument;


/* Prototypes for "semi-public" routines: should be called only by Search Score routines */

/* Defined in SearchScore.c */

INT16 CalcRelEstimate(ERRINFO errInfo, INT16 pitchTolerance, FASTFLOAT pitchWeight, INT16 patLen);
void FormatReportString(SEARCHPARAMS sp, SEARCHPAT searchPat, char *findLabel, char *str);
void ListMatches(MATCHINFO matchInfoA[], DB_LINK matchedObjFA[][MAX_PATLEN],
			DB_LINK matchedSubobjFA[][MAX_PATLEN], INT16 patLen, INT16 nFound,
			long elapsedTicks, SEARCHPARAMS sp, Boolean showScoreName);
Boolean SearchIsLegal(Boolean usePitch, Boolean useDuration);
INT16 N_SearchPatternLen(Boolean *pHaveRest);
DB_LINK SearchGetHitSet(DB_Document *doc, DB_LINK startL, SEARCHPAT searchPat,
			SEARCHPARAMS sp, Boolean foundVoice[], ERRINFO totalErrorInfo[],
			DB_LINK matchedObjA[][MAX_PATLEN], DB_LINK matchedSubobjA[][MAX_PATLEN]);
Boolean N_SearchScore2Pattern(Boolean includeRests, SEARCHPAT *pSearchPat,
										Boolean matchTiedDur, Boolean *pHaveChord);
void WarnHaveChord(void);

/* Defined in SearchScoreNative.c (as well as FindFirstNote(); see above */

DB_LINK SearchScore(DB_Document *doc, DB_LINK startL, INT16 startVoice, SEARCHPAT searchPat,
						SEARCHPARAMS sp, INT16 *pFoundVoice, ERRINFO *pTotalErrorInfo);
void SelectSubobjA(DB_Document *doc, LINK matchedObjA[][MAX_PATLEN],
			LINK matchedSubobjA[][MAX_PATLEN], INT16 matchVoice, INT16 count,
			Boolean matchTiedDur);
Boolean GetScoreLocIDString(DB_Document *doc, DB_LINK locL, char matchLocString[256]);

/* Defined in ResultList.c */

Boolean InitResultList(INT16 maxItems, INT16 patLen);
Boolean AddToResultList(char str[], MATCHINFO matchInfo, DB_LINK matchedObjA[MAX_PATLEN],
						DB_LINK matchedSubobjA[MAX_PATLEN]);
Boolean DoResultList(char label[]);

Boolean BuildDocList(Document *doc, short fontSize);
Boolean HandleResListUpdate(void);
Boolean HandleResListActivate(Boolean activ);
Boolean HandleResListMouseDown(Point where, int modifiers);
Boolean DoResultListDoc(char label[]);

/* Defined in ResultListDocument.c */

Boolean BuildResListDocument(register Document *doc);
Boolean DoCloseResListDocument(register Document *doc);
void ActivateResListDocument(register Document *doc, INT16 activ);
void ShowResListDocument(Document *doc);
Boolean DoOpenResListDocument(Document **pDoc);

/* Defined who knows where? */

void DB_GetNString(DB_LINK graphicL, Boolean isFilename, char theStr[]);
DB_LINK DB_StringSearch(DB_LINK startL, Boolean goLeft);
DB_LINK DB_LVSSearch(DB_LINK startL, INT16 voice, Boolean goLeft);

INT16 DB_Sel2UserNextVoice(DB_Document *doc);
INT16 DB_UserNextVoice(DB_Document *doc, INT16 voice);

Boolean DoResultList1(char label[]);

