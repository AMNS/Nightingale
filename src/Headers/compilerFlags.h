/* compilerFlags.h for Nightingale */

/* #define *_VERSION and check for consistency. Relationships among versions:

	PUBLIC_VERSION, which is a subset of...
	the internal (beta) version (no *_VERSION).

DEMO_ and VIEWER_ each require PUBLIC_, but DEMO_ and VIEWER_ are mutually exclusive. */

#define NoPUBLIC_VERSION

#define JG_NOTELIST

#define USE_GWORLDS	/* instead of 1-bit GrafPorts for dragging and a few other things */

//#define USE_NL2XML
