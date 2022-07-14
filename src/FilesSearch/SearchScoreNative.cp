/* SearchScoreNative.c for the OMRAS version of Nightingale */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "SearchScore.h"
#include "SearchScorePrivate.h"

#include "CarbonPrinting.h"

#ifndef SEARCH_DBFORMAT_MEF


/* --------------------------------------------------------------------- FindFirstNote -- */
/* Return the first Note/GRNote in the given voice and Sync/GRSync. */

LINK FindFirstNote(LINK pL,			/* Sync or GRSync */
					INT16 voice
					)
{
	LINK aNoteL, aGRNoteL;

	if (SyncTYPE(pL)) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			if (NoteVOICE(aNoteL)==voice)
				return aNoteL;
		}
	}
	else if (GRSyncTYPE(pL)) {
		aGRNoteL = FirstSubLINK(pL);
		for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
			if (GRNoteVOICE(aGRNoteL)==voice)
				return aGRNoteL;
		}
	}

	return NILINK;
}


/* -------------------------------------------------------------- Next Voice utilities -- */

/* Given an internal (not user!) voice number, return the internal (not user!) voice
number of a voice which, from the user's standpoint, might reasonably come "next". (After
the default voices that start the voice-mapping table, internal voice numbers can map to
parts in any way, so the next internal voice after a given voice might be many staves
away.)  If there is no next voice, return a value larger than MAXVOICES. */

INT16 UserNextVoice(DB_Document *doc, INT16 voice)
{
	INT16 nextVoice, v, partn;
	DB_LINK partL, nextPartL;
	
	partn = doc->voiceTab[voice].partn;
	partL = Partn2PartL(doc, partn);
	for (nextVoice = -1, v = voice+1; v<=MAXVOICES; v++)
		if (doc->voiceTab[v].partn==partn)
			{ nextVoice = v; break; }
	if (nextVoice<0) {
	 	nextPartL = NextPARTINFOL(partL);
	 	if (nextPartL==DB_NILINK)
	 		nextVoice = MAXVOICES+1;
	 	else
	 		nextVoice = PartFirstSTAFF(nextPartL);	/* Default voice no. for part's top staff */
	 }
	
	return nextVoice;
}


/* Based on the selection or insertion point, return the internal (not user!) voice
number of the voice which, from the user's standpoint, comes "next": cf. comments in
UserNextVoice. If there is no next voice, return a value larger than MAXVOICES. */

INT16 Sel2UserNextVoice(DB_Document *doc)
{
	INT16 currVoice, nextVoice;
	
	if (doc->selStartL==doc->selEndL)
		nextVoice = doc->selStaff;		/* Default voice no. for this staff */
	else {
		currVoice = GetVoiceFromSel(doc);
		nextVoice = UserNextVoice(doc, currVoice);
	}
	
	return nextVoice;
}


/* -------------------------------------------------------------------------------------- */

/* Search for a Graphic of subtype <GRString> in any voice. */

LINK StringSearch(
			LINK startL,				/* Place to start looking */
			Boolean goLeft				/* True if we should search left */
			)
{
	LINK graphicL;  SearchParam pbSearch;

	InitSearchParam(&pbSearch);
	pbSearch.voice = ANYONE;
	pbSearch.subtype = GRString;

	graphicL = L_Search(startL, GRAPHICtype, goLeft, &pbSearch);
	return graphicL;
}


/* Search for a Sync in the given voice. */

LINK LVSSearch(
			LINK startL,				/* Place to start looking */
			INT16 voice,				/* target voice number */
			Boolean goLeft				/* True if we should search left */
			)
{
	LINK syncL;
	
	syncL = LVSearch(startL, SYNCtype, voice, goLeft, False);
	return syncL;
}


static void SelectTiedNotes(LINK syncL, LINK aNoteL);
static void SelectTiedNotes(LINK syncL, LINK aNoteL)
{
	LINK syncPrevL, aNotePrevL, continL, continNoteL;
	INT16 voice;

	syncPrevL = syncL;
	aNotePrevL = aNoteL;
	voice = NoteVOICE(aNoteL);
	while (NoteTIEDR(aNotePrevL)) {
		continL = LVSearch(RightLINK(syncPrevL),			/* Tied note should always exist */
					SYNCtype, voice, GO_RIGHT, False);
		continNoteL = NoteNum2Note(continL, voice, NoteNUM(aNotePrevL));
		if (continNoteL==NILINK) break;						/* Should never happen */
		LinkSEL(continL) = True;
		NoteSEL(continNoteL) = True;

		syncPrevL = continL;
		aNotePrevL = continNoteL;
	}
}


/* Make the given objects and subobjects the selection. For now, the objects should
all be Syncs: objects of other types are ignored. Deselect everything, then select
notes/rests in the given arrays and voice, plus--if appropriate--any following notes
tied to them. Finally, update selection range and <setStaff> accordingly, and Inval
the window. NB: operates on Nightingale object list, not DB_ stuff. */

void SelectSubobjA(
			Document *doc,
			LINK matchedObjA[][MAX_PATLEN],
			LINK matchedSubobjA[][MAX_PATLEN],
			INT16 matchVoice,
			INT16 count,
			Boolean matchTiedDur
			)
{
	INT16 n, staffn=1;
	LINK pL, aNoteL;
	
	DeselAll(doc);
	
	for (n = 0; n<count; n++) {
		pL = matchedObjA[matchVoice][n];
		if (!SyncTYPE(pL)) continue;
		
		aNoteL = matchedSubobjA[matchVoice][n];
		LinkSEL(pL) = True;
		NoteSEL(aNoteL) = True;
		if (matchTiedDur) SelectTiedNotes(pL, aNoteL);
		staffn = NoteSTAFF(aNoteL);
	}

	doc->selStartL = matchedObjA[matchVoice][0];
	for (pL = LeftLINK(doc->tailL); pL!=LeftLINK(doc->selStartL); pL = LeftLINK(pL))
		if (LinkSEL(pL)) break;
	doc->selEndL = RightLINK(pL);
	doc->selStaff = staffn;

	InvalWindow(doc);
}


void GetNString(LINK graphicL, char theStr[])
{
	LINK aGraphicL;
	PAGRAPHIC aGraphic;
	StringOffset theStrOffset;

	aGraphicL = FirstSubLINK(graphicL);
	aGraphic = GetPAGRAPHIC(aGraphicL);
	theStrOffset = aGraphic->strOffset;
	Pstrcpy((StringPtr)PCopy(theStrOffset), (StringPtr)theStr);
	PToCString((StringPtr)theStr);
}


/* -------------------------------------------------------------------------------------- */


#define PREFIX_REQUIRED False
#define SECTION_ID_STR1 "\\l"	/* Prefix string for section ID strings (empty for none) */
#define SECTION_ID_STR2 ""		/* Prefix string for section ID strings (empty for none) */

/* If the given string begins with a section ID code (perhaps empty), return the number of
leading chars in the code (perhaps 0); else return -1. */

INT16 HasSectionIDPrefix(char theStr[256]);
INT16 HasSectionIDPrefix(char theStr[256])
{
	INT16 str1len = strlen(SECTION_ID_STR1);
	INT16 str2len = strlen(SECTION_ID_STR2);
	
	if (!PREFIX_REQUIRED) return 0;

	if (str1len>0 && strncmp(theStr, SECTION_ID_STR1, str1len)==0) return str1len;
	if (str2len>0 && strncmp(theStr, SECTION_ID_STR2, str2len)==0) return str2len;

	return -1;
}


/* Return a C string identifying the score or, preferably, section of the score
containing the given object. The section is identified by the last previous passage-
label string: with Nightingale-native data structures, a GRString (not GRLyric!)
Graphic in style SECTION_ID_NSTYLE starting with a code recognized by HasSectionIDPrefix().
This is intended for use in passage-level retrieval; it's particularly useful if the
"passages" are actually separate short pieces (e.g., folk songs or incipits).

Return True if the string is section identification. */

#define SECTION_ID_NSTYLE FONT_R9	/* Graphic <info> value (FONT_R1 to FONT_R9, or -1 = any) */
#define MAX_ID_LEN 30					/* Max. length to display, in chars. */

Boolean GetScoreLocIDString(Document *doc, LINK locL, char matchLocString[256])
{
	LINK stringL, startL;
	char locName[256], theStr[256];
	Boolean haveSectionName;
	INT16 matchLen;

	/* The default is the score's filename. */

	Pstrcpy(doc->name, (StringPtr)locName);
	PToCString((StringPtr)locName);
	haveSectionName = False;

	/* Look for something more specific to identify the section of the score. */
	
	startL = locL;
	
	while (True) {
		stringL = StringSearch(startL, GO_LEFT);

		if (stringL==NILINK) break;
		
		/* We have a string (text item but not lyrics): see if it passes closer scrutiny. */
		
		if (StringMaybeSectionID(stringL)) {
			(void)GetNString(stringL, theStr);
			matchLen = HasSectionIDPrefix(theStr);
			if (matchLen>=0) {
				/* This GRString passes our closest scrutiny, so accept it. */
				
				strcpy(locName, &(theStr[matchLen]));
				haveSectionName = True;
				break;
			}
		}
	startL = LeftLINK(stringL);
	}

	GoodStrncpy(matchLocString, locName, MAX_ID_LEN-1);
	return haveSectionName;
}


/* ----------------------------------------------------------------------------------- */

/* Search the given score for the contents of the Search Pattern score, starting at
the given DB_LINK. Exception: we don't accept any match starting with that DB_LINK but
in a voice "before" the given voice: this is intended for use by a "find next match"
feature.  For now, consider only notes and rests; match note/rest status and, for
notes, MIDI note numbers; and look for melodic patterns only. 

If we find a match, return the DB_LINK where the first one starts; return in parameters
the voice number it was found in and information on how good the match was; and select
the matched notes/rests. If we don't find a match, just return DB_NILINK.

If matches exist in several voices starting with the same DB_LINK, we find the first
one; if the starting DB_LINK of the match is where the search started, we find the
first one that's not "before" <startVoice>. */

DB_LINK SearchScore(
				DB_Document *doc,
				DB_LINK startL,
				INT16 startVoice,					/* <=1 for any voice */
				SEARCHPAT searchPat,
				SEARCHPARAMS sParm,
				INT16 *pFoundVoice,				/* voice in which pattern was found */
				ERRINFO *pTotalErrorInfo		/* Info on how good the match was */
				)
{
	Boolean foundVoiceA[MAXVOICES+1];
	ERRINFO totalErrorInfo[MAXVOICES+1];

	/* Since a DB_LINK occupies two bytes, the matchedObjA[] and matchedSubobjA[] arrays
	   take a total of 4*(MAXVOICES+1)*MAX_PATLEN bytes. Stack space is allocated in
	   InitMemory(); under Mac OS, with our current allocation scheme and MAXVOICES =
	   100 (the standard value), this is enough that if MAX_PATLEN is more than, say,
	   35, there's some danger of stack overflow. It'd be better to allocate these arrays
	   explicitly. */
	   
	DB_LINK matchedObjA[MAXVOICES+1][MAX_PATLEN];
	DB_LINK matchedSubobjA[MAXVOICES+1][MAX_PATLEN];
	DB_LINK foundL;
	INT16 v, matchVoice;
	Boolean needStartVoice;
	
	foundL = SearchGetHitSet(doc, startL, searchPat, sParm, foundVoiceA, totalErrorInfo,
										matchedObjA, matchedSubobjA);

//LogPrintf(LOG_DEBUG, ">SearchGetHitSet(startL=%d): foundL=%d\n", startL, foundL);
	if (foundL==DB_NILINK)
		return DB_NILINK;			/* Scanned the rest of the score and didn't find the pattern */
	
	/* We found matches; return the first one in an acceptable voice, if any. Note that
	   to avoid disasterous interactions with Search Again, we need the same concept of
	   voice order it has. */
	   
	matchVoice = 0;
	needStartVoice = True;

	if (foundL!=startL) needStartVoice = False;

	for (v = 1; v<=MAXVOICES; v = DB_UserNextVoice(doc, v)) {
		/* Ignore matches if they start with the first Sync we're looking at and their
		   voice is above (in the sense Search Again implements!) the top-voice
		   threshhold. */
		   
		if (v==startVoice) needStartVoice = False;
		if (!needStartVoice && foundVoiceA[v]) {
			matchVoice = v;
			break;
		}
	}

	if (matchVoice==0) {
		/* All matches were in unacceptable voices; try again to the right. */
		
		foundL = SearchGetHitSet(doc, RightLINK(startL), searchPat, sParm, foundVoiceA,
									totalErrorInfo, matchedObjA, matchedSubobjA);
	
		if (foundL==DB_NILINK)
			return DB_NILINK;							/* No matches to the right. */
		
		for (v = 1; v<=MAXVOICES; v = DB_UserNextVoice(doc, v)) {
			if (foundVoiceA[v]) {
				matchVoice = v;
				break;
			}
		}
	}

	/* Make the matched subobjects the selection. */
	
	SelectSubobjA(doc, matchedObjA, matchedSubobjA, matchVoice, searchPat.patLen, sParm.matchTiedDur);
	
	*pFoundVoice = matchVoice;
	*pTotalErrorInfo = totalErrorInfo[matchVoice];

	return foundL;	
}


/* Search the given score, starting in a place determined by the insertion point or
selection, for the next instance of the contents of the Search Pattern score. If we
find an instance, select its notes/rests, display it, and return True; if we don't find
one or there's an error, return False.

This can be used only interactively, so it expects a Nightingale object list. */

Boolean DoSearchScore(Document *doc, Boolean again, Boolean usePitch, Boolean useDuration,
							INT16 pitchSearchType, INT16 durSearchType, Boolean includeRests,
							INT16 maxTranspose, INT16 pitchTolerance, FASTFLOAT pitchWeight,
							Boolean pitchKeepContour, INT16 chordNotes, Boolean matchTiedDur)
{
	DB_LINK foundL=DB_NILINK, startL;
	INT16 foundVoice;
	Boolean haveChord;
	INT16 startVoice;
	ERRINFO totalErrorInfo;
	SEARCHPAT searchPat;
	SEARCHPARAMS sParm;
	char str[256];

	if (!SearchIsLegal(usePitch, useDuration))
		goto Cleanup;

	if (!N_SearchScore2Pattern(includeRests, &searchPat, matchTiedDur, &haveChord)) {
		sprintf(str, "The search pattern is too long: notes after the first %d will be ignored.",
					MAX_PATLEN);											// ??I18N BUG
		CParamText(str, "", "", "");
		CautionInform(GENERIC_ALRT);
	}

	if (haveChord)
		WarnHaveChord();

	/* Fill SEARCHPARAMS block and start the search. If it's _not_ a search "again", just
	   start at the insertion point. If it _is_ a search "again", start with the voice
	   after the first currently-selected one, or, if nothing is selected, the default
	   voice for the staff below the insertion point. If there are no staves below the
	   selection point, start with the next object, with the first voice (voice 1 for the
	   topmost part). */
	   
	sParm.usePitch = usePitch;
	sParm.useDuration = useDuration;
	sParm.pitchWeight = pitchWeight;
	sParm.pitchSearchType = pitchSearchType;
	sParm.durSearchType = durSearchType;
	sParm.includeRests = includeRests;
	sParm.maxTranspose = maxTranspose;
	sParm.pitchTolerance = pitchTolerance;
	sParm.pitchKeepContour = pitchKeepContour;
	sParm.chordNotes = chordNotes;
	sParm.matchTiedDur = matchTiedDur;

	startVoice = 1;
	startL = doc->selStartL;
	if (again) {
		startVoice = Sel2UserNextVoice(doc);
		
		if (startVoice>MAXVOICES) {
			startVoice = 1;
			if (DB_RightLINK(startL)!=DB_NILINK) startL = DB_RightLINK(startL);
		}
	}
	
	foundL = SearchScore(doc, startL, startVoice, searchPat, sParm, &foundVoice, &totalErrorInfo);

	if (foundL==DB_NILINK) {
		CParamText("The pattern was not found.", "", "", "");	// ??I18N BUG
		NoteInform(GENERIC_ALRT);
	}
	else
		GoToSel(doc);

	if (DETAIL_SHOW) {
		FormatReportString(sParm, searchPat, "Find Next", str);
		LogPrintf(LOG_DEBUG, "%s", str);
		LogPrintf(LOG_DEBUG, ", L%d V%d: ", startL, startVoice);
		if (foundL==DB_NILINK)
			sprintf(strBuf, "not found.");
		else {
			Boolean haveSectionName; char matchLocString[256];
			Boolean showPitchErr, showDurErr;

			haveSectionName = GetScoreLocIDString(doc, foundL, matchLocString);
			if (haveSectionName)
				sprintf(strBuf, "found in m.%d (%s) (L%d) V%d",
						GetMeasNum(doc, foundL), matchLocString, foundL, foundVoice);	// ??I18N BUG
			else
				sprintf(strBuf, "found in m.%d (L%d) V%d",
						GetMeasNum(doc, foundL), foundL, foundVoice);					// ??I18N BUG

			showPitchErr = (usePitch && pitchTolerance!=0);
			showDurErr = (useDuration && durSearchType==TYPE_DURATION_CONTOUR);
			if (showPitchErr || showDurErr) {
				INT16 relEst;
				
				relEst = CalcRelEstimate(totalErrorInfo, pitchTolerance, pitchWeight,
													searchPat.patLen);
				if (showPitchErr && showDurErr)
					sprintf(&strBuf[strlen(strBuf)], ", err=p%dd%d (%d%%)", totalErrorInfo.pitchErr,
								totalErrorInfo.durationErr, relEst);
				else if (showPitchErr)
					sprintf(&strBuf[strlen(strBuf)], ", err=p%d (%d%%)", totalErrorInfo.pitchErr,
								relEst);
				else if (showDurErr)
					sprintf(&strBuf[strlen(strBuf)], ", err=d%d (%d%%)", totalErrorInfo.durationErr,
								relEst);
			}
		}

		LogPrintf(LOG_DEBUG, "%s\n", strBuf);
	}

Cleanup:
	return (foundL!=DB_NILINK);
}


/* Find the Document described by the parameters in <documentTable> and return its index.
If it's not found, return -1. Similar to AlreadyInUse(), except that returns a pointer.
??This belongs in Documents.c, and AlreadyInUse should call it. */

static INT16 FindDocInTable(INT16 vrefnum,	unsigned char name[256]);
static INT16 FindDocInTable(
	INT16 vrefnum,					/* Directory file name is local to */
	unsigned char name[256]			/* File name */
	)
{
	INT16 idx;
	Document *doc;

	for (idx = 0, doc = documentTable; doc<topTable; idx++, doc++)
		if (doc->inUse && doc!=clipboard)
			if (doc->vrefnum == vrefnum)
				if (EqualString(doc->name, name, False, False)) return idx;

	return -1;																	/* Not found */
}


/* Search the given score for all instances of the given Search Pattern. If we find any
instances, add them to a list starting at a given location, and return the new total
number of hits found. */

static INT16 IRSearchScore(Document *doc, SEARCHPAT searchPat, SEARCHPARAMS sParm, MATCHINFO
							matchInfoA[], DB_LINK matchedObjFA[][MAX_PATLEN],
							DB_LINK matchedSubobjFA[][MAX_PATLEN], INT16 nFound);
static INT16 IRSearchScore(Document *doc,
					SEARCHPAT searchPat,
					SEARCHPARAMS sParm,
					MATCHINFO matchInfoA[],					/* list of hits found so far */
					DB_LINK matchedObjFA[][MAX_PATLEN],		/* Array of hit details, by voice */
					DB_LINK matchedSubobjFA[][MAX_PATLEN],	/* Array of hit details, by voice */
					INT16 nFound)							/* Number of hits found so far */
{
	INT16 v, userVoice, i;
	DB_LINK startL, foundL;
	char str[256];
	DB_LINK partL;
	PPARTINFO pPart;
	Boolean haveSectionName;
	char matchLocString[256], vInfoStr[256], scoreName[256];

	Boolean foundVoiceA[MAXVOICES+1];
	ERRINFO totalErrorInfoA[MAXVOICES+1];

	/* Since a DB_LINK occupies two bytes, the matchedObjA[] and matchedSubobjA[] arrays
	   take a total of 4*(MAXVOICES+1)*MAX_PATLEN bytes. Stack space is allocated in
	   InitMemory(); under Mac OS, with our current allocation scheme and MAXVOICES =
	   100 (the standard value), this is enough that if MAX_PATLEN is more than, say,
	   35, there's some danger of stack overflow. It'd be better to allocate these arrays
	   explicitly. */
	   
	DB_LINK matchedObjA[MAXVOICES+1][MAX_PATLEN];
	DB_LINK matchedSubobjA[MAXVOICES+1][MAX_PATLEN];

	/* Initialize and start the search. */
	
	startL = doc->headL;

	while (True) {
		foundL = SearchGetHitSet(doc, startL, searchPat, sParm, foundVoiceA, totalErrorInfoA,
										matchedObjA, matchedSubobjA);
		if (!foundL) break;
		
		/* Found instance(s) of the pattern: save them. Then prepare to look for another
		   instance, starting with the Sync after the one these instances started with. */
		   
		for (v = 1; v<=MAXVOICES; v++) {
			if (!foundVoiceA[v]) continue;
			if (nFound>=MAX_HITS) {
				sprintf(str, "Found more matches than the maximum of %d Nightingale can handle.",
						MAX_HITS);											// ??I18N BUG
				CParamText(str, "", "", "");
				StopInform(GENERIC_ALRT);
				nFound--;
				return nFound;
			}
			
			matchInfoA[nFound].docNum = FindDocInTable(doc->vrefnum, doc->name);
			if (matchInfoA[nFound].docNum<0) {
				sprintf(str, "Can't find '%s' in the document table.", doc->name); // ??I18N BUG
				CParamText(str, "", "", "");
				StopInform(GENERIC_ALRT);
			}
			matchInfoA[nFound].foundL = foundL;
	
			Pstrcpy(doc->name, (StringPtr)scoreName);
			PToCString((StringPtr)scoreName);
			GoodStrncpy(matchInfoA[nFound].scoreName, scoreName, 31);
		
			matchInfoA[nFound].measNum = GetMeasNum(doc, foundL);
			matchInfoA[nFound].foundVoice = v;
	
			haveSectionName = GetScoreLocIDString(doc, foundL, matchLocString);
			if (haveSectionName)
				strcpy(matchInfoA[nFound].locStr, matchLocString);
			else
				strcpy(matchInfoA[nFound].locStr, "");
			if (Int2UserVoice(doc, v, &userVoice, &partL)) {
				sprintf(vInfoStr, "voice %d of ", userVoice);
				pPart = GetPPARTINFO(partL);
				sprintf(&vInfoStr[strlen(vInfoStr)], (strlen(pPart->name)>14?
														pPart->shortName : pPart->name));
			}
			else
				sprintf(vInfoStr, "%d", v);
	
			GoodStrncpy(matchInfoA[nFound].vInfoStr, vInfoStr, 31);
			matchInfoA[nFound].totalError.pitchErr = totalErrorInfoA[v].pitchErr;
			matchInfoA[nFound].totalError.durationErr = totalErrorInfoA[v].durationErr;

			for (i = 0; i<searchPat.patLen; i++) {
				matchedObjFA[nFound][i] = matchedObjA[v][i];
				matchedSubobjFA[nFound][i] = matchedSubobjA[v][i];
			}
			nFound++;
		}
	
		startL = DB_RightLINK(foundL);							/* should always exist */
	}

	return nFound;

}


/* Search the given score for all instances of the contents of the Search Pattern score.
If we find any instances, display a result list, and return True; if we don't or there's
an error, return False. */

Boolean DoIRSearchScore(Document *doc, Boolean usePitch, Boolean useDuration,
							INT16 pitchSearchType, INT16 durSearchType, Boolean includeRests,
							INT16 maxTranspose, INT16 pitchTolerance, FASTFLOAT pitchWeight,
							Boolean pitchKeepContour, INT16 chordNotes, Boolean matchTiedDur)
{
	MATCHINFO matchInfoA[MAX_HITS];
	Boolean haveChord;
	INT16 nFound=0, n;
	SEARCHPAT searchPat;
	SEARCHPARAMS sParm;
	char str[256];
	long startTicks, elapsedTicks;
	DB_LINK *matchedObjFA = NULL, *matchedSubobjFA = NULL;
	long matchArraySize;

	if (!SearchIsLegal(usePitch, useDuration))
		goto Cleanup;

	if (!N_SearchScore2Pattern(includeRests, &searchPat, matchTiedDur, &haveChord)) {
		sprintf(str, "The search pattern is too long: notes after the first %d will be ignored.",
					MAX_PATLEN);																// ??I18N BUG
		CParamText(str, "", "", "");
		CautionInform(GENERIC_ALRT);
	}

	if (haveChord)
		WarnHaveChord();

	sParm.usePitch = usePitch;
	sParm.useDuration = useDuration;
	sParm.pitchWeight = pitchWeight;
	sParm.pitchSearchType = pitchSearchType;
	sParm.durSearchType = durSearchType;
	sParm.includeRests = includeRests;
	sParm.maxTranspose = maxTranspose;
	sParm.pitchTolerance = pitchTolerance;
	sParm.pitchKeepContour = pitchKeepContour;
	sParm.chordNotes = chordNotes;
	sParm.matchTiedDur = matchTiedDur;
	
	FormatReportString(sParm, searchPat, "FIND ALL", str);				// ??I18N BUG
	LogPrintf(LOG_NOTICE, "%s:\n", str);
	startTicks = TickCount();

	matchArraySize = (MAX_HITS+1)*MAX_PATLEN;
	matchedObjFA = (DB_LINK *)NewPtr(matchArraySize*sizeof(DB_LINK));
	if (!GoodNewPtr((Ptr)matchedObjFA))
		{ OutOfMemory(matchArraySize*sizeof(DB_LINK)); goto Cleanup; }
	matchedSubobjFA = (DB_LINK *)NewPtr(matchArraySize*sizeof(DB_LINK));
	if (!GoodNewPtr((Ptr)matchedSubobjFA))
		{ OutOfMemory(matchArraySize*sizeof(DB_LINK)); goto Cleanup; }

	nFound = IRSearchScore(doc, searchPat, sParm, matchInfoA,
									(DB_LINK (*)[MAX_PATLEN])matchedObjFA,
									(DB_LINK (*)[MAX_PATLEN])matchedSubobjFA,
									0);

	elapsedTicks = TickCount()-startTicks;

	for (n = 0; n<nFound; n++) {
		INT16 relEst;
				
		relEst = CalcRelEstimate(matchInfoA[n].totalError, pitchTolerance, pitchWeight,
					searchPat.patLen);
		matchInfoA[n].relEst = relEst;
	}

	ListMatches(matchInfoA, (DB_LINK (*)[MAX_PATLEN])matchedObjFA, (DB_LINK (*)[MAX_PATLEN])matchedSubobjFA,
					searchPat.patLen, nFound, elapsedTicks, sParm, True);

Cleanup:
	if (matchedObjFA) DisposePtr((char *)matchedObjFA);
	if (matchedSubobjFA) DisposePtr((char *)matchedSubobjFA);
	return (matchInfoA[nFound].foundL!=DB_NILINK);
}


/* --------------------------------------------------------------- Search a "database" -- */

static OSErr ConvertFSpToInfo(FSSpec *fs, CInfoPBRec *info, short *isFolder);
OSErr FSpStartFolderScan(FSSpec *folder);
Boolean FSpGetNextFile(FSSpec *fs, Boolean *isFolder, OSType *creator, OSType *type,
			unsigned long *modDate, short *ioVRefNum, short *ioFRefNum, StringPtr
			ioNamePtr);
OSErr FSpGetDirectoryID(FSSpec *folder, long *dirID);
static INT16 GetDirFileInfo(FSSpec *pFSFolder, FSSpec docFSSpecA[], short maxDocs);
OSErr FSpGetParentFolder(FSSpec *fs, FSSpec *fsParent);

/* Get all file system info about a given file/folder.  Sets *isFolder to 0 if file spec
is a file; to 1 if it's a directory (folder); and to 2 if it's a volume root folder. */

static OSErr ConvertFSpToInfo(FSSpec *fs, CInfoPBRec *info, short *isFolder)
{
	OSErr err;
	
	info->hFileInfo.ioCompletion = NULL;
	info->hFileInfo.ioNamePtr = fs->name;
	info->hFileInfo.ioVRefNum = fs->vRefNum;
	info->hFileInfo.ioFDirIndex = 0;
	info->hFileInfo.ioDirID = fs->parID;
	
	err = PBGetCatInfoSync(info);
	if (err == noErr) {
		*isFolder = (info->dirInfo.ioFlAttrib & ioDirMask)!=0;
		if (*isFolder)
			if (info->dirInfo.ioDrDirID == 2/*fsRtDir*/)
				*isFolder = 2;
	}
	
	return(err);
}


static long scanDirID;
static short scanVRefNum;
static long scanIndex;

/* Declare a folder whose contents you wish to scan and deliver using FSpGetNextFile().
The scan is _non-recursive_, i.e., it's only one level deep. */

OSErr FSpStartFolderScan(FSSpec *folder)
{
	OSErr err = FSpGetDirectoryID(folder,&scanDirID);
	scanVRefNum = folder->vRefNum;
	scanIndex = 1;
	
	return(err);
}


/* Set *fs to the next file or folder in an iteration over all the contents of the
previously-declared scan folder. If *isFolder is False, then the file's creator, type,
modDate, and vRefNum are delivered in the appropriate variables. Delivers True if found
another file/folder; False if scan is complete, or if any error occurred. If file or
folder, its last modification date is also returned. creator, type, or modDate can be
NULL if you're not interested in them. */

Boolean FSpGetNextFile(FSSpec *fs, Boolean *isFolder, OSType *creator, OSType *type,
		unsigned long *modDate, short *ioVRefNum, short *ioFRefNum, StringPtr ioNamePtr)
{
	CInfoPBRec info;  OSErr err;

	/* First get the usual paramblock file information out of the system for the next
	   indexed file. */
	   
	info.hFileInfo.ioCompletion = NULL;
	info.hFileInfo.ioNamePtr = fs->name;
	info.hFileInfo.ioVRefNum = scanVRefNum;
	info.hFileInfo.ioDirID = scanDirID;
	info.hFileInfo.ioFDirIndex = scanIndex++;
	
	err = PBGetCatInfoSync(&info);
	if (err == noErr) {
		/* Got one, break out answers for caller */
		
		fs->vRefNum = scanVRefNum;
		fs->parID = scanDirID;
		if (info.dirInfo.ioFlAttrib & ioDirMask) {
			*isFolder = True;
			if (modDate)
				*modDate = info.dirInfo.ioDrMdDat;
			if (ioVRefNum)
				*ioVRefNum = info.dirInfo.ioVRefNum;
			if (ioFRefNum)
				*ioFRefNum = info.dirInfo.ioFRefNum;
			if (ioNamePtr)
				Pstrcpy(info.hFileInfo.ioNamePtr, ioNamePtr);
		}
		 else {
			*isFolder = False;
			if (creator)
				*creator = info.hFileInfo.ioFlFndrInfo.fdCreator;
			if (type)
				*type = info.hFileInfo.ioFlFndrInfo.fdType;
			if (modDate)
				*modDate = info.hFileInfo.ioFlMdDat;
			if (ioVRefNum)
				*ioVRefNum = info.dirInfo.ioVRefNum;
			if (ioFRefNum)
				*ioFRefNum = info.dirInfo.ioFRefNum;
			if (ioNamePtr)
				Pstrcpy(info.hFileInfo.ioNamePtr, ioNamePtr);
		}
	}
	
	return (err==noErr);
}


/* Convert a folder's file spec to its directory ID (which is different from the
FSSpec's parID field). Sets dirID the answer, or delivers paramErr if file spec wasn't
a folder; or other system error. If folder is NULL, delivers the current scan dirID. */

OSErr FSpGetDirectoryID(FSSpec *folder, long *dirID)
{
	CInfoPBRec info; OSErr err; short isFolder;
 
 		if (folder) {
		err = ConvertFSpToInfo(folder, &info, &isFolder);
		if (err == noErr)
			if (isFolder)
				*dirID = info.dirInfo.ioDrDirID;
			 else
				err = paramErr;											/* Not a folder */
	}
	 else {
		*dirID = scanDirID;
		err = noErr;
	}
	return(err);
}


/* Open the given document file, open a window for the document, and return the document
if successful, NULL if not. */

static Document *SSFSpecOpenDocument(FSSpec *theFile);
static Document *SSFSpecOpenDocument(FSSpec *theFile)
{
	return FSpecOpenDocument(theFile);
}


/* Get information on all files in the given folder--namely Nightingale scores--that
are of interest to us. If there's a problem, give an error message and return the number
of scores found up to that point; otherwise return the number of scores found. */

static INT16 GetDirFileInfo(FSSpec *pFSFolder, FSSpec docFSSpecA[], short maxDocs)
{
	FSSpec theFSItem;
	Boolean isFolder, okay;
	OSType creator, type;
	unsigned long modDate;
	short ioVRefNum, ioFRefNum;
	Str255 itemName;
	INT16 ind;
	char str[256];
	
	if (FSpStartFolderScan(pFSFolder)!=noErr) {
		CParamText("Can't get information on the folder to search.", "", "", "");	// ??I18N BUG
		StopInform(GENERIC_ALRT);
		return 0;
	}
	
	theFSItem = *pFSFolder;
	ind = 0;
	do {
		okay = FSpGetNextFile(&theFSItem, &isFolder, &creator, &type, &modDate,
										&ioVRefNum, &ioFRefNum, itemName);
		if (okay) {
			/* We have a file or folder. If it's a file containing an Ngale score, save
			   its info. */
		
			if (!isFolder && type==documentType) {
				if (ind>=maxDocs) {
					sprintf(str, "Too many scores in the search folder: will search only the first %d.",
								maxDocs);									// ??I18N BUG
					CParamText(str, "", "", "");
					StopInform(GENERIC_ALRT);
					return maxDocs;
				}
				PToCString(itemName);
				if (DETAIL_SHOW) {
					char creatorCStr[5], typeCStr[5];
					MacTypeToString(creator, creatorCStr);
					MacTypeToString(type, typeCStr);
					LogPrintf(LOG_DEBUG, "  creator=%s type=%s modDate=%lu name=%s\n",
								creatorCStr, typeCStr, modDate, itemName);
				}
				docFSSpecA[ind] = theFSItem;
				ind++;
			}
		}
	} while (okay);

	return ind;
}


/* Deliver the parent folder of the given file or folder. */

OSErr FSpGetParentFolder(FSSpec *fs, FSSpec *fsParent)
{
	CInfoPBRec info; OSErr err;

	if (fs==NULL || fsParent==NULL)
		return(paramErr);
	
	if (fs->parID == 2/*fsRtDir*/)									/* Root directory */
		return(paramErr);
	
	/* Return information about fs's parent directory. */
	info.hFileInfo.ioCompletion = NULL;
	fsParent->name[0] = 0;
	info.hFileInfo.ioNamePtr = fsParent->name;
	info.hFileInfo.ioVRefNum = fs->vRefNum;
	info.hFileInfo.ioFDirIndex = -1;
	info.hFileInfo.ioDirID = fs->parID;
	
	err = PBGetCatInfoSync(&info);
	if (err == noErr) {
		fsParent->vRefNum = fs->vRefNum;
		fsParent->parID = info.dirInfo.ioDrParID;
	}
	
	return err;
}


/* Do any of our "normal" documents have visible windows? Ignore special documents
(e.g., clipboard). Cf. DoCloseAllDocWindows. ??PROBABLY BELONGS IN Windows.c. */

Boolean AnyOpenDocWindows(void);
Boolean AnyOpenDocWindows()
{
	WindowPtr wp;
	INT16 kind;

	/* Go thru the system window list and look for our documents. */
	
	for (wp = FrontWindow(); wp!=NULL; wp = GetNextWindow(wp)) {		
		kind = GetWindowKind(wp);
		
		if (DETAIL_SHOW) {
			LogPrintf(LOG_DEBUG, "AnyOpenDocWindows: wp=%lx kind=%d ", (long)wp, kind);
			LogPrintf(LOG_DEBUG, "%c ", (wp==(WindowPtr)clipboard? 'C' : '-'));
			LogPrintf(LOG_DEBUG, "%p\n", (kind==DOCUMENTKIND? ((Document *)wp)->name : "\p "));
		}
		if (kind == DOCUMENTKIND) {
			if (wp!=(WindowPtr)clipboard && wp!=(WindowPtr)searchPatDoc) return True;
		}
	}
	
	return False;
}

/* Search all scores in the same folder as a given file. If there are any to search
and we find any instances, display a result list, and return True; if there are none
to search, we don't find any instances, or there's an error, return False. */

#define MAX_DOCS 1000		/* Max. Nightingale scores in the folder we can handle */

static InfoIOWarning infoIOWarning;

Boolean DoIRSearchFiles(FSSpec *pTheFile, Boolean usePitch, Boolean useDuration,
							INT16 pitchSearchType, INT16 durSearchType, Boolean includeRests,
							INT16 maxTranspose, INT16 pitchTolerance, FASTFLOAT pitchWeight,
							Boolean pitchKeepContour, INT16 chordNotes, Boolean matchTiedDur)
{
	FSSpec parentFolder, *docFSSpecA=NULL;
	long docFSSpecALen;
	INT16 totalDocs, ind;
	Str255 itemName;
	Document *doc;
	MATCHINFO matchInfoA[MAX_HITS];
	Boolean haveChord;
	INT16 nFound, prevNFound, n;
	SEARCHPAT searchPat;
	SEARCHPARAMS sParm;
	char str[256], findStr[256];
	long startTicks, elapsedTicks;
	DB_LINK *matchedObjFA = NULL, *matchedSubobjFA = NULL;
	long matchArraySize;
	Boolean okay = False;
	Boolean nonuserIO;

	LogPrintf(LOG_DEBUG, "DoIRSearchFiles: FSSpec=%lx\n", (long)pTheFile);

	/* We want to search all files in the parent folder of the given file. */
	
	if (FSpGetParentFolder(pTheFile, &parentFolder)!=noErr) {
		AlwaysErrMsg("DoIRSearchFiles: can't get parent folder.");
		return False;
	}
	
	docFSSpecALen = (MAX_DOCS+1)*sizeof(FSSpec);
	docFSSpecA = (FSSpec *)malloc(docFSSpecALen);
	if (!docFSSpecA) {
		OutOfMemory(docFSSpecALen);
		return False;
	}
	
	totalDocs = GetDirFileInfo(&parentFolder, docFSSpecA, MAX_DOCS);
	if (totalDocs<=0)
		goto Cleanup;

	if (!SearchIsLegal(usePitch, useDuration))
		goto Cleanup;

	if (!N_SearchScore2Pattern(includeRests, &searchPat, matchTiedDur, &haveChord)) {
		sprintf(str, "The search pattern is too long: notes after the first %d will be ignored.",
					MAX_PATLEN);																// ??I18N BUG
		CParamText(str, "", "", "");
		CautionInform(GENERIC_ALRT);
	}

	if (haveChord)
		WarnHaveChord();

#if 2
	// ??UGLY KLUDGE To avoid mysterious crashes/hangs on most machines, urge user to
	// close all open doc windows. Cf. SearchInFilesHang/Crash.t .
	if (AnyOpenDocWindows()) {
		if (CautionAdvise(OPENDOCS_ALRT)==Cancel) goto Cleanup;
	}

#endif
	sParm.usePitch = usePitch;
	sParm.useDuration = useDuration;
	sParm.pitchWeight = pitchWeight;
	sParm.pitchSearchType = pitchSearchType;
	sParm.durSearchType = durSearchType;
	sParm.includeRests = includeRests;
	sParm.maxTranspose = maxTranspose;
	sParm.pitchTolerance = pitchTolerance;
	sParm.pitchKeepContour = pitchKeepContour;
	sParm.chordNotes = chordNotes;
	sParm.matchTiedDur = matchTiedDur;
	
	matchArraySize = (MAX_HITS+2)*MAX_PATLEN;
	matchedObjFA = (DB_LINK *)NewPtr(matchArraySize*sizeof(DB_LINK));
	if (!GoodNewPtr((Ptr)matchedObjFA))
		{ OutOfMemory(matchArraySize*sizeof(DB_LINK)); goto Cleanup; }
	matchedSubobjFA = (DB_LINK *)NewPtr(matchArraySize*sizeof(DB_LINK));
	if (!GoodNewPtr((Ptr)matchedSubobjFA))
		{ OutOfMemory(matchArraySize*sizeof(DB_LINK)); goto Cleanup; }

	sprintf(findStr, "FIND IN %d FILES", totalDocs);				// ??I18N BUG
	FormatReportString(sParm, searchPat, findStr, str);
	LogPrintf(LOG_NOTICE, "%s:\n", str);
	startTicks = TickCount();

	nonuserIO = True;
	infoIOWarning.nDocsMissingFonts = 0;
	infoIOWarning.nDocsNonstandardSize = 0;
	infoIOWarning.nDocsTooManyPages = 0;
	infoIOWarning.nDocsPageSetupPblm = 0;
	infoIOWarning.nDocsOther = 0;

	prevNFound = 0;
	for (ind = 0; ind<totalDocs; ind++) {
		Pstrcpy(docFSSpecA[ind].name, itemName);
		PToCString(itemName);
		if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ind=%d name=%s\n", ind, itemName);
		doc = SSFSpecOpenDocument(&docFSSpecA[ind]);	// ??BUT DON'T DISPLAY IT: cf. FSpecOpenDocument!
		if (doc==NULL) {
			sprintf(str, "Can't open file '%s': probably out of memory.",
						itemName);																// ??I18N BUG
			CParamText(str, "", "", "");
			StopInform(GENERIC_ALRT);
			goto Cleanup;
		}

		doc->changed = False;		/* Even if format converted, don't ask user to save on close */

		nFound = IRSearchScore(doc, searchPat, sParm, matchInfoA,
										(DB_LINK (*)[MAX_PATLEN])matchedObjFA,
										(DB_LINK (*)[MAX_PATLEN])matchedSubobjFA,
										prevNFound);
		
		/* If we didn't find any matches in the score, close it. FIXME: If doc was open
		   before the search, should probably leave it open regardless! */
		
		if (prevNFound==nFound) DoCloseDocument(doc);
		prevNFound = nFound;
	}

	elapsedTicks = TickCount()-startTicks;

	for (n = 0; n<nFound; n++) {
		INT16 relEst;
				
		relEst = CalcRelEstimate(matchInfoA[n].totalError, pitchTolerance, pitchWeight,
					searchPat.patLen);
		matchInfoA[n].relEst = relEst;
	}

	ListMatches(matchInfoA, (DB_LINK (*)[MAX_PATLEN])matchedObjFA, (DB_LINK (*)[MAX_PATLEN])matchedSubobjFA,
					searchPat.patLen, nFound, elapsedTicks, sParm, True);

	okay = True;

Cleanup:
	nonuserIO = False;
	if (infoIOWarning.nDocsMissingFonts>0 ||
		infoIOWarning.nDocsNonstandardSize>0 ||
		infoIOWarning.nDocsTooManyPages>0 ||
		infoIOWarning.nDocsPageSetupPblm>0 ||
		infoIOWarning.nDocsOther>0)
			LogPrintf(LOG_WARNING, "nDocsMissingFonts=%d nDocsNonstandardSize=%d nDocsTooManyPages=%d nDocsPageSetupPblm=%d nDocsOther=%d\n",
				infoIOWarning.nDocsMissingFonts, infoIOWarning.nDocsNonstandardSize,
				infoIOWarning.nDocsTooManyPages, infoIOWarning.nDocsPageSetupPblm,
				infoIOWarning.nDocsOther);

	if (matchedObjFA) DisposePtr((char *)matchedObjFA);
	if (matchedSubobjFA) DisposePtr((char *)matchedSubobjFA);
	if (docFSSpecA) free(docFSSpecA);
	return okay;
}

#endif /* SEARCH_DBFORMAT_MEF */
