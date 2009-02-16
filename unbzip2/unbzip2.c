//
// UNBZIP2 -- simple bzip2 decompressor
// Nicolas Falliere
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unbzip2.h"



typedef unsigned char           UINT8;
typedef unsigned short          UINT16;
typedef unsigned int            UINT32;

#define ALLOC(s)                malloc(s)
#define FREE(p)                 free(p)
#define SET(p, s, b)            memset(p, b, s)
#define PRINT                   __noop

/*#ifdef _DEBUG
#define PRINT                   printf
#else
#define PRINT                   __noop
#endif*/

#define P8(x, o)                *((UINT8*)((UINT8*)x + (o)))
#define P16(x, o)               *((UINT16*)((UINT8*)x + (o)))
#define P32(x, o)               *((UINT32*)((UINT8*)x + (o)))

#define BZIP2_MAX_NB_SYMBOLS    258
#define BZIP2_MAX_NB_GROUPS     6
#define BZIP2_MAX_NB_SELECTORS  (2 + (900000 / 50))

typedef struct
{
    UINT32  blocksize;
    UINT32  acrc;

    UINT8*  out;
    UINT32  outoff;
    UINT32  outmaxsize;

    UINT8*  in;
    UINT32  inoff;
    UINT32  insize;

    UINT32  bs_avail;
    UINT32  bs_data;
}
Bzip2Info;


static int bzip2_decompress_block(Bzip2Info* p);
static UINT32 bzip2_crc32_table[0x100];


UINT32 bzip2_decompress(UINT8* in, UINT32 insize, UINT8* out, UINT32 outmaxsize)
{
    Bzip2Info*  p;
    UINT8       bs;
    UINT32      a;
    UINT16      b;
    int         r;


    // stream header
    if(insize < 14)
        return -1;

    // file magic: 'BZh[1-9]'
    if((P32(in, 0) & 0xF0FFFFFF) != 0x30685A42)
        return -1;

    // block size (x100K)
    bs = in[3] - 0x30;
    if(bs < 1 || bs > 9)
        return -1;

    // quick check first block magic
    a = P32(in, 4);
    b = P16(in, 8);
    if(a != 0x26594131 || b != 0x5953)
    {
        if(a == 0x38457217 && b == 0x9050)
            return 0;
        else
            return -1;
    }

    // internal bzip2 data
    p = (Bzip2Info*)ALLOC(sizeof(Bzip2Info));
    p->blocksize    = bs * 100000;
    p->acrc         = 0;
    p->out          = out;
    p->outoff       = 0;
    p->outmaxsize   = outmaxsize;
    p->in           = in;
    p->inoff        = 4;
    p->insize       = insize;
    p->bs_avail     = 0;
    p->bs_data      = 0;

    // decompress the blocks
    while(1)
    {
        r = bzip2_decompress_block(p);

        if(r == -1)
            return -1;
        else if(r == 0)
            break;
    }

    // global decompressed size
    return p->outoff;
}


#define GET_BITS(v, n)\
    while(1)\
    {\
        if(p->bs_avail >= n)\
        {\
            v = (p->bs_data >> (p->bs_avail - n)) & ((1 << n) - 1);\
            p->bs_avail -= n;\
            break;\
        }\
        if(p->inoff >= p->insize)\
            goto error;\
        p->bs_data = (p->bs_data << 8) | (UINT32)p->in[p->inoff];\
        p->bs_avail += 8;\
        p->inoff++;\
    }

#define GET_BIT(v)\
    GET_BITS(v, 1)

#define GET_BYTE(v)\
    GET_BITS(v, 8)

#define EXPECT_BYTE(v)\
    GET_BYTE(b);\
    if(b != v)\
        goto error;

#define SEE_BITS(v, n)\
    GET_BITS(v, n);\
    p->bs_avail += n

#define ADD_BITS(n)\
    p->bs_avail -= n

// returns -1 if error, 1 if the block was decompressed successfully, 0 if the end of stream marker is found
static int bzip2_decompress_block(Bzip2Info* p)
{
    int     ret;
    UINT32  crc0, crc;

    UINT32  bwt_ptr;

    UINT8   h_map_sparse[0x10];
    UINT8   h_map[0x100];
    UINT8   h_real_symbols[0x100];
    UINT32  h_nb_real_symbols;
    UINT32  h_nb_symbols;

    UINT32  h_nb_groups;
    UINT32  h_nb_selectors;
    UINT8   h_selectors_mft[BZIP2_MAX_NB_SELECTORS];
    UINT8   h_selectors[BZIP2_MAX_NB_SELECTORS];

    UINT32  h_length[BZIP2_MAX_NB_GROUPS][BZIP2_MAX_NB_SYMBOLS];
    UINT32  h_length_min[BZIP2_MAX_NB_GROUPS];
    UINT32  h_length_max[BZIP2_MAX_NB_GROUPS];
    UINT32  h_order[BZIP2_MAX_NB_GROUPS][BZIP2_MAX_NB_SYMBOLS];
    UINT32  h_code[BZIP2_MAX_NB_GROUPS][BZIP2_MAX_NB_SYMBOLS];

    UINT32  i, j, k, n;
    UINT8   b;
    UINT32  tmp;
    UINT32  len, len_min, len_max;
    UINT8   pos[BZIP2_MAX_NB_GROUPS];
    UINT32* plength;
    UINT32* porder;
    UINT32  bits;
    UINT32* pcode;
    UINT32  code;
    UINT32  index;
    UINT32  rep, reppow;
    UINT32  offset, size, size2;
    UINT32  i_selector;
    int     switch_count;

    UINT32  buckets[256];
    UINT32  sum, t;
    UINT8*  src;
    UINT8*  dst = NULL;
    UINT32* indices = NULL;


    PRINT(">> Processing block\n");
    GET_BYTE(b);

    // not the block magic, BCD(pi)
    if(b != 0x31)
    {
        // not the end of stream magic, BCD(sqrt(pi)), error
        if(b != 0x17)
            goto error;

        // rest of end of stream magic
        EXPECT_BYTE(0x72);
        EXPECT_BYTE(0x45);
        EXPECT_BYTE(0x38);
        EXPECT_BYTE(0x50);
        EXPECT_BYTE(0x90);
        PRINT("End of stream detected\n");

        // stream crc
        GET_BYTE(b);
        crc0 = (UINT32)b << 24;
        GET_BYTE(b);
        crc0 |= (UINT32)b << 16;
        GET_BYTE(b);
        crc0 |= (UINT32)b << 8;
        GET_BYTE(b);
        crc0 |= (UINT32)b;

        if(p->acrc != crc0)
        {
            PRINT("Stream-CRC error: is 0x%X, should be 0x%X\n", p->acrc, crc0);
            goto error;
        }

        goto eos;
    }

    // rest of the block magic
    EXPECT_BYTE(0x41);
    EXPECT_BYTE(0x59);
    EXPECT_BYTE(0x26);
    EXPECT_BYTE(0x53);
    EXPECT_BYTE(0x59);
    PRINT("Compressed data block\n");

    // block crc
    GET_BYTE(b);
    crc0 = (UINT32)b << 24;
    GET_BYTE(b);
    crc0 |= (UINT32)b << 16;
    GET_BYTE(b);
    crc0 |= (UINT32)b << 8;
    GET_BYTE(b);
    crc0 |= (UINT32)b;

    PRINT("[+] Block info\n");

    // randomized block, deprecated and not supported
    GET_BIT(b);
    if(b == 1)
    {
        PRINT("Randomized blocks are not supported\n");
        goto error;
    }

    // starting pointer in BWT data after untransform
    GET_BITS(bwt_ptr, 24);
    PRINT("BWT pointer = %X\n", bwt_ptr);

    // Huffman sparse mapping table
    for(i = 0; i < 0x10; i++)
    {
        GET_BIT(b);
        h_map_sparse[i] = b;
    }
    // Huffman mapping table
    SET(h_map, 0x100, 0);
    for(i = 0; i < 0x10; i++)
    {
        if(h_map_sparse[i])
        {
            for(j = 0, k = i*0x10; j < 0x10; j++)
            {
                GET_BIT(b);
                h_map[k+j] = b;
            }
        }
    }
    // create the list of used symbols
    n = 0;
    for(i = 0; i < 0x100; i++)
    {
        if(h_map[i])
        {
            h_real_symbols[n] = i;
            n++;
        }
    }
    if(n == 0)
        goto error;

    h_nb_real_symbols = n;
    h_nb_symbols = n + 2; // +RUNA, +RUNB
    PRINT("%d symbols are in use (%d real symbols)\n", h_nb_symbols, h_nb_real_symbols);

    // Huffman groups
    GET_BITS(b, 3);
    if(b < 2 || b > 6)
        goto error;
    h_nb_groups = b;
    PRINT("%d groups (HT)\n", h_nb_groups);

    // used selectors
    GET_BITS(tmp, 15);
    if(tmp < 1)
        goto error;
    h_nb_selectors = tmp;
    PRINT("%d selectors\n", h_nb_selectors);


    // read MTF'ed selectors
    PRINT("[+] Reading HT selectors\n");
    PRINT("Raw: ");
    for(i = 0; i < h_nb_selectors; i++)
    {
        j = 0;
        while(1)
        {
            GET_BIT(b);
            if(b == 0)
                break;
            j++;
            if(j >= h_nb_groups)
                goto error;
        }
        h_selectors_mft[i] = j;
        PRINT("%d, ", h_selectors_mft[i]);
    }
    PRINT("\n");
    // un-MTF the selectors
    for(i = 0; i < h_nb_groups; i++)
        pos[i] = i;
    PRINT("MTF-1: ");
    for(i = 0; i < h_nb_selectors; i++)
    {
        j = h_selectors_mft[i];
        tmp = pos[j];
        while(j > 0)
        {
            pos[j] = pos[j - 1];
            j--;
        }
        pos[0] = tmp;
        h_selectors[i] = tmp;
        PRINT("%d, ", h_selectors[i]);
    }
    PRINT("\n");


    PRINT("[+] Huffman table\n");
    for(i = 0; i < h_nb_groups; i++)
    {
        PRINT("- Table #%d:\n", i);
        
        // first, read the table data
        plength = &(h_length[i][0]);
        len_max = 0;
        len_min = 21;
        
        GET_BITS(len, 5);
        for(j = 0; j < h_nb_symbols; j++)
        {
            while(1)
            {
                GET_BIT(b);
                if(b == 0)
                    break;
                GET_BIT(b);
                len += (b == 0) ? 1: -1;
            }

            if(len < 1 || len > 20)
                goto error;

            plength[j] = len;

            if(len < len_min)
                len_min = len;
            else if(len > len_max)
                len_max = len;
          
            PRINT("%d, ", len);
        }

        h_length_min[i] = len_min;
        h_length_max[i] = len_max;
        
        PRINT("code lengths: min=%d max=%d\n", len_min, len_max);

        // then, build the table
        porder  = &(h_order[i][0]);
        pcode   = &(h_code[i][0]);

        k = 0;
        for(len = len_min; len <= len_max; len++)
        {
            for(j = 0; j < h_nb_symbols; j++)
            {
                if(plength[j] == len)
                {
                    porder[k++] = j;
                    PRINT("%d, ", j);
                }
            }
        }
        PRINT("\n");

        n = -1;
        code = 0;
        for(j = 0; j < h_nb_symbols; j++)
        {
            k = porder[j];
            bits = plength[k];
            if(n != bits)
            {
                code <<= (bits - n);
                n = bits;
            }
            pcode[k] = code;
            PRINT("%d, ", code);
            code++;
        }
        PRINT("\n");

        // print table: symbol=index, bits=nb of bits, code=binary code in the compressed data
        /*PRINT("symbol bits code\n");
        for(j = 0; j < h_nb_symbols; j++)
        {
            k = porder[j];
            PRINT("%6d %4d %4d\n", k, plength[k], pcode[k]);
        }*/
    }


    PRINT("[+] Huffman decode\n");
    offset = p->outoff;
    rep = 0;
    reppow = 0;
    switch_count = 0;
    i_selector = 0;
    while(1)
    {
        // nb of characters/symbols to be processed before switching the HT
        switch_count--;

        // select a Huffman table
        if(switch_count <= 0)
        {
            if(i_selector < h_nb_selectors)
            {
                i = h_selectors[i_selector++];
                PRINT("Switching HT, now using HT #%d\n", i);

                len_min = h_length_min[i];
                len_max = h_length_max[i];
                plength = &(h_length[i][0]);
                porder  = &(h_order[i][0]);
                pcode   = &(h_code[i][0]);
            }

            switch_count = 50;
        }

        // find smallest matching code
        len = len_min;
        i = 0;
        while(1)
        {
            SEE_BITS(code, (int)len);
            while(1)
            {
                j = porder[i];
                if(plength[j] == len)
                {
                    if(pcode[j] == code)
                    {
                        index = j;
                        ADD_BITS(len);
                        goto next;
                    }
                }
                else if(plength[j] > len)
                {
                    len = plength[j];
                    break;
                }
                i++;
            }
            if(len > len_max)
                goto error;
        }

next:
        // RUNA, RUNB
        if(index == 0 || index == 1)
        {
            if(rep == 0)
                reppow = 1;
            rep += reppow << index;
            reppow *= 2;
            continue;
        }
    
        // RLE expansion
        if(rep > 0)
        {
            PRINT("RLE expansion, repeat count=%d\n", rep);
            if((offset + rep) > p->outmaxsize)
                goto error;

            b = h_real_symbols[0];
            SET(p->out + offset, rep, b);
            offset += rep;

            rep = 0;
        }
    
        // EOF
        if(index == (h_nb_symbols - 1))
        {
            PRINT("Finished\n");
            break;
        }
        // regular symbol
        else
        {
            if(offset >= p->outmaxsize)
                goto error;

            i = index - 1;

            b = h_real_symbols[i];
            p->out[offset++] = b;

            // move to front
            while(i > 0)
            {
                h_real_symbols[i] = h_real_symbols[i - 1];
                i--;
            }
            h_real_symbols[0] = b;
        }

        //Printbin(out, offset, -1);
    }
    // decoded size
    size = offset - p->outoff;


    // inverse BWT
    PRINT("[+] BWT untransform\n");
    src = p->out + p->outoff;
    dst = ALLOC(size);
    indices = ALLOC(size * sizeof(UINT32));

    SET(buckets, sizeof(buckets), 0);
    for(i = 0; i < size; i++)
    {
        indices[i] = buckets[src[i]];
        buckets[src[i]]++;
    }

    sum = 0;
    for(i = 0; i < 0x100; i++)
    {
        t = buckets[i];
        buckets[i] = sum;
        sum += t;
    }

    j = bwt_ptr;
    for(i = size - 1; i != (UINT32)-1; i--)
    {
        if(j >= size)
            goto error;

        dst[i] = src[j];
        j = buckets[src[j]] + indices[j];
    }

    // un-RLE
    PRINT("[+] Final RLE expansion\n");
    offset = p->outoff;
    size2 = size - 4;
    i = 0;
    while(i < size2)
    {
        b = dst[i];
        if(b == dst[i+1] && b == dst[i+2] && b == dst[i+3])
        {
            rep = 4 + dst[i+4];
            if((offset + rep) > p->outmaxsize)
                goto error;

            SET(p->out + offset, rep, b);
            offset += rep;
            i += 5;
        }
        else
        {
            if(offset >= p->outmaxsize)
                goto error;

            p->out[offset++] = b;
            i++;
        }
    }
    for(; i < size; i++)
    {
        if(offset >= p->outmaxsize)
            goto error;

        p->out[offset++] = dst[i];
    }

    // check block crc
    crc = -1;
    for(i = p->outoff; i < offset; i++)
        crc = (crc << 8) ^ bzip2_crc32_table[(crc >> 24) ^ ((UINT8)p->out[i])];
    crc = ~crc;
    if(crc != crc0)
    {
        PRINT("Block-CRC error: is 0x%X, should be 0x%X\n", crc, crc0);
        goto error;
    }

    // update stream crc
    p->acrc = ((p->acrc << 1) | (p->acrc >> 31)) ^ crc;

    // done!
    p->outoff = offset;
    ret = 1;
done:
    if(dst)
        FREE(dst);
    if(indices)
        FREE(indices);
    return ret;

error:
    PRINT("Error!\n");
    ret = -1;
    goto done;

eos:
    ret = 0;
    goto done;
}


static UINT32 bzip2_crc32_table[0x100] =
{
    0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
    0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
    0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
    0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
    0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9,
    0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
    0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
    0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
    0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
    0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
    0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81,
    0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
    0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49,
    0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
    0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1,
    0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,
    0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE,
    0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
    0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
    0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA,
    0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE,
    0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
    0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
    0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
    0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E,
    0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
    0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6,
    0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,
    0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
    0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
    0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686,
    0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,
    0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637,
    0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
    0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F,
    0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
    0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
    0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
    0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
    0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,
    0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7,
    0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
    0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F,
    0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
    0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7,
    0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
    0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F,
    0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
    0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
    0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
    0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
    0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,
    0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30,
    0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
    0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088,
    0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,
    0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
    0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
    0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
    0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
    0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0,
    0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
    0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,
    0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4,
};
