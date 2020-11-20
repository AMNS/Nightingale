/* PaletteGlobals.h for Nightingale */

/* Constants for tear-off palettes */

#define PALETTE_GLOBALS_TYPE		'PGLB'
#define MOVE_PALETTE_ITEM			(-1)
#define PALETTE_TITLE_BAR_HEIGHT	10

/* Initial h & v position of tool palette */
#define INIT_H						450
#define INIT_V						90

/* Tool or symbol palette constants */

#define DEFAULT_TOOL				4
#define TOOLS_ACROSS 				16
#define TOOLS_DOWN					16
#define TOOL_COUNT					((TOOLS_ACROSS * TOOLS_DOWN) + 1)
#define TOOLS_MARGIN				7

#define CLAVIER_ACROSS				1
#define CLAVIER_DOWN				1
#define CLAVIER_COUNT				((CLAVIER_ACROSS * CLAVIER_DOWN) + 1)
#define CLAVIER_MARGIN				2
#define DEFAULT_CLAVIER				1

#define HELP_ACROSS					1
#define HELP_DOWN					1
#define HELP_COUNT					((HELP_ACROSS * HELP_DOWN) + 1)
#define HELP_MARGIN					4
#define DEFAULT_HELP				1

/*
 *	The shared structure for palettes and their MDEF that lets them be torn off.
 */

#pragma options align=mac68k

typedef struct {
	short		currentItem;
	short		itemHilited;			/* (unused) */
	short		maxAcross;				/* used when zooming */
	short		maxDown;				/* used when zooming */
	short		across;
	short		down;
	short		oldAcross;				/* used when zooming */
	short		oldDown;				/* used when zooming */
	short		firstAcross;
	short		firstDown;
	void		(*drawMenuProc)();		/* not saved; filled in during initialization */
	short		(*findItemProc)();		/* not saved; filled in during initialization */
	void		(*hiliteItemProc)();	/* not saved; filled in during initialization */
	SysEnvRec	*environment;			/* not saved; filled in during initialization */
	WindowPtr	paletteWindow;			/* not saved; filled in during initialization */
	Rect		origPort;
	Point		position;
	short		zoomAcross;
	short		zoomDown;
	long		filler[6];				/* Room for expansion */
	
	} PaletteGlobals;

#pragma options align=reset
