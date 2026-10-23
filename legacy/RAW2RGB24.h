typedef enum{
	BGGR = 10,
	RGGB,
	GBRG,
	GRBG,
}Sequence_RAW;

int RAW1x_log2_RAW8(unsigned char *src_buffer, 
				int w, 
				int h, 
				int bits, 
				unsigned char *des_buffer);

int RAW1x_2_RAW8(unsigned char *src_buffer, 
				int w, 
				int h, 
				int bits, 
				unsigned char *des_buffer);

int RAW2RGB24(unsigned char *bayer, 
				int width, 
				int height, 
				int sequence, 
				unsigned char *bgr);

