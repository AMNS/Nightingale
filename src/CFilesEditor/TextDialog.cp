/***************************************************************************
*	FILE:	TextDialog.c													*
*	PROJ:	Nightingale, rev. for v.3.5										*
*	DESC:	Routines to handle text editing and Define Text Style dialogs	*
/***************************************************************************/

/*								NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* Symbolic Dialog Item Numbers */

static enum {			/* Items for text editing and many for Define Text Style dialogs */
	BUT1_OK = 1,
	BUT2_Cancel,
	STXT3_Text,
	POP4_StyleChoice,
	STXT5,
	STXT6_Name,
	POP7_Name,
	STXT8,
	RAD9_Absolute,
	STXT10_Absolute,	/* Can't use radio button for label bcs is also popup label */
	POP11_Absolute,
	EDIT12_Points,
	STXT13_pts,
	RAD14_Relative,
	STXT15_Relative,	/* Can't use radio button for label bcs is also popup label */
	POP16_Relative,
	CHK17_Plain,
	CHK18_Bold,
	CHK19_Italic,
	CHK20_Underline,
	CHK21_Outline,
	CHK22_Shadow,
	STXT23_Style,
	STXT24_Text,
	EDIT25_Text,
	USER26,
	USER27,
	USER28,
	USER29,
	USER30,
	CHK31_Lyric,
	BUT32_InsertChar
	} E_TextDlogItems;
	
static enum {			/* Different items for Define Text Style dialog */
	CHK23_Treat=23,
	STXT24_Enclosure,
	RAD25_None,
	RAD26_Box,
	STXT27_Sample,
	EDIT28_Text,
	USER31=31,
	USER32
	} E_DefineStyleItems;
	
static enum {
	TextDlog,
	DefineStyleDlog
	} E_TextDlogs;
	
static enum {			/* In the order of the Define Style Choice pop-up (ID 36) */
	NoSTYLE = 0,
	Regular1STYLE,
	Regular2STYLE,
	Regular3STYLE,
	Regular4STYLE,
	Regular5STYLE,
	Regular6STYLE,
	Regular7STYLE,
	Regular8STYLE,
	Regular9STYLE,
	PartNameSTYLE,
	MeasureNumSTYLE,
	RehearsalMarkSTYLE,
	TempoMarkSTYLE,
	ChordSymbolSTYLE,
	PageNumSTYLE,
	THISITEMONLY_STYLE
	} E_TextStyles;

static UserPopUp popup4;		/* For style choice menu */
static UserPopUp popup7;		/* For font name menu */
static UserPopUp popup11;		/* For font sizes and real sizes menu */
static UserPopUp popup16;		/* For staff-relative sizes menu */
static Boolean popupAns;		/* New choice has been made in popup itemHit event */

/* Global current values of what we're editing, so that we can draw the
text as it looks from within the Filter during an update event. */

static short theFont;
static short theLyric;
static short theEncl;
static short theSize;				/* Only of interest when isRelative is FALSE */
static short theStyle;			/* plain, bold, etc. (as opposed to "stylechoice") */
static Boolean isRelative;
static short theRelIndex;			/* Only of interest when isRelative is TRUE */
static short thePtSize;			/* The actual point size for either relative or absolute */
static DDIST theLineSpacing;	/* Between staff lines for this given context */
static short radioChoice;
static short theDlog;				/* Which dialog (text or Define Text Styles) being used */

static TEXTSTYLE	theRegular1, theRegular2, theRegular3, theRegular4, theRegular5,
						theRegular6, theRegular7, theRegular8, theRegular9, theMeasNum,
						thePartName, theRehearsalMark, theTempoMark, theChordSymbol,
						thePageNumber, theCurrent;
static Boolean	theRegular1Changed, theRegular2Changed, theRegular3Changed, theRegular4Changed,
					theRegular5Changed, theRegular6Changed, theRegular7Changed, theRegular8Changed,
					theRegular9Changed, theRehearsalMarkChanged, theChordSymbolChanged;

static Rect fontRect,sizeRect,faceRect,styleRect,dimRect;		/* Panel frames */


/* Prototypes for local routines */

static void DimStylePanels(DialogPtr dlog, Boolean dim);
static pascal  Boolean MyFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);

static void	SetFontPopUp(unsigned char *fontName, unsigned char *strbuf);
static void	SetAbsSizePopUp(short size, unsigned char *strbuf);
static void	SetStyleBoxes(DialogPtr dlog, short style);
static void	SetStylePopUp(short styleIndex);
static short	GetStyleChoice(void);
static void	SaveCurrentStyle(short currStyle);
static void	SetCurrentStyle(short currStyle);
static void	TSSetCurrentStyle(short currStyle);
static short	TDRelIndexToSize(short index);
static short	GetStrFontStyle(Document *doc, short styleChoice);
static void	UpdateDocStyles(Document *doc);
static Boolean ApplyDocStyle(Document *doc, LINK pL, TEXTSTYLE *style);
static void	GetRealSizes(void);
static void	TuneRadioIn(DialogPtr dlog,short itemHit, short *radio);
static void	DrawExampleText(DialogPtr dlog, unsigned char *string);
static void	InstallTextStyle(DialogPtr dlog, TEXTSTYLE *aStyle);

/* Given the current state of the popups, determine from the font menu popup's
current choice which sizes in the sizes menu are "real" ones and should
therefore be outlined.  Also defines the font number of the current font. */

static void GetRealSizes()
	{
		short i,nitems; unsigned char str[64]; long num;
		
		if (popup7.currentChoice) {
			/* Pull non-truncated menu item back into popup str storage */
			GetMenuItemText(popup7.menu,popup7.currentChoice,popup7.str);
			/* Convert to font number */
			GetFNum(popup7.str,&theFont);
			/* Set item style for each item in size popup's menu according to real font */
			nitems = CountMenuItems(popup11.menu);
			for (i=1; i<=nitems; i++) {
				GetMenuItemText(popup11.menu,i,str);
				StringToNum(str,&num);
				SetItemStyle(popup11.menu, i, RealFont(theFont,(short)num) ? outline : 0);
				}
			/* Restore font name popup string to truncated version, if any */
			TruncPopUpString(&popup7);
			}
	}


/* Given a font name and a temporary string buffer to place menu item strings,
look up the name in the popup font menu, and set the popup to display it,
if it exists. */

static void SetFontPopUp(unsigned char *fontName, unsigned char *strbuf)
	{
		short nitems,i;
		
		nitems = CountMenuItems(popup7.menu);
		for (i=1; i<=nitems; i++) {
			GetMenuItemText(popup7.menu,i,strbuf);
			if (EqualString(strbuf,fontName,FALSE,TRUE)) {
				ChangePopUpChoice(&popup7,i);
				return;
				}
			}
		ChangePopUpChoice(&popup7,0);	/* Wasn't found */
	}


/* Given a point size and a temporary string buffer to place menu items into,
look up the size in the size popup menu, and set the popup choice to display it,
if it exists. */

static void SetAbsSizePopUp(short size, unsigned char *strbuf)
	{
		short nitems,i; long num;
		
		nitems = CountMenuItems(popup11.menu);
		for (i=1; i<=nitems; i++) {
			GetMenuItemText(popup11.menu,i,strbuf);
			StringToNum(strbuf,&num);
			if (num == size) {
				ChangePopUpChoice(&popup11,i);
				return;
				}
			}
		ChangePopUpChoice(&popup11,0);	/* Wasn't found */
	}


/* Given a global style choice, set the "global style choice" popup to display it. */

static void SetStylePopUp(short styleIndex)
	{
		ChangePopUpChoice(&popup4,styleIndex);
	}


/* Return the current choice in the "global style choice" popup */

static short GetStyleChoice()
	{
		if (popup4.currentChoice) {
			/* Pull non-truncated menu item back into popup str storage */
			GetMenuItemText(popup4.menu,popup4.currentChoice,popup4.str);
			}
		return popup4.currentChoice;
	}


/* Copy the TEXTSTYLE <theCurrent> into the appropriate TEXTSTYLE (<theRegular1>,
<theRegular2>, etc.).  Handles all styles. */

static void SaveCurrentStyle(short currStyle)
	{
	 	switch (currStyle) {
			case Regular1STYLE:
				theRegular1 = theCurrent;
	 			break;
			case Regular2STYLE:
				theRegular2 = theCurrent;
	 			break;
			case Regular3STYLE:
				theRegular3 = theCurrent;
	 			break;
			case Regular4STYLE:
				theRegular4 = theCurrent;
	 			break;
			case Regular5STYLE:
				theRegular5 = theCurrent;
	 			break;
			case Regular6STYLE:
				theRegular6 = theCurrent;
	 			break;
			case Regular7STYLE:
				theRegular7 = theCurrent;
	 			break;
			case Regular8STYLE:
				theRegular8 = theCurrent;
	 			break;
			case Regular9STYLE:
				theRegular9 = theCurrent;
	 			break;
			case PartNameSTYLE:
				thePartName = theCurrent;
	 			break;
			case MeasureNumSTYLE:
				theMeasNum = theCurrent;
	 			break;
			case RehearsalMarkSTYLE:
				theRehearsalMark = theCurrent;
	 			break;
			case TempoMarkSTYLE:
				theTempoMark = theCurrent;
	 			break;
	 		case ChordSymbolSTYLE:
	 			theChordSymbol = theCurrent;
	 			break;
	 		case PageNumSTYLE:
	 			thePageNumber = theCurrent;
	 			break;
	 		case THISITEMONLY_STYLE:
	 			break;
	 		}
	}
	

/* Make the TEXTSTYLE <theCurrent> a copy of the specified style; handles all styles. */

static void SetCurrentStyle(short currStyle)
	{
	 	switch (currStyle) {
			case Regular1STYLE:
				theCurrent = theRegular1;
	 			break;
			case Regular2STYLE:
				theCurrent = theRegular2;
	 			break;
			case Regular3STYLE:
				theCurrent = theRegular3;
	 			break;
			case Regular4STYLE:
				theCurrent = theRegular4;
	 			break;
			case Regular5STYLE:
				theCurrent = theRegular5;
	 			break;
			case Regular6STYLE:
				theCurrent = theRegular6;
	 			break;
			case Regular7STYLE:
				theCurrent = theRegular7;
	 			break;
			case Regular8STYLE:
				theCurrent = theRegular8;
	 			break;
			case Regular9STYLE:
				theCurrent = theRegular9;
	 			break;
			case PartNameSTYLE:
				theCurrent = thePartName;
	 			break;
			case MeasureNumSTYLE:
				theCurrent = theMeasNum;
	 			break;
			case RehearsalMarkSTYLE:
				theCurrent = theRehearsalMark;
	 			break;
			case TempoMarkSTYLE:
				theCurrent = theTempoMark;
	 			break;
	 		case ChordSymbolSTYLE:
	 			theCurrent = theChordSymbol;
	 			break;
	 		case PageNumSTYLE:
	 			theCurrent = thePageNumber;
	 			break;
	 		case THISITEMONLY_STYLE:
	 			break;
	 		}
	}
	

/* Make the TEXTSTYLE <theCurrent> a copy of the specified style; handles only
the "regular" text styles, not those for measure numbers, tempo marks, etc. */

static void TSSetCurrentStyle(short currStyle)
	{
	 	switch (currStyle) {
			case TSRegular1STYLE:
				theCurrent = theRegular1;
	 			break;
			case TSRegular2STYLE:
				theCurrent = theRegular2;
	 			break;
			case TSRegular3STYLE:
				theCurrent = theRegular3;
	 			break;
			case TSRegular4STYLE:
				theCurrent = theRegular4;
	 			break;
			case TSRegular5STYLE:
				theCurrent = theRegular5;
	 			break;
			case TSRegular6STYLE:
				theCurrent = theRegular6;
	 			break;
			case TSRegular7STYLE:
				theCurrent = theRegular7;
	 			break;
			case TSRegular8STYLE:
				theCurrent = theRegular8;
	 			break;
			case TSRegular9STYLE:
				theCurrent = theRegular9;
	 			break;
	 		case TSThisItemOnlySTYLE:
	 			break;
	 		}
	}


/* Install a given style into dialog items */

static void SetStyleBoxes(DialogPtr dlog, short style)
	{
		PutDlgChkRadio(dlog,CHK17_Plain,style == 0);
		PutDlgChkRadio(dlog,CHK18_Bold,(style & bold)!=0);
		PutDlgChkRadio(dlog,CHK19_Italic,(style & italic)!=0);
		PutDlgChkRadio(dlog,CHK20_Underline,(style & underline)!=0);
		PutDlgChkRadio(dlog,CHK21_Outline,(style & outline)!=0);
		PutDlgChkRadio(dlog,CHK22_Shadow,(style & shadow)!=0);
	}


/* Maintain state of items subordinate to the main style choice popup.  If style is
not "This Item Only", disable the popups, check boxes, and radio buttons, hide
the EditText field, and dim everything.  Otherwise, do the opposite. */
		
#define HILITE_DITEM(itm, active) 	{	GetDialogItem(dlog, (itm), &type, &hndl, &box);		\
										HiliteControl((ControlHandle)hndl, (active)); }

static void DimStylePanels(DialogPtr dlog, Boolean dim)
{
	short		type, newType, ctlActive;
	Handle		hndl;
	Rect		box;
	
	newType = (dim? statText+itemDisable : statText);
	
	GetDialogItem(dlog,STXT10_Absolute,&type,&hndl,&box);
	SetDialogItem(dlog,STXT10_Absolute,newType,hndl,&box);
	GetDialogItem(dlog,STXT15_Relative,&type,&hndl,&box);
	SetDialogItem(dlog,STXT15_Relative,newType,hndl,&box);

	newType = (dim? statText+itemDisable : editText);

	GetDialogItem(dlog,EDIT12_Points,&type,&hndl,&box);
	SetDialogItem(dlog,EDIT12_Points,newType,hndl,&box);

	ctlActive = (dim? CTL_INACTIVE : CTL_ACTIVE);

	HILITE_DITEM(RAD9_Absolute, ctlActive);
	HILITE_DITEM(RAD14_Relative, ctlActive);

	HILITE_DITEM(CHK17_Plain, ctlActive);
	HILITE_DITEM(CHK18_Bold, ctlActive);
	HILITE_DITEM(CHK19_Italic, ctlActive);
	HILITE_DITEM(CHK20_Underline, ctlActive);
	HILITE_DITEM(CHK21_Outline, ctlActive);
	HILITE_DITEM(CHK22_Shadow, ctlActive);

	if (dim) {
		PenPat(NGetQDGlobalsGray());	
		PenMode(patBic);	
		PaintRect(&dimRect);						/* Dim everything within style panels */
		PenNormal();
	}
}


static void DrawMyItems(DialogPtr);
static void DrawMyItems(DialogPtr /*dlog*/)
{
	/* Outline the panels in background */
	FrameRect(&fontRect);
	FrameRect(&sizeRect);
	FrameRect(&faceRect);
	if (theDlog==TextDlog)
		FrameRect(&styleRect);

	/* Fill in the popup items */
	DrawPopUp(&popup4);
	DrawPopUp(&popup7);
	DrawPopUp(&popup11);
	DrawPopUp(&popup16);
}


/* Modal dialog filter has to consider disabling the "style panels" of the text
insertion dialog and look for popup items and convert to itemHits, as well
as the usual cutting/pasting and default button stuff. */

static pascal Boolean MyFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
	{
		Boolean ans=FALSE,doHilite=FALSE; WindowPtr w;
		short type,ch, currentStyle; Handle hndl;
		Rect box; Point where; unsigned char str[256];
		
		w = (WindowPtr)(evt->message);
		switch(evt->what) {
			case updateEvt:
				if (w == GetDialogWindow(dlog)) {
					BeginUpdate(GetDialogWindow(dlog));
					/* Draw the standard dialog items */
					currentStyle = GetStyleChoice();
					DrawMyItems(dlog);
					UpdateDialogVisRgn(dlog);
					/* Perhaps gray out everything */
					if (theDlog==TextDlog)
						DimStylePanels(dlog, (currentStyle!=TSThisItemOnlySTYLE));
					OutlineOKButton(dlog,TRUE);
					DrawExampleText(dlog,NULL);
					EndUpdate(GetDialogWindow(dlog));
					ans = TRUE;
					*itemHit = 0;
					}
#ifdef TRULY_ANNOYING_FOR_LARGE_SCORES
				/*
				 * In case the ChooseChar dlog has left holes in score. Unfortunately,
				 * can be slow enough to be very annoying.
				 */
				else if (IsDocumentKind(w) || IsPaletteKind(w)) {
					DoUpdate(w);
					ArrowCursor();
					}
				break;
#endif
			case activateEvt:
				if (w == GetDialogWindow(dlog)) {
					short activ = (evt->modifiers & activeFlag)!=0;
					}
				break;
			case mouseDown:
			case mouseUp:
				where = evt->where;
				GlobalToLocal(&where);
				popupAns = FALSE;
				
				if (PtInRect(where,&popup4.shadow)) {
					popupAns = ans = DoUserPopUp(&popup4);
					*itemHit = ans ? POP4_StyleChoice : 0;
					}
				else {
					/* If TextDlog and using a predefined style, ignore other popups */
					currentStyle = GetStyleChoice();
					if (theDlog!=TextDlog || currentStyle==TSThisItemOnlySTYLE) {
						 if (PtInRect(where,&popup7.shadow)) {
							popupAns = ans = DoUserPopUp(&popup7);
							*itemHit = ans ? POP7_Name : 0;
							}
						 else if (PtInRect(where,&popup11.shadow)) {
							popupAns = ans = DoUserPopUp(&popup11);
							*itemHit = ans ? POP11_Absolute : 0;
							}
						 else if (PtInRect(where,&popup16.shadow)) {
							popupAns = ans = DoUserPopUp(&popup16);
							*itemHit = ans ? POP16_Relative : 0;
							}
						}
					}
				break;
			case keyDown:
				ch = (unsigned char)evt->message;
				if (evt->modifiers & cmdKey) {
					switch (ch) {
						case 'x':
						case 'X':
							DialogCut(dlog);
							break;
						case 'c':
						case 'C':
							DialogCopy(dlog);
							break;
						case 'v':
						case 'V':
							DialogPaste(dlog);
							break;
						case '\'':
							if (evt->modifiers & shiftKey) {
								ch = '"';
								evt->message &= ~charCodeMask;
								evt->message |= (ch & charCodeMask);
								}
							break;
						case '.':
							FlashButton(dlog, Cancel);
							*itemHit = Cancel;
							return TRUE;
						}
					GetDlgString(dlog,EDIT25_Text,str);
					GetDialogItem(dlog,BUT1_OK,&type,&hndl,&box);
					HiliteControl((ControlHandle)hndl, (*str==0 ? CTL_INACTIVE : CTL_ACTIVE));
					/*
					 * Convert command-quote/-double-quote to std ASCII quotes so
					 * user can bypass "smart" quotes; ignore all other cmd-chars.
					 */
					ans = (ch!='"' && ch!='\'');
					}
				/* Let Enter key confirm dlog. Let Return key insert a CR in string  
					for Text dlog, and confirm Define Style dlog. */
				else if (ch==CH_ENTER || (theDlog==DefineStyleDlog && ch==CH_CR)) {
					GetDlgString(dlog,EDIT25_Text,str);
					if (*str != 0) {
						GetDialogItem(dlog,BUT1_OK,&type,&hndl,&box);
						*itemHit = BUT1_OK;
						doHilite = TRUE;
						}
					ans = TRUE;
					}
				 else {
				 	/* Do smart-quote conversion on single and double quotes */
				 	TEHandle textH = GetDialogTextEditHandle(dlog);
					ch = ConvertQuote(textH,ch);
					evt->message &= ~charCodeMask;
					evt->message |= (ch & charCodeMask);
					ans = FALSE;
					}
				break;
			}
		if (doHilite) {
			GetDialogItem(dlog,*itemHit,&type,&hndl,&box);
			/* if (type == (btnCtrl+ctrlItem)) */ HiliteControl((ControlHandle)hndl,1);
			}
		return(ans);
	}


/* Deliver the absolute size that the Tiny...Jumbo...StaffHeight menu item represents,
or 0 if out of bounds */

static short TDRelIndexToSize(short index)
	{
		return(RelIndexToSize(index,theLineSpacing));
	}


/* Install all dialog items according to the given style, font size, etc., by
setting our global variables and setting the dialog's controls accordingly. */

static void InstallTextStyle(DialogPtr dlog, TEXTSTYLE *aStyle)
	{
		unsigned char str[256]; short tmpSize, i;

		SetFontPopUp((unsigned char *)aStyle->fontName,str);
		
		GetRealSizes();
		isRelative = aStyle->relFSize;
		tmpSize = aStyle->fontSize;
		if (isRelative) thePtSize = TDRelIndexToSize(theRelIndex = tmpSize);
		 else 			thePtSize = tmpSize;

		theSize = thePtSize;
		TuneRadioIn(dlog,isRelative?RAD14_Relative:RAD9_Absolute,&radioChoice);
		if (isRelative) {
			PutDlgString(dlog,EDIT12_Points,"\p",FALSE);
			ChangePopUpChoice(&popup16,theRelIndex);
			}
		 else {
			PutDlgWord(dlog,EDIT12_Points,theSize,TRUE);
			SetAbsSizePopUp(theSize,str);
			}
			
		theStyle = aStyle->fontStyle;
		SetStyleBoxes(dlog,theStyle);
		
		theEncl = aStyle->enclosure;
		if (theDlog==DefineStyleDlog)
			for (i=RAD25_None; i<=RAD26_Box; i++)
				PutDlgChkRadio(dlog, i, i==(RAD25_None+theEncl));
		
		theLyric = aStyle->lyric;
		if (theDlog==DefineStyleDlog)
			PutDlgChkRadio(dlog, CHK23_Treat, theLyric);
		else
			PutDlgChkRadio(dlog, CHK31_Lyric, theLyric);
	}

/* Determine which font style the string is in, and copy the relevant
parameters into the global TEXTSTYLE record theCurrent. */

static short GetStrFontStyle(Document *doc, short styleChoice)
{
	short globalFontIndex;

	globalFontIndex = User2HeaderFontNum(doc, styleChoice);

	switch (globalFontIndex) {
		case FONT_R1:
			BlockMove(doc->fontName1,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular1STYLE;
		case FONT_R2:
			BlockMove(doc->fontName2,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular2STYLE;
		case FONT_R3:
			BlockMove(doc->fontName3,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular3STYLE;
		case FONT_R4:
			BlockMove(doc->fontName4,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular4STYLE;
		case FONT_R5:
			BlockMove(doc->fontName5,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular5STYLE;
		case FONT_R6:
			BlockMove(doc->fontName6,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular6STYLE;
		case FONT_R7:
			BlockMove(doc->fontName7,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular7STYLE;
		case FONT_R8:
			BlockMove(doc->fontName8,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular8STYLE;
		case FONT_R9:
			BlockMove(doc->fontName9,&theCurrent,sizeof(TEXTSTYLE));
			return TSRegular9STYLE;
		case FONT_THISITEMONLY:
			return TSThisItemOnlySTYLE;			/* Don't have a TEXTSTYLE record to use */
			
	}

	return -1;
}


/* Update the TEXTSTYLE records in the document header for all the dlog styles,
	and set changed flags for all the styles that need them. */

static void UpdateDocStyles(Document *doc)
{
	if (BlockCompare(&theRegular1, doc->fontName1, sizeof(TEXTSTYLE)))
		theRegular1Changed = TRUE;
	if (BlockCompare(&theRegular2, doc->fontName2, sizeof(TEXTSTYLE)))
		theRegular2Changed = TRUE;
	if (BlockCompare(&theRegular3, doc->fontName3, sizeof(TEXTSTYLE)))
		theRegular3Changed = TRUE;
	if (BlockCompare(&theRegular4, doc->fontName4, sizeof(TEXTSTYLE)))
		theRegular4Changed = TRUE;
	if (BlockCompare(&theRegular5, doc->fontName5, sizeof(TEXTSTYLE)))
		theRegular5Changed = TRUE;
	if (BlockCompare(&theRegular6, doc->fontName6, sizeof(TEXTSTYLE)))
		theRegular6Changed = TRUE;
	if (BlockCompare(&theRegular7, doc->fontName7, sizeof(TEXTSTYLE)))
		theRegular7Changed = TRUE;
	if (BlockCompare(&theRegular8, doc->fontName8, sizeof(TEXTSTYLE)))
		theRegular8Changed = TRUE;
	if (BlockCompare(&theRegular9, doc->fontName9, sizeof(TEXTSTYLE)))
		theRegular9Changed = TRUE;
	if (BlockCompare(&theRehearsalMark, doc->fontNameRM, sizeof(TEXTSTYLE)))
		theRehearsalMarkChanged = TRUE;
	if (BlockCompare(&theChordSymbol, doc->fontNameCS, sizeof(TEXTSTYLE)))
		theChordSymbolChanged = TRUE;

	BlockMove(&theRegular1, doc->fontName1, sizeof(TEXTSTYLE));
	BlockMove(&theRegular2, doc->fontName2, sizeof(TEXTSTYLE));
	BlockMove(&theRegular3, doc->fontName3, sizeof(TEXTSTYLE));
	BlockMove(&theRegular4, doc->fontName4, sizeof(TEXTSTYLE));
	BlockMove(&theRegular5, doc->fontName5, sizeof(TEXTSTYLE));
	BlockMove(&theRegular6, doc->fontName6, sizeof(TEXTSTYLE));
	BlockMove(&theRegular7, doc->fontName7, sizeof(TEXTSTYLE));
	BlockMove(&theRegular8, doc->fontName8, sizeof(TEXTSTYLE));
	BlockMove(&theRegular9, doc->fontName9, sizeof(TEXTSTYLE));
	BlockMove(&thePartName, doc->fontNamePN, sizeof(TEXTSTYLE));
	BlockMove(&theMeasNum, doc->fontNameMN, sizeof(TEXTSTYLE));
	BlockMove(&theRehearsalMark, doc->fontNameRM, sizeof(TEXTSTYLE));
	BlockMove(&theTempoMark, doc->fontNameTM, sizeof(TEXTSTYLE));
	BlockMove(&theChordSymbol, doc->fontNameCS, sizeof(TEXTSTYLE));
	BlockMove(&thePageNumber, doc->fontNamePG, sizeof(TEXTSTYLE));
}


/* Apply the given text style to <pL>, which must be a graphic of an 
appropriate type for the style. Return TRUE if ok, FALSE if no more
room for fonts in the document's font index table. */

static Boolean ApplyDocStyle(Document *doc, LINK pL, TEXTSTYLE *style)
{
	short		newFontIndex;
	PGRAPHIC	pGraphic;
	
	/* Get the new font's index, adding it to the table if necessary.
		But if the table overflows, give up. */
	newFontIndex = GetFontIndex(doc, style->fontName);
	if (newFontIndex < 0)
		return FALSE;

	/* Consider changing GRLyrics to GRStrings and vice-versa, based
		on the lyric flag for the style. */
	if (GraphicSubType(pL)==GRLyric || GraphicSubType(pL)==GRString)
		GraphicSubType(pL) = (style->lyric? GRLyric : GRString);

	pGraphic = GetPGRAPHIC(pL);
	pGraphic->relFSize = style->relFSize;
	pGraphic->fontSize = style->fontSize;
	pGraphic->fontStyle = style->fontStyle;
	pGraphic->enclosure = style->enclosure;
	pGraphic->fontInd = newFontIndex;

	return TRUE;
}


/* Change the current radio button item <*radio> to <itemHit>, and reset all that
depends on it. NB: <*radio> is both input and output! */

static void TuneRadioIn(DialogPtr dlog, short itemHit, short *radio)
{
	PutDlgChkRadio(dlog,*radio,FALSE);
	*radio = itemHit;
	PutDlgChkRadio(dlog,*radio,TRUE);

	if (itemHit == RAD9_Absolute) {
		ChangePopUpChoice(&popup16,0);
		isRelative = FALSE;
	}
	 else {
		ChangePopUpChoice(&popup11,0);
		isRelative = TRUE;
	}
}


/* Draw the current string from the edittext item into the example area, using
given current font, size, and style.  If string is NULL, then look it up;
otherwise it is the string to draw. */

static void DrawExampleText(DialogPtr dlog, unsigned char *string)
	{
		unsigned char str[256],*pStr;
		short oldFont,oldSize,oldStyle,type,y,userItem,editText,i,tmpLen,oldLen;
		Rect box; Handle hndl; RgnHandle oldClip;
		
		if (theDlog==TextDlog)
			 { userItem = USER26; editText = EDIT25_Text; }
		else { userItem = USER29; editText = EDIT28_Text; }

		GetDialogItem(dlog, userItem, &type, &hndl, &box);
		EraseRect(&box);
		
		if (thePtSize > 0) {					/* Reality check */
		
			if (string) pStr = string;
			 else	  { pStr = str; GetDlgString(dlog,editText,str); }
			
			tmpLen = oldLen = pStr[0];
			for (i = 1; i <= pStr[0]; i++)
				if (pStr[i] == CH_CR) {
					tmpLen = i;					/* It's a multi-line string. */
					break;
				}

			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
	
			/* Clip to box as well, in case of too large pt size */
			
			oldClip = NewRgn();
			if (oldClip) {
				GetClip(oldClip);
				ClipRect(&box);
				}
			
			TextFont(theFont); TextSize(thePtSize); TextFace(theStyle);
			
			y = box.bottom - box.top;
			MoveTo(box.left, (y/4) + (box.top+box.bottom)/2);
			pStr[0] = tmpLen;
			DrawString(pStr);
			pStr[0] = oldLen;
			
			if (oldClip) {
				SetClip(oldClip);
				DisposeRgn(oldClip);
				}
			
			TextFont(oldFont); TextSize(oldSize); TextFace(oldStyle);
			}
	}


/* TextDialog takes a global style choice index; the name and attributes of an
initial font; and the current text string to display, or the empty string if a
new string is wanted. If user okays the dialog, TextDialog returns TRUE, and all
these values are set according to how the user has set them.  Otherwise, they
are returned unchanged. */

Boolean TextDialog(
			Document *doc,
			short *styleChoice,		/* Item index into the Define Style Choice pop-up (ID 36) */
			Boolean *relFSize,		/* TRUE means size=1...9 for Tiny...StaffHeight */
			short *size,			/* If *relFSize, Tiny...StaffHeight index, else in points */
			short *style,			/* Standard style bits */
			short *enclosure,		/* Enclosure code */
			Boolean *lyric,			/* TRUE=lyric, FALSE=other */
			unsigned char *name,	/* Fontname or empty */
			unsigned char *string,	/* Current text string or empty */
			CONTEXT *context
			)
{
	short tmpSize,itemHit,type;
	Boolean okay=FALSE,keepGoing=TRUE;
	short val,i,currentStyle;
	Handle hndl; Rect box; long num;
	DialogPtr dlog; GrafPtr oldPort;
	unsigned char str[256];
	char fmtStr[256];
	ModalFilterUPP filterUPP;
	
	theDlog = TextDlog;

	/* First load the eight style records from the score header */
	BlockMove(doc->fontName1,&theRegular1,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName2,&theRegular2,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName3,&theRegular3,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName4,&theRegular4,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName5,&theRegular5,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName6,&theRegular6,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName7,&theRegular7,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName8,&theRegular8,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName9,&theRegular9,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNameMN,&theMeasNum,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNamePN,&thePartName,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNameRM,&theRehearsalMark,sizeof(TEXTSTYLE));
	
	/*
	 *	If string is empty, then we are creating a new graphic text object, so
	 *	take our text characteristics from the last one created.  If string is
	 *	non-empty, other arguments are presumed defined.
	 */
	
	if (*string) {
		/* Determine in which font's style this string is, if any */
		currentStyle = GetStrFontStyle(doc, *styleChoice);
		if (currentStyle==TSThisItemOnlySTYLE) {
			PStrCopy((StringPtr)name, (StringPtr)theCurrent.fontName);
			theCurrent.relFSize = *relFSize;
			theCurrent.fontSize = *size;
			theCurrent.fontStyle = *style;
			theCurrent.enclosure = *enclosure;
			theCurrent.lyric = *lyric;
			}
		}
	 else {
	 	currentStyle = Header2UserFontNum(doc->lastGlobalFont);
	 	TSSetCurrentStyle(currentStyle);
		PStrCopy((StringPtr)theCurrent.fontName, (StringPtr)name);
		}

	/* At this point, all arguments are defined whether it's a new or old string */
	
	theLineSpacing = LNSPACE(context);
	
	isRelative = theCurrent.relFSize;
	if (isRelative) {
		theRelIndex = theCurrent.fontSize;
		thePtSize = TDRelIndexToSize(theRelIndex);
		theSize = 0;
		}
	 else {
		theRelIndex = 0;
		thePtSize = theSize = theCurrent.fontSize;
		}
	theStyle = theCurrent.fontStyle;
	theEncl = theCurrent.enclosure;
	theLyric = theCurrent.lyric;

	/* Now go on and do dialog */
	
	filterUPP = NewModalFilterUPP(MyFilter);
	if (filterUPP == NULL) {
		MissingDialog(NEW_TEXT_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);

	dlog = GetNewDialog(NEW_TEXT_DLOG,NULL,BRING_TO_FRONT);
	if (dlog == NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(NEW_TEXT_DLOG);
		return FALSE;
	}

	CenterWindow(GetDialogWindow(dlog),0);
	SetPort(GetDialogWindowPort(dlog));
	
	ArrowCursor();
	
	/* Fill in dialog's values here */

	/* Get the background panel rectangles, as defined by some user items */
	
	GetDialogItem(dlog,USER27,&type,&hndl,&fontRect);
	GetDialogItem(dlog,USER28,&type,&hndl,&sizeRect);
	GetDialogItem(dlog,USER29,&type,&hndl,&faceRect);
	GetDialogItem(dlog,USER30,&type,&hndl,&styleRect);
	
	/* Get them out of the way so they don't hide any items underneath */
		
	HideDialogItem(dlog,USER27);
	HideDialogItem(dlog,USER28);
	HideDialogItem(dlog,USER29);
	HideDialogItem(dlog,USER30);
	
	dimRect = styleRect;
	dimRect.top -= 6;									/* Since a label extends above top panel */
	dimRect.bottom = sizeRect.bottom;

	/*
	 *	Init the popup for the font menu, and then search through it for
	 *	the initial name so we can reset the right choice in the popup
	 *	data structure prior to the first update.
	 */
	
	radioChoice = (!isRelative) ? RAD14_Relative : RAD9_Absolute;
	
	if (!InitPopUp(dlog,&popup4,POP4_StyleChoice,STXT3_Text, TEXTSTYLE_POPUP, 0)) goto broken;
	if (!InitPopUp(dlog,&popup7,POP7_Name,STXT6_Name, FONT_POPUP, 0)) goto broken;
	if (!InitPopUp(dlog,&popup11,POP11_Absolute,STXT10_Absolute,SIZE_POPUP,0)) goto broken;
	if (!InitPopUp(dlog,&popup16,POP16_Relative,STXT15_Relative,
								STAFFREL_POPUP,isRelative?theRelIndex:0)) goto broken;
	
	theFont = 0;
	AppendResMenu(popup7.menu,'FONT');
	
	InstallTextStyle(dlog,&theCurrent);
	SetStylePopUp(currentStyle);

	PutDlgString(dlog,EDIT25_Text,string,TRUE);		/* Leave string, if any, selected */

	GetDialogItem(dlog,BUT1_OK,&type,&hndl,&box);
	HiliteControl((ControlHandle)hndl, *string==0 ? CTL_INACTIVE : CTL_ACTIVE);
	
	ShowWindow(GetDialogWindow(dlog));

	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		if (itemHit <= 0) continue;
		GetDialogItem(dlog,itemHit,&type,&hndl,&box);
		switch(itemHit) {
			case BUT1_OK:
				if (theSize<MIN_TEXT_SIZE || theSize>MAX_TEXT_SIZE) {
					GetIndCString(fmtStr, TEXTERRS_STRS, 1);    /* "Text size must be between %d and %d points." */
					sprintf(strBuf, fmtStr, MIN_TEXT_SIZE, MAX_TEXT_SIZE); 
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
				}
				else {
					keepGoing = FALSE;
					okay = TRUE;
				}
				break;
			case BUT2_Cancel:
				keepGoing = FALSE;
				okay = FALSE;
				break;
			case POP4_StyleChoice:
				if (popupAns) {
					currentStyle = GetStyleChoice();
					if (currentStyle==TSThisItemOnlySTYLE) {
						theCurrent.relFSize = radioChoice==RAD14_Relative;
						theCurrent.fontSize = 
							(theCurrent.relFSize ? theRelIndex : thePtSize);
						theCurrent.fontStyle = theStyle;
						theCurrent.enclosure = theEncl;
						theCurrent.lyric = theLyric;
						if (popup7.currentChoice) {
							GetMenuItemText(popup7.menu,popup7.currentChoice,name);
							PStrnCopy((StringPtr)name,(StringPtr)theCurrent.fontName,63);
							}
						}
					 else
						TSSetCurrentStyle(currentStyle);
					PStrCopy((StringPtr)theCurrent.fontName, (StringPtr)name);
					TextEditState(dlog, TRUE);
					InstallTextStyle(dlog,&theCurrent);
					TextEditState(dlog, FALSE);
					DrawExampleText(dlog,NULL);
					if (currentStyle!=TSThisItemOnlySTYLE)
						DimStylePanels(dlog, TRUE);
					else
						InvalWindowRect(GetDialogWindow(dlog),&dimRect);		/* force redrawing by DimStylePanels */
#ifdef DEBUG_PRINTFONTS
					DebugPrintFonts(doc);
#endif
					}
				break;
			case POP7_Name:
				if (popupAns) {
					GetRealSizes();
					DrawExampleText(dlog,NULL);
					currentStyle = TSThisItemOnlySTYLE;
					ChangePopUpChoice(&popup4,currentStyle);
					}
				break;
			case POP11_Absolute:
				if (popupAns) {
					StringToNum(popup11.str,&num); theSize = num;
					TextEditState(dlog,TRUE);
					PutDlgWord(dlog,EDIT12_Points,theSize,FALSE);
					TextEditState(dlog,FALSE);
					TuneRadioIn(dlog,RAD9_Absolute,&radioChoice);
					thePtSize = theSize;
					SetAbsSizePopUp(theSize,str);
					DrawExampleText(dlog,NULL);
					currentStyle = TSThisItemOnlySTYLE;
					ChangePopUpChoice(&popup4,currentStyle);
					}
				break;
			case POP16_Relative:
				if (popupAns) {
					TextEditState(dlog,TRUE);
					PutDlgString(dlog,EDIT12_Points,"\p",FALSE);
					TextEditState(dlog,FALSE);
					TuneRadioIn(dlog,RAD14_Relative,&radioChoice);
					theRelIndex = popup16.currentChoice;
					thePtSize = TDRelIndexToSize(theRelIndex);
					DrawExampleText(dlog,NULL);
					currentStyle = TSThisItemOnlySTYLE;
					ChangePopUpChoice(&popup4,currentStyle);
					}
				break;
			case CHK17_Plain:
			case CHK18_Bold:
			case CHK19_Italic:
			case CHK20_Underline:
			case CHK21_Outline:
			case CHK22_Shadow:
				if (itemHit == CHK17_Plain) {
					/* Turn all others off */
					for (i=CHK18_Bold; i<=CHK22_Shadow; i++) {
						GetDialogItem(dlog,i,&type,&hndl,&box);
						SetControlValue((ControlHandle)hndl,FALSE);
						}
					theStyle = 0;
					}
				 else {
					/* Add or subtract style bit from current style */
					GetDialogItem(dlog,itemHit,&type,&hndl,&box);
					SetControlValue((ControlHandle)hndl,val = !GetControlValue((ControlHandle)hndl));
					i = 1 << (itemHit - CHK18_Bold);
					if (val) theStyle |=  i;
					 else	 theStyle &= ~i;
					}
				/* And force Plain on/off accordingly */
				GetDialogItem(dlog,CHK17_Plain,&type,&hndl,&box);
				SetControlValue((ControlHandle)hndl,theStyle==0);
				DrawExampleText(dlog,NULL);
				currentStyle = TSThisItemOnlySTYLE;
				ChangePopUpChoice(&popup4,currentStyle);
				break;
			case CHK31_Lyric:
				PutDlgChkRadio(dlog,CHK31_Lyric,!GetDlgChkRadio(dlog,CHK31_Lyric));
				currentStyle = TSThisItemOnlySTYLE;
				ChangePopUpChoice(&popup4,currentStyle);
				theLyric = GetDlgChkRadio(dlog,CHK31_Lyric);
				InvalWindowRect(GetDialogWindow(dlog),&dimRect);			/* force redrawing by DimStylePanels */
				break;
			case RAD9_Absolute:
			case STXT10_Absolute:
				if (radioChoice != RAD9_Absolute) {
					if (theSize == 0) theSize = thePtSize;
					TuneRadioIn(dlog,RAD9_Absolute,&radioChoice);
					TextEditState(dlog,TRUE);
					PutDlgWord(dlog,EDIT12_Points,theSize,FALSE);
					TextEditState(dlog,FALSE);
					SetAbsSizePopUp(theSize,str);
					thePtSize = theSize;
					DrawExampleText(dlog,NULL);
					}
				break;
			case RAD14_Relative:
			case STXT15_Relative:
				if (radioChoice != RAD14_Relative) {
					if (theRelIndex == 0) theRelIndex = GRMedium;
					TextEditState(dlog,TRUE);
					PutDlgString(dlog,EDIT12_Points,"\p",FALSE);
					TextEditState(dlog,FALSE);
					TuneRadioIn(dlog,RAD14_Relative,&radioChoice);
					ChangePopUpChoice(&popup16,theRelIndex);
					thePtSize = TDRelIndexToSize(theRelIndex);
					DrawExampleText(dlog,NULL);
					}
				break;
			case EDIT12_Points:
				GetDlgWord(dlog,itemHit,&tmpSize);
				if (tmpSize!=theSize && tmpSize>0) {
					theSize = tmpSize;
					TuneRadioIn(dlog,RAD9_Absolute,&radioChoice);
					thePtSize = theSize;
					DrawExampleText(dlog,NULL);
					SetAbsSizePopUp(theSize,str);
					}
				break;
			case EDIT25_Text:
				GetDlgString(dlog,itemHit,str);
				GetDialogItem(dlog,BUT1_OK,&type,&hndl,&box);
				HiliteControl((ControlHandle)hndl, (*str==0 ? CTL_INACTIVE : CTL_ACTIVE));
				DrawExampleText(dlog,str);
				break;
/* ???Warning: list allocation moves memory! Do we need to lock anything? */
			case BUT32_InsertChar:
				{
					unsigned char theChar[2];
					char str1[512], str2[512];
					short selStart, selEnd, len;
					
					/* ??If EDIT25_Text isn't the current edit field, selStart
					 * and selEnd probably won't be correct!
					 */
					if (ChooseCharDlog(theFont, thePtSize, &theChar[0])) {
						theChar[1] = '\0';
						TEHandle textH = GetDialogTextEditHandle(dlog);
						selStart = (*textH)->selStart;
						selEnd = (*textH)->selEnd;
						GetDlgString(dlog,EDIT25_Text,str);
						len = *str;
						strcpy(str1, (char *)PtoCstr(str));
						str1[selStart] = theChar[0];
						str1[selStart+1] = '\0';
						strcpy(str2, (char *)&str[selEnd]);
						str2[len-selEnd] = '\0';
						strcat(str1, str2);
						PutDlgString(dlog,EDIT25_Text,CtoPstr((StringPtr)str1),FALSE);
						textH = GetDialogTextEditHandle(dlog);
						(*textH)->selStart = selStart+1;
						(*textH)->selEnd = selStart+1;
						DrawExampleText(dlog,(unsigned char *)str1);
						HILITE_DITEM(BUT1_OK, CTL_ACTIVE);
					}
				}
				break;
			default:
				;
			}
		}

	if (okay) {
		doc->changed = TRUE;

		/* Pull chosen font name out of menu into name to be returned */

		if (popup7.currentChoice)
			GetMenuItemText(popup7.menu,popup7.currentChoice,name);
		
		/* And return the other dialog values, and set <lastGlobalFont> */
		
		*relFSize = isRelative;
		if (isRelative) *size = theRelIndex;
		 else			*size = theSize;
		*style = theStyle;
		*enclosure = theEncl;
		*lyric = theLyric;
		*styleChoice = popup4.currentChoice;

		GetDialogItem(dlog,EDIT25_Text,&type,&hndl,&box);
		GetDialogItemText(hndl,string);

		doc->lastGlobalFont = User2HeaderFontNum(doc, currentStyle);
		}
broken:
	DisposePopUp(&popup4);
	DisposePopUp(&popup7);
	DisposePopUp(&popup11);
	DisposePopUp(&popup16);
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return(okay);
}


/* DefineStyleDialog lets the user set the characteristics of any of the text
styles Nightingale knows about, both those for specific purposes (measure numbers,
rehearsal marks, etc.) and those for general use (Regular 1...). It continuously
displays an editable sample text string in the current font. It returns FALSE
in case of an error or Cancel, else TRUE. */

Boolean DefineStyleDialog(Document *doc,
							unsigned char *string,		/* Sample text */
							CONTEXT *context)
{
	LINK			pL;
	short			i, tmpSize, itemHit, type, val;
	Boolean		okay=FALSE, keepGoing=TRUE, tooManyFonts=FALSE;
	Handle		hndl;
	Rect			box;
	long			num;
	DialogPtr	dlog;
	GrafPtr		oldPort;
	unsigned char name[256], str[256];
	char			fmtStr[256];
	ModalFilterUPP filterUPP;
	static short currentStyle = Regular1STYLE;

	theDlog = DefineStyleDlog;
	
	/* First load the style records from the score header */
	
	BlockMove(doc->fontName1,&theRegular1,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName2,&theRegular2,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName3,&theRegular3,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName4,&theRegular4,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName5,&theRegular5,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName6,&theRegular6,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName7,&theRegular7,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName8,&theRegular8,sizeof(TEXTSTYLE));
	BlockMove(doc->fontName9,&theRegular9,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNameMN,&theMeasNum,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNamePN,&thePartName,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNameRM,&theRehearsalMark,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNameTM,&theTempoMark,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNameCS,&theChordSymbol,sizeof(TEXTSTYLE));
	BlockMove(doc->fontNamePG,&thePageNumber,sizeof(TEXTSTYLE));
	
	SetCurrentStyle(currentStyle);
	PStrCopy((StringPtr)theCurrent.fontName, (StringPtr)name);

	theRegular1Changed = theRegular2Changed = theRegular3Changed = theRegular4Changed =
		theRegular5Changed = theRegular6Changed = theRegular7Changed = theRegular8Changed =
		theRegular9Changed = theRehearsalMarkChanged = theChordSymbolChanged = FALSE;

	theLineSpacing = LNSPACE(context);
	
	isRelative = theCurrent.relFSize;
	if (isRelative) {
		theRelIndex = theCurrent.fontSize;
		thePtSize = TDRelIndexToSize(theRelIndex);
		theSize = 0;
		}
	 else {
		theRelIndex = 0;
		thePtSize = theSize = theCurrent.fontSize;
		}
	theStyle = theCurrent.fontStyle;
	theLyric = theCurrent.lyric;
	theEncl = theCurrent.enclosure;
	
	/* Now go on and do dialog */
	
	filterUPP = NewModalFilterUPP(MyFilter);
	if (filterUPP == NULL) {
		MissingDialog(TEXTSTYLE_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);

	dlog = GetNewDialog(TEXTSTYLE_DLOG,NULL,BRING_TO_FRONT);
	if (dlog == NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(TEXTSTYLE_DLOG);
		return FALSE;
	}

	CenterWindow(GetDialogWindow(dlog),0);
	SetPort(GetDialogWindowPort(dlog));
	
	ArrowCursor();
	
	/* Fill in dialog's values here */

	/* Get the background panel rectangles, as defined by some user items */
	
	GetDialogItem(dlog,USER30,&type,&hndl,&fontRect);
	GetDialogItem(dlog,USER31,&type,&hndl,&sizeRect);
	GetDialogItem(dlog,USER32,&type,&hndl,&faceRect);
	
	/*
	 *	Init the popup for the font menu, and then search through it for
	 *	the initial name so we can reset the right choice in the popup
	 *	data structure prior to the first update.
	 */
	
	radioChoice = (!isRelative) ? RAD14_Relative : RAD9_Absolute;
	
	if (!InitPopUp(dlog,&popup4,POP4_StyleChoice,STXT3_Text, STYLECHOICE_POPUP, 0)) goto broken;
	if (!InitPopUp(dlog,&popup7,POP7_Name,STXT6_Name, FONT_POPUP, 0)) goto broken;
	if (!InitPopUp(dlog,&popup11,POP11_Absolute,STXT10_Absolute,SIZE_POPUP,0)) goto broken;
	if (!InitPopUp(dlog,&popup16,POP16_Relative,STXT15_Relative,
								STAFFREL_POPUP,isRelative?theRelIndex:0)) goto broken;
	
	theFont = 0;
	AppendResMenu(popup7.menu,'FONT');
	
	InstallTextStyle(dlog,&theCurrent);
	SetStylePopUp(currentStyle);

	PutDlgString(dlog,EDIT28_Text,(unsigned char *)string,FALSE);
	
	ShowWindow(GetDialogWindow(dlog));

	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		if (itemHit <= 0) continue;
		GetDialogItem(dlog,itemHit,&type,&hndl,&box);
		switch(itemHit) {
			case BUT1_OK:
				if (theSize<MIN_TEXT_SIZE || theSize>MAX_TEXT_SIZE) {
					GetIndCString(fmtStr, TEXTERRS_STRS, 1);    /* "Text size must be between %d and %d points." */
					sprintf(strBuf, fmtStr, MIN_TEXT_SIZE, MAX_TEXT_SIZE); 
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
				}
				else {
					keepGoing = FALSE;
					okay = TRUE;
				}
				break;
			case BUT2_Cancel:
				keepGoing = FALSE;
				okay = FALSE;
				break;
			case POP4_StyleChoice:
				if (popupAns) {
					if (popup7.currentChoice)
						GetMenuItemText(popup7.menu,popup7.currentChoice,name);
					PStrnCopy((StringPtr)name, (StringPtr)theCurrent.fontName, 31);
					theCurrent.relFSize = isRelative;
					theCurrent.fontSize = (isRelative? theRelIndex : theSize);
					theCurrent.fontStyle = theStyle;
					theCurrent.lyric = theLyric;
					theCurrent.enclosure = theEncl;
					SaveCurrentStyle(currentStyle);
					
					currentStyle = GetStyleChoice();
					SetCurrentStyle(currentStyle);
					PStrCopy((StringPtr)theCurrent.fontName, (StringPtr)name);
					TextEditState(dlog, TRUE);
					InstallTextStyle(dlog,&theCurrent);
					TextEditState(dlog, FALSE);
					DrawExampleText(dlog,NULL);
#ifdef DEBUG_PRINTFONTS
					DebugPrintFonts(doc);
#endif
				}
				break;
			case POP7_Name:
				if (popupAns) {
					GetRealSizes();
					DrawExampleText(dlog,NULL);
					}
				break;
			case POP11_Absolute:
				if (popupAns) {
					StringToNum(popup11.str,&num); theSize = num;
					TextEditState(dlog,TRUE);
					PutDlgWord(dlog,EDIT12_Points,theSize,FALSE);
					TextEditState(dlog,FALSE);
					TuneRadioIn(dlog,RAD9_Absolute,&radioChoice);
					thePtSize = theSize;
					SetAbsSizePopUp(theSize,str);
					DrawExampleText(dlog,NULL);
					}
				break;
			case POP16_Relative:
				if (popupAns) {
					TextEditState(dlog,TRUE);
					PutDlgString(dlog,EDIT12_Points,"\p",FALSE);
					TextEditState(dlog,FALSE);
					TuneRadioIn(dlog,RAD14_Relative,&radioChoice);
					theRelIndex = popup16.currentChoice;
					thePtSize = TDRelIndexToSize(theRelIndex);
					DrawExampleText(dlog,NULL);
					}
				break;
			case CHK17_Plain:
			case CHK18_Bold:
			case CHK19_Italic:
			case CHK20_Underline:
			case CHK21_Outline:
			case CHK22_Shadow:
				if (itemHit == CHK17_Plain) {
					/* Turn all others off */
					for (i=CHK18_Bold; i<=CHK22_Shadow; i++) {
						GetDialogItem(dlog,i,&type,&hndl,&box);
						SetControlValue((ControlHandle)hndl,FALSE);
						}
					theStyle = 0;
					}
				 else {
					/* Add or subtract style bit from current style */
					GetDialogItem(dlog,itemHit,&type,&hndl,&box);
					val = !GetControlValue((ControlHandle)hndl);
					SetControlValue((ControlHandle)hndl,val);
					i = 1 << (itemHit - CHK18_Bold);
					if (val) theStyle |=  i;
					 else	 theStyle &= ~i;
					}
				/* And force plain on/off accordingly */
				GetDialogItem(dlog,CHK17_Plain,&type,&hndl,&box);
				SetControlValue((ControlHandle)hndl,theStyle==0);
				DrawExampleText(dlog,NULL);
				break;
			case RAD9_Absolute:
			case STXT10_Absolute:
				if (radioChoice != RAD9_Absolute) {
					if (theSize == 0) theSize = thePtSize;
					TuneRadioIn(dlog,RAD9_Absolute,&radioChoice);
					TextEditState(dlog,TRUE);
					PutDlgWord(dlog,EDIT12_Points,theSize,FALSE);
					TextEditState(dlog,FALSE);
					SetAbsSizePopUp(theSize,str);
					thePtSize = theSize;
					DrawExampleText(dlog,NULL);
					}
				break;
			case RAD14_Relative:
			case STXT15_Relative:
				if (radioChoice != RAD14_Relative) {
					if (theRelIndex == 0) theRelIndex = GRMedium;
					TextEditState(dlog,TRUE);
					PutDlgString(dlog,EDIT12_Points,"\p",FALSE);
					TextEditState(dlog,FALSE);
					TuneRadioIn(dlog,RAD14_Relative,&radioChoice);
					ChangePopUpChoice(&popup16,theRelIndex);
					thePtSize = TDRelIndexToSize(theRelIndex);
					DrawExampleText(dlog,NULL);
					}
				break;
			case CHK23_Treat:
				GetDialogItem(dlog,CHK23_Treat,&type,&hndl,&box);
				SetControlValue((ControlHandle)hndl,!GetControlValue((ControlHandle)hndl));
				theLyric = GetControlValue((ControlHandle)hndl);
				break;
			case RAD25_None:
				for (i=RAD25_None; i<=RAD26_Box; i++) {
					GetDialogItem(dlog,i,&type,&hndl,&box);
					SetControlValue((ControlHandle)hndl,i==RAD25_None);
					}
				theEncl = ENCL_NONE;
				break;
			case RAD26_Box:
				for (i=RAD25_None; i<=RAD26_Box; i++) {
					GetDialogItem(dlog,i,&type,&hndl,&box);
					SetControlValue((ControlHandle)hndl,i==RAD26_Box);
					}
				theEncl = ENCL_BOX;
				break;
			case EDIT12_Points:
				GetDlgWord(dlog,itemHit,&tmpSize);
				if (tmpSize!=theSize && tmpSize>0) {
					theSize = tmpSize;
					TuneRadioIn(dlog,RAD9_Absolute,&radioChoice);
					thePtSize = theSize;
					DrawExampleText(dlog,NULL);
					SetAbsSizePopUp(theSize,str);
					}
				break;
			case EDIT28_Text:
				GetDlgString(dlog,itemHit,str);
				GetDialogItem(dlog,USER28,&type,&hndl,&box);
				DrawExampleText(dlog,str);
				break;
			}
		}

	if (okay) {
		doc->changed = TRUE;
		
		/* Return the new sample string */
		
		GetDialogItem(dlog,EDIT28_Text,&type,&hndl,&box);
		GetDialogItemText(hndl,string);

		/* Change the current text style to new style */
		
		if (popup7.currentChoice)
			GetMenuItemText(popup7.menu,popup7.currentChoice,name);
		PStrnCopy((StringPtr)name, (StringPtr)theCurrent.fontName, 31);
		theCurrent.relFSize = isRelative;
		theCurrent.fontSize = (isRelative? theRelIndex : theSize);
		theCurrent.fontStyle = theStyle;
		theCurrent.lyric = theLyric;
		theCurrent.enclosure = theEncl;

		SaveCurrentStyle(currentStyle);
		
		/* Now copy all dlog styles back into document header, setting changed
			flags for use below. */
		UpdateDocStyles(doc);
		
		/* Change font info stored in every object in score to which the change applies. */
		for (pL=doc->headL; pL; pL=RightLINK(pL)) {
			if (GraphicTYPE(pL)) {
				if (theChordSymbolChanged && GraphicSubType(pL)==GRChordSym) {
					if (!ApplyDocStyle(doc, pL, &theChordSymbol)) { tooManyFonts = TRUE; break; }
				}
				else if (theRehearsalMarkChanged && GraphicSubType(pL)==GRRehearsal) {
					if (!ApplyDocStyle(doc, pL, &theRehearsalMark)) { tooManyFonts = TRUE; break; }
				}
				else if (GraphicSubType(pL)==GRString || GraphicSubType(pL)==GRLyric) {
					if (theRegular1Changed && GraphicINFO(pL)==FONT_R1) {
						if (!ApplyDocStyle(doc, pL, &theRegular1)) { tooManyFonts = TRUE; break; }
					}
					else if (theRegular2Changed && GraphicINFO(pL)==FONT_R2) {
						if (!ApplyDocStyle(doc, pL, &theRegular2)) { tooManyFonts = TRUE; break; }
					}
					else if (theRegular3Changed && GraphicINFO(pL)==FONT_R3) {
						if (!ApplyDocStyle(doc, pL, &theRegular3)) { tooManyFonts = TRUE; break; }
					}
					else if (theRegular4Changed && GraphicINFO(pL)==FONT_R4) {
						if (!ApplyDocStyle(doc, pL, &theRegular4)) { tooManyFonts = TRUE; break; }
					}
					else if (theRegular5Changed && GraphicINFO(pL)==FONT_R5) {
						if (!ApplyDocStyle(doc, pL, &theRegular5)) { tooManyFonts = TRUE; break; }
					}
					else if (theRegular6Changed && GraphicINFO(pL)==FONT_R6) {
						if (!ApplyDocStyle(doc, pL, &theRegular6)) { tooManyFonts = TRUE; break; }
					}
					else if (theRegular7Changed && GraphicINFO(pL)==FONT_R7) {
						if (!ApplyDocStyle(doc, pL, &theRegular7)) { tooManyFonts = TRUE; break; }
					}
					else if (theRegular8Changed && GraphicINFO(pL)==FONT_R8) {
						if (!ApplyDocStyle(doc, pL, &theRegular8)) { tooManyFonts = TRUE; break; }
					}
					else if (theRegular9Changed && GraphicINFO(pL)==FONT_R9) {
						if (!ApplyDocStyle(doc, pL, &theRegular9)) { tooManyFonts = TRUE; break; }
					}
				}
			}
		}
		if (tooManyFonts) {
			GetIndCString(strBuf, MISCERRS_STRS, 27);	/* "The font won't be changed." */
			CParamText(strBuf, "", "", "");
			CautionInform(MANYFONTS_ALRT);
		}

		InvalWindow(doc);
	}
broken:
	DisposePopUp(&popup4);
	DisposePopUp(&popup7);
	DisposePopUp(&popup11);
	DisposePopUp(&popup16);
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return(okay);
}


/* Null function to allow loading or unloading TextDialog.c's segment. */

void XLoadTextDialogSeg()
{
}
