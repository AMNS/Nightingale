/* MidiMap.c for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

// MAS

#include "MidiMap.h"


/* -------------------------------------------------------------------------------------- */
/* Constants */

#define MMLINELEN	256			/* Aside from lyrics, the longest line we've seen is 61 chars. */

#define MMRESTYPE 	'mmap'
#define MMRESID		1
#define MIN_NOTENUM	1

#undef MMPrintMidiMap
#define MMPrintMidiMap	1		/* Override setting in MidiMap.h - change June 1, 2011 1 to 0 */

/* -------------------------------------------------------------------------------------- */
/* Local prototypes and module globals */

static Boolean ParseMidiMapFile(Document *doc, Str255 fileName, FSSpec *fsSpec);
//static FILE *FSpOpenInputFile(Str255 macfName, FSSpec *fsSpec);
static Boolean ProcessMidiMap(Document *doc, FILE *f);
static Boolean NewMidiMap(Document *doc);
static void PrintMidiMap(Document *doc);

static long		gMMLineCount = 0L;
static char		gMMInBuf[MMLINELEN];


/* ------------------------------------------------------ OpenMidiMapFile (3 versions) -- */
/* Top level, public function: three versions with different calling sequences. Returns
True on success, False if error. */

Boolean OpenMidiMapFileNSD(Document *doc, Str255 fileName, NSClientDataPtr pNSD)
{
	Boolean		result;
	FSSpec		fsSpec;
	
	fsSpec = pNSD->nsFSSpec;
	result = ParseMidiMapFile(doc, fileName, &fsSpec);
	return result;
}

Boolean OpenMidiMapFileFNFS(Document *doc, Str255 fileName, FSSpec *fsSpec)
{
	Boolean		result;

	result = ParseMidiMapFile(doc, fileName, fsSpec);
	return result;
}

Boolean OpenMidiMapFile(Document *doc, FSSpec *fsSpec)
{
	Boolean		result;
	Str255		fileName;

	Pstrcpy(fileName, fsSpec->name);
	result = OpenMidiMapFileFNFS(doc, fileName, fsSpec);
	return result;
}

/* ------------------------------------------------------------------ ParseMidiMapFile -- */
/* The Midi Map parsing function. Returns True on success, False if error. */

static Boolean ParseMidiMapFile(Document *doc, Str255 fileName, FSSpec *fsSpec)
{
	Boolean		ok, printMidiMap = True;
	FILE		*f;

	f = FSpOpenInputFile(fileName, fsSpec);
	if (f==NULL) return False;
		
	ok = ProcessMidiMap(doc, f);
	CloseInputFile(f);									/* done with input file */
	if (!ok) goto MidiMapErr;

#if MMPrintMidiMap
	if (printMidiMap)
		PrintMidiMap(doc);
#endif

	return True;

MidiMapErr:
	return False;
}

static Boolean ExtractVal(char	*str,				/* source string */
							long	*val)			/* pass back extracted value */
{
	short ans = sscanf(str, "%ld", val);			/* read numerical value in string as a long */
	if (ans > 0) return True;
	*val = 0L;
	return False;
}


/* -------------------------------------------------------------------- ProcessMidiMap -- */
/* Process the MIDI map to create the array of note list mappings */

static Boolean ProcessMidiMap(Document *doc, FILE *f)
{
	Boolean ok = True;
	short ans = 0;
	gMMLineCount = 0;
	char patch[256], pn[64], nn[64], mn[64];
	long patchNum;
	long noteNum, mappedNum;
	PMMMidiMap pMidiMap;
	
	ok = NewMidiMap(doc);
	if (!ok) return False;
	pMidiMap = GetDocMidiMap(doc);
		
	while (ReadLine(gMMInBuf, MMLINELEN, f) && ok) {
		if (gMMLineCount == 0) {
			ans = sscanf(gMMInBuf, "%s %s", patch, pn);
			if (ans < 2) {
				ok = False;
			}
			else {
				ExtractVal(pn, &patchNum);
				pMidiMap->midiPatch = patchNum;			
			}
		}
		else {
			ans = sscanf(gMMInBuf, "%s %s", nn, mn);
			
			if (ans == 2) {
				ExtractVal(nn, &noteNum);
				ExtractVal(mn, &mappedNum);
				if (noteNum >=MIN_NOTENUM && noteNum <+ MAX_NOTENUM) {
					pMidiMap->noteNumMap[noteNum] = mappedNum;
				}
			}
		}
		gMMLineCount++;
	}
	
	ReleaseDocMidiMap(doc);
	
	return ok;
}

/* -------------------------------------------------------------------- InstallMidiMap -- */
/* Install the fsSpec of the midi map in the document */

static void HInstallMidiMap(Document *doc, Handle fsSpecHdl);
static void HInstallMidiMap(Document *doc, Handle fsSpecHdl)
{
	doc->midiMapFSSpecHdl = fsSpecHdl;	
}

void InstallMidiMap(Document *doc, FSSpec *fsSpec)
{
	Handle fsSpecHdl = NewHandle(sizeof(FSSpec));
	if (GoodNewHandle(fsSpecHdl)) {
		BlockMove(fsSpec, *fsSpecHdl, sizeof(FSSpec));		
		HInstallMidiMap(doc, fsSpecHdl);
	}
}

/* ---------------------------------------------------------------------- ClearMidiMap -- */
/* Clear the fsSpec of the midi map from the document */

void ClearMidiMap(Document *doc)
{
	doc->midiMapFSSpecHdl = NULL;	
}

/* ------------------------------------------------------------------------ HasMidiMap -- */
/* Return True if the document has an installed Midi Map */

Boolean HasMidiMap(Document *doc)
{
	return(doc->midiMapFSSpecHdl != NULL);	
}

/* -------------------------------------------------------------------------------------- */

/* Is the given midiMap fsSpec different from the document's installed Midi Map? */

Boolean ChangedMidiMap(Document *doc, FSSpec *fsSpec)
{
	if (!HasMidiMap(doc)) {
		return True;
	}
	
	Boolean changed = !EqualFSSpec((FSSpec *)*doc->midiMapFSSpecHdl, fsSpec);
	return(changed);	
}


FSSpec *GetMidiMapFSSpec(Document *doc) 
{
	FSSpec *pfsSpec = NULL;
	
	if (HasMidiMap(doc)) {
		HLock(doc->midiMapFSSpecHdl);
		pfsSpec = (FSSpec *)*doc->midiMapFSSpecHdl;
	}
	return pfsSpec;
}

void ReleaseMidiMapFSSpec(Document *doc) 
{
	if (HasMidiMap(doc)) {
		HUnlock(doc->midiMapFSSpecHdl);
	}
}


/* ----------------------------------------------------------------------- SaveMidiMap -- */
/* Save the document's installed Midi Map. Return True if the operation succeeded, False
if it failed.  */

Boolean SaveMidiMap(Document *doc)
{
	Boolean ok = True;
	short refnum, err;
	
	if (THIS_FILE_VERSION < 'N105')  {
		doc->midiMapFSSpecHdl = NULL;
		return True;
	}
	
	if (HasMidiMap(doc))
	{		
		FSSpec fsSpec = doc->fsSpec;
		HSetVol(NULL,fsSpec.vRefNum,fsSpec.parID);
		FSpCreateResFile(&fsSpec, creatorType, documentType, smRoman);
		refnum = FSpOpenResFile(&fsSpec,fsRdWrPerm);
		err = ResError();

		if (err == noErr) {
		
			/* We want to add the data in the print record to the resource file but
			   leave the record in memory independent of it, so we add it to the
			   resource file, write out the file, detach the handle from the resource,
			   and close the file. Before doing any of this, we RemoveResource the print
			   record; this should be unnecessary, since we DetachResource print records
			   after reading them in, but we're playing it safe. */
			   
			GetResAttrs(doc->midiMapFSSpecHdl);
			if (!ResError()) RemoveResource(doc->midiMapFSSpecHdl);

			AddResource(doc->midiMapFSSpecHdl,MMRESTYPE,MMRESID,"\pMidiMap");
			err = ResError();
			UpdateResFile(refnum);
			DetachResource(doc->midiMapFSSpecHdl);
			CloseResFile(refnum);
			UseResFile(appRFRefNum);						/* a precaution */
		}
		else {
			ok = False;
		}
	}
	
	return ok;
}
	
/* -------------------------------------------------------------------------GetMidiMap -- */
/* Get the Midi Map stored in the document's resource fork. */

void GetMidiMap(Document *doc, FSSpec *pfsSpec)
{
	short refnum;
	OSStatus err = noErr;
	
	if (THIS_FILE_VERSION < 'N105')  {
		doc->midiMapFSSpecHdl = NULL;
	}
	else {
		FSpOpenResFile(pfsSpec, fsRdWrPerm);		/* Open the file */	
		
		err = ResError();
		if (err!=noErr) {
			doc->midiMapFSSpecHdl = NULL;
			return;
		}
			
		refnum = CurResFile();
		doc->midiMapFSSpecHdl = Get1Resource(MMRESTYPE, MMRESID);
		
		/* Ordinarily, we'd now call ReportBadResource, which checks if the new Handle
		   is NULL as well as checking ResError. But in this case, we don't want to report
		   an error if it's NULL--or do we? */
		   
		err = ResError();
		if (err==noErr) {
			if (doc->midiMapFSSpecHdl) {
				LoadResource(doc->midiMapFSSpecHdl);			/* Guarantee that it's in memory */
				err = ResError();
				if (err==noErr) {	
					DetachResource(doc->midiMapFSSpecHdl);
					HNoPurge(doc->midiMapFSSpecHdl);
				}
			}
		}
		
		CloseResFile(refnum);		
	}
}


/* ------------------------------------------------------------------------ NewMidiMap -- */
/* Get the Midi Map stored in the document's resource fork.
 * Return True if the operation succeeded.
 */
static Boolean NewMidiMap(Document *doc)
{
	Handle	midiMapHdl;
	Boolean	ok = False;
	
	midiMapHdl = NewHandleClear(sizeof(MMMidiMap));
	if (GoodResource(midiMapHdl)) {
		doc->midiMap = midiMapHdl;
		ok = True;
	}
	return ok;
}

PMMMidiMap GetDocMidiMap(Document *doc) 
{
	if (doc->midiMap != NULL) {
		HLock(doc->midiMap);
		return (PMMMidiMap)*(doc->midiMap);		
	}
	
	return NULL;
}

void ReleaseDocMidiMap(Document *doc) 
{
	if (doc->midiMap != NULL) {
		HUnlock(doc->midiMap);
	}
}

void ClearDocMidiMap(Document *doc) 
{
	if (doc->midiMap != NULL) {
		doc->midiMap = NULL;
	}
}

Boolean IsPatchMapped(Document *doc, short patchNum) 
{
	if (!HasMidiMap(doc)) 
		return False;
	
	PMMMidiMap pMidiMap = GetDocMidiMap(doc);
	if (pMidiMap != NULL) {
		return (patchNum == pMidiMap->midiPatch);
	}
	
	return False;
}

short GetDocMidiMapPatch(Document *doc) 
{
	if (!HasMidiMap(doc)) return -1;
	
	PMMMidiMap pMidiMap = GetDocMidiMap(doc);
	if (pMidiMap == NULL) return -1;
	
	return (pMidiMap->midiPatch);
}

/* ------------------------------------------------------------------ GetMappedNoteNum -- */
/* Return the mapped noteNum corresponding to noteNum for the document's map. */
 
short GetMappedNoteNum(Document *doc, short noteNum) 
{
	if (noteNum < MIN_NOTENUM || noteNum > MAX_NOTENUM) return noteNum;
	
	short mappedNum = 0;
	PMMMidiMap pMidiMap = GetDocMidiMap(doc);
	if (pMidiMap != NULL) {
		mappedNum = pMidiMap->noteNumMap[noteNum];
	}
	
	if (mappedNum < MIN_NOTENUM || mappedNum > MAX_NOTENUM) {
		mappedNum = noteNum;
	}
	
	ReleaseDocMidiMap(doc);
	return mappedNum;
}
 
/* ---------------------------------------------------------------------- PrintMidiMap -- */
/* Print the array of note list mappings. FIXME: Unimplemented */

static void PrintMidiMap(Document * /* doc */)
{
}
