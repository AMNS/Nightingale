/* MIDICompat.h for Nightingale */

#ifndef MIDICOMPAT_H
#define MIDICOMPAT_H

typedef struct MMMIDIPacket {
  UInt8               flags;
  UInt8               len;
  long                tStamp;
  UInt8               data[249];
} MMMIDIPacket;

enum {										/* OffsetTimes  */
  midiGetEverything		= 0x7FFFFFFF,		/* get all packets, regardless of time stamps */
  midiGetNothing		= (long)0x80000000,	/* get no packets, regardless of time stamps */
  midiGetCurrent		= 0x00000000		/* get current packets only */
};

#endif
