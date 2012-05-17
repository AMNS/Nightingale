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

#if 0
// Listing 3-5 Handling a copy event


// 1.	There must be something in the selection, otherwise it doesn't make sense to copy. 
// The variable myTextObject is of type TXNObject. See the MLTE reference for more information.
//
// 2.	Set up some scrap references, one for the Carbon Event Manager's specific scrap, 
// and another for the current scrap (also known as the Clipboard). 
// As you'll see in the next few lines of code, you need to do some finessing to get the 
// data moved from the application to the service.
//
// 3.	Copy the selected text to the Clipboard by using the MLTE function 
// TXNConvertToPublicScrap to convert the private MLTE scrap content to the Clipboard. 
// This step is needed only for applications that use MLTE. Normally, you'd copy the 
// selected data to the scrap provided by the Carbon Event Manager, but MLTE automatically 
// copies the current selection to its own private scrap. The only way to get the data from 
// the MLTE private scrap is to copy it to the Clipboard using the function 
// TXNConvertToPublicScrap. Once the selection is on the Clipboard, it needs to be copied 
// to the scrap provided by the Carbon Event Manager. 
//
// 4.	Copy the current selection to the MLTE private scrap.
//
// 5.	The Scrap Manager function GetCurrentScrap obtains a scrap reference to the Clipboard. 
// This should now be a reference to the data copied from the text application. 
//
// 6.	Now get a reference to the scrap provided by the Carbon Event Manager. This is the scrap 
// on which you need to put your application data so you can send it to the service. 
//
// 7.	The for loop copies the data from the scrap associated with the application 
// (that is, the Clipboard) to the scrap provided by the Carbon Event Manager. For each 
// data type the application uses, use the function GetScrapFlavorSize to see if the type of 
// the data on the Clipboard matches. If it matches, allocate a buffer and get the data from 
// the Clipboard using the function GetScrapFlavorData. Then use the Scrap Manager function 
// PutScrapFlavor to copy the data to the scrap provided by the Carbon Event Manager.


if (!TXNIsSelectionEmpty (myTextObject))			//1
{
    ScrapRef    currentScrap, specificScrap;		//2
    short       count, index;
    TXNConvertToPublicScrap();						//3
    TXNCopy (myTextObject);							//4
    GetCurrentScrap (currentScrap);					//5
    GetEventParameter (inEvent,         
                        kEventParamScrapRef,
                        typeScrapRef,
                        NULL,
                        sizeof (ScrapRef), 
                        NULL, 
                        &specificScrap);			//6
    count = sizeof (MyAppsDataTypes) / sizeof (OSType);
    for (index = 0; index < count; index++) 		//7
    {
        Size        byteCount;
        OSStatus    err;
        err = GetScrapFlavorSize (currentScrap, 
                        MyAppsDataTypes [index], 
                        &byteCount); 
        if (err == noErr) 
        {
            void*   buffer = malloc (byteCount);
            if (buffer != NULL)
            {
                err = GetScrapFlavorData (currentScrap, 
                            MyAppsDataTypes [index], 
                            &byteCount, 
                            buffer );
                if (err == noErr) 
                {
                    PutScrapFlavor (specificScrap, 
                                MyAppsDataTypes [index],
                                0, 
                                byteCount,
                                buffer);
                }
                free (buffer);
            }
    }
}


// Listing 3-6 Handling a paste event

// Here's what the code does:

// 1.	Set up some scrap references, one for the Carbon Event Manager's specific scrap, 
// and another for the current scrap (also known as the Clipboard).
// 
// 2.	Use the Carbon Event Manager function GetEventParameter to get the scrap that is 
// provided by the Carbon Event Manager. This scrap contains data provided by a service.
// 
// 3.	The Scrap Manager function ClearCurrentScrap clears the Clipboard of its contents. You 
// should call this function immediately when the user requests a Copy or Cut operation. 
// In this case, the Carbon Event Manager is requesting a copy operation.
// 
// 4.	The function GetCurrentScrap gets a reference to the Clipboard.
// 
// 5.	The for statement copies the scrap provided by the Carbon Event Manager to the Clipboard so 
// that it's accessible to the application. The same operations are going on in this loop as in the 
// loop used for the copy event (see "Handling a Copy Event"), except in reverse.
// 
// 6.	The MLTE function TXNConvertFromPublicScrap converts the data on the Clipboard to MLTE's private 
// scrap. This makes the data available for pasting into a text object. 
// 
// 7.	If the scrap on MLTE's private scrap can be pasted, then paste it into the active text object. 


ScrapRef    specificScrap, currentScrap;		//1
CFIndex     index, count;

GetEventParameter (inEvent,         
                    kEventParamScrapRef, 
                    typeScrapRef, 
                    NULL
                    sizeof (ScrapRef), 
                    NULL
                    &specificScrap); 		//2
ClearCurrentScrap ();		//3
GetCurrentScrap (currentScrap);		//4
count = sizeof (MyAppsDataTypes) / sizeof (OSType);
for (index = 0; index < count; index++) 		//5
{
    Size        byteCount;
    OSStatus    err;
    err = GetScrapFlavorSize (specificScrap, 
                    MyAppsDataTypes [index],
                    &byteCount);
    if (err == noErr) 
    {
        void*   buffer = malloc (byteCount);
        if ( buffer != NULL ) 
        {
            err = GetScrapFlavorData (specificScrap, 
                            MyAppsDataTypes [index],
                            &byteCount,
                            buffer );
            if ( err == noErr ) 
            {
                PutScrapFlavor(
                            currentScrap, 
                            MyAppsDataTypes [index], 
                            0, byteCount,
                            buffer); 
            }
            free (buffer);
        }
    }
}
    TXNConvertFromPublicScrap () ;		//6
    if (TXNIsScrapPastable ())		//7
                TXNPaste (myTextObject));
                
// Listing 3-9 Giving data to the service requestor

// Here's what the code does:

// 1.	Declare a string. This is what you'll give to the service requestor.
// 
// 2.	The function ClearScrap is new with Mac OS X version 10.1. Unlike the function 
// ClearCurrentScrap, which can only clear the Clipboard, ClearScrap can clear any
// scrap passed to it. In this case, you need to pass a reference to the scrap 
// provided to your application by the Carbon Event Manager in the parameter kEventParamScrapRef.
// 
// 3.	After you've cleared the scrap, put the string you want to pass to the service 
// requestor on the scrap.

char    string[] = "Buy low, sell high.";		//1
                                    
ClearScrap (&specificScrap);		//2
PutScrapFlavor (specificScrap,
             	'TEXT',
             	0,
             	strlen (string),
             	string);		//3
 

//	Listing 3-10 Modifying data from the service requestor


//	Here's what the code does:

//	1.	Declare variables for error checking and to get the number of bytes on the scrap.
//	
//	2.	Find out the size of the Unicode text data that's on the scrap provided by the 
//	Carbon Event Manager. 
//	
//	3.	Allocate a buffer to accommodate the size of the data.
//	
//	4.	Get the data from the scrap.
//	
//	5.	Replace the first character with an exclamation point.
//	
//	6.	Call the Scrap Manager function ClearScrap to clear the scrap given to your 
//	application by the Carbon Event Manager. Recall that the function ClearScrap can 
//	clear any scrap passed to it, while the function ClearCurrentScrap is limited to 
//	clearing the Clipboard.
//	
//	7.	Put the modified data back on the scrap for the Carbon Event Manager.
//	
//	8.	Free the memory allocated for the buffer.

OSStatus    err;		//1
Size        byteCount;

err = GetScrapFlavorSize (specficScrap,
                    'utxt',
                    &byteCount);		//2
if (err == noErr)
{
    UniChar*    buffer = (UniChar*) malloc (byteCount);		//3
    if (buffer != NULL)
    {
        err = GetScrapFlavorData (specificScrap, 
                            'utxt', 
                            &byteCount,
                            buffer);		//4
        if ( err == noErr && byteCount > 1 )
        {
            buffer[0] = '!';		//5
            ClearScrap (&specficScrap);		//6
            PutScrapFlavor (specificScrap, 
                        'utxt',
                        0,
                        byteCount, 
                        buffer);		//7
            result = noErr;
        }
        free (buffer);		//8
    }
}

Copy =

	PutScrapFlavor
	
Paste =
	
	GetScrapFlavorCount
	
	for (i up to count)
	
		GetScrapFlavorSize
		
		GetScrapFlavorData

#endif
