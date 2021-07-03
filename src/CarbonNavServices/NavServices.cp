/* NavServices.c: Code to support Navigation Services in Nightingale */

// MAS
//#include "Nightingale.precomp.h"
#include "Nightingale_Prefix.pch"
// MAS
#include "Nightingale.appl.h"
#include "NavServices.h"

#ifndef nrequire
	#define nrequire(CONDITION, LABEL) if (True) {if ((CONDITION)) goto LABEL; }
#endif
#ifndef require
#define require(CONDITION, LABEL) if (True) {if (!(CONDITION)) goto LABEL; }
#endif

FSSpec gFSSpec;

static NavDialogRef gOpenFileDialog = NULL;
static NavDialogRef gSaveFileDialog = NULL;

static NavEventUPP GetNavOpenFileEventUPP(void);
static NavEventUPP GetNavSaveFileEventUPP(void);

static pascal void NSNavOpenEventProc(const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD);
static pascal void NSNavSaveEventProc(const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD);
								
static void NSHandleOpenEvent(NavCBRecPtr callbackParms);
static void NSHandleSaveEvent(NavCBRecPtr callbackParms);

static void NSHandleNavUserAction(NavDialogRef inNavDialog, NavUserAction inUserAction, void *inContextData);

static Handle NewOpenHandle(OSType applicationSignature, short numTypes, OSType typeList[]);

static void NSGetOpenFile(NavReplyRecord *pReply, NavCallBackUserData callbackUD);
static void NSGetSaveFile(NavReplyRecord *pReply, NavCallBackUserData callbackUD);

static void TerminateDialog(NavDialogRef inDialog);

#define CONTROL_LIST_ID	1320	/* DITL resource list */
#define NEW_COMMAND_DI 	1		/* Custom dialog item */


/* Dimensions we want for our open customization area */

#define CUSTOM_WIDTH	100
#define CUSTOM_HEIGHT	40

static short gLastTryWidth;
static short gLastTryHeight;

static Handle gDitlList;


/* OpenFileDialog */

OSStatus OpenFileDialog(OSType applicationSignature, 
								short numTypes, 
								OSType typeList[],
								NSClientDataPtr pNSD)
{
	OSStatus theErr = noErr;

	NavDialogCreationOptions	dialogOptions;
	NavTypeListHandle			openList = NULL;

	NavGetDefaultDialogCreationOptions(&dialogOptions);

	dialogOptions.modality = kWindowModalityAppModal;
	dialogOptions.clientName = CFStringCreateWithPascalString(NULL, LMGetCurApName(),
									GetApplicationTextEncoding());
	
	openList = (NavTypeListHandle)NewOpenHandle(applicationSignature, numTypes, typeList);
	
	theErr = NavCreateGetFileDialog(&dialogOptions, openList, GetNavOpenFileEventUPP(),
									NULL, NULL, pNSD, &gOpenFileDialog);

	if (theErr == noErr) {
		theErr = NavDialogRun(gOpenFileDialog);
		if (theErr != noErr) {
			NavDialogDispose(gOpenFileDialog);
			gOpenFileDialog = NULL;
		}
	}

	if (openList != NULL) DisposeHandle((Handle)openList);
	if (dialogOptions.clientName != NULL) CFRelease(dialogOptions.clientName);
	
	return theErr;
}


/* SaveFileDialog */

OSStatus SaveFileDialog(
	WindowRef parentWindow, 
	CFStringRef documentName, 
	OSType filetype, 
	OSType fileCreator, 
	NSClientDataPtr pNSD)
{
	NavDialogCreationOptions	dialogOptions;
	OSStatus					theErr = noErr;

	NavGetDefaultDialogCreationOptions(&dialogOptions);

	dialogOptions.clientName = CFStringCreateWithPascalString(NULL, LMGetCurApName(),
								GetApplicationTextEncoding());
	dialogOptions.saveFileName = documentName;
	dialogOptions.modality = kWindowModalityAppModal;
	dialogOptions.parentWindow = parentWindow;

	theErr = NavCreatePutFileDialog(&dialogOptions, filetype, fileCreator,
								GetNavSaveFileEventUPP(), pNSD, &gSaveFileDialog);
	
	if (theErr == noErr) {
		theErr = NavDialogRun(gSaveFileDialog);
		if (theErr != noErr) {
			NavDialogDispose(gSaveFileDialog);
			gSaveFileDialog = NULL;
		}
	}

	if (dialogOptions.clientName != NULL) CFRelease(dialogOptions.clientName);

	return theErr;
}


static NavEventUPP GetNavOpenFileEventUPP()
{
	static NavEventUPP	openEventUPP = NULL;
	
	if (openEventUPP == NULL) openEventUPP = NewNavEventUPP(NSNavOpenEventProc);
	return openEventUPP;
}

static NavEventUPP GetNavSaveFileEventUPP()
{
	static NavEventUPP	saveEventUPP = NULL;
	
	if (saveEventUPP == NULL) saveEventUPP = NewNavEventUPP(NSNavSaveEventProc);
	return saveEventUPP;
}

/* -------------------------------------------------------------------------------------- */
 
/* HandleNewButton */

void HandleNewButton(ControlHandle /* theButton */, NavCBRecPtr callBackParms)
{
	OSErr 	theErr = noErr;

	theErr = NavCustomControl(callBackParms->context, kNavCtlCancel, NULL);

	if (theErr == noErr) (void)DoFileMenu(FM_New);
}


/* -------------------------------------------------------------------------------------- */

/* HandleCustomMouseDown() */ 

void HandleCustomMouseDown(NavCBRecPtr callBackParms)
{
	OSErr			theErr = noErr;
	ControlHandle	whichControl;				
	Point 			where = callBackParms->eventData.eventDataParms.event->where;	
	short			theItem = 0;	
	UInt16 			firstItem = 0;
	short			realItem = 0;
	short			partCode = 0;
		
	/* Get the control */
	
	GlobalToLocal(&where);
	theItem = FindDialogItem(GetDialogFromWindow(callBackParms->window), where);
	partCode = FindControl(where,callBackParms->window, &whichControl);
	
	/* Ask NavServices for the first custom control's ID. If the context is correct, map
	   it to our DITL constants. */
	   
	if (callBackParms->context != 0) {
		theErr = NavCustomControl(callBackParms->context, kNavCtlGetFirstControlID,&firstItem);	
		realItem = theItem - firstItem + 1;			
	}
				
	if (realItem == NEW_COMMAND_DI) HandleNewButton(whichControl, callBackParms);
}


/* NSHandleOpenEvent */

static void NSHandleOpenEvent(NavCBRecPtr callbackParms)
{
	EventRecord *pEvent = callbackParms->eventData.eventDataParms.event;
	
	switch (pEvent->what) {
		case mouseDown:
			HandleCustomMouseDown(callbackParms);
			break;
			
		case updateEvt:
			DoUpdate((WindowPtr)pEvent->message);
			break;
			
		case activateEvt:
			DoActivate(pEvent,(pEvent->modifiers&activeFlag)!=0,False);
			break;
	}
}


/* Callback to handle "open" events for Nav Services */

static pascal void NSNavOpenEventProc(const NavEventCallbackMessage callbackSelector, 
												NavCBRecPtr callbackParms, 
												NavCallBackUserData callbackUD)
{
	switch (callbackSelector) {	
		case kNavCBEvent:
			switch (callbackParms->eventData.eventDataParms.event->what) {
				case mouseDown:
				case updateEvt:
				case activateEvt:
					NSHandleOpenEvent(callbackParms);
					break;
			}
			break;
			
		case kNavCBCustomize:
			{
				/* Set the desired dimensions for our custom area. */
				
				short neededWidth =  callbackParms->customRect.left + CUSTOM_WIDTH;
				short neededHeight = callbackParms->customRect.top + CUSTOM_HEIGHT;
//LogPrintf(LOG_DEBUG, "NSNavOpenEventProc 1: customRect.top=%d neededHeight=%d\n",
//callbackParms->customRect.top, neededHeight);
				            
				/* If this is 1st round of negotiations, tell NavServices the dimensions
				   we want */
				
				if ((callbackParms->customRect.right == 0) && (callbackParms->customRect.bottom == 0)) {
				   callbackParms->customRect.right = neededWidth;
				   callbackParms->customRect.bottom = neededHeight;
				}
				else {
				   /* We're in the middle of negotiating. Is the width or height enough? */
				   
				   if (gLastTryWidth != callbackParms->customRect.right)
				      if (callbackParms->customRect.right < neededWidth)   
				         callbackParms->customRect.right = neededWidth;

				   if (gLastTryHeight != callbackParms->customRect.bottom)
				      if (callbackParms->customRect.bottom < neededHeight)
				         callbackParms->customRect.bottom = neededHeight;
				}
				            
				/* Remember our last size so the next time we can re-negotiate. */
				
				gLastTryWidth = callbackParms->customRect.right;
				gLastTryHeight = callbackParms->customRect.bottom;
//LogPrintf(LOG_DEBUG, "NSNavOpenEventProc 2: customRect.top=%d neededHeight=%d\n",
//callbackParms->customRect.top, neededHeight);
			}
			break;

		case kNavCBStart:
			{
				UInt16 firstItem = 0;	
				OSStatus theErr = noErr;
				
				/* Add the rest of the custom controls via the DITL resource list: */
				
				gDitlList = GetResource('DITL',CONTROL_LIST_ID);
				if ((gDitlList != NULL) && (ResError() == noErr)) {
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
			switch (callbackParms->userAction) {
				case kNavUserActionOpen:
					NavReplyRecord	reply;
					OSStatus		status;
					
					status = NavDialogGetReply(callbackParms->context, &reply);
					if (status == noErr) NSGetOpenFile(&reply, (void*)callbackUD);
					
					status = NavDisposeReply(&reply);
					break;
					
				case kNavUserActionCancel:
                    {
						NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
						pNSD->nsOpCancel = True;
                    }
					break;
					
				default:
					NSHandleNavUserAction(callbackParms->context, callbackParms->userAction, callbackUD);
					break;
			}
			break;
		
		case kNavCBTerminate:
			if (gDitlList) {
				ReleaseResource(gDitlList);
				gDitlList = NULL;
			}	
				
			TerminateDialog(callbackParms->context);
			break;
	}
}


/* Callback to handle "save" events for Nav Services */

static pascal void NSNavSaveEventProc(const NavEventCallbackMessage callbackSelector, 
												NavCBRecPtr callbackParms, 
												NavCallBackUserData callbackUD)
{
	switch (callbackSelector) {	
		case kNavCBEvent:
			switch (callbackParms->eventData.eventDataParms.event->what) {
				case updateEvt:
				case activateEvt:
					NSHandleSaveEvent(callbackParms);
					break;
			}
			break;
			
		case kNavCBUserAction:
			switch (callbackParms->userAction) {
				case kNavUserActionOpen:
					NavReplyRecord	reply;
					OSStatus		status;
					
					status = NavDialogGetReply(callbackParms->context, &reply);
					if (status == noErr) NSGetOpenFile(&reply, (void*)callbackUD);
					
					status = NavDisposeReply(&reply);
					break;
					
				case kNavUserActionCancel:
                    {
						NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
						pNSD->nsOpCancel = True;
                    }
					break;
					
				default:
					NSHandleNavUserAction(callbackParms->context, callbackParms->userAction, callbackUD);
					break;
			}
			break;
		
		case kNavCBTerminate:
			TerminateDialog(callbackParms->context);
			break;
	}
}


/* NSHandleEvent */

static void NSHandleSaveEvent(NavCBRecPtr callbackParms)
{
	EventRecord * pEvent = callbackParms->eventData.eventDataParms.event;
	
	switch (pEvent->what) {
		case updateEvt:
			DoUpdate((WindowPtr)pEvent->message);
			break;
			
		case activateEvt:
			DoActivate(pEvent,(pEvent->modifiers&activeFlag)!=0,False);
			break;
	}
	
} // NSHandleEvent


/* NSHandleNavUserAction */

static void NSHandleNavUserAction(NavDialogRef inNavDialog, NavUserAction inUserAction,
							void *inContextData)
{
	// We only have to handle the user action if the context data is non-NULL, which
	// means it is an action that applies to a specific document.
	if (inContextData != NULL) {
		// The context data is a window data pointer
		NSClientDataPtr 	pNSD = (NSClientDataPtr)inContextData;	//	pData
		Boolean				discard = False;
				
		pNSD->nsOpCancel = False;

		switch(inUserAction) {
			case kNavUserActionCancel:
				// If we were closing, we're not now.
				pNSD->nsIsClosing = False;
				pNSD->nsOpCancel = True;
				break;
			
			case kNavUserActionSaveChanges:
				break;
			
			case kNavUserActionDontSaveChanges:
				// OK to throw away this document
				discard = True;
				break;
			
			case kNavUserActionSaveAs:
				NavReplyRecord	reply;
				OSStatus			status;
				
				status = NavDialogGetReply(inNavDialog, &reply);
				if (status == noErr) NSGetSaveFile(&reply, (void*)pNSD);
				
				status = NavDisposeReply(&reply);
				break;
		}
	}
}

/* NewOpenHandle */

static Handle NewOpenHandle(OSType applicationSignature, short numTypes, OSType typeList[])
{
	Handle hdl = NULL;
	
	if (numTypes > 0) {
	
		hdl = NewHandle(sizeof(NavTypeList) + numTypes * sizeof(OSType));
	
		if (hdl != NULL) {
			NavTypeListHandle open = (NavTypeListHandle)hdl;
			
			(*open)->componentSignature = applicationSignature;
			(*open)->osTypeCount	= numTypes;
			BlockMoveData(typeList, (*open)->osType, numTypes * sizeof(OSType));
		}
	}
	
	return hdl;
}

// 1. ??? Do we get this from the reply record for opens?

static void NSGetOpenFile(NavReplyRecord *pReply, NavCallBackUserData callbackUD)
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
	 
	if (err == noErr) {
		err = AEGetDescData(&actualDesc, (void *)&fsrFileToOpen, sizeof(FSRef));
		if (err == noErr) {
			err = FSGetCatalogInfo (&fsrFileToOpen, kFSCatInfoNodeID, 
											&fsCatInfo,		//FSCatalogInfo catalogInfo, 
											&theFileName,	//HFSUniStr255 outName, 
											&fsSpec, 		//FSSpec fsSpec, 
											NULL);			//FSRef parentRef
											
			theCFFileName = CFStringCreateWithCharacters(NULL, theFileName.unicode,
																theFileName.length);

		 	Boolean success = CFStringGetCString(theCFFileName, openFileName, 256, CFStringGetSystemEncoding());
			if (!success) {
				SysBeep(1);
				LogPrintf(LOG_WARNING, "CFStringGetCString failed.  (NSGetOpenFile)\n");
			}
		 	
		 	NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
		 	
		 	strcpy(pNSD->nsFileName,openFileName);
		 	pNSD->nsFSSpec = fsSpec;
		 	pNSD->nsOpCancel = False;	// 1 ???
		}
	}
}

// 1. ???

static void NSGetSaveFile(NavReplyRecord *pReply, NavCallBackUserData callbackUD)
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
	 
	if (err == noErr) {
		err = AEGetDescData(&actualDesc, (void *)&fsRefParent, sizeof(FSRef));
		if (err == noErr) {
			UniCharCount sourceLen = (UniCharCount)CFStringGetLength(pReply->saveFileName);
			UniChar *nameBuf = (UniChar *)NewPtr(sourceLen*2);
			
			if (nameBuf != NULL) {
				CFStringGetCharacters(pReply->saveFileName, CFRangeMake(0, sourceLen), &nameBuf[0]);
				
				err = FSMakeFSRefUnicode(&fsRefParent, sourceLen, nameBuf, kTextEncodingUnicodeDefault, &fsrFileToSave);
				if (err == noErr) {	
					err = FSGetCatalogInfo (&fsrFileToSave, kFSCatInfoNone, 
													NULL,			//FSCatalogInfo catalogInfo, 
													NULL,			//HFSUniStr255 outName, 
													&fsSpec, 		//FSSpec fsSpec, 
													NULL);			//FSRef parentRef
													
				 	Boolean success = CFStringGetCString(pReply->saveFileName, saveFileName,
															256, CFStringGetSystemEncoding());
					if (!success) {
						SysBeep(1);
						LogPrintf(LOG_WARNING, "CFStringGetCString failed.  (NSGetSaveFile 1)\n");
					}
				 	
				 	NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
				 	
				 	strcpy(pNSD->nsFileName,saveFileName);
				 	pNSD->nsFSSpec = fsSpec;
				 	pNSD->nsOpCancel = False;
			 	}
			 	else if (err == fnfErr) {
					err = FSGetCatalogInfo (&fsRefParent, kFSCatInfoNodeID, 
													&fsCatInfo,		//FSCatalogInfo catalogInfo, 
													NULL,			//HFSUniStr255 outName, 
													&fsSpec, 		//FSSpec fsSpec, 
													NULL);			//FSRef parentRef
													
				 	Boolean success = CFStringGetCString(pReply->saveFileName, saveFileName,
															256, CFStringGetSystemEncoding());
					if (!success) {
						SysBeep(1);
						LogPrintf(LOG_WARNING, "CFStringGetCString failed.  (NSGetSaveFile 2)\n");
					}
				 	
				 	CToPString(saveFileName);
				 	if (saveFileName[0] > 32) saveFileName[0] = 32;		/* Truncate very long names */
						
				 	NSClientDataPtr pNSD = (NSClientDataPtr)callbackUD;
				 	FSSpec childFSSpec;
				 	
				 	err = FSMakeFSSpec(fsSpec.vRefNum,fsCatInfo.nodeID,(StringPtr)saveFileName,&childFSSpec);
					if (err == noErr || err == fnfErr) {
					 	strcpy(pNSD->nsFileName,saveFileName);
					 	pNSD->nsFSSpec = childFSSpec;
					 	pNSD->nsOpCancel = False;
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
		FSRef			dirRef;
		UniChar			name[ kMaxNameLen ];

		if ( len > kMaxNameLen ) len = kMaxNameLen;
	
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
OSStatus CompleteSave( NavReplyRecord* inReply, FSRef* inFileRef, Boolean inDidWriteFile)
{
	OSStatus theErr = noErr;
	
	if (inReply->validRecord) {
		if (inDidWriteFile) theErr = NavCompleteSave(inReply, kNavTranslateInPlace);
		else if (!inReply->replacing) {
			// Write failed, not replacing, so delete the file
			// that was created in BeginSave.
			FSDeleteObject(inFileRef);
		}

		theErr = NavDisposeReply(inReply);
	}

	return theErr;
}



//
// TerminateDialog
//
static void TerminateDialog(NavDialogRef /* inDialog */)
{
	// MAS: this is apparently unnecessary
	// in Leopard, NavCustomControl never returns, so presumably
	// the dialog has been closed already
//	NavCustomControl(inDialog, kNavCtlTerminate, NULL);
	return;
}
