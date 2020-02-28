#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#include "def.h"
#include "RAW2RGB24.h"
#include "BMPOperation.h"
#include "ImageProcess.h"
#include "Gamma.h"

int n_Width = 1280; 
int n_Heigh = 720; 
int n_Sequecne = 10; //
int bitWidth = 8;

void help()
{
	printf("Please input factor, like:\n"); 
	printf("ImageProcess.exe xxxx[Width] XXXX[Hegih] XXXX[Format] XXXX[Sequence] XXXX[RAW file] XXXX[Out]\n");
	printf("eg. ImageProcess.exe 1280 720 RAW8 BGGR RAW8.raw test.bmp\n");
	printf("[Format]: YUV, RAW8, RAW10, RAW12, RAW14, RAW16, RGB565, RGB555,RGB888 ... \n");
	printf("[Sequence]: YUV: YUYV,YVYU,UYVY,VYUY; RAW: BGGR,RGGB,GBRG,GRBG ...\n");
}
int main(int argc, char* argv[])
{
	if(argc<6)
	{
		help();	
		return -1;
	}else
	{
		n_Width = atoi(argv[1]);
		n_Heigh = atoi(argv[2]);

		if(n_Width<1 || n_Heigh<1 || argv[3]==NULL || argv[4]==NULL || argv[5]==NULL || argv[6]==NULL){
			printf("parameter is wrong\n");
			help();
			return -1;
		}
		printf("Size=%d*%d, format=%s, sequence=%s input=%s, out=%s\n",
			n_Width, n_Heigh, argv[3], argv[4], argv[5], argv[5]);
	}	

	long size;
	if(strncmp(argv[3], "RAW8", 4) == 0)	  { bitWidth = 8 ; size=n_Width*n_Heigh*1;}
	else if(strncmp(argv[3], "RAW10", 5) == 0){	bitWidth = 10; size=n_Width*n_Heigh*2;}
	else if(strncmp(argv[3], "RAW12", 5) == 0){	bitWidth = 12; size=n_Width*n_Heigh*2;}
	else if(strncmp(argv[3], "RAW14", 5) == 0){	bitWidth = 14; size=n_Width*n_Heigh*2;}
	else if(strncmp(argv[3], "RAW16", 5) == 0){	bitWidth = 16; size=n_Width*n_Heigh*2;}
	else {printf("format=%s is wrong!\n", argv[3]);	return -1;}

	if(strncmp(argv[4], "BGGR", 4) == 0)	 {n_Sequecne = BGGR;}
	else if(strncmp(argv[4], "RGGB", 4) == 0){n_Sequecne = RGGB;}
	else if(strncmp(argv[4], "GBRG", 4) == 0){n_Sequecne = GBRG;}
	else if(strncmp(argv[4], "GRBG", 4) == 0){n_Sequecne = GRBG;}
	else {printf("Sequence=%s is wrong!\n", argv[4]);	return -1;}

	//load RAW file
	FILE *fp;	 
	if((fp=fopen(argv[5],"rb")) !=0 )
	{
		printf("open file failed!\n");
		return -1;
	}
	BYTE RawBuf[size];
	memset(RawBuf, 0 ,sizeof(RawBuf));
	memset(RawBuf, 0, n_Width*n_Heigh);
	long cnt=fread(RawBuf, 1, size, fp); 
	if(cnt != size)
	{
		printf("Read cnt=%d, size=%d, it's wrong!\n", cnt, size);
		return -1;
	}
	fclose(fp);	
	
	//RAW1X convert ot RAW8
	BYTE Raw8Buf[n_Width*n_Heigh];
	memset(Raw8Buf, 0 ,sizeof(Raw8Buf));
	//if(RAW1x_log2_RAW8(RawBuf, n_Width, n_Heigh, bitWidth, Raw8Buf)!=0){
	if(RAW1x_2_RAW8(RawBuf, n_Width, n_Heigh, bitWidth, Raw8Buf)!=0){
		printf("RAW1x to RAW8 error!\n");
		return -1;
	}
	
	//Gamma curve
	float fGamma = 1.6f;
	//scanf("Please input gamma curve factor(eg. 2.2): %f", &fGamma);
	BYTE GammaBuf[n_Width*n_Heigh];
	memset(GammaBuf, 0 ,sizeof(GammaBuf));
	if(GammaCorrection(Raw8Buf, n_Width, n_Heigh, fGamma, GammaBuf)!=0)
	{
		printf("Gamma Correction is error!\n");
		return -1;
	}

	//RAW8 convert to RGB24, include RAW8 interpolation
	BYTE Rgb24Buf[n_Width*n_Heigh*3];
	memset(Rgb24Buf, 0 ,sizeof(Rgb24Buf));
	if(RAW2RGB24(GammaBuf, n_Width, n_Heigh, n_Sequecne, Rgb24Buf)!=0)
	{
		printf("RAW to RGB24 error!\n");
		return -1;
	}

	//Save RGB24 to BMP picture
	if(save_bmp(Rgb24Buf, n_Width, n_Heigh, argv[6])!=0)
	{
		printf("Save BMP error!\n");
		return -1;
	}

	return 0;
}