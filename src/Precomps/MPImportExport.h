/* MPImportExport.h for Nightingale */

short SDAddPart(Document *, short, short);
short SDStaff2Part(Document *, short);
Boolean SDDeletePart(Document *, short, short);
Boolean AllocSDGlobals(Document *);
void DisposeSDGlobals(Document *);
void ImportEnvironment(Document *);
void ExportEnvironment(Document *,DDIST *);
void ExportPartList(Document *doc);

short SDInstrDialog(Document *doc, short firstStf);
void MPSDDeletePart(Document *doc, short firstStf, short lastStf);

void DelChangePart(Document *doc, short firstStf, short lastStf);
void AddChangePart(Document *doc, short firstStf,short nadd, short nper, short showLines);
void Make1StaffChangeParts(Document *doc, short stf,short nadd, short showLines);
void GroupChangeParts(Document *doc, short firstStf, short lastStf, Boolean connectBars,
						short connectType);
void UngroupChangeParts(Document *doc, short firstStf, short lastStf);

short ExportChanges(Document *doc);