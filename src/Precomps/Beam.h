/* Beam.h for Nightingale */

void DoBeam(Document *);
void DoUnbeam(Document *);
void Unbeam(Document *);
LINK Rebeam(Document *, LINK);
Boolean GetBeamEndYStems(Document *, short, LINK [], LINK [], LINK, DDIST, short, Boolean,
							DDIST *, DDIST *);
DDIST Staff2TopStaff(DDIST, short, STFRANGE, DDIST);
void GetBeamNotes(LINK , LINK[]);

void ExtendStems(Document *, LINK, short, short, short[], short[]);
void SlantBeamNoteStems(Document *, LINK [], short, short, DDIST, DDIST, STFRANGE, Boolean);

LINK AddNewBeam(Document *, LINK, short, short, short *, Boolean, Boolean);
void FillSlantBeam(Document *, LINK, short, short, LINK[], LINK[],
												DDIST, DDIST, STFRANGE, Boolean);
short CheckBeamVars(short, Boolean, short, short);
LINK CreateXSysBEAMSET(Document *, LINK, LINK, short, short, short, Boolean, Boolean);
LINK CreateNonXSysBEAMSET(Document *, LINK, LINK, short, short, Boolean, Boolean, short);
LINK CreateBEAMSET(Document *, LINK, LINK, short, short, Boolean, short);
void UnbeamV(Document *, LINK, LINK, short);
Boolean UnbeamRange(Document *, LINK, LINK, short);
void FixBeamsOnStaff(Document *, LINK, LINK, short, Boolean);
void FixBeamsInVoice(Document *, LINK, LINK, short, Boolean);
void RemoveBeam(Document *, LINK, short, Boolean);
void DoBreakBeam(Document *);
void DoFlipFractionalBeam(Document *);

DDIST CalcXStem(Document *, LINK, short, short, DDIST, PCONTEXT, Boolean);
DDIST NoteXStfYStem(LINK, STFRANGE, DDIST, DDIST, Boolean);

short AnalyzeBeamset(LINK, short *, short [], short [], SignedByte []);
short BuildBeamDrawTable(LINK, short, short, short [], short [], SignedByte [], BEAMINFO [], 
							short);
short SkipPrimaryLevels(short, short, short, short);
void SetBeamRect(Rect *, short, short, short, short, Boolean, DDIST);
void DrawBEAMSET(Document *, LINK, CONTEXT []);
void Draw1Beam(DDIST, DDIST, DDIST, DDIST, Boolean, DDIST, short, short, PCONTEXT);

Boolean GetBeamSyncs(Document *, LINK, LINK, short, short, LINK[], LINK[], Boolean);
LINK SelBeamedNote(Document *);
Boolean CountXSysBeamable(Document *, LINK, LINK, short, short *, short *, Boolean);
short CountBeamable(Document *, LINK, LINK, short, Boolean);
DDIST GetCrossStaffYLevel(Document *, LINK, STFRANGE);
DDIST CalcBeamYLevel(Document *, short, LINK[], LINK[], LINK *, DDIST, DDIST,
						Boolean, short, Boolean *);
LINK VHasBeamAcross(LINK, short);
LINK HasBeamAcross(LINK, short);	
LINK VCheckBeamAcross(LINK, short);	
LINK VCheckBeamAcrossIncl(LINK, short);	
LINK FirstInBeam(LINK);
LINK LastInBeam(LINK);

void XLoadBeamSeg(void);
