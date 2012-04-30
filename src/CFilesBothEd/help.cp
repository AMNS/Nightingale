/* The MacTutor Help facility. Just call Init_Help() to tell MacTutor Help
where your resources are, and then Do_Help()  to bring up the help screen.
That's all there is to it!

Copyright © Joe Pillera, 1990.  All Rights Reserved.
Copyright © MacTutor Magazine, 1990.  All Rights Reserved. */

/* Modifications for THINK C 5 and 6 mostly by Erik Olson. Further modifications,
including for THINK C 7, by Donald Byrd, 12/94 and later. */

/* CAUTION! If this is compiled separately, be sure the following #include
is for the correct file, and that it does not call for the Consolation library. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#undef ON
#undef OFF
#undef FRONT

#include "helpPrivate.h"


/* Global variables */
static  ListHandle  My_List_Handle;   	/* Handle for a list in the dialog 		*/
static  Rect   		List_Rect; 			/* Rectangle (user-item) of topic list 	*/
static	Rect		Display_Rect;		/* Rectangle (user-item) of the display */
static	Rect		Scroller_Rect;		/* Rectangle (computed) of scroll bar	*/
static 	Rect		Dest_Rect;			/* Destination of TE rectangle			*/
static	Rect		View_Rect;			/* Actual viewing region of text		*/
static	Rect		Frame_Rect;			/* The rectangle to frame for text		*/
static  INT16		Help_Topic;			/* What topic does user want info on?	*/
static  INT16		Current_Pict;		/* Latest picture ID being displayed	*/
static  DialogPtr	helpPtr;			/* Pointer to the help dialog box		*/
static  TEHandle	myTEHandle;			/* The TE Manager data structure	    */
static  Handle		myTEXTHandle;		/* My handle to the TEXT resource		*/
static  ViewMode	Last_Type;			/* Was last one a pict or text?			*/ 
static	ControlHandle myControl;		/* Handle to the text scroller			*/
static 	GrafPtr		savePort;			/* Save a pointer to the old grafport	*/
static	INT16		refnum;				/* Result of trying to open help file	*/

/* Keep track of the screen types */
static	ViewMode	screen_mode[MAX_TOPICS];

/* Disable his menus */
static	INT16		First_Menu;
static	INT16		Last_Menu;

/* Array traversal constants */
static	INT16		START;
static	INT16		FINIS;

/* Log positions of 'PICT' & 'TEXT' resources */
static	TopicStr	topic_name[MAX_TOPICS];
static	INT16		resource_start[MAX_TOPICS];
static	INT16		resource_end[MAX_TOPICS];


/***********************************************************************\
|	 		  	void  Add_List_String( theString, theList )				|
|-----------------------------------------------------------------------|
| Purpose :: To add a (pascal) string to a specified list.				|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	theString	-> Pascal string to add.								|
| 	theList		-> Pointer to the target list.							|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void  Add_List_String( TopicStr *theString, ListHandle theList )
{ 
	short   theRow; 		/* The Row that we are adding 	*/
	Point   cSize;  		/*  Pointer to a cell in a list */
 
	if (theList != NULL) {
		cSize.h = 0;    				
		theRow = LAddRow(1, 32000, theList);
		cSize.v = theRow;   			
		LSetCell((*theString + 1), *theString[0], cSize,theList);
		/* LDraw(cSize, theList);  */ 		
	} 
}

 
/***********************************************************************\
|	 				void  Bold_Button( dPtr, itemNum )					|
|-----------------------------------------------------------------------|
| Purpose :: To draw a thick black line around any dialog box item.		|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	dPtr	-> Pointer to an already opened window.						|
| 	itemNum	-> Item within the dialog to highlight.						|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

void Bold_Button( DialogPtr dPtr, INT16 itemNum )
{
 	Rect		iBox;			/* The rectangle for the button		*/
 	short		iType;			/* Type of dialog item				*/
 	Handle		iHandle;		/* Pointer to the item				*/
 	PenState	oldPenState;	/* Preserve the old drawing state	*/

 	SetPort(GetDialogWindowPort(dPtr));
 	GetPenState(&oldPenState);
 	GetDialogItem(dPtr, itemNum, &iType, &iHandle, &iBox);
 	InsetRect(&iBox, -4, -4);
 	PenSize(3,3);
 	FrameRoundRect(&iBox, 16, 16);
 	SetPenState(&oldPenState);
 }


/***********************************************************************\
|	 			void  Center_Window( theWindow, bumpUp )				|
|-----------------------------------------------------------------------|
| Purpose :: To center a currently opened - but still invisible - 		|
|			 window on the screen.  Once the window is centered, it 	|
|			 should then be made visible for the user.					|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	theWindow	-> Pointer to an already opened window.					|
| 	bumpUp		-> A percentage from center to move the window up.		|
|				   Sometimes a perfectly centered window (bumpUp=0) is	|
|				   annoying to a user, so I provide this facility. Note	|
|				   that a negative value will push the window down.    	|
|	isModeless	-> If so, add an additional 20 pixels for the drag bar. |
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

void Center_Window(DialogPtr theWindow, INT16 bumpUp, Boolean isModeless)
{
	Rect  tempRect;   	/* Temporary rectangle 					*/
	INT16	pixels;			/* Raise from center this amount		*/
	INT16	menuBar = 20;	/* Compensate 20 pixels for menu bar	*/
	Rect screenBitBounds;
	
	/* Compute centering information */

	screenBitBounds = GetQDScreenBitsBounds();
	GetDialogPortBounds(theWindow,&tempRect);
	tempRect.top = ((screenBitBounds.bottom + menuBar + (isModeless ? 20 : 0) - 
	                 screenBitBounds.top) - (tempRect.bottom - tempRect.top)) / 2;
	tempRect.left = ((screenBitBounds.right - screenBitBounds.left) - 
	                (tempRect.right - tempRect.left)) / 2;

	/* Apply any bump-up factor */
	pixels = tempRect.top * (bumpUp / 100.0);
	tempRect.top = tempRect.top - pixels;

	/* Now center window & make it visible */
	MoveWindow(GetDialogWindow(theWindow), tempRect.left, tempRect.top, TRUE);
	SetPort(GetDialogWindowPort(theWindow));  			
  }
  
 
/***********************************************************************\
|	 		  			Boolean Create_Help( void )						|
|-----------------------------------------------------------------------|
| Purpose :: To bring up and initialize MacTutor Help.					|
|-----------------------------------------------------------------------|
| Arguments :: vRefNum and directory ID in which to look for Help file.	|
|-----------------------------------------------------------------------|
| Returns :: TRUE if all went well, FALSE otherwise.  					|
\***********************************************************************/

#define HELP_STRS 530

/*static*/ Boolean Create_Help(short helpVRefNum, long helpDirID)
{
	short	    DType; 				/* Type of dialog item 						*/
	Point   	cSize;  			/* Pointer to a cell in a list  			*/
	Rect    	tempRect;   		/* Temporary rectangle 						*/
	Rect    	dataBounds; 		/* Rect to setup the list  					*/
	Handle   	DItem;    			/* Handle to the dialog item 				*/
	FontInfo   	ThisFontInfo;    	/* Used to determine List element height  	*/
	INT16		index;				/* Loop thru the topic list names			*/
 	Rect		iBox;				/* The rectangle for the next/prev buttons	*/
 	short		iType;				/* Type of dialog item (button)				*/
 	Handle		iHandle;			/* Pointer to the item						*/
 	Str255		filename;
	char		fmtStr[256];
	OSErr		resErr;

	/* -- Preserve pointer to former GrafPort -- */
	GetPort(&savePort);
	
	/* -- First make sure the help file is out there! -- */
	strcpy((char *)filename, HELP_FILE_NAME);
	refnum = HOpenResFile(helpVRefNum, helpDirID, CToPString((char *)filename), fsRdPerm);
	if (refnum==NOT_FOUND) {
	
		StrFileName dummyVolName;
 		SInt16 vRN; 
   	SInt32 appDrID;
   	OSErr	 err;
		
		resErr = ResError();
		dummyVolName[0] = 0;
		err = HGetVol(dummyVolName, &vRN, &appDrID);
	
		GetIndCString(fmtStr, HELP_STRS, 1);   		 		/* "Could not find the file '%s'. It must be in the same..." */
		sprintf(strBuf, fmtStr, HELP_FILE_NAME, PROGRAM_NAME);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return(FALSE);
	}
	
	/* Set up pointers to the introduction screen */
	Help_Topic = START;
	Current_Pict = resource_start[START];
	Last_Type = screen_mode[START];
	
	/* -- Bring up the help screen -- */
	helpPtr = GetNewDialog(Help_Window, NULL, FRONT);
	Center_Window(helpPtr,0,TRUE);
	ShowWindow(GetDialogWindow(helpPtr));   		
	SelectWindow(GetDialogWindow(helpPtr)); 		
	SetPort(GetDialogWindowPort(helpPtr));  		
	Bold_Button(helpPtr,OK_Button);
	
	/* Hide the next & previous buttons for the intro screen */
	GetDialogItem(helpPtr, Next_Button, &iType, &iHandle, &iBox);
	HideControl((ControlHandle)iHandle);
	GetDialogItem(helpPtr, Prev_Button, &iType, &iHandle, &iBox);
	HideControl((ControlHandle)iHandle);

	/* -- Now build the list -- */
	GetDialogItem(helpPtr, Topics_Area, &DType, &DItem, &tempRect);
	SetRect(&List_Rect, tempRect.left, tempRect.top, tempRect.right, tempRect.bottom);
	/* Start w/ full size of the dialog's user item */
	tempRect = List_Rect;    		
	/* Make room for the scroll bar on the right */
	tempRect.right = tempRect.right - 15;
	/* Safety check */
	if (tempRect.right <= (tempRect.left + 15)) 
		tempRect.right = tempRect.left + 15;
	/* Frame it */
	InsetRect(&tempRect, -1, -1);   	
	FrameRect(&tempRect);   			
	InsetRect(&tempRect, 1, 1); 		
	SetRect(&dataBounds, 0, 0, 1, 0);
	/* Width of the list */
	cSize.h = tempRect.right - tempRect.left;
	TextFont(SYSFONTID_SANSSERIF);
	TextSize(10);
	/* Get the current font sizes */
	GetFontInfo(&ThisFontInfo); 		
	/* Height of a cell */
	cSize.v = ThisFontInfo.ascent + ThisFontInfo.descent + ThisFontInfo.leading;
	/* Make the list */
	My_List_Handle =  LNew(&tempRect, &dataBounds, cSize, 0, GetDialogWindow(helpPtr), TRUE, FALSE, FALSE, TRUE);
	/* Set the attributes */
	(*My_List_Handle)->selFlags = lOnlyOne /* + lNoNilHilite */;
	/* Turn drawing off while building list */
	//LSetDrawingMode(FALSE, My_List_Handle);    	
	cSize.v = 0;    					
	LSetSelect(TRUE, cSize, My_List_Handle);
	/* Refresh_Topics();  */
	
	/* -- Fill in the initial list contents -- */
	for ( index = START + 1; index <= FINIS; index++ ) 
	  Add_List_String((TopicStr *)topic_name[index], My_List_Handle);
	  
	/* Now enable drawing of list */
	LSetDrawingMode(TRUE, My_List_Handle);
	
 	/* -- Compute critical info once & for all -- */
 	GetDialogItem(helpPtr, Display_Area, &DType, &DItem, &tempRect);
 	
 	/* The entire display area */
	SetRect(&Display_Rect,	tempRect.left,
	                  		tempRect.top,
	                  		tempRect.right,
	                  		tempRect.bottom);
	                  		
 	/* The scroll bar to the right of it */
	SetRect(&Scroller_Rect,	Display_Rect.right - 15,
	                  		Display_Rect.top,
	                  		Display_Rect.right + 1,
	                  		Display_Rect.bottom);

	/* Create the text-edit clipping regions */
	SetRect(&Frame_Rect,Display_Rect.left,
	                    Display_Rect.top,
	                    Display_Rect.right-14,
	                    Display_Rect.bottom);

	/* Compute the Text Edit destination rectangle */
	SetRect(&Dest_Rect, Frame_Rect.left+4,
	                    Frame_Rect.top+4,
	                    Frame_Rect.right-RMARGIN,
	                    9000);
	                    
	/* The clipping region for the actual text */
	SetRect(&View_Rect,Dest_Rect.left,
	                   Dest_Rect.top+2,
	                   Dest_Rect.right,
	                   Display_Rect.bottom-7);

	/*  For simplicity's sake, assume intro
		screen is always a picture resource. */          
	Display_Pict( Initial_Picture );
	
	FlushDialogPortRect(helpPtr);
	
	return(TRUE);
}


/***********************************************************************\
|	 		  		void  Display_Pict( whichOne )						|
|-----------------------------------------------------------------------|
| Purpose :: Display a new picture in the picture box!					|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	whichOne	-> An offset from the resource id of the first PICT in	|
|				   that category.										|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void Display_Pict( INT16 whichOne )
{
	INT16		nextPict;		/* Next picture to be displayed		*/
 	Rect		iBox;			/* Temporary rectangle 				*/
 	short		iType;			/* Type of dialog item				*/
 	Handle		iHandle;		/* Pointer to the item				*/
	PicHandle	newPict;		/* Pointer to the picture area 		*/
	INT16 		mesg_this_one;	/* The screen we're on now			*/
	INT16		mesg_max_one;	/* Total number of screens in topic */
	Str255 		s1,s2,s3,s4,s5;	/* Temporary strings for message	*/
	
	/* Enable the next & previous buttons? */
	if ( (resource_end[Help_Topic] - resource_start[Help_Topic]) >= 1 ) {
		GetDialogItem(helpPtr, Next_Button, &iType, &iHandle, &iBox);
		ShowControl((ControlHandle)iHandle);
		GetDialogItem(helpPtr, Prev_Button, &iType, &iHandle, &iBox);
		ShowControl((ControlHandle)iHandle);
	}
	else {
		GetDialogItem(helpPtr, Next_Button, &iType, &iHandle, &iBox);
		HideControl((ControlHandle)iHandle);
		GetDialogItem(helpPtr, Prev_Button, &iType, &iHandle, &iBox);
		HideControl((ControlHandle)iHandle);
	}
	
	/* Compute which picture to display */
	nextPict = resource_start[ Help_Topic ] + whichOne;
	mesg_this_one = nextPict - resource_start[ Help_Topic ] + 1;
	mesg_max_one = resource_end[ Help_Topic ] - resource_start[ Help_Topic ] + 1;

	/* Display picture */
	newPict = GetPicture(nextPict);
	
	/* Saftey check */
	if (newPict == NULL) {
	  Error_Message( err_no_pict );
	  return; }

	/* Dispose of previous text record, if there was one */
	if (Last_Type == text) {
		DisposeControl(myControl);
	  	TEDispose(myTEHandle); 
	}

	iBox = Display_Rect;
	InsetRect(&iBox, 1, 1);
	EraseRect(&iBox);
	DrawPicture(newPict,&Display_Rect);
	
		/* Draw border */
	DrawBorder();
	
	/* Avoid an unnecessary update event */
	ValidWindowRect(GetDialogWindow(helpPtr),&Display_Rect);

	/* Display new "status" message, or clear it if intro screen */
	if (Help_Topic == START)
		User_Message("\p");
	else {
		NumToString(mesg_this_one,s1);
		NumToString(mesg_max_one,s2);
		pStrCat( "\pScreen ", s1, s3);
		pStrCat( s3, "\p of ", s4 );
		pStrCat( s4, s2, s5);
		User_Message(s5);
	}

	Current_Pict = nextPict;
	Last_Type = pict;
}


/***********************************************************************\
|	 		  		void  Display_Text( void )							|
|-----------------------------------------------------------------------|
| Purpose :: Display a text help system for the current topic.			|
|-----------------------------------------------------------------------|
| Arguments :: None.													|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void Display_Text()
{
 	short		iType;			/* Type of dialog item			*/
 	Handle		iHandle;		/* Pointer to the item			*/
	Rect 		tempRect;		/* For moving the scroll bar	*/
	
	if (Last_Type == text) HUnlock(myTEXTHandle);

	/* Get a handle to the TEXT resource */
	myTEXTHandle = GetResource('TEXT', resource_start[ Help_Topic ]);
	
	/* Safety check! */
	if (myTEXTHandle == NULL) {
		Error_Message( err_no_text );
		return;
	}

	HLock(myTEXTHandle);

	/* Dispose of previous text record, if there was one */
	if (Last_Type == text) {
		HUnlock((Handle)myTEHandle);
	  	TEDispose(myTEHandle); 
	}
	
	/* Hide the next & previous buttons */
	GetDialogItem(helpPtr, Next_Button, &iType, &iHandle, &tempRect);
	HideControl((ControlHandle)iHandle);
	GetDialogItem(helpPtr, Prev_Button, &iType, &iHandle, &tempRect);
	HideControl((ControlHandle)iHandle);

	/* Erase the display area */
        if (Last_Type == pict) tempRect = Display_Rect;
        else				   tempRect = Frame_Rect;
        InsetRect(&tempRect, 1, 1);
        EraseRect(&tempRect);
	
    /* Clear the "status" message */
    User_Message("\p");

	/* Bring up the control */
	if (Last_Type != text)
		myControl = NewControl(GetDialogWindow(helpPtr),&Scroller_Rect,"\p",FALSE,1,1,10,16,0L);
	
 	/* Dim the picture scrolling buttons */
 	Set_Button_State( Next_Button, FALSE );
  	Set_Button_State( Prev_Button, FALSE );
	 
	/* Create a text record */
	myTEHandle = TENew(&Dest_Rect,&View_Rect);
	HLock((Handle)myTEHandle);
	
	/* Display the goods */
	TESetText(*myTEXTHandle, GetResourceSizeOnDisk(myTEXTHandle), myTEHandle);
   TEUpdate(&View_Rect,myTEHandle);
    	
	/* Draw border */
	DrawBorder();
	
	/* Do we need an active scroll bar? */
	if ( ( (View_Rect.bottom - View_Rect.top) / (*myTEHandle)->lineHeight)  <
	     ( (*myTEHandle)->nLines ) )
	  HiliteControl(myControl,ON);
	else
	  HiliteControl(myControl,OFF);

	/* Avoid an unnecessary update event */
	ValidWindowRect(GetDialogWindow(helpPtr),&Display_Rect);
	
    /* Set important values and exit */
	SetControlMaximum(myControl,((*myTEHandle)->nLines - 
	                     ((View_Rect.bottom - View_Rect.top)/(*myTEHandle)->lineHeight)));
	SetControlValue (myControl, 0);

	/* Now draw the scroll bar */
	ShowControl(myControl);
		
	Last_Type = text;
}


/***********************************************************************\
|	 		  	void  Do_Help( short, long )							|
|-----------------------------------------------------------------------|
| Purpose :: To present an extensive help system to the user.  This		|
|			 routine handles scrolling lists and dynamic pictures.		|
|-----------------------------------------------------------------------|
| Arguments :: vRefNum and directory ID in which to look for Help file.	|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/
 
void   Do_Help( short helpVRefNum, long helpDirID )
{
	MenuHandle	thisMenu;		/* Current menu bar to disable   */
	INT16		index;			/* Loop thru menu resources		 */
	
	/* Bring up the help box */
	if ( Create_Help(helpVRefNum, helpDirID) ) {
		/* Disable The Menus */
		for (index = First_Menu; index <= Last_Menu; index++) {
			thisMenu = GetMenuHandle(index);
			if (thisMenu!=NULL) DisableMenuItem(thisMenu,0);
		}
		DrawMenuBar();
		
		/* Trap help window events */
		Help_Event_Loop();
	
		/* Re-enable the menu bar and exit */
		for (index = First_Menu; index <= Last_Menu; index++) {
			thisMenu = GetMenuHandle(index);
			if (thisMenu!=NULL) EnableMenuItem(thisMenu,0);
		}
		DrawMenuBar();
	}
}


/***********************************************************************\
|	 					void  Error_Message( theError )					|
|-----------------------------------------------------------------------|
| Purpose :: To warn the user about bad information discovered while	|
|			 reading in the HTBL resource.								|
|-----------------------------------------------------------------------|
| Arguments ::	An enumerated type describing the error detected.		|
| 				These should never occur if Nightingale's resources and	|
|				the Help file are valid and consistent with each other!	|
|-----------------------------------------------------------------------|
| Returns :: Nothing.													|
\***********************************************************************/

/*static*/ void Error_Message( ErrorTypes theError )
  {
  	Cell		aCell;					/* Change list selection when restoring previous topic */
  	Boolean		goIntro = FALSE;		/* Go back to previous topic? */
	  	  
  	/* Alert with appropriate text message */
  	switch (theError) {
  		case err_no_HTBL :
   			CParamText("No HTBL (Help Table) [ID=128] resource was found in your resource fork!  Please create one before continuing.", "", "", "");
			StopInform(GENERIC_ALRT);
  			break;

  		case err_min_res :
   			CParamText("The help system needs at least an intro screen and one help screen.", "", "", "");
			StopInform(GENERIC_ALRT);
  			break;

  		case err_intro_pict :
   			CParamText("This program assumes that the introduction screen is always a picture.  Please change this screen.", "", "", "");
			StopInform(GENERIC_ALRT);
  			break;

  		case err_bad_type :
   			CParamText("One of your HTBL fields for screen type is incorrect.  Screen types are either PICT or TEXT.", "", "", "");
			StopInform(GENERIC_ALRT);
  			break;

  		case err_no_pict :
   			CParamText("The requested PICT for this topic does not exist!  Will return to introduction screen.", "", "", "");
			StopInform(GENERIC_ALRT);
  			goIntro = TRUE;
  			break;

  		case err_no_text :
   			CParamText("The TEXT for this topic does not exist!  Will return to introduction screen.", "", "", "");
			StopInform(GENERIC_ALRT);
  			goIntro = TRUE;
  			break;
   		
  		default :
  			break;
  	}
  	
	/* Return to intro screen if appropriate */
 	if (goIntro) {
 		aCell = (*My_List_Handle)->lastClick;		/* get cell of nonexistent topic */
 		LSetSelect (FALSE, aCell, My_List_Handle);	/* de-select its name in list */
 		Handle_List_Event(Help_Topic = START-1);	/* return to intro screen */
 	}
  }


/***********************************************************************\
|	 		  	void  Handle_List_Event( whatHit )						|
|-----------------------------------------------------------------------|
| Purpose :: To process any mouse-downs in the topics list.				|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	whatHit		-> Which item was selected by the user.					|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void Handle_List_Event( INT16 whatHit )
{
  	/* Get the selected topic from the user */
  	Help_Topic = whatHit + 1;
  	
  	/* If there are >1 screens, enable the "Next" button */
  	if (resource_end[Help_Topic] - resource_start[Help_Topic]) {
  	  	Set_Button_State( Next_Button, TRUE );
  	  	Set_Button_State( Prev_Button, FALSE );
  	}
  	else {
  	  	Set_Button_State( Next_Button, FALSE );
  	  	Set_Button_State(Prev_Button, FALSE );
  	}
  	
  	/* Display first screen in topic */
  	if ( screen_mode[ Help_Topic ] == pict )
  	  	Display_Pict( Initial_Picture );
  	else if ( screen_mode[ Help_Topic ] == text )
  	  	Display_Text();
  	else
  	  	SysBeep(1);
}


/***********************************************************************\
|	 				void  Handle_Update( void )							|
|-----------------------------------------------------------------------|
| Purpose :: To handle any update to the dialog.  For example: it will	|
|		     refresh the screen when another dialog has been placed		|
|			 over it, etc.												|
|-----------------------------------------------------------------------|
| Arguments ::	None.													|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void Handle_Update()
{
	PicHandle	thePict;		/* Pointer to the picture area 		*/
	GrafPtr		oldPort;		/* Restore old graf port when done	*/

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(helpPtr));  		
	BeginUpdate(GetDialogWindow(helpPtr));
	
	Refresh_Topics();
	Bold_Button(helpPtr,OK_Button);

    	/* Specific code for text or pictures... */
    if ( screen_mode[ Help_Topic ] == text ) {
    	TEUpdate(&View_Rect,myTEHandle);
    }
	
	else if ( screen_mode[ Help_Topic ] == pict ) {
		thePict = GetPicture(Current_Pict);
		/* Saftey check */
		if (thePict == NULL) {
	  		SysBeep(1);
	  		return;
		}
		EraseRect(&Display_Rect);
		DrawPicture(thePict,&Display_Rect);
	}
	
		/* Draw border */
	DrawBorder();

	/* DrawDialog(helpPtr); */
	UpdateDialogVisRgn(helpPtr);
	
	EndUpdate(GetDialogWindow(helpPtr));
	SetPort(oldPort);
}


/***********************************************************************\
|	 				void  Help_Event_Loop( void )						|
|-----------------------------------------------------------------------|
| Purpose :: To handle any events inside my window.  Once an event 		|
|		     takes place outside this window's domain, I return			|
|			 immediately, and let the main event loop handle it.		|
|-----------------------------------------------------------------------|
| Arguments ::	None.													|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void Help_Event_Loop()
{
	EventRecord 		theEvent;		/* Most current real-time user event		*/
	Rect					tempRect;		/* Rectangle of the about... window			*/
	Point   				myPt;    		/* Current list selection point 			*/
	WindowPtr 			wPtr;				/* The pointer to the current window		*/
	INT16 				thePart;			/* The type of mouse-down that occured		*/
	INT16 				key;				/* Did the user hit the <return> key?		*/
	INT16					whatHit;			/* Integer id of the dialog item user hit	*/
	Rect   				DRect;    		/* Rectangle used for finding hit point 	*/
	short    			DType; 			/* Type of dialog item (for OK button)		*/
	INT16    			itemHit;   		/* Get selection from the user				*/
	Handle   			DItem;    		/* Handle to the dialog item 				*/
	Boolean   			DoubleClick;  	/* Flag to see if double click on a list 	*/
	INT16					oldValue;		/* Former value of scroll bar before event  */
	INT16 				rc;				/* Return code from TrackControl() - ignore	*/
	Cell 					cellHit;			/* Find out where user single-clicked 		*/
	ControlHandle		aControl;		/* Event took place in my scroller			*/
	static  INT16 		lastHit;			/* Nicety: prevent redundant processing 	*/
	WindowPtr			w;
	
	/* -- Enter the event loop -- */
	InitCursor();
	HiliteMenu(0);
	while (TRUE) {
	SetPort(GetDialogWindowPort(helpPtr));  		
		SelectWindow(GetDialogWindow(helpPtr));
		InitCursor();

		if (GetNextEvent(everyEvent, &theEvent)) {
			switch (theEvent.what)
			{
			  case updateEvt:
				w = (WindowPtr)theEvent.message;
			  	if (w == GetDialogWindow(helpPtr)) 
					Handle_Update();
				else if (GetWindowKind(w) == DOCUMENTKIND || GetWindowKind(w) == PALETTEKIND)
					DoUpdate(w);
 		  		break;
			  
			  case keyDown:
			    /* If it's a <return> or <enter> key I want it  */
			    key = ( (unsigned char) theEvent.message & charCodeMask );
			    if ( (key == '\r' || key==0x03 /* charCode for enterKey */) ) {
			    	/* Treat this like a mouse-down in the OK button */
					SetPort(GetDialogWindowPort(helpPtr));  		
					GetDialogItem(helpPtr, OK_Button, &DType, &DItem, &DRect);
					HiliteControl((ControlHandle)DItem,1);
					SleepTicks(8);
					Kill_Window();
					return;
			    }
			  	break;

			  case mouseDown:
			  	thePart = FindWindow(theEvent.where, &wPtr);
			  	/* The following is tricky: it allows you to switch tasks, but
			  		but will not let you pull down any of the disabled menus. */
			  	DialogPtr d = GetDialogFromWindow(wPtr);
		     	if (d != helpPtr) {
			  		if (thePart != inMenuBar)
			  		  SysBeep(1);
			  		continue;
			  	} 
			  	switch (thePart)
				{
				  case inMenuBar:
				  	rc = MenuSelect(theEvent.where);
				  	break;
				  	
			  	  case inDrag:
			  	   tempRect = GetQDScreenBitsBounds();
					SetRect(&tempRect, tempRect.left + 10, tempRect.top + 25, 
								       tempRect.right - 10,tempRect.bottom - 10);
			  	  	DragWindow(GetDialogWindow(helpPtr), theEvent.where, &tempRect);
			  	  	break;

			  	  case inContent:
			  	  	if (DialogSelect(&theEvent, &helpPtr, &whatHit)) {
						myPt = theEvent.where;     	
						GlobalToLocal(&myPt);   		
							
			  	  	  	/* Is the user all done? */
			  	  	  	if (whatHit == OK_Button) {
			  	  	    	Kill_Window();
				  	  		return;
				  	  	}
		
		  	  			else if ((whatHit == Next_Button) || (whatHit == Prev_Button))
  	  					  Scroll_Picture( whatHit );

							/* Was it an event in the topics list? */
						else if (PtInRect(myPt,&List_Rect) == TRUE)  {
							lastHit = Help_Topic -1;
							DoubleClick = LClick(myPt, theEvent.modifiers, My_List_Handle);
							cellHit = LLastClick(My_List_Handle);			/* Click below last topic returns cell >= FINIS */
							if (cellHit.v >= FINIS) continue;			/* Ignore click below last topic; go to top of event loop */
							SetPt(&cellHit, 0, 0);
							LGetSelect(TRUE, &cellHit, My_List_Handle);		/* Display topic selected on mouseUp */
							itemHit = cellHit.v;						/* NB: MouseUp below last topic returns cell==0 (same as 1st topic) */
							if ((itemHit >= 0) && (itemHit < FINIS) && (itemHit != lastHit)) {
			  					Handle_List_Event(itemHit);
							}
						 } 
		 
						/* Was it a text scroller event? */
						else if ( (PtInRect(myPt, &Scroller_Rect))  && 
		          				  (screen_mode[ Help_Topic ] == text) ) {
		  					thePart = FindControl(myPt, wPtr, &aControl);
		  					if (aControl) {				/* make sure controlHandle != NULL */
			  					oldValue = GetControlValue(aControl);
			  					if ( thePart == kControlIndicatorPart ) {
			    			 		rc = TrackControl(aControl, myPt, (ControlActionUPP)0);
			     					Scroll_Text( oldValue, GetControlValue(aControl) );
			  					}
			  					else if ( (thePart == kControlUpButtonPart) || (thePart == kControlDownButtonPart) ||
			            				  (thePart == kControlPageUpPart)   || (thePart == kControlPageDownPart) ) { 
									ControlActionUPP actionUPP;
									actionUPP = NewControlActionUPP(My_Scroll_Filter);
									if (actionUPP) {
			     						rc = TrackControl(aControl, myPt, actionUPP);
			     						DisposeControlActionUPP(actionUPP);
			     					}
		   					 	}
		   					 }
		   				}
					}
					break;
				}
			}
		}
	}
}


/***********************************************************************\
|	 		  				void Init_Help( void )						|
|-----------------------------------------------------------------------|
| Purpose :: Call this routine just once, to read in all of the user's	|
|			 information from the HTBL resource fork. This information  |
|			 is then stored in global variables for later use.			|
|			 NOTE!  This routine assumes HTBL has ID=128.				|
|-----------------------------------------------------------------------|
| Arguments ::	None.													|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

void Init_Help()
{	
	register INT16		index;		/* Loop thru user's information 	*/
	char				*HTBL_ptr;	/* Establish the basline pointer	*/
	Help_Info_Handle	h;			/* Cast HTBL to this data structure	*/
	long				mode;		/* Longint version of PICT or TEXT	*/
	char				ch;			/* First letter of PICT or TEXT 	*/
	
	/* Get handle to the HTBL resource and lock it. If none, it's an error. */
	h = (Help_Info_Handle) GetResource('HTBL',128);
	if ((h == NULL) || (ResError() != noErr))
		Error_Message( err_no_HTBL );
	HLock((Handle)h);
	
	/* Establish pointer to start of HTBL resource */
 	HTBL_ptr = (char *) *h;
    First_Menu = ParseInt( (char **) &HTBL_ptr );
    Last_Menu = ParseInt( (char **) &HTBL_ptr );
	START = 0;
	FINIS = ParseInt( (char **) &HTBL_ptr );
	    
    /* Enough help screens to proceed? */
    if (FINIS < 1) 
    	Error_Message( err_min_res );
    
    /* Now loop thru all screens & get the information */
  	for (index = 0; index <= FINIS; index++) {
  		/* Is this a 'PICT' or 'TEXT' screen? */
	    mode = ParseOSType( (char **) &HTBL_ptr );
	    ch = (char)(mode >> 24);
	    if ( (ch == 'P') || (ch == 'p') ) 
	    	screen_mode[index]	  = pict;
	    else if ( (ch == 'T') || (ch == 't') )
	    	screen_mode[index]	  = text;
	    else 
	    	/* Bad screen type! */
	    	Error_Message( err_bad_type );
	    
	    /* Is intro screen a PICT? */
	    if ((index == 0) && (screen_mode[index] != pict)) 
	    	Error_Message( err_intro_pict );
	    
	    /* Get resource bounds */
	    resource_start[index] = ParseInt( (char **) &HTBL_ptr );
	    resource_end[index]   = ParseInt( (char **) &HTBL_ptr );
	    
	    /* Retrieve title of this topic */
	    ParseString( (char *)topic_name[index], (char **) &HTBL_ptr );
	}
	HUnlock((Handle)h);
}


/***********************************************************************\
|  						void Kill_Window( void )						|
|-----------------------------------------------------------------------|
| Purpose: When we get here, the user has clicked on the "done" button.	|
|    	   Dispose of all dynamic (heap) data structures and restore	| 
|		   the original grafport.										|
|-----------------------------------------------------------------------|
| Arguments ::	None.													|
|-----------------------------------------------------------------------|
| Returns :: Void.														|
\***********************************************************************/

/*static*/ void Kill_Window()
{

	if (Last_Type == text) {
	  	TEDispose(myTEHandle); 
	}
	LDispose(My_List_Handle);
	DisposeDialog(helpPtr);     	
	CloseResFile(refnum);
	SetPort(savePort);
}


/***********************************************************************\
|  				void My_Scroll_Filter( theControl, thePart )			|
|-----------------------------------------------------------------------|
| Purpose: TrackControl() will call this routine for all events except	|
|    	   dragging the thumb wheel by the user.  All "up & down" 		| 
|		   events by the user are then translated into the correct		|
|		   TextEdit Manager calls to correctly position the text.		|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	theControl	-> Handle the the scroller control.						|
| 	thePart		-> That part of the scroller the user interacted with.	|
|-----------------------------------------------------------------------|
| Returns :: Void.														|
\***********************************************************************/

/*static*/ pascal void My_Scroll_Filter( ControlHandle theControl, INT16 thePart )
{
	INT16	start, stop;	/* The outer boundry values of the scroller */
	INT16	page;			/* The computed amount to correctly page	*/
	INT16	newValue;		/* The new value to scroll up or down		*/
	INT16	oldValue;		/* The previous value of the control		*/
	
	/* Get the upper & lower limits to perform saftey checks */
	start = GetControlMinimum(theControl);
	stop  = GetControlMaximum(theControl);
	oldValue = GetControlValue(theControl);
	
	/* Compute the amount of scrolling for page ups & downs */
	page = (Display_Rect.bottom - Display_Rect.top) / (*myTEHandle)->lineHeight;
	
	/* Get the current value of the control */
	newValue = GetControlValue(theControl);

	/* Process the event accordingly */
	switch ( thePart ) {
	
		case kControlUpButtonPart	: 	/* Scroll up one line */
				  		  	newValue -= 1;
				  		  	if (newValue < start)
				    		  newValue = start;
				  			break;
				  
		case kControlDownButtonPart: 	/* Scroll down one line */
				  			newValue += 1;
				  			if (newValue > stop)
				    		  newValue = stop;
				  			break;

		case kControlPageUpPart	: 	/* Perform a page up */
				  			newValue -= page;
				  			if (newValue < start)
				    		  newValue = start;
				  			break;

		case kControlPageDownPart	: 	/* Perform a page down */
				  			newValue += page;
				  			if (newValue > stop)
				    		  newValue = stop;
				  			break;

		default			: 	break;
	}
	
	/* Now set the control to the proper amount */
	SetControlValue( theControl, newValue );
	SleepTicks(2); 
    Scroll_Text( oldValue, newValue );
}
  

/***********************************************************************\
|  						INT16 ParseInt( pPointer )						|
|-----------------------------------------------------------------------|
| Purpose: Get the next integer from the HTBL resource.					|
|-----------------------------------------------------------------------|
| Arguments ::	Handle into the resource.								|
|-----------------------------------------------------------------------|
| Returns :: Next integer in the HTBL resource.							|
\***********************************************************************/

/*static*/ INT16 ParseInt( char **pPointer )
{
	INT16	result=0;			/* Final integer result		   */
	Byte	hiWord, lowWord;	/* Store 2 halves of the INT16 */
	
	/* Get the raw data */
	hiWord = (Byte) *((*pPointer)++);
	lowWord = (Byte) *((*pPointer)++);
	
	/* Now pack these into the integer! */
	result = result | hiWord;
	result = (result << 8) | lowWord;
	
 	return(result);
}
	
	
/***********************************************************************\
|  						INT16 ParseOSType( pPointer )					|
|-----------------------------------------------------------------------|
| Purpose: Get the next screen type ('PICT' or 'TEXT') from the HTBL 	|
|		   resource.													|
|-----------------------------------------------------------------------|
| Arguments ::	Handle into the resource.								|
|-----------------------------------------------------------------------|
| Returns :: Next longint in the HTBL resource.							|
\***********************************************************************/

/*static*/ long ParseOSType( char **pPointer )
{
 	long	result=0;		/* Final longint to return		*/
	char	nextByte;		/* 1/4 of the longint 			*/
	INT16	index;			/* Loop thru bytes in a long	*/
	
	/* Get the first of four bytes */
	nextByte = (char) *((*pPointer)++);
	result = result | (long) nextByte;
	
	/* Now loop thru the rest! */
	for (index = 1; index < sizeof(long); index++) {
		nextByte = (char) *((*pPointer)++);
		result = (result << 8) | (long) nextByte;
	}
 	return(result);
}


/***********************************************************************\
|  					INT16 ParseString( pDest, pSource )					|
|-----------------------------------------------------------------------|
| Purpose: Get the next string from the HTBL resource.					|
|-----------------------------------------------------------------------|
| Arguments ::	pDest ->   Returned string.								|
|				pSource -> Handle to the HTBL resource.					|
|-----------------------------------------------------------------------|
| Returns :: The (pascal) string for the next topics title.				|
\***********************************************************************/

/*static*/ void ParseString( char *pDest, char **pSource )
  {
	register INT16 x;
	register INT16 iLen = *pDest++ = *((*pSource)++);
	
	/* if resource string is longer than allowed, truncate */
	if (iLen>TOPIC_NAME_LENGTH) {
		*(--pDest) = TOPIC_NAME_LENGTH;			/* go back and set revised length */
		*pDest++;
		for (x=1; x <= TOPIC_NAME_LENGTH; x++, iLen--)
			*pDest++ = *((*pSource)++);
		while (--iLen >= 0) *((*pSource)++);	/* increment pointer without copying chars */
	}
	else {
		while (--iLen >= 0) 
			*pDest++ = *((*pSource)++);
	}
  }


/***********************************************************************\
|	 					void  pStrCat( p1, p2, p3 )						|
|-----------------------------------------------------------------------|
| Purpose :: Concatenate two pascal strings "p1" and "p2" together, 	|
|			 giving	result "p3". Not compatible with Ngale's PStrCat!	|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	p1	-> Source string one.											|
| 	p2	-> Source string two.											|
| 	p3	-> Destination string.											|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void pStrCat( const Str255 p1, const Str255 p2, Str255 p3 )
  {
    INT16 len1, len2;
	
	len1 = *p1++;
	len2 = *p2++;
	
	*p3++ = len1 + len2;
	while (--len1 >= 0) *p3++=*p1++;
	while (--len2 >= 0) *p3++=*p2++;
  }

 
/***********************************************************************\
|	 		  		void  Refresh_Topics( void )						|
|-----------------------------------------------------------------------|
| Purpose :: To draw (or redraw) the contents of the topics list.		|
|-----------------------------------------------------------------------|
| Arguments ::	None.													|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void  Refresh_Topics() 
{ 
	Rect     tempRect;   	/* Temporary rectangle 		*/
 
 	LUpdateDVisRgn(helpPtr,My_List_Handle);
	tempRect = List_Rect;    		
	InsetRect(&tempRect, -1, -1);   	
	FrameRect(&tempRect);   			
} 


/***********************************************************************\
|	 		  	void  Scroll_Picture( whatHit )							|
|-----------------------------------------------------------------------|
| Purpose :: To display the next (or previous) picture in a help topic.	|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	whatHit	-> Equal to Next_Button or Prev_Button in the dialog box.	|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void Scroll_Picture( INT16 whatHit )
{
	INT16	theMax;			/* High bounds of help topic 	*/
	INT16	theMin;			/* Low bounds of that topic	 	*/
	
	/* Find out which pictures are relevant */
	theMin = resource_start[ Help_Topic ];
	theMax = resource_end[ Help_Topic ];
 	
 	/* Compute the new picture to display! */
 	if ( (whatHit == Next_Button) && (Current_Pict < theMax) ) {
		Current_Pict++;
 		Set_Button_State( Prev_Button, TRUE );
 		if (Current_Pict < theMax)
 		  Set_Button_State( Next_Button, TRUE );
 		else
 		  Set_Button_State( Next_Button, FALSE );
 		Display_Pict( Current_Pict - theMin );
   	}
   	else if ( (whatHit == Prev_Button) && (Current_Pict > theMin) ) {
		Current_Pict--;
 		Set_Button_State( Next_Button, TRUE );
 		if (Current_Pict > theMin)
 		  Set_Button_State( Prev_Button, TRUE );
 		else
 		  Set_Button_State( Prev_Button, FALSE );
 		Display_Pict( Current_Pict - theMin );
   	}
   	else
   		SysBeep(1);
}


/***********************************************************************\
|	 		  	void  Scroll_Text( old, new )							|
|-----------------------------------------------------------------------|
| Purpose :: To dynamically scroll the text edit window for the user.	|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	old	-> The previous value of the scroll bar before the user event.	|
| 	new	-> The new value to scroll the text.							|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void Scroll_Text( INT16 oldValue, INT16 newValue )
{
  TEScroll(0, (oldValue - newValue) * (*myTEHandle)->lineHeight, myTEHandle);
}


/***********************************************************************\
|	 		void  Set_Button_State( itemNum, state )					|
|-----------------------------------------------------------------------|
| Purpose :: To either enable or disable a button within a dialog box.	|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	itemNum	-> Item within the dialog to modify.						|
|	state	-> TRUE = on, FALSE = off.									|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void Set_Button_State( INT16 itemNum, Boolean state )
 {
 	Rect		iBox;			/* The rectangle for the button		*/
 	short		iType;			/* Type of dialog item				*/
 	Handle		iHandle;		/* Pointer to the item				*/
 	
 	GetDialogItem(helpPtr, itemNum, &iType, &iHandle, &iBox);
 	if ( state )
 	  HiliteControl( (ControlHandle) iHandle, 0 );
 	else
 	  HiliteControl( (ControlHandle) iHandle, 255);
 }


/***********************************************************************\
|	 					void  User_Message( theStr )					|
|-----------------------------------------------------------------------|
| Purpose :: To display a status message in the "Picture Scrolling"		|
|			 window.													|
|-----------------------------------------------------------------------|
| Arguments ::															|
| 	theStr	-> The pascal string to display.							|
|-----------------------------------------------------------------------|
| Returns :: void.														|
\***********************************************************************/

/*static*/ void User_Message( const unsigned char *theStr )
{
 	Rect		iBox;			/* The rectangle of the picture		*/
 	short		iType;			/* Type of dialog item				*/
 	Handle		iHandle;		/* Pointer to the item				*/

	GetDialogItem(helpPtr, Message_Area, &iType, &iHandle, &iBox);
	SetDialogItemText(iHandle, theStr);
	/* DrawDialog(helpPtr); */
	UpdateDialogVisRgn(helpPtr);
}

/*static*/ void DrawBorder()
{
	Rect tempR;
	
	if (Help_Topic==START) return;		/* Don't draw border for intro screen */

   	if (screen_mode[Help_Topic]==pict)
   		tempR = Display_Rect;
   	else tempR = Frame_Rect;
	
	FrameRect(&tempR);
}
