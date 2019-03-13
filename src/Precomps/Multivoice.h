/* Multivoice.h for Nightingale	*/

/* Constants for voice roles. Caution: These item numbers appear in files, so changing
them is an issue for compatibility! The current values start at 3 for compatibility
because the Multivoice dialog formerly returned its item numbers for voice roles and
these are its corresponding item numbers.  --DAB, Nov. 2017 */

#define	VCROLE_UPPER 3
#define	VCROLE_LOWER 4
#define	VCROLE_CROSS 5
#define	VCROLE_SINGLE 6
#define VCROLE_UNKNOWN 7		/* Unused as of this writing (v. 5.8b4) */

Boolean MultivoiceDialog(short, short *, Boolean *, Boolean *);
QDIST GetRestMultivoiceRole(PCONTEXT, short, Boolean);
void DoMultivoiceRules(Document *, short, Boolean, Boolean);
void TryMultivoiceRules(Document *, LINK, LINK);
