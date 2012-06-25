/* compilerFlags.h for Nightingale */

/* #define *_VERSION and check for consistency. Relationships among versions:

	VIEWER_VERSION has the least functionality: it's a subset of...
	PUBLIC_VERSION, which is a subset of...
	the internal (beta) version (no *_VERSION).

	And now there's LIGHT_VERSION, which is like PUBLIC_VERSION, but with
		a few limitations (fewer pages) and some features missing.  -JGG

DEMO_ and VIEWER_ each require PUBLIC_, but DEMO_ and VIEWER_ are mutually exclusive. */

#define NoPUBLIC_VERSION
#define NoVIEWER_VERSION
#define NoLIGHT_VERSION

#if defined(VIEWER_VERSION) && !defined(PUBLIC_VERSION)
#error "VIEWER_VERSION and not PUBLIC_VERSION is illegal."
#endif

#if defined(LIGHT_VERSION) && defined(VIEWER_VERSION)
#error "LIGHT_VERSION and VIEWER_VERSION is illegal."
#endif


#define JG_NOTELIST

#define USE_GWORLDS	/* instead of 1-bit GrafPorts for dragging and a few other things */

//#define USE_NL2XML
