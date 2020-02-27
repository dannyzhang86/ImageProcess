#include "stdio.h"
#include "string.h"
#include "math.h"
#include "wingdi.h"


#define BYTE_PER_LINE(w,c)	(((((w))*(c)+31)/32)*4)
int save_bmp(unsigned char* buf,int width, int height, char* filepath)
{
	printf("save_bmp\n");

	FILE * bmpfile;
	bmpfile = fopen(filepath,"wb");
	if (!bmpfile) {
		fprintf (stderr, "Could not open file \"%s\".\n", filepath);
		return -1;
	}
	
	int pixelnum = width*height;

	int	m_nBitCount = 24;
	int m_nWidth = width;
	int m_nHeight = height;
	int nByteWidth=BYTE_PER_LINE(m_nWidth,m_nBitCount);

	BITMAPFILEHEADER bm;
	bm.bfType='M'*256+'B';
	bm.bfSize=nByteWidth*m_nHeight;
	bm.bfReserved1=0;
	bm.bfReserved2=0;
	bm.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	fwrite(&bm,sizeof(char),sizeof(BITMAPFILEHEADER),bmpfile);		//дλͼ�ļ�ͷ
	BITMAPINFOHEADER bmi;
	bmi.biSize=sizeof(BITMAPINFOHEADER);
	bmi.biWidth=m_nWidth;
	bmi.biHeight=m_nHeight;
	bmi.biPlanes=1;
	bmi.biBitCount=m_nBitCount;
	bmi.biCompression=BI_RGB;
	bmi.biSizeImage=0;
	bmi.biXPelsPerMeter=0;
	bmi.biYPelsPerMeter=0;
	bmi.biClrUsed=0;
	bmi.biClrImportant=0;
	fwrite(&bmi,sizeof(char),sizeof(BITMAPINFOHEADER),bmpfile);	//дλͼ��Ϣ
	//vertical flip

	BYTE *p1,*p2,*pm;
	pm=new BYTE[nByteWidth];
	for(int y=0;y<m_nHeight/2;y++)
	{
		p1=buf+y*nByteWidth;
		p2=buf+(m_nHeight-y-1)*nByteWidth;
		memcpy(pm,p1,nByteWidth);
		memcpy(p1,p2,nByteWidth);
		memcpy(p2,pm,nByteWidth);
	}
	if (nByteWidth == 76) nByteWidth = 80;
	fwrite(buf,sizeof(unsigned char),nByteWidth*m_nHeight,bmpfile);	//д����
	fclose(bmpfile);

	delete []pm;
	return 0;
}

int read_bmp(char* filepath, unsigned char* buf,int width,int height)
{
	printf("read_bmp\n");
	FILE* bmpfile = fopen(filepath,"rb");
	if (!bmpfile) {
		fprintf (stderr, "Could not open file \"%s\".\n", filepath);
		return -1;
	}
	BITMAPFILEHEADER bm;
	fread(&bm,sizeof(char),sizeof(BITMAPFILEHEADER),bmpfile);
	BITMAPINFOHEADER bmi;
	fread(&bmi,sizeof(char),sizeof(BITMAPINFOHEADER),bmpfile);
	int m_nWidth = bmi.biWidth;
	width = m_nWidth;
	int m_nHeight = bmi.biHeight;
	height = m_nHeight;
	int	m_nBitCount = bmi.biBitCount;
	int nByteWidth=BYTE_PER_LINE(m_nWidth,m_nBitCount);
	if(buf==NULL)
	   buf = new BYTE[nByteWidth*m_nHeight];
	fread(buf,sizeof(char),nByteWidth*m_nHeight,bmpfile);
	//vertical flip
	BYTE *p1,*p2,*pm;
	pm=new BYTE[nByteWidth];
	for (int y=0;y<m_nHeight/2;y++)
	{
		p1=buf+y*nByteWidth;
		p2=buf+(m_nHeight-y-1)*nByteWidth;
		memcpy(pm,p1,nByteWidth);
		memcpy(p1,p2,nByteWidth);
		memcpy(p2,pm,nByteWidth);
	}
	fclose(bmpfile);
	delete [] pm;

	return 0;
}
