/* Finalize.c for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean ReplacePrefsResource(Handle, Handle, ResType, short, const unsigned char *);
static Boolean UpdatePrefsFile(void);

/* -------------------------------------------------------------- ReplacePrefsResource -- */

/* In the current resource file, remove <resH> and replace it with <hndl> (which
must not be a resource when this is called). Return True if all OK, give an error
message and return False if there's a problem. */

static Boolean ReplacePrefsResource(Handle resH, Handle hndl, ResType type, short id,
								const unsigned char *name)
{
	/* Remove the old resource */
	RemoveResource(resH);
	if (ReportResError()) return False;
	DisposeHandle(resH);
	UpdateResFile(CurResFile());
	if (ReportResError()) return False;
	
	/* Write the new resource */
	AddResource(hndl, type, id, name);
	if (ReportResError()) return False;
	WriteResource(hndl);
	if (ReportResError()) return False;
	UpdateResFile(CurResFile());
	if (ReportResError()) return False;
	ReleaseResource(hndl);
	if (ReportResError()) return False;
	
	return True;
}


/* ------------------------------------------------------------------- UpdatePrefsFile -- */
/* If they've been changed, save current config struct in Prefs (formerly "Setup") file's
'CNFG' resource, MIDI dynamics table in its 'MIDI' resource, and MIDI modifier prefs tables
in its 'MIDM' resource, . Doesn't update any other resources that may be in the Prefs file,
.e.g., tool palette 'PICT'/'PLCH', 'PLMP'. We assume Prefs file is open. Return True if all
OK, give an error message and return False if there's a problem. */

#define CNFG_RES_NAME	"\pNew Config"
#define MIDI_RES_NAME	"\pNew velocity table"
#define MIDM_RES_NAME	"\pMIDI modifier prefs"

static Boolean UpdatePrefsFile()
{
	Handle			cnfgResH, midiResH, midiModNRResH;	/* Handles to resource */
	Configuration	**cnfgHndl;							/* Handle to global struct */
	MIDIPreferences **midiHndl;
	MIDIModNRPreferences **midiModNRHndl;
	OSErr			result;
	Boolean			cnfgChanged, midiTabChanged, midiModNRTabChanged, save;
	short			i;
	
	/* Undo any changes to config made internally by Nightingale. */
	
	config.maxDocuments--;		/* So that we're not including clipboard doc in count */
	EndianFixConfig();
		
	/*
	 * Get the config resource and the MIDI resource from file and compare them
	 * to our internal values. If either has been changed, update the file.
	 */
	cnfgResH = GetResource('CNFG', THE_CNFG);
	if (ReportResError()) return False;
	midiResH = GetResource('MIDI', PREFS_MIDI);
	if (ReportResError()) return False;
	midiModNRResH = GetResource('MIDM', PREFS_MIDI_MODNR);
	if (ReportResError()) return False;
			
	cnfgChanged = memcmp( (void *)*cnfgResH, (void *)&config,
								(size_t)sizeof(Configuration) );
		
	for (midiTabChanged = False, i = 1; i<LAST_DYNAM; i++)
		if (dynam2velo[i]!=((MIDIPreferences *)*midiResH)->velocities[i-1])
			{ midiTabChanged = True; break; }
	
	for (midiModNRTabChanged = False, i = 0; i<32; i++) {
		if (modNRVelOffsets[i]!=((MIDIModNRPreferences *)*midiModNRResH)->velocityOffsets[i])
			{ midiModNRTabChanged = True; break; }
		if (modNRDurFactors[i]!=((MIDIModNRPreferences *)*midiModNRResH)->durationFactors[i])
			{ midiModNRTabChanged = True; break; }
	}

	/* NB: With the addition of the MIDI Modifier Prefs, the wording of these warning
		messages is getting a little too complicated. I'm bundling the MIDI Dynamics
		Prefs and MIDI Modifier Prefs in one message, but this isn't the best solution.
		-JGG, 6/24/01 */
 
	if (cnfgChanged || midiTabChanged || midiModNRTabChanged) {
		if (config.dontAskSavePrefs) save = True;
		else {
			if (cnfgChanged) {
				if (midiTabChanged)
					GetIndCString(strBuf, SAVEPREFS_STRS, 1);    		/* "General, Engraver and/or MIDI Preferences, and the MIDI Dynamics Prefs" */
				else
					GetIndCString(strBuf, SAVEPREFS_STRS, 2);    		/* "General, Engraver and/or MIDI Preferences" */
			}
			else
				GetIndCString(strBuf, SAVEPREFS_STRS, 3);    			/* "MIDI Dynamics Prefs and/or MIDI Modifier Prefs" */
			CParamText(strBuf, "", "", "");
			save = CautionAdvise(SAVECNFG_ALRT);
		}

 		if (save) {
			LogPrintf(LOG_NOTICE, "Updating the Prefs file...\n");
			/* Create a handle to global config struct */
			cnfgHndl = (Configuration **)NewHandle((long) sizeof(Configuration));
			if ((result = MemError())) {
				MayErrMsg("UpdatePrefsFile: can't allocate cnfgHndl.  Error %d", result);
				return False;
			}
			BlockMove(&config, *cnfgHndl, (Size)sizeof(Configuration));
	
			ReplacePrefsResource(cnfgResH, (Handle)cnfgHndl, 'CNFG', THE_CNFG,
									CNFG_RES_NAME);

			/* Create a handle to a MIDIPreferences and fill it with updated values. */
			midiHndl = (MIDIPreferences **)NewHandle((long) sizeof(MIDIPreferences));
			if ((result = MemError())) {
				MayErrMsg("UpdatePrefsFile: can't allocate midiHndl.  Error %d", result);
				return False;
			}
			BlockMove(*midiResH, *midiHndl, (Size) sizeof(MIDIPreferences));
			for (i = 1; i<LAST_DYNAM; i++)
				(*midiHndl)->velocities[i-1] = dynam2velo[i];
	
			ReplacePrefsResource(midiResH, (Handle)midiHndl, 'MIDI', PREFS_MIDI,
									MIDI_RES_NAME);

			/* Create a handle to a MIDIModNRPreferences and fill it with updated values. */
			midiModNRHndl = (MIDIModNRPreferences **)NewHandle((long) sizeof(MIDIModNRPreferences));
			if ((result = MemError())) {
				MayErrMsg("UpdatePrefsFile: can't allocate midiModNRHndl.  Error %d", result);
				return False;
			}
			BlockMove(*midiModNRResH, *midiModNRHndl, (Size) sizeof(MIDIModNRPreferences));
			for (i = 0; i<32; i++) {
				(*midiModNRHndl)->velocityOffsets[i] = modNRVelOffsets[i];
				(*midiModNRHndl)->durationFactors[i] = modNRDurFactors[i];
			}

			ReplacePrefsResource(midiModNRResH, (Handle)midiModNRHndl, 'MIDM', PREFS_MIDI_MODNR,
									MIDM_RES_NAME);
 		}
	}
	
	return True;
}


/* -------------------------------------------------------------------- ClosePrefsFile -- */
/* Closes the Prefs file; returns True is successful, False if error. From InsideMac:
If there's no resource file open with the given reference number, CloseResFile will
do nothing and the ResError function will return the result code resFNotFound. */

Boolean ClosePrefsFile()
{	
	Boolean anErr = False;

	CloseResFile(setupFileRefNum);
	if (ResError() != noErr) anErr = True;
	UseResFile(appRFRefNum);
	if (ResError() != noErr) anErr = True;
	
	return !anErr;
}


/* -------------------------------------------------------------------------- Finalize -- */
/* Do final cleanup before quitting. */

void Finalize()
{
	if (!OpenPrefsFile()) return;			/* If it fails, there's nothing worth doing ??REALLY? */
	
	(void)UpdatePrefsFile();

	ClosePrefsFile();
}
