#include <math.h>
#include "stdio.h"
#include "stdafx.h"
 
//typedef unsigned char UNIT8; //用 8 位无符号数表示 0～255 之间的整数
unsigned char g_GammaLUT[256];
//全局数组：包含256个元素的gamma校正查找表
//Buildtable()函数对0-255执行如下操作：
//①归一化、预补偿、反归一化;
//②将结果存入 gamma 查找表。
//从公式得fPrecompensation=1/gamma

void BuildTable(float fPrecompensation )
{
	int i;
	float f;
	for( i=0;i<256;i++)
	{
		f=(i+0.5F)/256;						//normalization
		f=(float)pow(f,fPrecompensation);
		g_GammaLUT[i]=(unsigned char)(f*256-0.5F);  //unnormalization
	}
}
 
int GammaCorrection(unsigned char *src, int iWidth, int iHeight, float fGamma, unsigned char *Dst)
{
	printf("GammaCorrection, Gamma=%f\n",fGamma);

	int iCols,iRows;
	BuildTable(1/fGamma);//gamma LUT initialize

	if( src==NULL)
	{
		printf("input data is error!\n");
		return -1;
	}

	//correct pixel by pixel
	for(iRows=0; iRows<iHeight; iRows++)
	{
		for(iCols=0; iCols<iWidth; iCols++)
		{
			Dst[iRows*iWidth+iCols]=g_GammaLUT[src[iRows*iWidth+iCols]];
		}
	}
	return 0;
}