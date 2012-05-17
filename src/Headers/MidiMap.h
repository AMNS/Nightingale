/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 *
 * Copyright © 1986-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

/* MidiMap.h for Nightingale, 
 *	rev. for v 5.1 File format N105 and greater
 */

#ifdef PUBLIC_VERSION
	#define MMPrintMidiMap		0
#else
	#define MMPrintMidiMap		1
#endif

#define MMNumMaps					128

/* Midi Note Number Mapping 
 * map from noteNum to mappedNum
 *	unused: simply map noteNum at map index to mappedNum
 */
 
typedef struct {
	short 	noteNum;				/* Original Midi Note Number */
	short		mappedNum;			/* Mapped Midi Note Number */
} MMNoteNumMap, *PMMNoteNumMap;


/* Midi Mapping 
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