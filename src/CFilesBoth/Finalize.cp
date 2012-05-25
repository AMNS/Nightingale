/* Finalize.c for Nightingale - slightly revised for v. 3.1 */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1991-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

Boolean ReplaceSetupResource(Handle, Handle, ResType, short, const unsigned char *);
static Boolean UpdateSetupFile(void);

/* --------------------------------------------------------- ReplaceSetupResource -- */

/* In the current resource file, remove <resH> and replace it with <hndl> (which
must not be a resource when this is called). Return TRUE if all OK, give an error
message and return FALSE if there's a problem. */

Boolean ReplaceSetupResource(Handle resH, Handle hndl, ResType type, short id,
								const unsigned char *name)
{
	/* Remove the old resource */
	RemoveResource(resH);
	if (ReportResError()) return FALSE;
	DisposeHandle(resH);
	UpdateResFile(CurResFile());
	if (ReportResError()) return FALSE;
	
	/* Write the new resource */
	AddResource(hndl, type, id, name);
	if (ReportResError()) return FALSE;
	WriteResource(hndl);
	if (ReportResError()) return FALSE;
	UpdateResFile(CurResFile());
	if (ReportResError()) return FALSE;
	ReleaseResource(hndl);
	if (ReportResError()) return FALSE;
	
	return TRUE;
}


/* ------------------------------------------------------------- UpdateSetupFile -- */
/* Save current config struct in Prefs (formerly "Setup") file's 'CNFG' resource,
MIDI dynamics table in Prefs file's 'MIDI' resource, and MIDI modifier prefs tables
in Prefs file's 'MIDM' resource, if they've been changed. Does NOT update any other
resources that may be in the Prefs file, e.g., MIDI Manager 'port' resources, tool
palette 'PICT'/'PLCH', 'PLMP'. We assume Prefs file is open. Return TRUE if all OK,
give an error message and return FALSE if there's a problem. */

#define CNFG_RES_NAME	"\pNew Config"
#define MIDI_RES_NAME	"\pNew velocity table"
#define MIDM_RES_NAME	"\pMIDI modifier prefs"

static Boolean UpdateSetupFile()
{
	Handle			cnfgResH, midiResH, midiModNRResH;	/* Handles to resource */
	Configuration	**cnfgHndl;			/* Handle to global struct */
	MIDIPreferences **midiHndl;
	MIDIModNRPreferences **midiModNRHndl;
	OSErr			result;
	Boolean			cnfgChanged, midiTabChanged, midiModNRTabChanged, save;
	short			i;
	
	/* Undo any changes to config made internally by Nightingale. */
	
		config.maxDocuments--;		/* So that we're not including clipboard doc in count */
		
	/*
	 * Get the config resource and the MIDI resource from file and compare them
	 * to our internal values. If either has been changed, update the file.
	 */
	cnfgResH = GetResource('CNFG', THE_CNFG);
	if (ReportResError()) return FALSE;
	midiResH = GetResource('MIDI', PREFS_MIDI);
	if (ReportResError()) return FALSE;
	midiModNRResH = GetResource('MIDM', PREFS_MIDI_MODNR);
	if (ReportResError()) return FALSE;
			
	cnfgChanged = memcmp( (void *)*cnfgResH, (void *)&config,
								(size_t)sizeof(Configuration) );
	
	for (midiTabChanged = FALSE, i = 1; i<LAST_DYNAM; i++)
		if (dynam2velo[i]!=((MIDIPreferences *)*midiResH)->velocities[i-1])
			{ midiTabChanged = TRUE; break; }
	
	for (midiModNRTabChanged = FALSE, i = 0; i<32; i++) {
		if (modNRVelOffsets[i]!=((MIDIModNRPreferences *)*midiModNRResH)->velocityOffsets[i])
			{ midiModNRTabChanged = TRUE; break; }
		if (modNRDurFactors[i]!=((MIDIModNRPreferences *)*midiModNRResH)->durationFactors[i])
			{ midiModNRTabChanged = TRUE; break; }
	}

	/* NB: With the addition of the MIDI Modifier Prefs, the wording of these warning
		messages is getting a little too complicated. I'm bundling the MIDI Dynamics
		Prefs and MIDI Modifier Prefs in one message, but this isn't the best solution.
		-JGG, 6/24/01 */
 
	if (cnfgChanged || midiTabChanged || midiModNRTabChanged) {
		if (config.dontAsk) save = TRUE;
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
 		if (save==OK) {
			/* Create a handle to global config struct */
			cnfgHndl = (Configuration **)NewHandle((long) sizeof(Configuration));
			if (result = MemError()) {
				MayErrMsg("UpdateSetupFile: can't allocate cnfgHndl.  Error %d", result);
				return FALSE;
			}
			BlockMove(&config, *cnfgHndl, (Size) sizeof(Configuration));
	
			ReplaceSetupResource(cnfgResH, (Handle)cnfgHndl, 'CNFG', THE_CNFG,
									CNFG_RES_NAME);

			/* Create a handle to a MIDIPreferences and fill it with updated values. */
			midiHndl = (MIDIPreferences **)NewHandle((long) sizeof(MIDIPreferences));
			if (result = MemError()) {
				MayErrMsg("UpdateSetupFile: can't allocate midiHndl.  Error %d", result);
				return FALSE;
			}
			BlockMove(*midiResH, *midiHndl, (Size) sizeof(MIDIPreferences));
			for (i = 1; i<LAST_DYNAM; i++)
				(*midiHndl)->velocities[i-1] = dynam2velo[i];
	
			ReplaceSetupResource(midiResH, (Handle)midiHndl, 'MIDI', PREFS_MIDI,
									MIDI_RES_NAME);

			/* Create a handle to a MIDIModNRPreferences and fill it with updated values. */
			midiModNRHndl = (MIDIModNRPreferences **)NewHandle((long) sizeof(MIDIModNRPreferences));
			if (result = MemError()) {
				MayErrMsg("UpdateSetupFile: can't allocate midiModNRHndl.  Error %d", result);
				return FALSE;
			}
			BlockMove(*midiModNRResH, *midiModNRHndl, (Size) sizeof(MIDIModNRPreferences));
			for (i = 0; i<32; i++) {
				(*midiModNRHndl)->velocityOffsets[i] = modNRVelOffsets[i];
				(*midiModNRHndl)->durationFactors[i] = modNRDurFactors[i];
			}

			ReplaceSetupResource(midiModNRResH, (Handle)midiModNRHndl, 'MIDM', PREFS_MIDI_MODNR,
									MIDM_RES_NAME);
 		}
	}
	
	return TRUE;
}


/* Save current built-in MIDI parameters in the Prefs (formerly "Setup") file's 'BIMI'
resource, if they've been changed. We assume Prefs file is open. Return TRUE if all OK,
give an error message and return FALSE if there's a problem. */

#define BIMI_RES_NAME	"\pBuilt-in MIDI driver settings"

static Boolean BIMIDIUpdateSetupFile(void);
static Boolean BIMIDIUpdateSetupFile()
{
	Handle hBIMIRes; BIMIDI **hBIMIDI; Boolean changed; OSErr result;
	
	/*
	 * Get the BIMI resource from file and compare it to our internal values.
	 * If it's been changed, ask for permission to update the file.
	 */
	hBIMIRes = GetResource('BIMI', THE_BIMI);
	if (ReportResError()) return FALSE;

	changed = (((BIMIDI *)*hBIMIRes)->portSetting!=portSettingBIMIDI ||
			   ((BIMIDI *)*hBIMIRes)->interfaceSpeed!=interfaceSpeedBIMIDI);
	if (!changed) return TRUE;
	
	GetIndCString(strBuf, SAVEPREFS_STRS, 4);    			/* "Built-in MIDI driver settings" */
	CParamText(strBuf, "", "", "");
	if (CautionAdvise(SAVECNFG_ALRT)==OK) {
		/* Create a handle to a BIMIDI and fill it with updated values. */
		hBIMIDI = (BIMIDI **)NewHandle((long) sizeof(BIMIDI));
		if (result = MemError()) {
			MayErrMsg("UpdateSetupFile: can't allocate hBIMIDI.  Error %d", result);
			return FALSE;
		}
		BlockMove(*hBIMIRes, *hBIMIDI, (Size)sizeof(BIMIDI));

		(*hBIMIDI)->portSetting = portSettingBIMIDI;
		(*hBIMIDI)->interfaceSpeed = interfaceSpeedBIMIDI;

		ReplaceSetupResource(hBIMIRes, (Handle)hBIMIDI, 'BIMI', THE_BIMI,
								BIMI_RES_NAME);
	}
	
	return TRUE;
}

/* -------------------------------------------------------------- CloseSetupFile -- */
/* Closes the Prefs file; returns TRUE is successful, FALSE if error. From InsideMac:
If there's no resource file open with the given reference number, CloseResFile will
do nothing and the ResError function will return the result code resFNotFound. */

Boolean CloseSetupFile()
{	
	Boolean anErr = FALSE;

	CloseResFile(setupFileRefNum);
	if (ResError() != noErr) anErr = TRUE;
	UseResFile(appRFRefNum);
	if (ResError() != noErr) anErr = TRUE;
	
	return !anErr;
}


/* -------------------------------------------------------------------- Finalize -- */
/* Do final cleanup before quitting. */

void Finalize()
{
	if (!OpenSetupFile()) return;			/* If it fails, there's nothing worth doing ??REALLY? */
	
	(void)UpdateSetupFile();

#ifndef TARGET_API_MAC_CARBON_MIDI
	if (useWhichMIDI==MIDIDR_MM)
		MMClose();
	else if (useWhichMIDI==MIDIDR_OMS)
		OMSClose();
	else if (useWhichMIDI==MIDIDR_FMS)
		FMSClose();
#endif

	CloseSetupFile();
}
