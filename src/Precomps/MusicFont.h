/*	MusicFont.h for Nightingale */

void PrintMusFontTables(Document *);
Byte MapMusChar(short, Byte);
void MapMusPString(short, Byte []);
DDIST MusCharXOffset(short, Byte, DDIST);
DDIST MusCharYOffset(short, Byte, DDIST);
Boolean MusFontHas16thFlag(short);
Boolean MusFontHasCurlyBraces(short);
Boolean MusFontHasRepeatDots(short);
Boolean MusFontUpstemFlagsHaveXOffset(short);
short MusFontStemSpaceWidthPixels(Document *, short, DDIST);
DDIST MusFontStemSpaceWidthDDIST(short, DDIST);
DDIST UpstemExtFlagLeading(short, DDIST);
DDIST DownstemExtFlagLeading(short, DDIST);
DDIST Upstem8thFlagLeading(short, DDIST);
DDIST Downstem8thFlagLeading(short, DDIST);
void InitDocMusicFont(Document *);

void SetTextSize(Document *);
void BuildCharRectCache(Document *);
Rect CharRect(short);

void NumToSonataStr(long, unsigned char *);
void GetMusicAscDesc(Document *, unsigned char *, short, short *, short *);
short GetMFontSizeIndex(short);
short GetMNamedFontSize(short);
short Staff2MFontSize(DDIST);
short GetActualFontSize(short);

short GetYHeadFudge(short fontSize);
short GetYRestFudge(short fontSize, short durCode);
