/* VersionStrings.h for Nightingale - strings dependent on _VERSIONs, most often
THIS_FILE_VERSION. These can't be in a precompiled header like defs.h because the
_VERSION isn't yet defined. NB: if PREFS_FILE_NAME is changed, CREATE_PREFS_PMSTR
may also need to be changed, until we can get rid of that silly thing.

Note that this stuff has nothing to with the actual program version number! We get
that (and as of this writing have for many years gotten it) from Info.plist. See
VersionString(). */

#define PROGRAM_NAME 	"Nightingale"								/* C string */

/* NB: The The path separators are mandatory in these #defines. */
#define PREFS_PATH		"\p:Preferences:"
#define PREFS_FILE_PATH "\p:Nightingale AMNF Prefs"
#define PREFS_FILE_NAME	"\pNightingale AMNF Prefs"

#define SETUP_TEXTFILE_PATH 	"\p:Nightingale_AMNF_Prefs.txt"
#define SETUP_TEXTFILE_NAME 	"\pNightingale_AMNF_Prefs.txt"

#define HELP_FILE_NAME 	"Nightingale 2004 Help"						/* C string */

