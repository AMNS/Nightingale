/* OpcodeNightingaleData.h for Nightingale */

/* This is what remains of a lengthy series of declarations Ray Spears used. For
clarity and to avoid future problems, most or all of it should be moved somewhere
else or, better, eliminated completely, but I don't think it's causing any problems
now (v. 3.1). */

#ifdef MAIN
#undef GLOBAL
#define GLOBAL 
#else
#define GLOBAL extern
#endif

/* exporting features */

GLOBAL struct TSig xTS;

/* Sequencer to SIM variables */

/* Watersheds which break analysis-regions (strength > -1 per D. Byrd) */

GLOBAL struct RhythmClarification *RCPtr;
GLOBAL struct RhythmClarification *RCP;

/* Durations in 26880ths, i.e., units of FineFactor * LCD4 = 14 * 1920 */

GLOBAL long CurrentTimEX, MeasureLengthEX, MeasureStartEX, MeasureEndEX;
