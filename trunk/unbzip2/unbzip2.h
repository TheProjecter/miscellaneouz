//
// UNBZIP2 -- simple bzip2 decompressor
// Nicolas Falliere
//


#ifndef __UNBZIP2_H__
#define __UNBZIP2_H__


// bzip2_decompress():
//   in: BZIP2 compressed data (starts with 'BZh...')
//   insize: input size
//   out: allocated pointer to store the decompressed data
//   outmaxsize: maximum decompressed size
// Note that the BZIP2 format itself does not store decompressed block/stream lengths.
// These are usually provided in metadata fields of the archive file format that chooses to use BZIP2.
unsigned int bzip2_decompress(unsigned char* in, unsigned int insize, unsigned char* out, unsigned int outmaxsize);


#endif