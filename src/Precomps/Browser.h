/* Browser.h for Nightingale - header file for Browser routines */

void BrowserUpdate(void);
void Browser(Document *doc, LINK, LINK);
void ShowContext(Document *doc);

void HeapBrowser(short heapNum);

/* As of v. 3.1, MemBrowser is only in NightBrowser, not in the Nightingale project. */
void MemBrowser(Ptr baseAddr, long length, short heapNum);
