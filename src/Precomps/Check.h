/* Check.h for Nightingale - header file for Check.c */

/* Select modes (used by CheckXXX functions) */

#define SMClick			0
#define SMDrag			1
#define SMStaffDrag		2
#define SMSelect		3
#define SMDeselect		4
#define SMThread		5
#define SMSymDrag		6
#define SMFind			7
#define SMFindNote		8
#define SMDblClick		9
#define SMHilite		10
#define SMFlash			11
#define SMSelNoHilite	12
#define SMDeselNoHilite	13
#define SMSelectRange	14

short CheckPAGE(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckSYSTEM(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckSTAFF(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckCONNECT(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckCLEF(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckDYNAMIC(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckRPTEND(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckGRAPHIC(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckTEMPO(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckSPACER(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckENDING(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckKEYSIG(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckSYNC(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point, Point);
short CheckGRSYNC(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckTIMESIG(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckMEASURE(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckPSMEAS(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckBEAMSET(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE);
short CheckTUPLET(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckOTTAVA(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckSLUR(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE);

void XLoadCheckSeg(void);
