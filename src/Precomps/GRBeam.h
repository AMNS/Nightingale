/* GRBeam.h: header file for GRBeam.c for Nightingale */

void CreateGRBeams(Document *, INT16);
void FixXStfGRBeams(Document *);
Boolean CheckSelGRBeam(Document *);
INT16 CountGRBeamable(Document *, LINK, LINK, INT16, Boolean);
void GetGRBeamNotes(LINK , LINK[]);
LINK CreateGRBEAMSET(Document *, LINK, LINK, INT16, INT16, Boolean, Boolean);
void GRUnbeam(Document *, LINK, LINK, INT16);
void FixGRBeamsInRange(Document *, LINK, LINK, INT16, Boolean);
void GRUnbeamx(Document *, LINK, LINK, INT16);
Boolean GRUnbeamRange(Document *, LINK, LINK, INT16);

void GetBeamGRNotes(LINK, LINK []);
DDIST CalcGRXStem(Document *, LINK, INT16, INT16, DDIST, PCONTEXT, Boolean);
DDIST GRNoteXStfYStem(LINK, STFRANGE, DDIST, DDIST, Boolean);
INT16 AnalyzeGRBeamset(LINK, INT16 *, INT16 []);
INT16 BuildGRBeamDrawTable(LINK, INT16, INT16, INT16 [], BEAMINFO [], INT16);
void DrawGRBEAMSET(Document *, LINK, CONTEXT []);

LINK FirstInGRBeam(LINK);
LINK LastInGRBeam(LINK);
void RemoveGRBeam(Document *, LINK, INT16, Boolean);
LINK VHasGRBeamAcross(LINK node, INT16 voice);
LINK VHasGRBeamAcrossIncl(LINK node, INT16 voice);	
LINK HasGRBeamAcross(LINK, INT16);	
LINK VCheckGRBeamAcross(LINK, INT16);	
LINK VCheckGRBeamAcrossIncl(LINK, INT16);	
LINK HasGRBeamAcrossIncl(LINK, INT16);
void RecomputeGRNoteStems(Document *, LINK, LINK, INT16);
