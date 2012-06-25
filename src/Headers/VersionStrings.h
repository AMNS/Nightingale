/* VersionStrings.h for Nightingale - strings dependent on the _VERSION. These can't
be in a precompiled header like defs.h because the _VERSION isn't yet defined. NB:
if SETUP_FILE_NAME is changed, CREATESETUP_PMSTR may also need to be changed, until
we can get rid of that silly thing. */

#define PROGRAM_NAME 	"Nightingale"										/* C string */

#define PREFS_PATH		"\p:Preferences:"									/* The path separators are */
#define SETUP_FILE_PATH "\p:Nightingale Devel Prefs"						/*		mandatory.. 			*/
#define SETUP_FILE_NAME	"\pNightingale Devel Prefs"							/* 	in these defines 		*/

#define SETUP_TEXTFILE_PATH 	"\p:Nightingale_Devel_Prefs.txt"			/*		mandatory.. 			*/
#define SETUP_TEXTFILE_NAME 	"\pNightingale_Devel_Prefs.txt"

#define HELP_FILE_NAME 	"Nightingale 2004 Help"							/* C string */

