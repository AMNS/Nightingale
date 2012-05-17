/* MPImportExport.h for Nightingale */

INT16 SDAddPart(Document *, INT16, INT16);
INT16 SDStaff2Part(Document *, INT16);
Boolean SDDeletePart(Document *, INT16, INT16);
Boolean AllocSDGlobals(Document *);
void DisposeSDGlobals(Document *);
void ImportEnvironment(Document *);
void ExportEnvironment(Document *,DDIST *);
void ExportPartList(Document *doc);

INT16 SDInstrDialog(Document *doc, INT16 firstStf);
void MPSDDeletePart(Document *doc, INT16 firstStf, INT16 lastStf);

void DelChangePart(Document *doc, INT16 firstStf, INT16 lastStf);
void AddChangePart(Document *doc, INT16 firstStf,INT16 nadd, INT16 nper, INT16 showLines);
void Make1StaffChangeParts(Document *doc, INT16 stf,INT16 nadd, INT16 showLines);
void GroupChangeParts(Document *doc, INT16 firstStf, INT16 lastStf, Boolean connectBars,
						INT16 connectType);
void UngroupChangeParts(Document *doc, INT16 firstStf, INT16 lastStf);

INT16 ExportChanges(Document *doc);