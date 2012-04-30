/***********************************************************************

 Copyright 1994 Cindy Grande
 
      Grande Software, Inc.
      19004 37th Avenue South
      Seattle, Washington  98188
      (206) 439-9828


       NPIFStructs.h
       Structures for use in the NoteScan Notation Program Interface File
       
************************************************************************/

/* 9/18/93: changed npifFILE record, to make xResolution & yResolution long integers
   11/1/93: added version number to npifFILE record.
         added tied switch to note record (to be set on second note of tie)
   12/20/93: added visibleStaves to System Header record. 
    1/17/93: added maxStavesPerSystem to FILE and PAGE record. 
    2/8/94:  added pageWidth and pageDepth to npifPAGE record.
             npifSTFF record horizontalPlacement and topRow are now relative
             to the boundary of the music, rather than the boundary of the TIFF file. 
*/

/*
       Position of first two fields: recordType and recordLength
       	must not change.
       Data Structs for:
       	NightScanner v. .3
*/


//#if TARGET_CPU_PPC
#pragma options align=mac68k
//#endif

typedef struct npifFILE {  				/*  NPIF File Header */

   char  recordType[4];
   short recordLength;
   long  fileLength;   						/* total number of bytes in npif file */
   long XResolution;
   long YResolution;
   short numberOfPages;
   char version[4];
   short maxStavesPerSystem;
   
} npifFILE,*npifFILEPtr;
   
typedef struct npifPAGE {  				/*  NPIF Page Header */

   char  recordType [4];
   short recordLength;
   long  pageLength;   						/* total number of bytes in page */
   short numberOfSystems;
   short maxStavesPerSystem;
   short pageWidth;							/* width of page, in pixels.
														this is the width from the leftmost edge of the staff
														which extends farthest to the left,
														to the rightmost edge of the staff which extends farthest
														to the right. */
   short pageDepth;							/* depth of page, in pixels, from top of first staff to
														bottom of last staff.          */
   
} npifPAGE,*npifPAGEPtr;
   
typedef struct npifSYST {  				/*  NPIF System Header */

   char  recordType [4];
   short recordLength;
   long  systemLength;   					/* total number of bytes in system */
   short numberOfStaves;
   short visibleStaves;        
   
} npifSYST,*npifSYSTPtr;

   
typedef struct npifSTFF {    				/* NPIF Staff Header record */

   char recordType [4];
   short recordLength;
   long staffLength;   						/*  number of bytes in npif file for this staff */
   short horizontalPlacement;
   short width;
   short topRow;
   short numberOfStaffLines;  
   short bar1Visible; 
   short spacePixels;
   
} npifSTFF,*npifSTFFPtr;
   
typedef struct npifCLEF {    				/* NPIF Clef record */

   char recordType [4];
   short recordLength;
   short clef;
   short horizontalPlacement;
   short width;
   
} npifCLEF,*npifCLEFPtr;

   
typedef struct npifKYSG {    				/* NPIF Key Signature record */

   char recordType [4];
   short recordLength;
   short key;            /* same as MIDI file key signature representation:
                            0 = no sharps/flats, 1 = 1 sharp, 2 = 2 sharps, etc.
                            8 = 1 flat, 9 = 2 flats, etc.  */
   short horizontalPlacement;
   short width;
   
} npifKYSG,*npifKYSGPtr;

typedef struct npifTMSG {    				/* NPIF Time Signature record */

   char recordType [4];
   short recordLength;
   short visible;
   short topNumber;
   short bottomNumber;
   short specialSign;
   short horizontalPlacement;
   short width;
} npifTMSG,*npifTMSGPtr;

typedef struct stemStruct {
       char direction;            /* 1 = down, 2 = up  */
       char termScalePosition;
       char termOffset;
       char beamOrFlag;           /* 1 = flag, 2 = beam */
        
} stemStruct, *stemStructPtr;
   
   
typedef struct npifNOTE {    				/* NPIF Note record */

   char  recordType [4];
   short  recordLength;
   short  horizontalPlacement;
   char verticalScalePos;					/* 0=middle line of staff, 1=space above, etc. */
   char timeValue;
   char numberOfDots;
   char accidental;
      
   struct stemStruct stem;         
   char beamsToLeft;
   char beamsToRight;
   char tied;
   char notUsed;
   
} npifNOTE,*npifNOTEPtr;

   
typedef struct beamTerm {        		/*   declaration of struct beamTerm:
                              					start or end of one beam line */
   short  noteHPlacement;
   char noteVScalePos;
   char stemTermScalePos;
   char stemTermOffset;
   char stemDirection;
} beamTerm, *beamTermPtr;

typedef struct startEnd {        		/*  declaration of struct startEnd:
                             						start and end of one beam line   */
    char   partial;
    char   partialDirection;   /* 1 = left, 2 = right */
    struct beamTerm start;
    struct beamTerm end;
} startEnd, *startEndPtr; 


typedef struct npifBEAM {    				/*   complete beam structure */

   char recordType [4];
   short recordLength;
   short numberOfParts;   
   struct startEnd beamLine [16]; 
   
} npifBEAM,*npifBEAMPtr;


struct npifSLUR {    						/*   slur/tie structure */

   char recordType [4];
   short recordLength;
   short startHPlacement;  
   short startVScalePos;
   short endHPlacement;  
   short endVScalePos;
   short apexHPlacement;
   short apexVScalePos;
   
};


typedef struct npifREST   {    			/*  NPIF Rest Record  */   
   char recordType [4];
   short recordLength;
   short  horizontalPlacement;
   char verticalScalePos;
   char timeValue;
   char numberOfDots;
   char filler;
   
} npifREST,*npifRESTPtr;


typedef struct npifBAR   {    			/*  NPIF Bar Record  */   
   char recordType [4];
   short recordLength;
   short barStart;
   
} npifBAR,*npifBARPtr;

   
typedef struct npifEOF {    				/* NPIF End of File record */

   char recordType [4];
   short recordLength;
   
} npifEOF,*npifEOFPtr;


struct beamInfoStruct {
       short beamPartsActive;
       short nextPart;
       struct npifBEAM npifBeam;
       struct npifNOTE latestBeamNote;
       short latestBeamRight;
       long  latestNotePtr;
       };


/* Obselete: from NightScanner v. .1 */
typedef struct npifOTHER {
	 char recordType[4];
	 short recordLength;
} npifOther,*npifOtherPtr;


//#if TARGET_CPU_PPC
#pragma options align=reset
//#endif
