/*
	File:		NavServices.c

	Contains:	Code to support Navigation Services in Nightingale

	Version:	Mac OS X

*/

#include "Nightingale.precomp.h"
#include "Nightingale.appl.h"
#include "NavServices.h"

#ifndef nrequire
	#define nrequire(CONDITION, LABEL) if (true) {if ((CONDITION)) goto LABEL; }
#endif
#ifndef require
#define require(CONDITION, LABEL) if (true) {if (!(CONDITION)) goto LABEL; }
#endif


static NavDialogRef gOpenFileDialog = NULL;


static pascal void NSAppEventProc( const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD );
static pascal void NSLocalEventProc( const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD );
								
static Boolean AdjustMenus(WindowPtr pWindow, Boolean editDialogs, Boolean forceTitlesOn);
static void NSHandleEvent(EventRecord * pEvent);
static void NSHandleNavUserAction( NavDialogRef inNavDialog, NavUserAction inUserAction, void *inContextData );


//
// GetEventUPP
//
static NavEventUPP GetEventUPP()
{
	static NavEventUPP	eventUPP = NULL;				
	if ( eventUPP == NULL )
	{
		eventUPP = NewNavEventUPP( NSAppEventProc );
	}
	return eventUPP;
}


//
// GetPrivateEventUPP
//
static NavEventUPP GetPrivateEventUPP()
{
	static NavEventUPP	privateEventUPP = NULL;				
	if ( privateEventUPP == NULL )
	{
		privateEventUPP = NewNavEventUPP( NSLocalEventProc );
	}
	return privateEventUPP;
}


//
// NewOpenHandle
//
static Handle NewOpenHandle(OSType applicationSignature, short numTypes, OSType typeList[])
{
	Handle hdl = NULL;
	
	if ( numTypes > 0 )
	{
	
		hdl = NewHandle(sizeof(NavTypeList) + numTypes * sizeof(OSType));
	
		if ( hdl != NULL )
		{
			NavTypeListHandle open		= (NavTypeListHandle)hdl;
			
			(*open)->componentSignature = applicationSignature;
			(*open)->osTypeCount		= numTypes;
			BlockMoveData(typeList, (*open)->osType, numTypes * sizeof(OSType));
		}
	}
	
	return hdl;
}



//
// OpenFileDialog
//
OSStatus OpenFileDialog(OSType applicationSignature, 
								short numTypes, 
								OSType typeList[], 
								NavDialogRef *outDialog )
{
	OSStatus theErr = noErr;
	if ( gOpenFileDialog == NULL )
	{
		NavDialogCreationOptions	dialogOptions;
		NavTypeListHandle			openList	= NULL;
	
		NavGetDefaultDialogCreationOptions( &dialogOptions );
	
		dialogOptions.modality = kWindowModalityAppModal;
		dialogOptions.clientName = CFStringCreateWithPascalString( NULL, LMGetCurApName(), GetApplicationTextEncoding());
		
		openList = (NavTypeListHandle)NewOpenHandle( applicationSignature, numTypes, typeList );
		
		theErr = NavCreateGetFileDialog( &dialogOptions, openList, GetPrivateEventUPP(), NULL, NULL, NULL, &gOpenFileDialog );

		if ( theErr == noErr )
		{
			theErr = NavDialogRun( gOpenFileDialog );
			if ( theErr != noErr )
			{
				NavDialogDispose( gOpenFileDialog );
				gOpenFileDialog = NULL;
			}
		}

		if (openList != NULL)
		{
			DisposeHandle((Handle)openList);
		}
		
		if ( dialogOptions.clientName != NULL )
		{
			CFRelease( dialogOptions.clientName );
		}
	}
	else
	{
		if ( NavDialogGetWindow( gOpenFileDialog ) != NULL )
		{
			SelectWindow( NavDialogGetWindow( gOpenFileDialog ));
		}
	}
	
	if ( outDialog != NULL )
	{
		*outDialog = gOpenFileDialog;
	}

	return NULL;
}


//
// TerminateOpenFileDialog
//
void TerminateOpenFileDialog()
{
	if ( gOpenFileDialog != NULL )
	{
		TerminateDialog( gOpenFileDialog );
	}
}


//
// TerminateDialog
//
void TerminateDialog( NavDialogRef inDialog )
{
	NavCustomControl( inDialog, kNavCtlTerminate, NULL );
}


//
// UniversalConfirmSaveDialog
//
static OSStatus UniversalConfirmSaveDialog(WindowRef parentWindow, 
															CFStringRef documentName, 
															Boolean quitting, 
															void* inContextData,
															NavDialogRef *outDialog,
															NavUserAction *outUserAction )
{
	OSStatus 						theErr 			= noErr;
	NavAskSaveChangesAction		action 			= 0;
	NavDialogRef					dialog 			= NULL;
	NavUserAction					userAction 		= kNavUserActionNone;
	NavDialogCreationOptions	dialogOptions;
	NavEventUPP						eventUPP;
	Boolean							disposeAfterRun;

	NavGetDefaultDialogCreationOptions( &dialogOptions );

	action = quitting ? kNavSaveChangesQuittingApplication : kNavSaveChangesClosingDocument;
	dialogOptions.modality = ( parentWindow != NULL ) ? kWindowModalityWindowModal : kWindowModalityAppModal;
	dialogOptions.parentWindow = parentWindow;

	dialogOptions.clientName = CFStringCreateWithPascalString( NULL, LMGetCurApName(), GetApplicationTextEncoding());
	if ( documentName != NULL )
	{
		dialogOptions.saveFileName = documentName;
	}

	eventUPP = ( inContextData == NULL ) ? GetPrivateEventUPP() : GetEventUPP();
	disposeAfterRun = ( dialogOptions.modality == kWindowModalityAppModal && inContextData == NULL );

	theErr = NavCreateAskSaveChangesDialog(	
				&dialogOptions,
				action,
				eventUPP,
				inContextData,
				&dialog );
	
	if ( theErr == noErr )
	{
		theErr = NavDialogRun( dialog );
		if ( theErr != noErr || disposeAfterRun )
		{
			userAction = NavDialogGetUserAction( dialog );
			NavDialogDispose( dialog );
			dialog = NULL;
		}
	}

	if ( dialogOptions.clientName != NULL )
	{
		CFRelease( dialogOptions.clientName );
	}
	if ( outDialog != NULL )
	{
		*outDialog = dialog;
	}
	if ( outUserAction != NULL )
	{
		*outUserAction = userAction;
	}
	return theErr;
}


//
// ConfirmSaveDialog
//
OSStatus ConfirmSaveDialog(
	WindowRef parentWindow, 
	CFStringRef documentName, 
	Boolean quitting, 
	void* inContextData,
	NavDialogRef *outDialog )
{
	return UniversalConfirmSaveDialog( parentWindow, documentName, quitting, inContextData, outDialog, NULL );
}


//
// ModalConfirmSaveDialog
//
OSStatus ModalConfirmSaveDialog(
	CFStringRef documentName, 
	Boolean quitting, 
	NavUserAction *outUserAction )
{
	return UniversalConfirmSaveDialog( NULL, documentName, quitting, NULL, NULL, outUserAction );
}


//
// SaveFileDialog
//
OSStatus SaveFileDialog(
	WindowRef parentWindow, 
	CFStringRef documentName, 
	OSType filetype, 
	OSType fileCreator, 
	void *inContextData,
	NavDialogRef *outDialog )
{
	NavDialogCreationOptions	dialogOptions;
	OSStatus					theErr = noErr;

	NavGetDefaultDialogCreationOptions( &dialogOptions );

	dialogOptions.clientName = CFStringCreateWithPascalString( NULL, LMGetCurApName(), GetApplicationTextEncoding());
	dialogOptions.saveFileName = documentName;
	dialogOptions.modality = ( parentWindow != NULL ) ? kWindowModalityWindowModal : kWindowModalityAppModal;
	dialogOptions.parentWindow = parentWindow;

	theErr = NavCreatePutFileDialog( &dialogOptions, filetype, fileCreator, GetEventUPP(), inContextData, outDialog );
	
	if ( theErr == noErr )
	{
		theErr = NavDialogRun( *outDialog );
		if ( theErr != noErr )
		{
			NavDialogDispose( *outDialog );
		}
		if ( theErr != noErr || dialogOptions.modality == kWindowModalityAppModal )
		{
			*outDialog = NULL;	// The dialog has already been disposed.
		}
	}

	if ( dialogOptions.clientName != NULL )
	{
		CFRelease( dialogOptions.clientName );
	}

	return theErr;
}


//
// BeginSave
//
OSStatus BeginSave( NavDialogRef inDialog, NavReplyRecord* outReply, FSRef* outFileRef )
{
	OSStatus status = paramErr;
	AEDesc		dirDesc;
	AEKeyword	keyword;
	CFIndex		len;

	require( outReply, Return );
	require( outFileRef, Return );

	status = NavDialogGetReply( inDialog, outReply );
	nrequire( status, Return );
	
	status = AEGetNthDesc( &outReply->selection, 1, typeWildCard, &keyword, &dirDesc );
	nrequire( status, DisposeReply );
	
	len = CFStringGetLength( outReply->saveFileName );

	if ( dirDesc.descriptorType == typeFSRef )
	{
		const UInt32	kMaxNameLen = 255;
		FSRef		dirRef;
		UniChar		name[ kMaxNameLen ];

		if ( len > kMaxNameLen )
		{
			len = kMaxNameLen;
		}
	
		status = AEGetDescData( &dirDesc, &dirRef, sizeof( dirRef ));
		nrequire( status, DisposeDesc );
		
		CFStringGetCharacters( outReply->saveFileName, CFRangeMake( 0, len ), &name[0] );
		
		status = FSMakeFSRefUnicode( &dirRef, len, &name[0], GetApplicationTextEncoding(), outFileRef );
		if (status == fnfErr )
		{
                        // file is not there yet - create it and return FSRef
			status = FSCreateFileUnicode( &dirRef, len, &name[0], 0, NULL, outFileRef, NULL );
		}
		else
		{
                        // looks like file is there. Just make sure there is no error
			nrequire( status, DisposeDesc );
		}
	}
	else if ( dirDesc.descriptorType == typeFSS )
	{
                FSSpec	theSpec;
		status = AEGetDescData( &dirDesc, &theSpec, sizeof( FSSpec ));
		nrequire( status, DisposeDesc );

		if ( CFStringGetPascalString( outReply->saveFileName, &(theSpec.name[0]), 
					sizeof( StrFileName ), GetApplicationTextEncoding()))
		{
                        status = FSpMakeFSRef(&theSpec, outFileRef);
			nrequire( status, DisposeDesc );
			status = FSpCreate( &theSpec, 0, 0, smSystemScript );
			nrequire( status, DisposeDesc );
		}
		else
		{
			status = bdNamErr;
			nrequire( status, DisposeDesc );
		}
	}

DisposeDesc:
	AEDisposeDesc( &dirDesc );

DisposeReply:
	if ( status != noErr )
	{
		NavDisposeReply( outReply );
	}

Return:
	return status;
}


//
// CompleteSave
//
OSStatus CompleteSave( NavReplyRecord* inReply, FSRef* inFileRef, Boolean inDidWriteFile )
{
	OSStatus theErr;
	
	if ( inReply->validRecord )
	{
		if ( inDidWriteFile )
		{
			theErr = NavCompleteSave( inReply, kNavTranslateInPlace );
		}
		else if ( !inReply->replacing )
		{
			// Write failed, not replacing, so delete the file
			// that was created in BeginSave.
			FSDeleteObject( inFileRef );
		}

		theErr = NavDisposeReply( inReply );
	}

	return theErr;
}


//
// Callback to handle events that occur while navigation dialogs are up but really should be handled by the application
//

extern void NSHandleEvent( EventRecord * pEvent );
extern void NSHandleNavUserAction( NavDialogRef inNavDialog, NavUserAction inUserAction, void* inContextData );

// Callback to handle event passing betwwn the navigation dialogs and the application

static pascal void NSAppEventProc( const NavEventCallbackMessage callbackSelector, 
											NavCBRecPtr callbackParms, 
											NavCallBackUserData callbackUD )
{
	switch ( callbackSelector )
	{	
		case kNavCBEvent:
			switch (callbackParms->eventData.eventDataParms.event->what)
			{
				case updateEvt:
				case activateEvt:
					NSHandleEvent(callbackParms->eventData.eventDataParms.event);
				break;
			}
			break;

		case kNavCBUserAction:
			NSHandleNavUserAction( callbackParms->context, callbackParms->userAction, callbackUD );
			break;
		
		case kNavCBTerminate:
			NavDialogDispose( callbackParms->context );
			break;
	}
}



//
// NSLocalEventProc
//
static pascal void NSLocalEventProc( const NavEventCallbackMessage callbackSelector, 
												   NavCBRecPtr callbackParms, 
												   NavCallBackUserData /*callbackUD*/ )
{
	switch ( callbackSelector )
	{	
		case kNavCBEvent:
			switch (callbackParms->eventData.eventDataParms.event->what)
			{
				case updateEvt:
				case activateEvt:
					NSHandleEvent(callbackParms->eventData.eventDataParms.event);
				break;
			}
			break;

		case kNavCBUserAction:
			switch ( callbackParms->userAction)
			{
				case kNavUserActionOpen:
					NavReplyRecord	reply;
					OSStatus		status;
					
					status = NavDialogGetReply( callbackParms->context, &reply );
					if ( status == noErr )
					{
//						SendOpenAE( reply.selection );
						NavDisposeReply( &reply );
					}
					break;
					
				case kNavUserActionSaveAs:
					break;
			}
			break;
		
		case kNavCBTerminate:
			if ( callbackParms->context == gOpenFileDialog )
			{
				NavDialogDispose( gOpenFileDialog );
				gOpenFileDialog = NULL;
			}
			
			// if after dismissing the dialog SimpleText has no windows open (so Activate event will not be sent) -
			// call AdjustMenus ourselves to have at right menus enabled
			if (FrontWindow() == nil) AdjustMenus(nil, true, false);
			break;
	}
}


//
// AdjustMenus
//
static Boolean AdjustMenus(WindowPtr /*pWindow*/, Boolean /*editDialogs*/, Boolean /*forceTitlesOn*/)
{
	return false;
}


//
// NSHandleEvent
//
static void NSHandleEvent(EventRecord * pEvent)
{
} // NSHandleEvent


//
// NSHandleNavUserAction
//
static void NSHandleNavUserAction( NavDialogRef inNavDialog, NavUserAction inUserAction, void *inContextData )
{
	OSStatus	status = noErr;

	// We only have to handle the user action if the context data is non-NULL, which
	// means it is an action that applies to a specific document.
	if ( inContextData != NULL )
	{
#if 0
		// The context data is a window data pointer
		WindowDataPtr	pData = (WindowDataPtr)inContextData;
		Boolean			discard = false;

		// The dialog is going away, so clear the window's reference
		pData->navDialog = NULL;

		switch( inUserAction )
		{
			case kNavUserActionCancel:
				// If we were closing, we're not now.
				pData->isClosing = false;
				// If we were closing all, we're not now!
				gMachineInfo.isClosing = false;
				// If we were quitting, we're not now!!
				gMachineInfo.isQuitting = false;
				break;
			
			case kNavUserActionSaveChanges:
				// Do the save, which may or may not trigger the save dialog
				status = DoCommand( pData->theWindow, cSave, 0, 0 );
				break;
			
			case kNavUserActionDontSaveChanges:
				// OK to throw away this document
				discard = true;
				break;
			
			case kNavUserActionSaveAs:
				{
					if ( pData->pSaveTo )
					{
						OSStatus		completeStatus;
						NavReplyRecord		reply;
						FSRef			theRef;
						status = BeginSave( inNavDialog, &reply, &theRef );
						nrequire( status, BailSaveAs );

						status = (*(pData->pSaveTo))( pData->theWindow, pData, &theRef, reply.isStationery );
						completeStatus = CompleteSave( &reply, &theRef, status == noErr );
						if ( status == noErr )
						{
							status = completeStatus;	// So it gets reported to user.
						}
						nrequire( status, BailSaveAs );

						// Leave both forks open
						if ( pData->dataRefNum == -1 )
						{
							status = FSOpenFork( &theRef, 0, NULL, fsRdWrPerm, &pData->dataRefNum );
							nrequire( status, BailSaveAs );
						}

						if ( pData->resRefNum == -1 )
						{
							pData->resRefNum = FSOpenResFile( &theRef, fsRdWrPerm );
							status = ResError();
							nrequire( status, BailSaveAs );
						}

					}
					
BailSaveAs:
				}
				break;
				
			default:
				break;
		}
#endif
#if 0		
		// Now, close the window if it isClosing and
		// everything got clean or was discarded
		if ( status != noErr )
		{
			// Cancel all in-progress actions and alert user.
			pData->isClosing = false;
			gMachineInfo.isClosing = false;
			gMachineInfo.isQuitting = false;
			ConductErrorDialog( status, cSave, cancel );
		}
		else if ( pData->isClosing && ( discard || CanCloseWindow( pData->theWindow )))
		{
			status = DoCloseWindow( pData->theWindow, discard, 0 );

			// If we are closing all then start the close
			// process on the next window
			if ( status == noErr && gMachineInfo.isClosing )
			{
				WindowPtr nextWindow = FrontWindow();
				if ( nextWindow != NULL )
				{
					DoCloseWindow( nextWindow, false, 0 );
				}
			}
		}
#endif
	}
}


