/* FileInput.h for Nightingale */

#ifdef NOMORE
#include "stdioxtra.h"		/* NG can't easily include <stdio.h>! */
#endif

LINK FICreateTUPLET(Document *doc, short voice, short staffn, LINK beforeL, short nInTuple,
							short tupleNum, short tupleDenom, Boolean numVis, Boolean denomVis, Boolean brackVis);
void FIFixTupletLinks(Document *doc, LINK startL, LINK endL, short voice);
Boolean FIReplaceKeySig(Document *doc, LINK keySigL, INT16 staffn, INT16 sharpsOrFlats);
Boolean FIInsertWholeMeasRest(Document *doc, LINK insertBeforeL, short staffn, short iVoice, Boolean visible);
LINK FIInsertBarline(Document *doc, LINK insertBeforeL, short barlineType);
Boolean SetMeasureSubType(LINK measL, short subType);
LINK FIInsertClef(Document *doc, short staffn, LINK insertBeforeL, short clefType, Boolean small);
LINK FIInsertTimeSig(Document *doc, short staff, LINK insertBeforeL,
								short type, short numerator, short denominator);
LINK FIInsertKeySig(Document *doc, short staff, LINK insertBeforeL, short sharpsOrFlats);
Boolean FICombineKeySigs(Document *doc, LINK startL, LINK endL);
LINK FIInsertDynamic(Document *doc, short staffn, LINK insertBeforeL, LINK anchorL, short dynamicType);
LINK FIInsertGRString(Document *doc, short staffn, short iVoice, LINK insertBeforeL, LINK anchorL,
								Boolean isLyric, short textStyle, PTEXTSTYLE pStyleRec, char string[]);
LINK FIInsertTempo(Document *doc, short staffn, LINK insertBeforeL, LINK anchorL, char durCode,
							Boolean dotted, Boolean hideMM, char tempoStr[], char metroStr[]);
LINK FIInsertSync(Document *doc, LINK insertBeforeL, short numSubobjects);
LINK FIInsertGRSync(Document *doc, LINK insertBeforeL, short numSubobjects);
LINK FIAddNoteToSync(Document *doc, LINK syncL);
LINK FIInsertModNR(Document *doc, char modCode, char data, LINK syncL, LINK aNoteL);
LINK FIInsertSlur(Document *doc, LINK firstSyncL, LINK firstNoteL,
							LINK lastSyncL, LINK lastNoteL, Boolean setShapeNow);
short FICreateAllSlurs(Document *doc);
void FIReshapeSlursTies(Document *doc);
void FIFixAllNoteSlurTieFlags(Document *doc);
Boolean FIAnchorAllJDObjs(Document *doc);
void FIAutoMultiVoice(Document *doc, Boolean doSingle);
Boolean FIJustifySystem(Document *doc, LINK systemL);
Boolean FIJustifyAll(Document *doc);

FILE *OpenInputFile(Str255 macfName, short vRefNum);
short CloseInputFile(FILE *f);
Boolean ReadLine(char inBuf[], short maxChars, FILE *f);
Boolean SaveTextToFile(Ptr pText, Str255 suggestedFileName, short promptStrID);

short FSOpenInputFile(FSSpec *fsSpec, short *refNum);
Boolean FSReadLine(char inBuf[], short maxChars, short refNum);
short FSCloseInputFile(short refNum);
short FSRewind(short refNum);
short FSReadChar(short refNum, char *c);
short FSUngetChar(short refNum);
