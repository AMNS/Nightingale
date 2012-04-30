/* rts:30 Nightingale Source:NewNightingaleDefs.h
  89/07/04 12:07:31

89/07/04 12:07:33	created

92/07/20 06:14:29 decimated. As far as I know, the remaining stuff is needed only for
									AutoBeam.c, except several modules use <noError> and OpcodeUtils.c
									uses TSig...DAB.

96/05/29					Further simplified.
*/

//#if TARGET_CPU_PPC
	#pragma options align=mac68k
//#endif


#define LCDsPerBeat 480L
#define LCD4 (LCDsPerBeat * 4)

struct TSig {
	Byte num;
	Byte denompH;		/* negative exponent of denominator as in DMCS */
	Byte ClickTime;
	Byte beat;
	};

#define noError 0		/* ??Now used in a bunch of places: change all to Apple's <noErr>. */

#define Simple 0		/* divisible by 2 but not 3 */
#define Compound 1	/* divisible by 3 */
#define Imperfect 2	/* not divisible by 2 or 3 */
#define UserDefined 3	/* User entered metric breaks */

/* These are the units into which a measure can be subdivided using integers for everything */

#define WholeNote				3 * 5 * 7 * 256		/* 26880 = 0x6900 */
#define ThirdWhole 			2 * 5 * 7 * 256
#define FifthWhole 			3 * 4 * 7 * 256
#define SeventhWhole 		3 * 5 * 4 * 256

/* FineFactor is the relation between my internal beats and Dave's.
My beats are (3 * 5 * 7 * 64) to the quarter note and his are LCDsPerBeat */

#define FineFactor (long)((3 * 5 * 7 * 64) / LCDsPerBeat)


struct QNote {
	long EventTimEX;					/* Time into measure it comes on/off, in my beats */
	union {
		struct QNote *OffNote;
		struct QNote *OnNote;
	} ntype;
	struct QNote *NextNote;	/* next link in chain */
	struct QNote *LastNote;	/* next link in chain */
	short Measure128;				/* Mth 128th note in measure */
	Byte IthTuple;					/* Ith tuple in this beat */
	Byte num;								/* this many */
	Byte denom;							/* replace that many */
	Byte denompH;						/* negative exponent of denominator as in DMCS */
	Byte type;							/* noteOn or noteOff or ?? */
	Byte MIDInote;					/* 0 to 7f, 80 is rest */
	Byte Channel;
	Byte Velocity;					/* 0 to 7f */
	Byte ReleaseVelocity;
	Byte spare;
	};

struct RhythmClarification {
	Byte BeatsThisRunoff;
	Byte RunoffdenompH;
	short Watershed128;
	long WatershedEX;
	Byte RelativeStrength;
	Byte NextStrongBeat;
	};


//#if TARGET_CPU_PPC
	#pragma options align=reset
//#endif
