/* DurationPopUp.h for Nightingale */

#pragma options align=mac68k

/* Format of 'chgd' (character grid) resource */

typedef struct {
	short			numChars;				/* size of chars[] */
	unsigned char	numColumns;
	char			filler;
	short			fontSize;				/* popup char. font size */
	unsigned char	fontName[];				/* even length (incl. length byte) Pascal string */

	/* numChar bytes follow. */

} CHARGRID, *PCHARGRID, **HCHARGRID;

typedef struct {
	Rect			box, bounds, shadow;
	short			currentChoice, menuID;
	MenuHandle		menu;
	short			fontNum;
	short			fontSize;
	unsigned char	numRows;
	unsigned char	numColumns;
	short			numItems;				/* size of itemChars[] */
	char			*itemChars;				/* 0-based array */
} GRAPHIC_POPUP, *PGRAPHIC_POPUP;

typedef struct {
	char			durCode;
	char			numDots;
} POPKEY;

#pragma options align=reset

POPKEY *InitDurPopupKey(PGRAPHIC_POPUP);
short GetDurPopItem(PGRAPHIC_POPUP, POPKEY *, short, short);
Boolean DurPopupKey(PGRAPHIC_POPUP, POPKEY *, unsigned char);

