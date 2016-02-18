/* Check.h for Nightingale - header file for Check.c */

enum {				/* various Select Modes (used by CheckCLEF, etc) */
	SMClick,
	SMDrag,
	SMStaffDrag,
	SMSelect,
	SMDeselect,
	SMThread,
	SMSymDrag,
	SMFind,
	SMFindNote,
	SMDblClick,
	SMHilite,
	SMFlash,
	SMSelNoHilite,
	SMDeselNoHilite,
	SMSelectRange
};

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
short CheckOCTAVA(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE, Point);
short CheckSLUR(Document *, LINK, CONTEXT [], Ptr, short, STFRANGE);

void XLoadCheckSeg(void);
