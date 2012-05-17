/* Undo.h for Nightingale - header file for Undo.c */

void DisableUndo(Document *, Boolean);
void SetupUndo(Document *, INT16, char *);
void PrepareUndo(Document *, LINK, INT16, short);
void DoUndo(Document *);
