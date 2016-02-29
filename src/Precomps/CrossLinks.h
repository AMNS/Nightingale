/* Prototypes for CrossLinks.c for Nightingale */

void FixAllBeamLinks(Document *, Document *doc, LINK, LINK);
void FixBeamLinks(Document *, Document *doc, LINK, LINK);
void FixGRBeamLinks(Document *, Document *doc, LINK, LINK);
void FixOttavaLinks(Document *, Document *, LINK, LINK);
void FixTupletLinks(Document *, Document *, LINK startL, LINK endL);
void FixStructureLinks(Document *, Document *, LINK, LINK);
void FixCrossLinks(Document *, Document *, LINK, LINK);
void FixExtCrossLinks(Document *doc, Document *fixDoc, LINK startL, LINK endL);
