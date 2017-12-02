/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* MidiMap.h for Nightingale, rev. for v. 5.1 File format 'N105' and later */

#ifdef PUBLIC_VERSION
	#define MMPrintMidiMap		0
#else
	#define MMPrintMidiMap		1
#endif

#define MMNumMaps				128

/* MIDI Note Number Mapping 
 * map from noteNum to mappedNum
 *	unused: simply map noteNum at map index to mappedNum.
 */
 
typedef struct {
	short 	noteNum;				/* Original MIDI note number */
	short	mappedNum;				/* Mapped MIDI note number */
} MMNoteNumMap, *PMMNoteNumMap;


/* MIDI Mapping 
 * maps midiPatch using map noteNumMap
 * noteNumMap[noteNum] = mappedNum
 */
 
typedef struct {
	short		midiPatch;
	short		noteNumMap[MMNumMaps];	
} MMMidiMap, *PMMMidiMap;


/* --------------------------------------------------------------------------------- */
/* Prototypes of MidiMap functions
 */

PMMMidiMap GetDocMidiMap(Document *doc);
void ReleaseDocMidiMap(Document *doc);
short GetMappedNoteNum(Document *doc, short noteNum);
Boolean IsPatchMapped(Document *doc, short patchNum);
FSSpec *GetMidiMapFSSpec(Document *doc);
void ReleaseMidiMapFSSpec(Document *doc);
void ClearDocMidiMap(Document *doc) ;
short GetDocMidiMapPatch(Document *doc);

/* --------------------------------------------------------------------------------- */
/* Prototypes of MidiMapInfo functions
 */

void MidiMapInfo(void);