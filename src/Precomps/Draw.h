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

void DrawPageContent(Document *, INT16, Rect *, Rect *);
void ScrollDrawPage(void);
void DrawRange(Document *, LINK, LINK, Rect *, Rect *);

void DrawPAGE(Document *, LINK, Rect *, CONTEXT []);
void DrawSYSTEM(Document *, LINK, Rect *, CONTEXT []);

void SetFontFromTEXTSTYLE(Document *, TEXTSTYLE *, DDIST);

void Draw1Staff(Document *, INT16, Rect *, PCONTEXT, INT16);
void DrawSTAFF(Document *, LINK, Rect *, CONTEXT [], INT16, INT16);
void DrawCONNECT(Document *, LINK, CONTEXT [], INT16);
void DrawCLEF(Document *, LINK, CONTEXT []);
void DrawKEYSIG(Document *, LINK, CONTEXT []);
void DrawTIMESIG(Document *, LINK, CONTEXT []);
void DrawDYNAMIC(Document *, LINK, CONTEXT [], Boolean);
void DrawRPTEND(Document *, LINK, CONTEXT []);
void DrawENDING(Document *, LINK, CONTEXT []);
void DrawGRAPHIC(Document *, LINK, CONTEXT [], Boolean);
void DrawSPACE(Document *, LINK, CONTEXT []);
void DrawTEMPO(Document *, LINK, CONTEXT [], Boolean);
void DrawMEASURE(Document *, LINK, CONTEXT []);
void DrawPSMEAS(Document *, LINK, CONTEXT []);
void DrawSYNC(Document *, LINK, CONTEXT []);
void DrawGRSYNC(Document *, LINK, CONTEXT []);
void DrawSLUR(Document *, LINK, CONTEXT []);

unsigned char GetNoteheadInfo(INT16, INT16, INT16 *, INT16 *);

void Draw1ModNR(Document *, DDIST, DDIST, INT16, unsigned char, CONTEXT *, INT16, Boolean);
void DrawModNR(Document *, LINK, DDIST, CONTEXT *);
DDIST AccXOffset(INT16, PCONTEXT);
void DrawAcc(Document *, PCONTEXT, LINK, DDIST, DDIST, Boolean, INT16, Boolean);
void DrawNote(Document *, LINK, PCONTEXT, LINK, Boolean *, Boolean *);
void DrawRest(Document *, LINK, PCONTEXT, LINK, Boolean *, Boolean *);
void DrawGRNote(Document *, LINK, PCONTEXT, LINK, Boolean *, Boolean *);
