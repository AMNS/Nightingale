/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/*
 *	Routines to deal with importing and exporting the desk scrap
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

void ImportDeskScrap()
	{
	}

void ExportDeskScrap()
	{
	}
	
Boolean ClipboardChanged(ScrapRef scrap)
	{
		return (scrap != gNightScrap);
	}
