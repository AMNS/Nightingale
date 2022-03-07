/* MidiMapInfo.c for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "MidiMap.h"

/* ----------------------------------------------------------------------  MidiMapInfo -- */

/* Constants and module globals */

#define OK_DI 1
#define CANCEL_DI 2
#define TEXT_DI 3
#define INSTALL_DI 4
#define CLEAR_DI 5

#define LEADING 11			/* Vertical dist. between lines displayed (pixels) */

static Rect textRect;
static short linenum;
static char *str;

/* Local Prototypes */

static void MMIDrawLine(char *);
static Boolean GetMidiMapFile(Str255 macfName, NSClientDataPtr pNSD);


/* -------------------------------------------------------------------------------------- */

/* Draw the specified C string on the next line in MIDI Map Info dialog */

static void MMIDrawLine(char *s)
{
	MoveTo(textRect.left, textRect.top+linenum*LEADING);
	DrawCString(s);
	++linenum;
}

static Boolean GetMidiMapFile(Str255 macfName, NSClientDataPtr pNSD)
{
	OSStatus 	err = noErr;
	OSType 		inputType[MAXINPUTTYPE];
	
	inputType[0] = 'TEXT';
	inputType[1] = '****';

	err = OpenFileDialog(kNavGenericSignature, 0, inputType, pNSD);
	
	if (err != noErr || pNSD->nsOpCancel) return False;
	
	FSSpec fsSpec = pNSD->nsFSSpec;
	Pstrcpy(macfName, fsSpec.name);
	return True;
}


/* Draw text border */
	
static void FrameTextRect(Rect *txRect) 
{
	PenState pnState;
	GetPenState(&pnState);
	PenPat(NGetQDGlobalsGray());
	FrameRect(txRect);
	SetPenState(&pnState);	
}

/* ------------------------------------------------------------------------- PrintMidiMap -- */
/* Print the array of note list mappings */

static void PrintMidiMap(Document *doc)
{
	char fmtStr[256];
	
	GetIndCString(fmtStr, MIDIMAPINFO_STRS, 7);   		/* "        A system contains %d staves." */
	sprintf(str, fmtStr);
	MMIDrawLine(str);
	
	GetIndCString(fmtStr, MIDIMAPINFO_STRS, 8);   		/* "        A system contains %d staves." */
	PMMMidiMap pMidiMap = GetDocMidiMap(doc); 
			
	short i=0, j=0;
	
	for ( ; j < 32; j++, i+=4) {		
		sprintf(str, fmtStr, i, pMidiMap->noteNumMap[i], 
								 i + 1, pMidiMap->noteNumMap[i+1],
								 i + 2, pMidiMap->noteNumMap[i+2],
								 i + 3,  pMidiMap->noteNumMap[i+3]);
								 
		MMIDrawLine(str);
	}
	ReleaseDocMidiMap(doc);
}


static void DrawMidiMapText(Document *doc)
{
	short patchNum;
	char fmtStr[256];
	
	linenum = 1;
	GetIndCString(fmtStr, MIDIMAPINFO_STRS, 1);					/* "MIDI Map Information" */
	sprintf(str, fmtStr);
	MMIDrawLine(str);
	if (doc) {
		if (HasMidiMap(doc)) {
			Str255 fName;
			FSSpec *fsSpec = (FSSpec *)*doc->midiMapFSSpecHdl;
			Pstrcpy(fName, fsSpec->name);
			GetIndCString(fmtStr, MIDIMAPINFO_STRS, 4);   		/* "    MIDI Map File: %s" */
			sprintf(str, fmtStr, PtoCstr(fName));
			MMIDrawLine(str);
			
			patchNum = GetDocMidiMapPatch(doc);
			if (patchNum < 0)  {
				GetIndCString(fmtStr, MIDIMAPINFO_STRS, 6);   	/* "    No MIDI patch for score %s" */
				sprintf(str, fmtStr, doc->name);
				MMIDrawLine(str);
			}
			else {
				GetIndCString(fmtStr, MIDIMAPINFO_STRS, 2);   	/* "    Patch:  %d" */
				sprintf(str, fmtStr, patchNum);
				MMIDrawLine(str);				
			}
			
			PrintMidiMap(doc);
		}
		else {
			GetIndCString(fmtStr, MIDIMAPINFO_STRS, 5);   		/* "    No MIDI Map for document %s." */
			sprintf(str, fmtStr, doc->name);
			MMIDrawLine(str);
		}
	}
	else {
		GetIndCString(fmtStr, SCOREINFO_STRS, 7);   			/* "    Map:" */
		sprintf(str, fmtStr);
		MMIDrawLine(str);
	}

}

static void EraseMidiMapText(Rect *txRect)
{
	EraseRect(txRect);
}

static void RedrawMidiMapText(Document *doc, Rect *txRect) 
{
	FrameTextRect(txRect);
	EraseMidiMapText(txRect);
	DrawMidiMapText(doc);
}


void MidiMapInfo()
	{
		DialogPtr dialogp;  GrafPtr oldPort;
		short ditem, aShort;  Handle aHdl;
		Document *doc=GetDocumentFromWindow(TopDocument);
		Boolean keepGoing;
		ModalFilterUPP	filterUPP;

		filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP==NULL) {
			MissingDialog(MIDIMAP_DLOG);
			return;
		}
		 
		str = (char *)NewPtr(256);
		if (!GoodNewPtr((Ptr)str))
			{ OutOfMemory(256L); return; }

		GetPort(&oldPort);
		dialogp = GetNewDialog(MIDIMAP_DLOG, NULL, BRING_TO_FRONT);
		if (!dialogp) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(SCOREINFO_DLOG);
			return;
		}
		SetPort(GetDialogWindowPort(dialogp));
		GetDialogItem(dialogp, TEXT_DI, &aShort, &aHdl, &textRect);
		
		//SetDlgFont(dialogp, textFontNum, textFontSmallSize, 0);
		FMFontFamily fMonaco = FMGetFontFamilyFromName("\pMonaco");
		SetDlgFont(dialogp, fMonaco, textFontSmallSize, 0);
		
		CenterWindow(GetDialogWindow(dialogp), 55);
		ShowWindow(GetDialogWindow(dialogp));
						
		ArrowCursor();
		OutlineOKButton(dialogp, True);
	
		DrawMidiMapText(doc);
		keepGoing = True;

		while (keepGoing) {
			ModalDialog(filterUPP, &ditem);
			switch (ditem) {
			
				case INSTALL_DI:
					if (doc) {						
						NSClientData mmnsData;
						Str255 mmfilename;
						
						if (GetMidiMapFile(mmfilename, &mmnsData)) {
							if (ChangedMidiMap(doc,  &mmnsData.nsFSSpec)) {
								doc->changed = True;
								InstallMidiMap(doc, &mmnsData.nsFSSpec);
								OpenMidiMapFileNSD(doc, mmfilename, &mmnsData);
								RedrawMidiMapText(doc, &textRect);
							}
						}
					}
					break;
					
				case CLEAR_DI:
					if (doc) {
						if (HasMidiMap(doc)) {
							doc->changed = True;
							ClearMidiMap(doc);
							ClearDocMidiMap(doc);
							RedrawMidiMapText(doc, &textRect);
						}						
					}
					break;
			
				case OK_DI:
				case CANCEL_DI:
					keepGoing = False;
					break;
				
			}
		}
		HideWindow(GetDialogWindow(dialogp));

		/* If user OK'd dialog and changed the comment, save the change. FIXME: NOPE! */
		
		if (ditem==OK_DI) {
		}
		
		DisposePtr((Ptr)str);
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dialogp);										/* Free heap space */
		SetPort(oldPort);
	}
