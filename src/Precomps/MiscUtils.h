/* MiscUtils.h for Nightingale */

#ifndef _MISCUTILS_
#define _MISCUTILS_
#endif

#ifndef MAXINPUTTYPE
#define MAXINPUTTYPE	4
#endif

/*
 *	Codes for various open commands as delivered by GetInputName
 */

enum {
		OP_Cancel,
		OP_OpenFile,
		OP_NewFile,
		OP_QuitInstead
	};

/*
 *	Prototypes for routines in MiscUtils.c
 */

pascal long GrowMemory(Size memoryNeeded);
Boolean		PreflightMem(short nKBytes);
void		ZeroMem(void *m, long nBytes);
Boolean		GoodNewPtr(Ptr p);
Boolean		GoodNewHandle(Handle hndl);
Boolean		GoodResource(Handle hndl);
void		*NewZPtr(Size size);

Boolean		CheckAbort(void);
Boolean		IsDoubleClick(Point clickPt, short tol, long now);
OSType		CanPaste(short n, ...);
void		DrawGrowBox(WindowPtr w, Point pt, Boolean drawit);

void		UseStandardType(OSType type);
void		ClearStandardTypes(void);

short		GetInputName(char *prompt, Boolean newName, unsigned char *name, short *vrefnum, NSClientDataPtr pnsData);
Boolean		GetOutputName(short promptsID, short promptInd, unsigned char *name, short *vrefnum, NSClientDataPtr pnsData);
void		EndianFixConfig(void);

void		GetSelection(WindowPtr w, Rect *sel, Rect *pin, void (*CallBack)());
void		DrawTheSelection(void);

unsigned char	*VersionString(void);
OSErr		SysEnvirons(short versionRequested,SysEnvRec *theWorld);
