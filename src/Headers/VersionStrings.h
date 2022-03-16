/* VersionStrings.h for Nightingale - strings that might be dependent on _VERSIONs, e.g.,
THIS_FILE_VERSION. However, none of them are thse days! An ancient comment here said
"These can't be in a precompiled header like defs.h because the VERSION isn't yet
defined." So now they _could_ be in a precompiled header, and FIXME: they probably should
be.

NB: if PREFS_FILE_NAME is changed, CREATE_PREFS_PMSTR may also need to be changed, until
we can get rid of that silly thing.

NB2: This stuff has nothing to with the actual program version number! We get that (and
as of this writing have done so for many years) from Info.plist. See VersionString(). */

#define PROGRAM_NAME 	"Nightingale"								/* C string */

/* NB: The path separators are mandatory in these #defines. */

#define PREFS_PATH		"\p:Preferences:"
#define PREFS_FILE_PATH "\p:Nightingale AMNF Prefs"
#define PREFS_FILE_NAME	"\pNightingale AMNF Prefs"

#define PREFS_TEXTFILE_PATH 	"\p:Nightingale_AMNF_Prefs.txt"
#define PREFS_TEXTFILE_NAME 	"\pNightingale_AMNF_Prefs.txt"

#define HELP_FILE_NAME 	"Nightingale Help"							/* C string */

