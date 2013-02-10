/* Undo.h for Nightingale - header file for Undo.c */

void DisableUndo(Document *, Boolean);
void SetupUndo(Document *, short, char *);
void PrepareUndo(Document *, LINK, short, short);
void DoUndo(Document *);
