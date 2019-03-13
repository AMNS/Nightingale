/*
	File:		NavServices.h

	Contains:	Code to support Navigation Services in Nightingale

	Version:	Mac OS X

*/

#ifndef _NAVSERVICES_
#define _NAVSERVICES_

#if defined(USE_UMBRELLA_HEADERS) && USE_UMBRELLA_HEADERS
#include <Carbon.h>
#else
//#include <Files.h>
//#include <Navigation.h>
#endif

#define dontSaveChanges	3

OSStatus OpenFileDialog(
	OSType applicationSignature, 
	short numTypes, 
	OSType typeList[],
	NSClientDataPtr pNSCD );
// Displays the NavGetFile modal dialog.
 
OSStatus SaveFileDialog(
	WindowRef parentWindow, 
	CFStringRef documentName, 
	OSType filetype, 
	OSType fileCreator, 
	NSClientDataPtr pNSCD );
// Displays the NavPutFile modal dialog.


OSStatus BeginSave( NavDialogRef inDialog, NavReplyRecord* outReply, FSRef* outFileRef );
// Call BeginSave when you're ready to write the file. It will create the
// data fork for you and return the FSSpec to it. Pass the reply record
// to CompleteSave after writing the file.


OSStatus CompleteSave( NavReplyRecord* inReply, FSRef* inFileRef, Boolean inDidWriteFile );
// Call CompleteSave after savibg a documen,t passing back the reply returned by 
// BeginSave. This call performs any file tranlation needed and disposes the reply.
// If the save didn't succeed, pass False for inDidWriteFile to avoid
// translation but still dispose the reply.
 
#endif // _NAVSERVICES_
