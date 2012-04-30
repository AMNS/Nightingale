/*
	File:		NavServices.h

	Contains:	Code to support Navigation Services in Nightingale

	Version:	Mac OS X

*/

#if defined(USE_UMBRELLA_HEADERS) && USE_UMBRELLA_HEADERS
#include <Carbon.h>
#else
#include <Files.h>
#include <Navigation.h>
#endif

#define dontSaveChanges	3

 
 
OSStatus OpenFileDialog(
	OSType applicationSignature, 
	short numTypes, 
	OSType typeList[], 
	NavDialogRef *outDialog );
// Displays the NavGetFile modeless dialog. Safe to call multple times,
// if the dialog is already displayed, it will come to the front.
 
 
void TerminateOpenFileDialog();
// Closes the OpenFileDialog, if there is one. Call when the app is quitting


void TerminateDialog( NavDialogRef inDialog );
// Closes the referenced dialog


OSStatus ConfirmSaveDialog(
	WindowRef parentWindow, 
	CFStringRef documentName, 
	Boolean quitting, 
	void* inContextData,
	NavDialogRef *outDialog );
// Displays the save confirmation dialog attached to the parent window

  
OSStatus ModalConfirmSaveDialog(
	CFStringRef documentName, 
	Boolean quitting, 
	NavUserAction *outUserAction );
// Display a modal confirmation dialog, returns the user action
	

OSStatus SaveFileDialog(
	WindowRef parentWindow, 
	CFStringRef documentName, 
	OSType filetype, 
	OSType fileCreator, 
	void *inContextData,
	NavDialogRef *outDialog );
// Displays the NavPutFile dialog attached to the parent window


OSStatus BeginSave( NavDialogRef inDialog, NavReplyRecord* outReply, FSRef* outFileRef );
// Call BeginSave when you're ready to write the file. It will create the
// data fork for you and return the FSSpec to it. Pass the reply record
// to CompleteSave after writing the file.


OSStatus CompleteSave( NavReplyRecord* inReply, FSRef* inFileRef, Boolean inDidWriteFile );
// Call CompleteSave after savibg a documen,t passing back the reply returned by 
// BeginSave. This call performs any file tranlation needed and disposes the reply.
// If the save didn't succeed, pass false for inDidWriteFile to avoid
// translation but still dispose the reply.
 
