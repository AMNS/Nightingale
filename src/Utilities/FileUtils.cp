/*
 *  FileUtils.c
 *  NightCarbonMachO
 *
 *  Created by Michel Alexandre Salim on 2/4/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "FileUtils.h"

// MAS
#include "Nightingale_Prefix.pch"
#include "NResourceID.h"

FILE *FSpOpenInputFile(Str255 macfName, FSSpec *fsSpec)
{
	OSErr	result;
	char	ansifName[256];
	FILE	*f;

	result = HSetVol(NULL,fsSpec->vRefNum,fsSpec->parID);
	if (result!=noErr) goto err;
	
	sprintf(ansifName, "%#s", (char *)macfName);
	f = fopen(ansifName, "r");
	if (f==NULL) {
		// fopen puts error in errno, but this is of type int.
		result = errno;
		goto err;
	}
	
	return f;
err:
	ReportIOError(result, OPENTEXTFILE_ALRT);
	return NULL;
}
