/* InstrDialog.h: Nightingale header file for InstrDialog.c */

short InstrDialog(Document *doc, PARTINFO *Mp);
Boolean PartMIDIDialog(Document *doc, PARTINFO *mp, Boolean *allParts);

short OMSInstrDialog(Document *doc, PARTINFO *mp, OMSUniqueID *mpDevice);
Boolean OMSPartMIDIDialog(Document *doc, PARTINFO *mp, OMSUniqueID *mpDevice);

short CMInstrDialog(Document *doc, PARTINFO *mp, MIDIUniqueID *mpDevice);
Boolean CMPartMIDIDialog(Document *doc, PARTINFO *mp, MIDIUniqueID *mpDevice, Boolean *allParts);

void XLoadInstrDialogSeg(void);

#ifndef _table_
#define _table_

/* define, allocate and initialize structs for moving range */

#define TOP				0
#define TRANS			1
#define BOT				2
#define T_TRNS_B		3				/* "top_transpose_bottom" */	
#define NM_SIZE			32
#define AB_SIZE			12
#define MAXMIDI			MAX_NOTENUM		/* MIDI note no. */
#define MINMIDI			0				/* MIDI note no. */
#define MAXOCT			11
#define MAXSCALE		16
#define TWELVE			12
#define RSIZE			3				/* max size of midinote value as string */

#pragma options align=mac68k

struct range
{
	char midiNote;
	char name;
	char acc;
};

typedef struct rangeMaster
{
	struct range ttb[T_TRNS_B];		/* "ttb": acronym for "top, transpose, bottom" */
	char name[NM_SIZE];
	char abbr[NM_SIZE];
	unsigned char channel;			/* MIDI channel no. */
	unsigned char patchNum;			/* MIDI program no. */
	char	velBalance;				/* MIDI playback velocity offset */
	unsigned short device;			/* device number; can be either OMSUniqueID or fmsUniqueID */
	unsigned long cmDevice;			/* MIDIUniqueID */
	unsigned char validDeviceChannel;
	unsigned char changedDeviceChannel;	
} rangeMaster;

typedef rangeMaster **rangeHandle;

struct scaleTblEnt
{
	struct range n;
	short v_pos;
};


#pragma options align=reset

#endif

