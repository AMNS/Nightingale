/*	Draw.h: Nightingale header file for DrawXXX.c (except DrawUtils.c) */

#ifdef DRAW
	#define DRAWGLOBAL
#else
	#define DRAWGLOBAL extern
#endif

#ifdef NOTINVARS
DRAWGLOBAL WindowPtr	wDetail;					/* ptr to Detail window */
#endif

DRAWGLOBAL WindowRecord	wrDetail;			/* windowrec for Detail */

#ifdef NOTINVARS

extern FASTFLOAT relFSizeTab[];

#endif

/* The following char. codes are used in DrawRest; there should be a better way...  */

#define BREVEdur		221		/* unused */
#define UNKNOWNdur	126		/* unused */
#define EIGHth			228
#define SIXTEENth 	197
#define THIRTY2nd 	168
#define SIXTY4th 		244
#define ONETWENTY8th 229			

void DrawPageContent(Document *, short, Rect *, Rect *);
void ScrollDrawPage(void);
void DrawRange(Document *, LINK, LINK, Rect *, Rect *);

void DrawPAGE(Document *, LINK, Rect *, CONTEXT []);
void DrawSYSTEM(Document *, LINK, Rect *, CONTEXT []);

void SetFontFromTEXTSTYLE(Document *, TEXTSTYLE *, DDIST);

void Draw1Staff(Document *, short, Rect *, PCONTEXT, short);
void DrawSTAFF(Document *, LINK, Rect *, CONTEXT [], short, short);
void DrawCONNECT(Document *, LINK, CONTEXT [], short);
void DrawCLEF(Document *, LINK, CONTEXT []);
void DrawKEYSIG(Document *, LINK, CONTEXT []);
void DrawTIMESIG(Document *, LINK, CONTEXT []);
void DrawDYNAMIC(Document *, LINK, CONTEXT [], Boolean);
void DrawRPTEND(Document *, LINK, CONTEXT []);
void DrawENDING(Document *, LINK, CONTEXT []);
Boolean DrawTextBlock(Document *, DDIST, DDIST, LINK, PCONTEXT, Boolean, short, short, short);
void DrawGRAPHIC(Document *, LINK, CONTEXT [], Boolean);
void DrawSPACER(Document *, LINK, CONTEXT []);
void DrawTEMPO(Document *, LINK, CONTEXT [], Boolean);
void DrawMEASURE(Document *, LINK, CONTEXT []);
void DrawPSMEAS(Document *, LINK, CONTEXT []);
void DrawSYNC(Document *, LINK, CONTEXT []);
void DrawGRSYNC(Document *, LINK, CONTEXT []);
void DrawSLUR(Document *, LINK, CONTEXT []);

unsigned char GetNoteheadInfo(short, short, short *, short *);

void Draw1ModNR(Document *, DDIST, DDIST, short, unsigned char, CONTEXT *, short, Boolean);
void DrawModNR(Document *, LINK, DDIST, CONTEXT *);
DDIST AccXOffset(short, PCONTEXT);
void DrawAcc(Document *, PCONTEXT, LINK, DDIST, DDIST, Boolean, short, Boolean);
void DrawNote(Document *, LINK, PCONTEXT, LINK, Boolean *, Boolean *);
void DrawRest(Document *, LINK, PCONTEXT, LINK, Boolean *, Boolean *);
void DrawGRNote(Document *, LINK, PCONTEXT, LINK, Boolean *, Boolean *);
