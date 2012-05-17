/* Beam.h for Nightingale */

void DoBeam(Document *);
void DoUnbeam(Document *);
void Unbeam(Document *);
LINK Rebeam(Document *, LINK);
Boolean GetBeamEndYStems(Document *, INT16, LINK [], LINK [], LINK, DDIST, INT16, Boolean,
							DDIST *, DDIST *);
DDIST Staff2TopStaff(DDIST, INT16, STFRANGE, DDIST);
void GetBeamNotes(LINK , LINK[]);

void ExtendStems(Document *, LINK, INT16, INT16, INT16[], INT16[]);
void SlantBeamNoteStems(Document *, LINK [], INT16, INT16, DDIST, DDIST, STFRANGE, Boolean);

LINK AddNewBeam(Document *, LINK, INT16, INT16, INT16 *, Boolean, Boolean);
void FillSlantBeam(Document *, LINK, INT16, INT16, LINK[], LINK[],
												DDIST, DDIST, STFRANGE, Boolean);
INT16 CheckBeamVars(INT16, Boolean, INT16, INT16);
LINK CreateXSysBEAMSET(Document *, LINK, LINK, INT16, INT16, INT16, Boolean, Boolean);
LINK CreateNonXSysBEAMSET(Document *, LINK, LINK, INT16, INT16, Boolean, Boolean, INT16);
LINK CreateBEAMSET(Document *, LINK, LINK, INT16, INT16, Boolean, INT16);
void UnbeamV(Document *, LINK, LINK, INT16);
Boolean UnbeamRange(Document *, LINK, LINK, INT16);
void FixBeamsInRange(Document *, LINK, LINK, INT16, Boolean);
void RemoveBeam(Document *, LINK, INT16, Boolean);
void DoBreakBeam(Document *);
void DoFlipFractionalBeam(Document *);

DDIST CalcXStem(Document *, LINK, INT16, INT16, DDIST, PCONTEXT, Boolean);
DDIST NoteXStfYStem(LINK, STFRANGE, DDIST, DDIST, Boolean);

INT16 AnalyzeBeamset(LINK, INT16 *, INT16 [], INT16 [], SignedByte []);
INT16 BuildBeamDrawTable(LINK, INT16, INT16, INT16 [], INT16 [], SignedByte [], BEAMINFO [], 
							INT16);
INT16 SkipPrimaryLevels(INT16, INT16, INT16, INT16);
void SetBeamRect(Rect *, INT16, INT16, INT16, INT16, Boolean, DDIST);
void DrawBEAMSET(Document *, LINK, CONTEXT []);
void Draw1Beam(DDIST, DDIST, DDIST, DDIST, Boolean, DDIST, INT16, INT16, PCONTEXT);

Boolean GetBeamSyncs(Document *, LINK, LINK, INT16, INT16, LINK[], LINK[], Boolean);
LINK SelBeamedNote(Document *);
Boolean CountXSysBeamable(Document *, LINK, LINK, INT16, INT16 *, INT16 *, Boolean);
INT16 CountBeamable(Document *, LINK, LINK, INT16, Boolean);
DDIST GetCrossStaffYLevel(Document *, LINK, STFRANGE);
DDIST CalcBeamYLevel(Document *, INT16, LINK[], LINK[], LINK *, DDIST, DDIST,
						Boolean, INT16, Boolean *);
LINK VHasBeamAcross(LINK, INT16);
LINK HasBeamAcross(LINK, INT16);	
LINK VCheckBeamAcross(LINK, INT16);	
LINK VCheckBeamAcrossIncl(LINK, INT16);	
LINK FirstInBeam(LINK);
LINK LastInBeam(LINK);

void XLoadBeamSeg(void);
