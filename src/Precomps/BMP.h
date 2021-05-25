/*  BMP.h for Nightingale */

#define BMP_HEADER_SIZE 14 + 4 		/* 4 for reading the dib header size */

typedef struct
{
	unsigned int		fileSize;
	unsigned short int	reserved1, reserved2;
	unsigned int		offsetToPixelArray;
} BMPFileHeader;

typedef struct
{   unsigned int infoHdrSize; /* Header size in bytes */
	int width, height; /* Width / Height of image */
	unsigned short int planes; /* Number of colour planes */
	unsigned short int bits; /* Bits per pixel */
	unsigned int compression; /* Compression type */
	unsigned int imageSize; /* Image size in bytes */
	int xResolution, yResolution;/* Pixels per meter */
	unsigned int colors; /* Number of colors */
	unsigned int importantColors;/* Important colors */
} BMPInfoHeader;  /* 40 Bytes */

typedef struct
{   unsigned char r; /* Red */
	unsigned char b; /* Blue */
	unsigned char g; /* Green */
} IMAGE;
