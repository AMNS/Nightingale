/* Handle Manager.h - Header File for Handle Manager for Nightingale, rev. for 3.1 */

#ifndef _HandleManager_
#define _HandleManager_

//#include <QuickDraw.h>

/* Every file that includes this file will have our memory manager calls substituted for
the Mac's. */

#define NewHandle(size)			OurNewHandle((long)(size))
#define DisposeHandle(hndl)		OurDisposeHandle((Handle)(hndl))
#define DetachResource(hndl)	OurDetachResource((Handle)(hndl))
#define RemoveResource(hndl)	OurRemoveResource((Handle)(hndl))
#define HandToHand(hndlPtr)		OurHandToHand((Handle *)(hndlPtr))
#define OpenPicture(frame)		OurOpenPicture((Rect *)(frame))
#define KillPicture(hndl)		OurKillPicture(hndl)

/* Prototypes for our calls */

Handle		OurNewHandle(long size);
void		OurDisposeHandle(Handle theHandle);
void		OurDetachResource(Handle theHandle);
void		OurRemoveResource(Handle theHandle);
OSErr		OurHandToHand(Handle *theHandle);
PicHandle	OurOpenPicture(Rect *frame);
void		OurKillPicture(PicHandle pict);

Boolean		SkipRegistering(Boolean how);

Handle		**GetHandleTable(long *pNumHandles);

#endif _HandleManager_
