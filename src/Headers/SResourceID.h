
/*
 *	These are symbolic definitions for resource ID's of various Sheets resources
 */

/* Alerts and dialogs */

enum {
	lowMemoryID = 2000,
	errorMsgID,
	saveChangesID,
	discardChangesID,
	
	sheetSetupID = 3000,
	aboutBoxID,
	gotoPageID,
	
	eatCommaID
	};

/* Windows */

enum {
	docWindowID = 2000
	};

/* String List IDs */

enum {
		MiscStringsID = 129,
		ErrorStringsID = 228,
		ConversionScalesID = 2000,
		PaperSizesID,
		/* These ID's reserved for up to 20 measurement systems */
		firstFreeSlotID = 2021
	};

/* Pattern List ID's */

enum {
		MiscPatternsID = 128,
		MarchingAntsID
	};

/* Picture ID's */

enum {
		BRACE_ID = 200,
		AboutBoxPictID = 2000,				/* (Not used by Nightingale) */
		ToolPaletteID,
		ClavierPaletteID
	};

/* Cursor ID's */

enum {
		DragUpDownID = 2000,
		DragRightLeftID = 2001
	};

enum {
		ToolPaletteWDEF_ID = 10
	};
