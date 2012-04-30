// MS
#include <Carbon/Carbon.h>
#define OMS_STRING_LEN 255
// MS

/* Primitive types */
typedef unsigned char OMSBool;			/* same as Boolean */

typedef short OMSErr;

typedef unsigned short OMSUniqueID;

typedef unsigned long OMSSignature;		/* same as OSType */

typedef unsigned char OMSString[OMS_STRING_LEN + 1];	/* pascal string */

typedef unsigned char *OMSStringPtr;	/* points to pascal string */

/* Other defs */
typedef Handle OMSDeviceMenuH;

typedef Point OMSPoint;

#ifdef __cplusplus
	#define OMSAPI(rettype)	extern "C" pascal rettype
#else
	#define OMSAPI(rettype)	pascal rettype
#endif

OMSAPI(OMSBool) TestOMSDeviceMenu(OMSDeviceMenuH menu, OMSPoint pt);

OMSAPI(short)	ClickOMSDeviceMenu(OMSDeviceMenuH menu);

OMSAPI(short)	OMSCountDeviceMenuItems( OMSDeviceMenuH theDeviceMenu );

OMSAPI(void)	DisposeOMSDeviceMenu(OMSDeviceMenuH menu);

OMSAPI(void)	DrawOMSDeviceMenu(OMSDeviceMenuH menu);

OMSAPI(void)	SetOMSDeviceMenuSelection(OMSDeviceMenuH menu, unsigned char itemType, 
					OMSUniqueID uniqueID, OMSStringPtr name, OMSBool selected);
			
OMSAPI(void)	RebuildOMSDeviceMenu(OMSDeviceMenuH menu);

OMSAPI(void)	AppendOMSDeviceMenu(OMSDeviceMenuH menu, unsigned char itemType, 
					OMSUniqueID uniqueID, OMSStringPtr itemName);

OMSAPI(short)	OMSUniqueIDToRefNum(OMSUniqueID);
