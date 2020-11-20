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

static void EndianFixForFinalize();
static Boolean ReplacePrefsResource(Handle, Handle, ResType, short, const unsigned char *);
static Boolean UpdatePrefsFile(Handle cnfgResH, Handle midiResH, Handle midiModNRResH);
static Boolean CheckUpdatePrefsFile(void);

/* -------------------------------------------------------------- EndianFixForFinalize -- */
/* Nightingale keeps certain things in its Prefs file in Big Endian form. If we're on a
Little Endian CPU, we should have converted them to that form when we launched, and we
need to convert them back now. */

static void EndianFixForFinalize()
{
	EndianFixConfig();				/* Ensure <config> fields are in Big Endian form */
	
	/* Ensure palette globals for each palette are in Big Endian form */
	
	for (short idx=0; idx<TOTAL_PALETTES; idx++) {	
		paletteGlobals[idx] = (PaletteGlobals **)GetResource('PGLB', ToolPaletteWDEF_ID+idx);
		if (!GoodResource((Handle)paletteGlobals[idx])) {
			LogPrintf(LOG_WARNING, "Can't get globals for palette %d.  (EndianFixForFinalize)\n",
				idx); 
			return;
		}
		EndianFixPaletteGlobals(idx);
	}

	EndianFixMIDIModNRTable();		/* Ensure MIDI modNR tables are in Big Endian form */
}

/* -------------------------------------------------------------- ReplacePrefsResource -- */

/* In the current resource file, remove <resH> and replace it with <hndl> (which must
not be a resource when this is called). Return True if all OK, give an error message and
return False if there's a problem. */

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


/* -------------------------------------------------------------- CheckUpdatePrefsFile -- */

#define CNFG_RES_NAME	"\pNew Config"
#define MIDI_RES_NAME	"\pNew velocity table"
#define MIDM_RES_NAME	"\pMIDI modifier prefs"

static Boolean UpdatePrefsFile(Handle cnfgResH, Handle midiResH, Handle midiModNRResH)
{
	Configuration	**cnfgHndl;							/* Handle to global struct */
	MIDIPreferences **midiHndl;
	MIDIModNRPreferences **midiModNRHndl;
	OSErr			result;
	short			i;

		LogPrintf(LOG_NOTICE, "Updating the Prefs file...  (UpdatePrefsFile)\n");
		
		/* Create handle to global config, fill with updated values, and write it. */
		
		cnfgHndl = (Configuration **)NewHandle((long) sizeof(Configuration));
		if ((result = MemError())) {
			MayErrMsg("UpdatePrefsFile: can't allocate cnfgHndl.  Error %d", result);
			return False;
		}
		BlockMove(&config, *cnfgHndl, (Size)sizeof(Configuration));
		ReplacePrefsResource(cnfgResH, (Handle)cnfgHndl, 'CNFG', THE_CNFG, CNFG_RES_NAME);

		/* Create handle to MIDIPreferences, fill with updated values, and write it. */
		
		midiHndl = (MIDIPreferences **)NewHandle((long) sizeof(MIDIPreferences));
		if ((result = MemError())) {
			MayErrMsg("UpdatePrefsFile: can't allocate midiHndl.  Error %d", result);
			return False;
		}
		BlockMove(*midiResH, *midiHndl, (Size) sizeof(MIDIPreferences));
		for (i = 1; i<LAST_DYNAM; i++)
			(*midiHndl)->velocities[i-1] = dynam2velo[i];
		ReplacePrefsResource(midiResH, (Handle)midiHndl, 'MIDI', PREFS_MIDI, MIDI_RES_NAME);

		/* Create handle to MIDIModNRPreferences,  fill with updated values, and write it. */
		
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
		
		return True;
}

/* If they've been changed, save current config struct in the Prefs (formerly "Setup")
file's 'CNFG' resource, MIDI dynamics table in its 'MIDI' resource, and MIDI modifier
prefs tables in its 'MIDM' resource. Doesn't update any other resources that may be in
the Prefs file, .e.g., tool palette 'PICT'/'PLCH', 'PLMP'. We assume Prefs file is open.
Return True if all OK, give an error message and return False if there's a problem. NB:
If we're running on a Little Endian machine, this converts multibyte numbers to Big
Endian form; that could be disastrous if we do anything after this but quit! */

static Boolean CheckUpdatePrefsFile()
{
	Handle		cnfgResH, midiResH, midiModNRResH;					/* Handles to resource */
	Boolean		cnfgChanged, midiVeloTabChanged, midiModNRTabChanged, save;
	short		i;
	
	/* Undo any changes to Prefs values made internally by Nightingale. */
	
	config.maxDocuments--;		/* So that we're not including clipboard doc in count */
		
	EndianFixForFinalize();

	/* Get the config resource and MIDI resources from Prefs file and compare them to
	   our internal values. If any have been changed, update the file. */
	
	cnfgResH = GetResource('CNFG', THE_CNFG);
	if (ReportResError()) return False;
	midiResH = GetResource('MIDI', PREFS_MIDI);
	if (ReportResError()) return False;
	midiModNRResH = GetResource('MIDM', PREFS_MIDI_MODNR);
	if (ReportResError()) return False;
			
	cnfgChanged = memcmp( (void *)*cnfgResH, (void *)&config,
								(size_t)sizeof(Configuration) );
#ifdef DEBUG_INTEL_PREFS
	if (cnfgChanged) {
		NHexDump(LOG_DEBUG, "*cnfgResH", (unsigned char *)*cnfgResH, (size_t)sizeof(Configuration), 4, 16);
		NHexDump(LOG_DEBUG, "config   ", (unsigned char *)&config, (size_t)sizeof(Configuration), 4, 16);
	}
#endif

	for (midiVeloTabChanged = False, i = 1; i<LAST_DYNAM; i++)
		if (dynam2velo[i]!=((MIDIPreferences *)*midiResH)->velocities[i-1])
			{ midiVeloTabChanged = True;  break; }
	
	for (midiModNRTabChanged = False, i = 0; i<32; i++) {
		if (modNRVelOffsets[i]!=((MIDIModNRPreferences *)*midiModNRResH)->velocityOffsets[i])
			{ midiModNRTabChanged = True; break; }
		if (modNRDurFactors[i]!=((MIDIModNRPreferences *)*midiModNRResH)->durationFactors[i])
			{ midiModNRTabChanged = True; break; }
	}

	LogPrintf(LOG_INFO, "Prefs changed: cnfg=%d midiTab=%d midiModNRTab=%d  (CheckUpdatePrefsFile)\n",
					cnfgChanged, midiVeloTabChanged, midiModNRTabChanged);

	/* FIXME: With the addition of the MIDI Modifier Prefs, the wording of these warning
	   messages is getting a little too complicated. I'm bundling the MIDI Dynamics
	   Prefs and MIDI Modifier Prefs in one message, but this isn't the best solution.
		-JGG, 6/24/01 */
 
	if (cnfgChanged || midiVeloTabChanged || midiModNRTabChanged) {
		if (config.dontAskSavePrefs) save = True;
		else {
			if (cnfgChanged) {
				if (midiVeloTabChanged)
					GetIndCString(strBuf, SAVEPREFS_STRS, 1);    /* "General, Engraver and/or MIDI Prefs, and MIDI Dynamics Prefs" */
				else
					GetIndCString(strBuf, SAVEPREFS_STRS, 2);    /* "General, Engraver and/or MIDI Prefs" */
			}
			else
				GetIndCString(strBuf, SAVEPREFS_STRS, 3);    	/* "MIDI Dynamics Prefs and/or MIDI Modifier Prefs" */
			CParamText(strBuf, "", "", "");
			save = CautionAdvise(SAVECNFG_ALRT);
		}

 		if (save)
			if (!UpdatePrefsFile(cnfgResH, midiResH, midiModNRResH)) return False;
	}
	
	return True;
}


/* -------------------------------------------------------------------- ClosePrefsFile -- */
/* Closes the Prefs file; returns True is successful, False if error. From InsideMac:
If there's no resource file open with the given reference number, CloseResFile will do
nothing and the ResError function will return the result code resFNotFound. */

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
	
	if (!CheckUpdatePrefsFile()) LogPrintf(LOG_ERR, "Couldn't update the Nightingale Preferences file!  (Finalize)\n");

	ClosePrefsFile();
	
	LogPrintf(LOG_NOTICE, "QUITTING NIGHTINGALE %s  (Finalize)\n", applVerStr);

}
