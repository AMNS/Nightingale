/* StringToolbox.c: versions of Mac toolbox functions that expect C, not Pascal, strings */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
		GetIndCString			CParamText				DrawCString
		CStringWidth			GetWCTitle				SetWCTitle
		SetDialogItemCText		SetMenuItemCText
		AppendCMenu
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* ================================================================ C string "toolbox" == */
/* The following are versions of Mac toolbox functions that expect C strings instead
of Pascal strings. These are useful for two reasons: to aid portability (other
platforms' toolboxes and C environments know nothing of Pascal strings), and to
simplify code on all platforms (even on the Mac, C can handle C strings better than
Pascal strings). */

/* --------------------------------------------------------------------- GetIndCString -- */
/* Our standard utility for getting a string from a resource. Given a string list ID
and index, return a C string: identical to the Mac toolbox's GetIndString except that
the latter returns a Pascal string. ??Better yet, write, e.g., ResString, which
allocates space itself for the strings in a rotating pool (ala MapInfo), and plug
calls to ResString directly into CParamText's parameter list. 
GetIndString uses 1-based indexing. */

void GetIndCString(char *pString, short strListID, short strIndex)
{
	GetIndString((StringPtr)pString, strListID, strIndex);
	PToCString((StringPtr)pString);
}


/* ------------------------------------------------------------------------ CParamText -- */
/* Our standard utility for substituting strings into dialogs/alerts. Given four C
strings, set them as the strings to be used when a subsequently-created dialog or alert
contains a static text item that includes a "text replacement variable" ^0, ^1, ^2, or
^3. Identical to the Mac toolbox's ParamText except that the latter expects Pascal
strings.*/

void CParamText(char cStr1[], char cStr2[], char cStr3[], char cStr4[])
{
	Str255 str1, str2, str3, str4;
	
	strcpy((char *)str1, cStr1); CToPString((char *)str1);
	strcpy((char *)str2, cStr2); CToPString((char *)str2);
	strcpy((char *)str3, cStr3); CToPString((char *)str3);
	strcpy((char *)str4, cStr4); CToPString((char *)str4);

	ParamText(str1, str2, str3, str4);
}


/* ----------------------------------------------------------------------- DrawCString -- */
/* Our standard function for drawing a text string. Identical to the Mac toolbox's
DrawString except that the latter expects a Pascal string.*/

void DrawCString(char cStr[])
{
	Str255 str;

	strcpy((char *)str, cStr);
	CToPString((char *)str);
	DrawString(str);
}


/* ---------------------------------------------------------------------- CStringWidth -- */
/* Our standard function for getting the width of a text string in the current font.
Identical to the Mac toolbox's StringWidth except that the latter expects a Pascal
string.*/

short CStringWidth(char cStr[])
{
	Str255 str;
	
	strcpy((char *)str, cStr);
	CToPString((char *)str);

	return StringWidth(str);
}


/* ------------------------------------------------------------------------ GetWCTitle -- */
/* Our standard function for getting the title of a window. Identical to the Mac
toolbox's GetWTitle except that the latter expects a Pascal string.*/

void GetWCTitle(WindowPtr theWindow, char theCTitle[])
{
	Str255 theTitle;

	GetWTitle(theWindow, theTitle);
	PToCString(theTitle);
	strcpy(theCTitle, (char *)theTitle);
}

/* ------------------------------------------------------------------------ SetWCTitle -- */
/* Our standard function for setting the title of a window. Identical to the Mac
toolbox's SetWTitle except that the latter expects a Pascal string.*/

void SetWCTitle(WindowRef theWindow, char theCTitle[])
{
	Str255 theTitle;

	strcpy((char *)theTitle, theCTitle);
	CToPString((char *)theTitle);
	SetWTitle(theWindow, theTitle);
}


/* ---------------------------------------------------------------- SetDialogItemCText -- */
/* Our standard function for setting a dialog item to a text string. Identical to the
Mac toolbox's SetDialogItemText except that the latter expects a Pascal string.*/

void SetDialogItemCText(Handle lHdl, char cStr[])
{
	Str255 str;

	strcpy((char *)str, cStr);
	CToPString((char *)str);
	SetDialogItemText(lHdl, str);
}


/* ------------------------------------------------------------------ SetMenuItemCText -- */
/* Our standard function for changing the text of a menu item. Identical to the
Mac toolbox's SetMenuItemText except that the latter expects a Pascal string. */

void SetMenuItemCText(MenuHandle theMenu, short item, char cItemString[])
{
	Str255 itemString;

	strcpy((char *)itemString, cItemString);
	CToPString((char *)itemString);
	SetMenuItemText(theMenu, item, itemString);
}

/* ---------------------------------------------------------------------- AppendCMenu -- */
/* Our standard function for adding one or more items to a menu. Identical to the
Mac toolbox's AppendMenu except that the latter expects a Pascal string. */

void AppendCMenu(MenuHandle theMenu, char cItemText[])
{
	Str255 itemText;

	strcpy((char *)itemText, cItemText);
	CToPString((char *)itemText);
	AppendMenu(theMenu, itemText);
}
