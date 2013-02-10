/* GRBeam.h: header file for GRBeam.c for Nightingale */

void CreateGRBeams(Document *, short);
void FixXStfGRBeams(Document *);
Boolean CheckSelGRBeam(Document *);
short CountGRBeamable(Document *, LINK, LINK, short, Boolean);
void GetGRBeamNotes(LINK , LINK[]);
LINK CreateGRBEAMSET(Document *, LINK, LINK, short, short, Boolean, Boolean);
void GRUnbeam(Document *, LINK, LINK, short);
void FixGRBeamsInRange(Document *, LINK, LINK, short, Boolean);
void GRUnbeamx(Document *, LINK, LINK, short);
Boolean GRUnbeamRange(Document *, LINK, LINK, short);

void GetBeamGRNotes(LINK, LINK []);
DDIST CalcGRXStem(Document *, LINK, short, short, DDIST, PCONTEXT, Boolean);
DDIST GRNoteXStfYStem(LINK, STFRANGE, DDIST, DDIST, Boolean);
short AnalyzeGRBeamset(LINK, short *, short []);
short BuildGRBeamDrawTable(LINK, short, short, short [], BEAMINFO [], short);
void DrawGRBEAMSET(Document *, LINK, CONTEXT []);

LINK FirstInGRBeam(LINK);
LINK LastInGRBeam(LINK);
void RemoveGRBeam(Document *, LINK, short, Boolean);
LINK VHasGRBeamAcross(LINK node, short voice);
LINK VHasGRBeamAcrossIncl(LINK node, short voice);	
LINK HasGRBeamAcross(LINK, short);	
LINK VCheckGRBeamAcross(LINK, short);	
LINK VCheckGRBeamAcrossIncl(LINK, short);	
LINK HasGRBeamAcrossIncl(LINK, short);
void RecomputeGRNoteStems(Document *, LINK, LINK, short);
