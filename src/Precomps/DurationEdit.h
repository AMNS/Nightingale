/* DurationEdit.h for Nightingale */

/* Constants and variables for the duration palette */

#define DP_NROWS 3							/* No. of rows in full palette */
#define DP_NCOLS 9							/* No. of columns */
#define DP_NDURATIONS	DP_NROWS*DP_NCOLS

#define DP_ROW_HEIGHT 26	/* in pixels */
#define DP_COL_WIDTH 24		/* in pixels; should be a multiple of 8 */

#define SWITCH_DPCELL(curIdx, newIdx, pBox)	HiliteDurCell((curIdx), (pBox), durPalCell); \
			curIdx = (newIdx); HiliteDurCell((curIdx), (pBox), durPalCell)

/* Constants and variables for the simple duration-palette dialog */

#define DP_NROWS_SD 1	/* Show just the top row (no dots) of duration palette */

#define DP_DURCHOICE_DI	4
#define DP_LAST_DI_SD	DP_DURCHOICE_DI

/* Prototypes for generic duration-palette functions */

	void		InitDurationCells(Rect *pBox, short nCols, short nRows, Rect durCell[]);
	short		FindDurationCell(Point where, Rect *pBox, short nCols, short nRows, Rect durCell[]);
	void		HiliteDurCell(short durIdx, Rect *pBox, Rect durCell[]);
	short		DurationKey(unsigned char theChar, unsigned short numDurations);
	short		DurPalChoiceDlog(short durPalIdx);
	Boolean		SetDurDialog(Document *, Boolean *, short *, short *, short *, Boolean *, 
								short *, Boolean *, Boolean *);

