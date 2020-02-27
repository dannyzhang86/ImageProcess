#include "stdio.h"
#include "wingdi.h"
#include "windef.h"

#include "RAW2RGB24.h"
#include "LUT10_8.h"
#include "LUT12_8.h"
#include "LUT14_8.h"
#include "LUT16_8.h"

//convert RAW1x to RAW8
//f(in) = 2^( log(in) * 8 / bitWidth );
//use the lookup table
int RAW1x_log2_RAW8(unsigned char *src_buffer, int w, int h, int bits, unsigned char *des_buffer)
{
	printf("RAW1x_2_RAW8\n");
	int i=0;
	switch(bits){
		case 8: {for(i=0; i<w*h; i++)des_buffer[i]=src_buffer[i];break;}
		case 10: {for(i=0; i<w*h; i++)des_buffer[i]=LUT10_8[src_buffer[2*i]|((src_buffer[2*i+1]&0x03)<<8)]; break;}
		case 12: {for(i=0; i<w*h; i++)des_buffer[i]=LUT12_8[src_buffer[2*i]|((src_buffer[2*i+1]&0x0f)<<8)];;break;}
		case 14: {for(i=0; i<w*h; i++)des_buffer[i]=LUT14_8[src_buffer[2*i]|((src_buffer[2*i+1]&0x3f)<<8)];;break;}
		case 16: {for(i=0; i<w*h; i++)des_buffer[i]=LUT16_8[src_buffer[2*i]|((src_buffer[2*i+1]&0xff)<<8)];;break;}
		default:{printf("bitwidth=%s, it's wrong!\n",bits);return -1;}
	}
	return 0;
}

//convert RAW1x to RAW8
//f(in) = 2^( log(in) * 8 / bitWidth );
//use the lookup table
int RAW1x_2_RAW8(unsigned char *src_buffer, int w, int h, int bits, unsigned char *des_buffer)
{
	printf("RAW1x_2_RAW8\n");
	int i=0;
	switch(bits){
		case 8: {for(i=0; i<w*h; i++)des_buffer[i]=src_buffer[i];break;}
		case 10: {for(i=0; i<w*h; i++)des_buffer[i]=(src_buffer[2*i]|((src_buffer[2*i+1]&0x03)<<8))>>2; break;}
		case 12: {for(i=0; i<w*h; i++)des_buffer[i]=(src_buffer[2*i]|((src_buffer[2*i+1]&0x0f)<<8))>>4;;break;}
		case 14: {for(i=0; i<w*h; i++)des_buffer[i]=(src_buffer[2*i]|((src_buffer[2*i+1]&0x3f)<<8))>>6;;break;}
		case 16: {for(i=0; i<w*h; i++)des_buffer[i]=(src_buffer[2*i]|((src_buffer[2*i+1]&0xff)<<8))>>8;;break;}
		default:{printf("bitwidth=%s, it's wrong!\n",bits);return -1;}
	}
	return 0;
}

/* inspired by OpenCV's Bayer decoding */  
void v4lconvert_border_bayer_line_to_bgr24(unsigned char *bayer, 
										unsigned char *adjacent_bayer,  
										unsigned char *bgr, int width, 
										int start_with_green, 
										int blue_line)  
{  
	printf("v4lconvert_border_bayer_line_to_bgr24\n");
    int t0, t1;  
  
    if (start_with_green) {  
        /* First pixel */  
        if (blue_line) {  
            *bgr++ = bayer[1];  
            *bgr++ = bayer[0];  
            *bgr++ = adjacent_bayer[0];  
        } else {  
            *bgr++ = adjacent_bayer[0];  
            *bgr++ = bayer[0];  
            *bgr++ = bayer[1];  
        }  
        /* Second pixel */  
        t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;  
        t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;  
        if (blue_line) {  
            *bgr++ = bayer[1];  
            *bgr++ = t0;  
            *bgr++ = t1;  
        } else {  
            *bgr++ = t1;  
            *bgr++ = t0;  
            *bgr++ = bayer[1];  
        }  
        bayer++;  
        adjacent_bayer++;  
        width -= 2;  
    } else {  
        /* First pixel */  
        t0 = (bayer[1] + adjacent_bayer[0] + 1) >> 1;  
        if (blue_line) {  
            *bgr++ = bayer[0];  
            *bgr++ = t0;  
            *bgr++ = adjacent_bayer[1];  
        } else {  
            *bgr++ = adjacent_bayer[1];  
            *bgr++ = t0;  
            *bgr++ = bayer[0];  
        }  
        width--;  
    }  
  
    if (blue_line) {  
        for ( ; width > 2; width -= 2) {  
            t0 = (bayer[0] + bayer[2] + 1) >> 1;  
            *bgr++ = t0;  
            *bgr++ = bayer[1];  
            *bgr++ = adjacent_bayer[1];  
            bayer++;  
            adjacent_bayer++;  
  
            t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;  
            t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;  
            *bgr++ = bayer[1];  
            *bgr++ = t0;  
            *bgr++ = t1;  
            bayer++;  
            adjacent_bayer++;  
        }  
    } else {  
        for ( ; width > 2; width -= 2) {  
            t0 = (bayer[0] + bayer[2] + 1) >> 1;  
            *bgr++ = adjacent_bayer[1];  
            *bgr++ = bayer[1];  
            *bgr++ = t0;  
            bayer++;  
            adjacent_bayer++;  
  
            t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;  
            t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;  
            *bgr++ = t1;  
            *bgr++ = t0;  
            *bgr++ = bayer[1];  
            bayer++;  
            adjacent_bayer++;  
        }  
    }  
  
    if (width == 2) {  
        /* Second to last pixel */  
        t0 = (bayer[0] + bayer[2] + 1) >> 1;  
        if (blue_line) {  
            *bgr++ = t0;  
            *bgr++ = bayer[1];  
            *bgr++ = adjacent_bayer[1];  
        } else {  
            *bgr++ = adjacent_bayer[1];  
            *bgr++ = bayer[1];  
            *bgr++ = t0;  
        }  
        /* Last pixel */  
        t0 = (bayer[1] + adjacent_bayer[2] + 1) >> 1;  
        if (blue_line) {  
            *bgr++ = bayer[2];  
            *bgr++ = t0;  
            *bgr++ = adjacent_bayer[1];  
        } else {  
            *bgr++ = adjacent_bayer[1];  
            *bgr++ = t0;  
            *bgr++ = bayer[2];  
        }  
    } else {  
        /* Last pixel */  
        if (blue_line) {  
            *bgr++ = bayer[0];  
            *bgr++ = bayer[1];  
            *bgr++ = adjacent_bayer[1];  
        } else {  
            *bgr++ = adjacent_bayer[1];  
            *bgr++ = bayer[1];  
            *bgr++ = bayer[0];  
        }  
    }  
}  
  
/* From libdc1394, which on turn was based on OpenCV's Bayer decoding */  
int RAW2RGB24(unsigned char *bayer, 
				int width, 
				int height, 
				int sequence, 
				unsigned char *bgr)
{
	printf("RAW2RGB24\n");
	int start_with_green;
	int blue_line;

	if(sequence == BGGR){start_with_green=0; blue_line=1;}
	else if(sequence == RGGB){start_with_green=0; blue_line=0;}
	else if(sequence == GBRG){start_with_green=1; blue_line=0;}
	else if(sequence == GRBG){start_with_green=1; blue_line=1;}
	else {printf("RAW swqeunce is wrong!\n"); return -1;}

    /* render the first line */  
    v4lconvert_border_bayer_line_to_bgr24(bayer, bayer + width, bgr, width, 
		start_with_green, blue_line);  
    bgr += width * 3;  
  
    /* reduce height by 2 because of the special case top/bottom line */  
    for (height -= 2; height; height--) {  
        int t0, t1;  
        /* (width - 2) because of the border */  
        const unsigned char *bayer_end = bayer + (width - 2);  
  
        if (start_with_green) {  
            /* OpenCV has a bug in the next line, which was 
               t0 = (bayer[0] + bayer[width * 2] + 1) >> 1; */  
            t0 = (bayer[1] + bayer[width * 2 + 1] + 1) >> 1;  
            /* Write first pixel */  
            t1 = (bayer[0] + bayer[width * 2] + bayer[width + 1] + 1) / 3;  
            if (blue_line) {  
                *bgr++ = t0;  
                *bgr++ = t1;  
                *bgr++ = bayer[width];  
            } else {  
                *bgr++ = bayer[width];  
                *bgr++ = t1;  
                *bgr++ = t0;  
            }  
  
            /* Write second pixel */  
            t1 = (bayer[width] + bayer[width + 2] + 1) >> 1;  
            if (blue_line) {  
                *bgr++ = t0;  
                *bgr++ = bayer[width + 1];  
                *bgr++ = t1;  
            } else {  
                *bgr++ = t1;  
                *bgr++ = bayer[width + 1];  
                *bgr++ = t0;  
            }  
            bayer++;  
        } else {  
            /* Write first pixel */  
            t0 = (bayer[0] + bayer[width * 2] + 1) >> 1;  
            if (blue_line) {  
                *bgr++ = t0;  
                *bgr++ = bayer[width];  
                *bgr++ = bayer[width + 1];  
            } else {  
                *bgr++ = bayer[width + 1];  
                *bgr++ = bayer[width];  
                *bgr++ = t0;  
            }  
        }  
  
        if (blue_line) {  
            for (; bayer <= bayer_end - 2; bayer += 2) {  
                t0 = (bayer[0] + bayer[2] + bayer[width * 2] +  
                    bayer[width * 2 + 2] + 2) >> 2;  
                t1 = (bayer[1] + bayer[width] + bayer[width + 2] +  
                    bayer[width * 2 + 1] + 2) >> 2;  
                *bgr++ = t0;  
                *bgr++ = t1;  
                *bgr++ = bayer[width + 1];  
  
                t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;  
                t1 = (bayer[width + 1] + bayer[width + 3] + 1) >> 1;  
                *bgr++ = t0;  
                *bgr++ = bayer[width + 2];  
                *bgr++ = t1;  
            }  
        } else {  
            for (; bayer <= bayer_end - 2; bayer += 2) {  
                t0 = (bayer[0] + bayer[2] + bayer[width * 2] +  
                    bayer[width * 2 + 2] + 2) >> 2;  
                t1 = (bayer[1] + bayer[width] + bayer[width + 2] +  
                    bayer[width * 2 + 1] + 2) >> 2;  
                *bgr++ = bayer[width + 1];  
                *bgr++ = t1;  
                *bgr++ = t0;  
  
                t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;  
                t1 = (bayer[width + 1] + bayer[width + 3] + 1) >> 1;  
                *bgr++ = t1;  
                *bgr++ = bayer[width + 2];  
                *bgr++ = t0;  
            }  
        }  
  
        if (bayer < bayer_end) {  
            /* write second to last pixel */  
            t0 = (bayer[0] + bayer[2] + bayer[width * 2] +  
                bayer[width * 2 + 2] + 2) >> 2;  
            t1 = (bayer[1] + bayer[width] + bayer[width + 2] +  
                bayer[width * 2 + 1] + 2) >> 2;  
            if (blue_line) {  
                *bgr++ = t0;  
                *bgr++ = t1;  
                *bgr++ = bayer[width + 1];  
            } else {  
                *bgr++ = bayer[width + 1];  
                *bgr++ = t1;  
                *bgr++ = t0;  
            }  
            /* write last pixel */  
            t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;  
            if (blue_line) {  
                *bgr++ = t0;  
                *bgr++ = bayer[width + 2];  
                *bgr++ = bayer[width + 1];  
            } else {  
                *bgr++ = bayer[width + 1];  
                *bgr++ = bayer[width + 2];  
                *bgr++ = t0;  
            }  
            bayer++;  
        } else {  
            /* write last pixel */  
            t0 = (bayer[0] + bayer[width * 2] + 1) >> 1;  
            t1 = (bayer[1] + bayer[width * 2 + 1] + bayer[width] + 1) / 3;  
            if (blue_line) {  
                *bgr++ = t0;  
                *bgr++ = t1;  
                *bgr++ = bayer[width + 1];  
            } else {  
                *bgr++ = bayer[width + 1];  
                *bgr++ = t1;  
                *bgr++ = t0;  
            }  
        }  
  
        /* skip 2 border pixels */  
        bayer += 2;  
  
        blue_line = !blue_line;  
        start_with_green = !start_with_green;  
    }  
  
    /* render the last line */  
    v4lconvert_border_bayer_line_to_bgr24(bayer + width, bayer, bgr, width, 
		!start_with_green, !blue_line);  

	return 0;
}  

