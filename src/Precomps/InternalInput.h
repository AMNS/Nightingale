/* InternalInput.h for Nightingale */

LINK IICreateTUPLET(Document *doc, short voice, short staffn, LINK beforeL, short nInTuple,
							short tupleNum, short tupleDenom, Boolean numVis, Boolean denomVis, Boolean brackVis);
void IIFixTupletLinks(Document *doc, LINK startL, LINK endL, short voice);
Boolean IIReplaceKeySig(Document *doc, LINK keySigL, short staffn, short sharpsOrFlats);
Boolean IIInsertWholeMeasRest(Document *doc, LINK insertBeforeL, short staffn, short iVoice, Boolean visible);
LINK IIInsertBarline(Document *doc, LINK insertBeforeL, short barlineType);
Boolean SetMeasureSubType(LINK measL, short subType);
LINK IIInsertClef(Document *doc, short staffn, LINK insertBeforeL, short clefType, Boolean small);
LINK IIInsertTimeSig(Document *doc, short staff, LINK insertBeforeL,
								short type, short numerator, short denominator);
LINK IIInsertKeySig(Document *doc, short staff, LINK insertBeforeL, short sharpsOrFlats);
Boolean IICombineKeySigs(Document *doc, LINK startL, LINK endL);
LINK IIInsertDynamic(Document *doc, short staffn, LINK insertBeforeL, LINK anchorL, short dynamicType);
LINK IIInsertGRString(Document *doc, short staffn, short iVoice, LINK insertBeforeL, LINK anchorL,
								Boolean isLyric, short textStyle, PTEXTSTYLE pStyleRec, char string[]);
LINK IIInsertTempo(Document *doc, short staffn, LINK insertBeforeL, LINK anchorL, char durCode,
							Boolean dotted, Boolean hideMM, char tempoStr[], char metroStr[]);
LINK IIInsertSync(Document *doc, LINK insertBeforeL, short numSubobjects);
LINK IIInsertGRSync(Document *doc, LINK insertBeforeL, short numSubobjects);
LINK IIAddNoteToSync(Document *doc, LINK syncL);
LINK IIInsertSlur(Document *doc, LINK firstSyncL, LINK firstNoteL,
							LINK lastSyncL, LINK lastNoteL, Boolean setShapeNow);
short IICreateAllSlurs(Document *doc);
void IIReshapeSlursTies(Document *doc);
void IIFixAllNoteSlurTieFlags(Document *doc);
Boolean IIAnchorAllJDObjs(Document *doc);
void IIAutoMultiVoice(Document *doc, Boolean doSingle);
Boolean IIJustifySystem(Document *doc, LINK systemL);
Boolean IIJustifyAll(Document *doc);

FILE *OpenInputFile(Str255 macfName, short vRefNum);
short CloseInputFile(FILE *f);
Boolean ReadLine(char inBuf[], short maxChars, FILE *f);
Boolean SaveTextToFile(Ptr pText, Str255 suggestedFileName, short promptStrID);

short IIOpenInputFile(FSSpec *fsSpec, short *refNum, Boolean readOnly);
Boolean IIReadLine(char inBuf[], short maxChars, short refNum);
short IICloseInputFile(short refNum);
short IIRewind(short refNum);
short IIReadChar(short refNum, char *c);
short IIUngetChar(short refNum);
