typedef unsigned short	fmsUniqueID;

typedef union destinationMatch
{
	struct
	{
		short		destinationType;	/* One of the types enumerated above */
		char			reserved[20];		/* For alignment of name with other types */
		Str255		name;
	} basic;
	struct
	{
		short		destinationType;	/* deviceDestination */
		fmsUniqueID	uniqueID;
		OSType		portDriver;
		short		portNumber;
		short		output;
		long			mfrCode;
		long			modelCode;
		short		index;
		Str255		name;
	} device;
	struct
	{
		short		destinationType;	/* virtualDestination */
		char			reserved[20];		/* For alignment of name with other types */
		Str255		name;
	} instrument;
	struct
	{
		short		destinationType;	/* softwareDestination */
		OSType		signature;
		char			reserved[16];		/* For alignment of name with other types */
		Str255		name;
	} application;
	struct
	{
		short		destinationType;	/* directDestination */
		fmsUniqueID	uniqueID;
		OSType		portDriver;
		short		portNumber;
		short		output;
		char			reserved[10];
		Str255		name;
	} directDest;
} destinationMatch, *destinationMatchPtr;

#define	noUniqueID				0				/* uniqueID */

/* FreeMIDI System Extension creator */
#define	FreeMIDISelector	'FMS_'
