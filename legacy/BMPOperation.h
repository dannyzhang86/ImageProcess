#include "def.h"

typedef struct tagBITMAPFILEHEADER
{ 
    WORD			bfType;
    DWORD 			bfSize;
    WORD			bfReserved1;
    WORD			bfReserved2;
    DWORD 			bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADE
{ 
    DWORD 			biSize;
    LONG			biWidth;
    LONG			biHeight;
    WORD			biPlanes;
    WORD			biBitCount;
    DWORD 			biCompression;
    DWORD 			biSizeImage;
    LONG			biXPelsPerMeter;
    LONG			biYPelsPerMeter;
    DWORD 			biClrUsed;
    DWORD 			biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;


int save_bmp(unsigned char* buf, int width, int height, char* filepath);
int read_bmp(char* filepath, unsigned char* buf,int width,int height);