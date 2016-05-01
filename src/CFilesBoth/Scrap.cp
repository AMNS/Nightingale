/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
 *	Scrap.c - Routines to deal with importing and exporting the desk scrap
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
