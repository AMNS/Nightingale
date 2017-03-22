/* ApplicData.h: generic and semi-generic "Sheets" globals for Nightingale */

GLOBAL Configuration config;			/* Nightingale configuration info */
GLOBAL Boolean		inBackground;		/* Application state: TRUE if we're frontmost */
GLOBAL Boolean		closingAll;			/* TRUE when closing all windows */
GLOBAL Boolean		quitting;			/* TRUE when closing down */
GLOBAL short		oldEventMask;		/* Event mask state to restore */
GLOBAL Boolean 		alreadyDrawn;		/* Don't include controls in next update */
GLOBAL Boolean		notMenu;			/* Is operation in a torn off window or not */
GLOBAL Rect			*updateRectTable;	/* Table of nUpdateRects rectangles to be redrawn */
GLOBAL short		nUpdateRects;		/* Size of above, or -1 for standard update region */
GLOBAL Boolean		holdCursor;			/* Don't reset arrow while this is TRUE */

GLOBAL WindowPtr	TopDocument;		/* Frontmost document, or NULL */
GLOBAL WindowPtr	TopWindow;			/* Frontmost window, or NULL */
GLOBAL WindowPtr	TopPalette;			/* Frontmost palette, or NULL */
GLOBAL WindowPtr	BottomPalette;		/* Most buried palette, or NULL */

GLOBAL short		toolCellWidth,		/* Size of a tool palette cell */
					toolCellHeight;
GLOBAL short		clavierCellWidth,	/* Size of clavier cell */
					clavierCellHeight;
GLOBAL Rect			toolsFrame;			/* Frame for symbols picture in palette */
GLOBAL Rect			clavierFrame;		/* Same for clavier palette */
GLOBAL Boolean		outOfMemory;		/* TRUE when grow zone function has been called */
GLOBAL Handle		memoryBuffer;		/* Handle to grow zone free block */

#if TARGET_API_MAC_CARBON
GLOBAL MenuRef
#else
GLOBAL MenuHandle
#endif	
					appleMenu,			/* Standard menus */
					fileMenu,
					editMenu,
					testMenu,
					scoreMenu,
					notesMenu,
					groupsMenu,
					viewMenu,
					playRecMenu,
					magnifyMenu,
					masterPgMenu,
					formatMenu,
					symbolMenu,
					clavierMenu;

GLOBAL unsigned char *tmpStr;				/* General string storage for anyone */
GLOBAL Pattern		paperBack;				/* What background behind paper looks like */

GLOBAL Rect			theSelection;			/* Non-empty ==> a currently selected area */

GLOBAL EventRecord	theEvent;				/* The current event */

GLOBAL WindowPtr	palettes[TOTAL_PALETTES];			/* All palettes */
GLOBAL short		palettesVisible[TOTAL_PALETTES];	/* Which were visible before suspend */
GLOBAL PaletteGlobals **paletteGlobals[TOTAL_PALETTES];	/* Each palette's shared MDEF resource */
GLOBAL Rect			toolRects[TOOL_COUNT];
GLOBAL Rect			clavierRects[CLAVIER_COUNT];

GLOBAL Document		*documentTable,			/* Ptr to head of score table */
					*topTable,				/* 1 slot past last entry in table */
					*clipboard,				/* Ptr to clipboard score */
					*searchPatDoc;			/* Ptr to search-pattern score */

#ifdef FUTUREEVENTS
GLOBAL ResType		myAppType;				/* 'BYRD' or 'KAHL' depending on environment */
#endif
