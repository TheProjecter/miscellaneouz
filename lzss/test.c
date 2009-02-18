#include "base.h"
#include "lzss.h"



int lzss_test(char* filename, unsigned int* psize, unsigned int* pcompsize)
{
    int             ret;
    unsigned char*  in = NULL;
    unsigned int    insize;
    unsigned char*  out = NULL;
    unsigned int    outsize, outmaxsize;
    unsigned int    crc0, crc1;


    print("File: %s\n", filename);
    if(!file_get(filename, &in, &insize))
        goto error;

    *psize = insize;

    crc32_init();
    crc0 = crc32_buffer(in, insize);
    print("crc: %08X\n", crc0);
    print("uncompressed size: %d bytes\n", insize);

    outmaxsize = 2 * insize;
    out = memory_alloc(outmaxsize);

    outsize = lzss_compress(in, insize, out, outmaxsize);
    if(outsize == -1)
    {
        print("Compression error\n");
        goto error;
    }

    *pcompsize = outsize;

    print("compressed size: %d bytes\n", outsize);
    print("ratio: %d%%\n", (outsize * 100) / insize);

    memory_set(in, insize, 0);
    outsize = lzss_decompress(out, outsize, in, insize);

    crc1 = crc32_buffer(in, insize);
    if(crc0 != crc1)
    {
        print("CRCs do not match!\n");
        goto error;
    }

    //if(!file_put("out", in, outsize))
    //    goto error;

    print("Success.\n\n");
    ret = 1;

done:
    if(in)
        memory_free(in);
    if(out)
        memory_free(out);
    return ret;

error:
    ret = 0;
    goto done;
}


int main(int argc, char* argv[])
{
    unsigned int    size, tsize, compsize, tcompsize;
    char**          filenames;

    
    char* canterbury[] = {
        "canterbury/alice29.txt",
        "canterbury/asyoulik.txt",
        "canterbury/cp.html",
        "canterbury/fields.c",
        "canterbury/grammar.lsp",
        "canterbury/kennedy.xls",
        "canterbury/lcet10.txt",
        "canterbury/plrabn12.txt",
        "canterbury/ptt5",
        "canterbury/sum",
        "canterbury/xargs.1",
        NULL
    };

    char* artificial[] = {
        "artificial/a.txt",
        "artificial/aaa.txt",
        "artificial/alphabet.txt",
        "artificial/random.txt",
        NULL
    };

    char* large[] = {
        "large/bible.txt",
        "large/E.coli",
        "large/world192.txt",
        NULL
    };

    char* misc[] = {
        "misc/pi.txt",
        NULL
    };

    char* calgary[] = {
        "calgary/bib",
        "calgary/book1",
        "calgary/book2",
        "calgary/geo",
        "calgary/news",
        "calgary/obj1",
        "calgary/obj2",
        "calgary/paper1",
        "calgary/paper2",
        "calgary/paper3",
        "calgary/paper4",
        "calgary/paper5",
        "calgary/paper6",
        "calgary/pic",
        "calgary/progc",
        "calgary/progl",
        "calgary/progp",
        "calgary/trans",
        NULL
    };

    char** corpus[] = {canterbury, artificial, large, misc, calgary, NULL};


    if(0)
    {
        lzss_test(argv[1], &size, &compsize);
        return 0;
    }   

    filenames = corpus[4];
    tsize = 0;
    tcompsize = 0;

    crc32_init();

    while(*filenames)
    {
        if(!lzss_test(*filenames++, &size, &compsize))
            return -1;

        tsize += size;
        tcompsize += compsize;
    }

    print("Grand total:\n");
    print("  Size:       %d bytes\n", tsize);
    print("  Compressed: %d bytes\n", tcompsize);
    print("  Ratio:      %d%%\n", (tcompsize * 100) / tsize);

    return 0;
}
