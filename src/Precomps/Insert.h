/* Insert.h	for Nightingale */

Boolean InsertNote(Document *, Point, Boolean);
Boolean InsertGRNote(Document *, Point, Boolean);
Boolean InsertArpSign(Document *, Point);
Boolean InsertLine(Document *, Point);
Boolean InsertGraphic(Document *, Point);
Boolean InsertMusicChar(Document *doc, Point pt);
Boolean InsertMODNR(Document *, Point);
Boolean InsertRptEnd(Document *, Point);
Boolean InsertEnding(Document *, Point);
Boolean InsertMeasure(Document *, Point);
Boolean InsertPseudoMeas(Document *, Point);
Boolean InsertClef(Document *, Point);
Boolean InsertKeySig(Document *, Point);
Boolean InsertTimeSig(Document *, Point);
Boolean InsertDynamic(Document *, Point);
Boolean InsertSlur(Document *, Point);
Boolean InsertTempo(Document *, Point);
Boolean InsertSpace(Document *, Point);

LINK AddNote(Document *, INT16, char, INT16, INT16, INT16, INT16);
LINK AddGRNote(Document *, INT16, char, INT16, INT16, INT16, INT16);
void XLoadInsertSeg(void);
