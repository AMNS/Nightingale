/* InsNew.h for Nightingale */

void UpdateBFClefStaff(LINK firstClefL,short staffn,short subtype);
void UpdateBFKSStaff(LINK firstKSL,short staffn,KSINFO newKSInfo);
void UpdateBFTSStaff(LINK firstTSL,short staffn,short subType,short numerator,short denominator);

LINK ReplaceClef(Document *, LINK, short, char);
Boolean ClefBeforeBar(Document *, LINK, char, short);
LINK ReplaceKeySig(Document *, LINK, short, short);
Boolean KeySigBeforeBar(Document *, LINK, short, short);
void ReplaceTimeSig(Document *, LINK, short, short, short, short);
Boolean TimeSigBeforeBar(Document *, LINK, short, short, short, short);

void AddDot(Document *, LINK, short);
Boolean NewArpSign(Document *, Point, char, short, short);
Boolean NewLine(Document *, short, short, char, short, short, LINK);

void NewGraphic(Document *, Point, char, short, short, short, Boolean,
					short, short, short, short, Boolean, Boolean, unsigned char *,
					unsigned char *, short);
void NewMeasure(Document *, Point, char);
LINK CreateMeasure(Document *, LINK, short, short, CONTEXT);
void AddToClef(Document *, char, short);
void NewClef(Document *, short, char, short);
void NewPseudoMeas(Document *, Point, char);
short ModNRPitchLev(Document *, Byte, LINK, LINK);
void NewMODNR(Document *, char, short, short, short, LINK, LINK);
void AddXSysDynamic(Document *, short, short, char, short, short, LINK);
LINK AddNewDynamic(Document *, short, short, DDIST *, char, short *, PCONTEXT, Boolean); 
void NewRptEnd(Document *, short, char, short);

void NewKeySig(Document *, short, short, short);
void NewTimeSig(Document *, short, char, short, short, short, short);
void NewDynamic(Document *, short, short, char, short, short, LINK, Boolean);
void NewEnding(Document *, short, short, char, short, STDIST, LINK, short, short);

void NewTempo(Document *, Point, char, short, STDIST, Boolean, short, Boolean, Boolean,
				unsigned char *, unsigned char *);
void NewSpace(Document *, Point, char, short, short, STDIST);


