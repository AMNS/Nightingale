/*
	File:		NavServices.c

	Contains:	Code to support Navigation Services in Nightingale

	Version:	Mac OS X

*/

// MAS
//#include "Nightingale.precomp.h"
#include "Nightingale_Prefix.pch"
// MAS
#include "Nightingale.appl.h"
#include "NavServices.h"

#ifndef nrequire
	#define nrequire(CONDITION, LABEL) if (true) {if ((CONDITION)) goto LABEL; }
#endif
#ifndef require
#define require(CONDITION, LABEL) if (true) {if (!(CONDITION)) goto LABEL; }
#endif

FSSpec gFSSpec;

static NavDialogRef gOpenFileDialog = NULL;
static NavDialogRef gSaveFileDialog = NULL;

static NavEventUPP GetNavOpenFileEventUPP(void);
static NavEventUPP GetNavSaveFileEventUPP(void);


static pascal void NSNavOpenEventProc( const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD );
static pascal void NSNavSaveEventProc( const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD );
								
static void NSHandleOpenEvent(NavCBRecPtr callbackParms);
static void NSHandleSaveEvent(NavCBRecPtr callbackParms);

static void NSHandleNavUserAction( NavDialogRef inNavDialog, NavUserAction inUserAction, void *inContextData );

static Handle NewOpenHandle(OSType applicationSignature, short numTypes, OSType typeList[]);

static void NSGetOpenFile(NavReplyRecord	*pReply, NavCallBackUserData callbackUD);
static void NSGetSaveFile(NavReplyRecord	*pReply, NavCallBackUserData callbackUD);

static void TerminateDialog( NavDialogRef inDialog );

// custom dialog item:
#define kControlListID		1320
#define kNewCommand 			1

// the requested dimensions for our sample open customization area:

#define kCustomWidth			100
#define kCustomHeight		40

static short gLastTryWidth;
static short gLastTryHeight;

static Handle gDitlList;

//
// OpenFileDialog
//
OSStatus OpenFileDialog(OSType applicationSignature, 
								short numTypes, 
								OSType typeList[],
								NSClientDataPtr pNSD)
{
	OSStatus theErr = noErr;

	NavDialogCreationOptions	dialogOptions;
	NavTypeListHandle				openList	= NULL;

	NavGetDefaultDialogCreationOptions( &dialogOptions );

	dialogOptions.modality = kWindowModalityAppModal;
	dialogOptions.clientName = CFStringCreateWithPascalString( NULL, LMGetCurApName(), GetApplicationTextEncoding());
	
	openList = (NavTypeListHandle)NewOpenHandle( applicationSignature, numTypes, typeList );
	
	theErr = NavCreateGetFileDialog( &dialogOptions, openList, GetNavOpenFileEventUPP(), NULL, NULL, pNSD, &gOpenFileDialog );

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
	
	return theErr;
}


//
// SaveFileDialog
//
OSStatus SaveFileDialog(
	WindowRef parentWindow, 
	CFStringRef documentName, 
	OSType filetype, 
	OSType fileCreator, 
	NSClientDataPtr pNSD)
{
	NavDialogCreationOptions	dialogOptions;
	OSStatus							theErr = noErr;

	NavGetDefaultDialogCreationOptions( &dialogOptions );

	dialogOptions.clientName = CFStringCreateWithPascalString( NULL, LMGetCurApName(), GetApplicationTextEncoding());
	dialogOptions.saveFileName = documentName;
	dialogOptions.modality = kWindowModalityAppModal;
	dialogOptions.parentWindow = parentWindow;

	theErr = NavCreatePutFileDialog( &dialogOptions, filetype, fileCreator, GetNavSaveFileEventUPP(), pNSD, &gSaveFileDialog );
	
	if ( theErr == noErr )
	{
		theErr = NavDialogRun( gSaveFileDialog );
		if ( theErr != noErr )
		{
			NavDialogDispose( gSaveFileDialog );
			gSaveFileDialog = NULL;
		}
	}

	if ( dialogOptions.clientName != NULL )
	{
		CFRelease( dialogOptions.clientName );
	}

	return theErr;
}


static NavEventUPP GetNavOpenFileEventUPP()
{
	static NavEventUPP	openEventUPP = NULL;				
	if ( openEventUPP == NULL )
	{
		openEventUPP = NewNavEventUPP( NSNavOpenEventProc );
	}
	return openEventUPP;
}

static NavEventUPP GetNavSaveFileEventUPP()
{
	static NavEventUPP	saveEventUPP = NULL;				
	if ( saveEventUPP == NULL )
	{
		saveEventUPP = NewNavEventUPP( NSNavSaveEventProc );
	}
	return saveEventUPP;
}

// -----------------------------------------------------------------------------
// 
// 	HandleNewButton()
// 	

void HandleNewButton(ControlHandle theButton, NavCBRecPtr callBackParms)
{
	OSErr 	theErr = noErr;

	theErr = NavCustomControl(callBackParms->context,kNavCtlCancel,NULL);

	if (theErr == noErr) {
		Boolean ok = DoFileMenu(FM_New);
	}
}


// -----------------------------------------------------------------------------
// 
// 	HandleCustomMouseDown()	
// 

void HandleCustomMouseDown(NavCBRecPtr callBackParms)
{
	OSErr				theErr = noErr;
	ControlHandle	whichControl;				
	Point 			where = callBackParms->eventData.eventDataParms.event->where;	
	short				theItem = 0;	
	UInt16 			firstItem = 0;
	short				realItem = 0;
	short				partCode = 0;
		
	GlobalToLocal(&where);
	theItem = FindDialogItem(GetDialogFromWindow(callBackParms->window),where);	// get the item number of the control
	partCode = FindControl(where,callBackParms->window,&whichControl);	// get the control itself
	
	// ask NavServices for the first custom control's ID:
	if (callBackParms->context != 0)	// always check to see if the context is correct
	{
		theErr = NavCustomControl(callBackParms->context,kNavCtlGetFirstControlID,&firstItem);	
		realItem = theItem - firstItem + 1;		// map it to our DITL constants:	
	}
				
	if (realItem == kNewCommand)
		HandleNewButton(whichControl,callBackParms);
}


//
// NSHandleOpenEvent
//
static void NSHandleOpenEvent(NavCBRecPtr callbackParms)
{
	EventRecord * pEvent = callbackParms->eventData.eventDataParms.event;
	
	switch (pEvent->what)
	{
		case mouseDown:
			HandleCustomMouseDown(callbackParms);
			break;
			
		case updateEvt:
			DoUpdate((WindowPtr)pEvent->message);
			break;
			
		case activateEvt:
			DoActivate(pEvent,(pEvent->modifiers&activeFlag)!=0,FALSE);
			break;
	}
	
}


// Callback to handle events for Nav Services

static pascal void NSNavOpenEventProc( const NavEventCallbackMessage callbackSelector, 
												NavCBRecPtr callbackParms, 
												NavCallBackUserData callbackUD )
{
	switch ( callbackSelector )
	{	
		case kNavCBEvent:
			switch (callbackParms->eventData.eventDataParms.event->what)
			{
				case mouseDown:
				case updateEvt:
				case activateEvt:
					NSHandleOpenEvent(callbackParms);
					break;
			}
			break;
			
		case kNavCBCustomize:
			{
				// here are the desired dimensions for our custom area:
				short neededWidth =  callbackParms->customRect.left + kCustomWidth;
				short neededHeight = callbackParms->customRect.top + kCustomHeight;
				            
				// check to see if this is the first round of negotiations:
				if (( callbackParms->customRect.right == 0) && (callbackParms->customRect.bottom == 0 ))
				{
				   // it is, so tell NavServices what dimensions we want:
				   callbackParms->customRect.right = neededWidth;
				   callbackParms->customRect.bottom = neededHeight;
				}
				else
				{
				   // we are in the middle of negotiating:
				   if ( gLastTryWidth != callbackParms->customRect.right )
				      if ( callbackParms->customRect.right < neededWidth )   
				         // is the given width too small for us?
				         callbackParms->customRect.right = neededWidth;

				   // is the given height too small for us?
				   if ( gLastTryHeight != callbackParms->customRect.bottom )
				      if ( callbackParms->customRect.bottom < neededHeight )
				         callbackParms->customRect.bottom = neededHeight;
				}
				            
				// remember our last size so the next time we can re-negotiate:
				gLastTryWidth = callbackParms->customRect.right;
				gLastTryHeight = callbackParms->customRect.bottom;
			}
			break;

		case kNavCBStart:
			{
				UInt16 firstItem = 0;	
				OSStatus theErr = noErr;
				
				// add the rest of the custom controls via the DITL resource list:
				gDitlList = GetResource('DITL',kControlListID);
				if ((gDitlList != NULL) && (ResError() == noErr))
				{
					if ((theErr = NavCustomControl(callbackParms->context,kNavCtlAddControlList,gDitlList)) == noErr)
					{
						// ask NavServices for our first control ID
						// debugging only
						theErr = NavCustomControl(callbackParms->context,kNavCtlGetFirstControlID,&firstItem);	
					}
				}
			}			
			break;
			
		case kNavCBUserAction:
			switch ( callbackParms->userAction)
			{
				case kNavUserActionOpen:
					NavReplyRecord	reply;
					OSStatus			status;
					
					status = NavDialogGetReply( callbackParms->context, &reply );
					if (status == noErr)
					{
						NSGetOpenFile(&reply, (void*)callbackUD);
					}
					
					status = NavDisposeReply(&reply);
					break;
					
				case kNavUserActionCancel:
		 			NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
		 			pNSD->nsOpCancel = TRUE;
					break;
					
				default:
					NSHandleNavUserAction( callbackParms->context, callbackParms->userAction, callbackUD );
					break;
			}
			break;
		
		case kNavCBTerminate:
			if (gDitlList) {
				ReleaseResource(gDitlList);
				gDitlList = NULL;
			}	
				
			TerminateDialog( callbackParms->context );
			break;
	}
}

// Callback to handle events for Nav Services

static pascal void NSNavSaveEventProc( const NavEventCallbackMessage callbackSelector, 
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
					NSHandleSaveEvent(callbackParms);
					break;
			}
			break;
			
		case kNavCBUserAction:
			switch ( callbackParms->userAction)
			{
				case kNavUserActionOpen:
					NavReplyRecord	reply;
					OSStatus			status;
					
					status = NavDialogGetReply( callbackParms->context, &reply );
					if (status == noErr)
					{
						NSGetOpenFile(&reply, (void*)callbackUD);
					}
					
					status = NavDisposeReply(&reply);
					break;
					
				case kNavUserActionCancel:
		 			NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
		 			pNSD->nsOpCancel = TRUE;
					break;
					
				default:
					NSHandleNavUserAction( callbackParms->context, callbackParms->userAction, callbackUD );
					break;
			}
			break;
		
		case kNavCBTerminate:
			TerminateDialog( callbackParms->context );
			break;
	}
}


//
// NSHandleEvent
//
static void NSHandleSaveEvent(NavCBRecPtr callbackParms)
{
	EventRecord * pEvent = callbackParms->eventData.eventDataParms.event;
	
	switch (pEvent->what)
	{
		case updateEvt:
			DoUpdate((WindowPtr)pEvent->message);
			break;
			
		case activateEvt:
			DoActivate(pEvent,(pEvent->modifiers&activeFlag)!=0,FALSE);
			break;
	}
	
} // NSHandleEvent


//
// NSHandleNavUserAction
//
static void NSHandleNavUserAction( NavDialogRef inNavDialog, NavUserAction inUserAction, void *inContextData )
{
	// We only have to handle the user action if the context data is non-NULL, which
	// means it is an action that applies to a specific document.
	if ( inContextData != NULL )
	{
		// The context data is a window data pointer
		NSClientDataPtr 	pNSD = (NSClientDataPtr)inContextData;	//	pData
		Boolean				discard = false;
				
		pNSD->nsOpCancel = false;

		switch( inUserAction )
		{
			case kNavUserActionCancel:
				// If we were closing, we're not now.
				pNSD->nsIsClosing = false;
				pNSD->nsOpCancel = true;
				break;
			
			case kNavUserActionSaveChanges:
				break;
			
			case kNavUserActionDontSaveChanges:
				// OK to throw away this document
				discard = true;
				break;
			
			case kNavUserActionSaveAs:
				NavReplyRecord	reply;
				OSStatus			status;
				
				status = NavDialogGetReply( inNavDialog, &reply );
				if (status == noErr)
				{
					NSGetSaveFile(&reply, (void*)pNSD);
				}
				
				status = NavDisposeReply(&reply);
				break;
				
			default:
				break;
		}
	}
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
			NavTypeListHandle open = (NavTypeListHandle)hdl;
			
			(*open)->componentSignature = applicationSignature;
			(*open)->osTypeCount	= numTypes;
			BlockMoveData(typeList, (*open)->osType, numTypes * sizeof(OSType));
		}
	}
	
	return hdl;
}

// 1. ??? Do we get this from the reply record for opens?

static void NSGetOpenFile(NavReplyRecord	*pReply, NavCallBackUserData callbackUD)
{
	AEDesc actualDesc;
	FSRef fsrFileToOpen;
	OSStatus err;
	FSSpec fsSpec;
	FSCatalogInfo fsCatInfo;
	HFSUniStr255 theFileName;
	CFStringRef theCFFileName;
	char openFileName[256];
	
	err = AECoerceDesc(&pReply->selection, typeFSRef, &actualDesc);
	 
	if (err == noErr)
	{
		err = AEGetDescData(&actualDesc, (void *)&fsrFileToOpen, sizeof(FSRef));
		if (err == noErr)
		{
			err = FSGetCatalogInfo (&fsrFileToOpen, kFSCatInfoNodeID, 
											&fsCatInfo,		//FSCatalogInfo catalogInfo, 
											&theFileName,	//HFSUniStr255 outName, 
											&fsSpec, 		//FSSpec fsSpec, 
											NULL);			//FSRef parentRef
											
			theCFFileName = CFStringCreateWithCharacters(NULL,
																		theFileName.unicode,
																		theFileName.length);

		 	Boolean success = CFStringGetCString(theCFFileName, openFileName, 256, CFStringGetSystemEncoding());
		 	
		 	NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
		 	
		 	strcpy(pNSD->nsFileName,openFileName);
		 	pNSD->nsFSSpec = fsSpec;
		 	pNSD->nsOpCancel = FALSE;	// 1 ???
		}
	}
}

// 1. ???

static void NSGetSaveFile(NavReplyRecord	*pReply, NavCallBackUserData callbackUD)
{
	AEDesc actualDesc;
	FSRef fsRefParent;
	OSStatus err;
	FSSpec fsSpec;
	FSCatalogInfo fsCatInfo;
//	HFSUniStr255 theFileName;
//	CFStringRef theCFFileName;
	char saveFileName[256];
	FSRef fsrFileToSave;
	
	err = AECoerceDesc(&pReply->selection, typeFSRef, &actualDesc);
	 
	if (err == noErr)
	{
		err = AEGetDescData(&actualDesc, (void *)&fsRefParent, sizeof(FSRef));
		if (err == noErr)
		{
			UniCharCount sourceLen = (UniCharCount)CFStringGetLength(pReply->saveFileName);
			UniChar *nameBuf = (UniChar *)NewPtr(sourceLen*2);
			
			if (nameBuf != NULL) {
			
				CFStringGetCharacters(pReply->saveFileName, CFRangeMake(0, sourceLen), &nameBuf[0]);
				
				err = FSMakeFSRefUnicode(&fsRefParent, sourceLen, nameBuf, kTextEncodingUnicodeDefault, &fsrFileToSave);
				
				if (err == noErr) {	
					err = FSGetCatalogInfo (&fsrFileToSave, kFSCatInfoNone, 
													NULL,				//FSCatalogInfo catalogInfo, 
													NULL,				//HFSUniStr255 outName, 
													&fsSpec, 		//FSSpec fsSpec, 
													NULL);			//FSRef parentRef
													
				 	Boolean success = CFStringGetCString(pReply->saveFileName, saveFileName, 256, CFStringGetSystemEncoding());
				 	
				 	NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
				 	
				 	strcpy(pNSD->nsFileName,saveFileName);
				 	pNSD->nsFSSpec = fsSpec;
				 	pNSD->nsOpCancel = FALSE;
			 	}
			 	else if (err == fnfErr) {
					err = FSGetCatalogInfo (&fsRefParent, kFSCatInfoNodeID, 
													&fsCatInfo,		//FSCatalogInfo catalogInfo, 
													NULL,				//HFSUniStr255 outName, 
													&fsSpec, 		//FSSpec fsSpec, 
													NULL);			//FSRef parentRef
													
				 	Boolean success = CFStringGetCString(pReply->saveFileName, saveFileName, 256, CFStringGetSystemEncoding());
				 	
				 	CToPString(saveFileName);
				 	if (saveFileName[0] > 32)
						saveFileName[0] = 32;
						
				 	NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
				 	FSSpec childFSSpec;
				 	
				 	err = FSMakeFSSpec(fsSpec.vRefNum,fsCatInfo.nodeID,(StringPtr)saveFileName,&childFSSpec);
					if (err == noErr || err == fnfErr) {
					 	strcpy(pNSD->nsFileName,saveFileName);
					 	pNSD->nsFSSpec = childFSSpec;
					 	pNSD->nsOpCancel = FALSE;
				 	}			 	
			 	}
			}
		}
	}
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
// TerminateDialog
//
static void TerminateDialog( NavDialogRef inDialog )
{
	// MAS: this is apparently unnecessary
	// in Leopard, NavCustomControl never returns, so presumably
	// the dialog has been closed already
//	NavCustomControl( inDialog, kNavCtlTerminate, NULL );
	return;
}
