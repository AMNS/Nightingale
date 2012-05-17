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

INT16 CheckPAGE(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckSYSTEM(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckSTAFF(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckCONNECT(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckCLEF(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckDYNAMIC(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckRPTEND(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckGRAPHIC(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckTEMPO(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckSPACE(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckENDING(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckKEYSIG(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckSYNC(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point, Point);
INT16 CheckGRSYNC(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckTIMESIG(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckMEASURE(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckPSMEAS(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckBEAMSET(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE);
INT16 CheckTUPLET(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckOCTAVA(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE, Point);
INT16 CheckSLUR(Document *, LINK, CONTEXT [], Ptr, INT16, STFRANGE);

void XLoadCheckSeg(void);
