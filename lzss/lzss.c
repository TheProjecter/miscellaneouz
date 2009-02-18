#include <stdio.h>



#define OFFBITS     12   // nb of bits to code the offset
#define OFFMIN      1
#define WINSIZE     (1 << OFFBITS)

#define LENBITS     4   // nb of bits to code the length
#define LENMIN      3
#define LENMAX      (LENMIN + (1 << LENBITS) - 1)

unsigned int lzss_compress(unsigned char* in, unsigned int insize, unsigned char* out, unsigned int outmaxsize)
{
    unsigned char*  src;
    unsigned char*  dst;
    unsigned char*  dstmax;
    
    unsigned int    sleft, outsize;
    int             winsize, s;
    int             bm_length, bm_offset;
    int             i, j;

    unsigned char*  hdr;
    unsigned char   flags;
    int             cnt;
    

    if(insize == 0)
        return 0;
    if(outmaxsize == 0)
        return -1;

    winsize = 0;
    src = in;
    dst = out;
    dstmax = out + outmaxsize;
    sleft = insize;

    //hdr = dst++;
    //flags = 0;
    //cnt = 0;
    goto start;

    while(sleft > 0)
    {
        // find longest match
        bm_length = LENMIN - 1;
        bm_offset = 0;

        s = (sleft >= LENMAX) ? LENMAX: (int)sleft;

        for(i = 1; i <= winsize; i++)
        {
            for(j = 0; j < s; j++)
            {
                if(src[j] != src[-i+j])
                {
                    if(j > bm_length)
                    {
                        bm_offset = i;
                        bm_length = j;
                    }
                    break;
                }
            }
        }

        // no good match
        if(bm_length < LENMIN)
        {
            bm_length = 1;

            if(dst >= dstmax)
                goto error;

            *dst++ = *src;
            //printf("%c", *src);
        }
        else
        {
            unsigned short  v;

            flags |= 1 << cnt;

            if((dst + 1) >= dstmax)
                goto error;

            v = ((bm_offset - OFFMIN) << LENBITS) | (bm_length - LENMIN);
            *((unsigned short*)dst) = v;
            dst += 2;
            //printf("\n[[ len=%d, off=%d ]]\n", bm_length, bm_offset);
        }

        src += bm_length;
        sleft -= bm_length;

        if(winsize < WINSIZE)
        {
            winsize += bm_length;
            if(winsize > WINSIZE)
                winsize = WINSIZE;
        }

        if(++cnt == 8)
        {
            *hdr = flags;
start:
            hdr = dst++;
            flags = 0;
            cnt = 0;
        }
    }
    if(cnt)
        *hdr = flags;

    outsize = (unsigned int)(dst - out);
    return outsize;

error:
    return -1;
}


unsigned int lzss_decompress(unsigned char* in, unsigned int insize, unsigned char* out, unsigned int outmaxsize)
{
    unsigned char*  src;
    unsigned char*  srcmax;
    unsigned char*  dst;
    unsigned char*  dstmax;
    unsigned int    outsize;
    int             length, offset;
    int             i;
    unsigned char   flags;
    int             cnt;


    if(insize == 0)
        return 0;
    if(outmaxsize == 0)
        return -1;

    src = in;
    srcmax = in + insize;
    dst = out;
    dstmax = out + outmaxsize;

    //flags = *src++;
    //cnt = 0;
    goto start;

    while(dst < dstmax)
    {
        if(!(flags & 1))
        {
            if(src >= srcmax)
                goto error;

            *dst++ = *src;
            src++;
        }
        else
        {
            unsigned short v;

            if((src + 1) >= srcmax)
                break;

            v = *((unsigned short*)src);
            src += 2;
            
            offset = (v >> 4) + OFFMIN;
            length = (v & 0xF) + LENMIN;

            // not enough data decoded
            if(offset > (dst - out))
                goto error;

            // output buffer is too small
            if((dstmax - dst) < length)
                goto error;

            for(i = 0; i < length; i++)
                dst[i] = dst[i-offset];
            dst += length;
        }

        flags >>= 1;

        if(++cnt == 8)
        {
            if(src >= srcmax)
                break;
start:
            flags = *src++;
            cnt = 0;
        }
    }

    outsize = (unsigned int)(dst - out);
    return outsize;

error:
    return -1;
}
