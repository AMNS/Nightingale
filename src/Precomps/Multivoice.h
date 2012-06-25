/* Multivoice.h for Nightingale	*/

/* The following enum for dialog item numbers has to be available to other
 *	files because some of their item numbers are used as return values.
 */
 
enum										/* Multivoice Dialog item numbers */
{
	UPPER_DI=3,
	LOWER_DI,
	CROSS_DI,
	SINGLE_DI,
	ONLY2V_DI,
	MVASSUME_DI=10,
	MVASSUME_BOTTOM_DI=12
};

Boolean MultivoiceDialog(short, short *, Boolean *, Boolean *);
QDIST GetRestMultivoiceRole(PCONTEXT, short, Boolean);
void DoMultivoiceRules(Document *, short, Boolean, Boolean);
void TryMultivoiceRules(Document *, LINK, LINK);
