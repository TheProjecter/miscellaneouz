#include <stdio.h>
#include <stdlib.h>
#include "unbzip2.h"



int main(int argc, char* argv[])
{
    FILE*           fp;
    unsigned char*  in;
    unsigned int    insize;
    unsigned char*  out;
    unsigned int    outsize, outmaxsize;


    if(argc != 3)
    {
        printf("BZIP2 decompressor\n", argv[0]);
        printf("Usage: %s <infile> <outfile>\n", argv[0]);
        return -1;
    }

    fp = fopen(argv[1], "rb");
    if(fp == NULL)
    {
        printf("Could not read input file\n");
        return -2;
    }
    fseek(fp, 0, SEEK_END);
    insize = ftell(fp);
    rewind(fp);
    in = malloc(insize);
    fread(in, 1, insize, fp);
    fclose(fp);

    outmaxsize = insize * 10;
    out = malloc(outmaxsize);

    outsize = bzip2_decompress(in, insize, out, outmaxsize);
    if(outsize == -1)
    {
        printf("Decompression error\n");
        free(in);
        free(out);
        return -3;
    }

    fp = fopen(argv[2], "wb");
    if(fp == NULL)
    {
        printf("Could not write output file\n");
        free(in);
        free(out);
        return -4;
    }
    fwrite(out, 1, outsize, fp);
    fclose(fp);

    printf("Done!\n");
    free(in);
    free(out);
    return 0;
}
