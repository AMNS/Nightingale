/* compilerFlags.h for Nightingale */

/* #define *_VERSION. We used to have DEMO_, VIEWER_, PUBLIC_, and  internal (beta)
versions, and DEMO_ and VIEWER_ each required PUBLIC_, but DEMO_ and VIEWER_ were
mutually exclusive, so we also checked for consistency here. Now we have only PUBLIC_
and internal versions.

Relationships among versions:

	PUBLIC_VERSION, which is a subset of...
	the internal (beta) version (no *_VERSION).
*/

#define NoPUBLIC_VERSION

/* Other compiler flags */

#define USE_GWORLDS	/* instead of 1-bit GrafPorts for dragging and a few other things */

//#define USE_NL2XML
