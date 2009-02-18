#ifndef __LZSS_H__
#define __LZSS_H__


unsigned int lzss_compress(unsigned char* in, unsigned int insize, unsigned char* out, unsigned int outmaxsize);
unsigned int lzss_decompress(unsigned char* in, unsigned int insize, unsigned char* out, unsigned int outmaxsize);


#endif