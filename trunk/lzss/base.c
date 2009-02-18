#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "base.h"



//
// process termination routine
//

void terminate(void)
{
    exit(0);
}


//
// file manipulation (class-like)
//

typedef struct
{
    FILE*   fp;
    UINT32  size;
}
FILEHANDLE_;

FILEHANDLE* file_open(char* filename)
{
    FILEHANDLE_* f;
    FILE* fp;

    fp = fopen(filename, "rb");
    if(fp == NULL)
        return NULL;

    f = (FILEHANDLE_*)memory_alloc(sizeof(FILEHANDLE_));
    f->fp = fp;

    fseek(fp, 0, SEEK_END);
    f->size = ftell(fp);
    rewind(fp);

    return (FILEHANDLE*)f;
}

void file_close(FILEHANDLE* f)
{
    fclose(((FILEHANDLE_*)f)->fp);
    memory_free((FILEHANDLE_*)f);
}

int file_size(FILEHANDLE* f)
{
    return ((FILEHANDLE_*)f)->size;
}

int file_read(FILEHANDLE* f, void* buffer, int size)
{
    return (int)fread(buffer, 1, size, ((FILEHANDLE_*)f)->fp);
}

int file_seek(FILEHANDLE* f, int offset, UINT32 method)
{
    switch(method)
    {
    case 0: return (fseek(((FILEHANDLE_*)f)->fp, offset, SEEK_SET) ? 0: 1);
    case 1: return (fseek(((FILEHANDLE_*)f)->fp, offset, SEEK_CUR) ? 0: 1);
    case 2: return (fseek(((FILEHANDLE_*)f)->fp, offset, SEEK_END) ? 0: 1);
    default: return 0;
    }
}

int file_tell(FILEHANDLE* f)
{
    return ftell(((FILEHANDLE_*)f)->fp);
}

int file_seekread(FILEHANDLE* f, int offset, void* buffer, int size)
{
    if(fseek(((FILEHANDLE_*)f)->fp, offset, SEEK_SET) != 0)
        return 0;

    return (int)fread(buffer, 1, size, ((FILEHANDLE_*)f)->fp);
}


//
// file full-read and dump routines
//

int file_get(char* filename, UINT8** ppdata, UINT32* psize)
{
    FILE* fp = fopen(filename, "rb");
    if(fp == NULL)
        return 0;

    fseek(fp, 0, SEEK_END);
    *psize = ftell(fp);
    rewind(fp);

    *ppdata = (UINT8*)memory_alloc(*psize);
    fread(*ppdata, *psize, 1, fp);
    fclose(fp);

    return 1;
}

int file_put(char* filename, UINT8* pdata, UINT32 size)
{
    FILE* fp = fopen(filename, "wb");
    if(fp == NULL)
        return 0;

    fwrite(pdata, size, 1, fp);
    fclose(fp);

    return 1;
}


//
// standard malloc/free wrappers
//

PVOID memory_alloc(UINT32 size)
{
    PVOID p = malloc(size);
    if(!p)
        //terminate("Allocation error\n");
        terminate();

    return p;
}

PVOID memory_alloc_zero(UINT32 size)
{
    PVOID p = calloc(1, size);
    if(!p)
        //terminate("Allocation error\n");
        terminate();

    return p;
}

void memory_free(PVOID p)
{
    if(!p)
        return;

    free(p);
}

PVOID memory_realloc(PVOID p, UINT32 size)
{
    PVOID p2 = realloc(p, size);
    if(!p2)
        //terminate("Allocation error\n");
        terminate();

    return p2;
}


//
// memory/string manipulation routines
//

void memory_set(void* buffer, unsigned int size, unsigned char value)
{
    memset(buffer, value, size);
}

void memory_copy(void* dst, void* src, unsigned int size)
{
    memcpy(dst, src, size);
}

int memory_compare(void* p1, void* p2, unsigned int size)
{
    return memcmp(p1, p2, size);
}

int memory_find(PVOID buffer, UINT32 size, UINT8 c)
{
    UINT32 i;
    for(i = 0; i < (int)size && P8(buffer, i) != c; i++);
    return ((i >= size) ? -1: (int)i);
}

void string_copy(char* dst, char* src, int dstmaxsize)
{
    if(dstmaxsize > 0)
        strncpy(dst, src, dstmaxsize);
}

void string_cat(char* dst, char* src, int dstmaxsize)
{
    if(dstmaxsize > 0)
        strncat(dst, src, dstmaxsize);
}

int string_length(char* s)
{
    return (int)strlen(s);
}

int string_length_wide(wchar* s)
{
    return (int)wcslen((wchar_t*)s);
}

int string_length_safe(char* s, int maxsize)
{
    int i;
    for(i = 0; i < maxsize && s[i]; i++);
    return i;
}

int string_compare(char* s1, char* s2)
{
    return strcmp(s1, s2);
}

int string_icompare(char* s1, char* s2)
{
    return stricmp(s1, s2);
}

int string_ncompare(char* s1, char* s2, UINT32 n)
{
    return strncmp(s1, s2, n);
}

int string_nicompare(char* s1, char* s2, UINT32 n)
{
    return strnicmp(s1, s2, n);
}

char* string_lower(char* s)
{
    return strlwr(s);
}

void format(char* dst, int dstmaxsize, char* format, ...)
{
    va_list ap;

    va_start(ap, format);
    _vsnprintf(dst, dstmaxsize, format, ap);
}

void formatcat(char* dst, int dstmaxsize, char* format, ...)
{
    va_list ap;
    int     n;

    va_start(ap, format);

    n = (int)strlen(dst);
    if(n >= dstmaxsize)
        return;

    _vsnprintf(dst + n, dstmaxsize - n, format, ap);
}

void formatbin(char* dst, int dstmaxsize, PVOID data, int datasize)
{
    int i;

    if(dstmaxsize <= 0)
        return;

    dst[0] = 0;
    for(i = 0; i < datasize; i++)
        formatcat(dst, dstmaxsize, "%02X ", P8(data, i));
}


//
// printf-like routine (to stdout)
//

void print(char* format, ...)
{
    va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
}

void printbin(unsigned char* buffer, unsigned int size, unsigned int address)
{
    unsigned int    i, j;


    for(i = 0; i < size; i +=16)
    {
        if(address != 0xFFFFFFFF)
            print("%08X   ", address + i);

        for(j = i; j < (i + 16) && j < size; j++)
            print("%02x ", buffer[j]);

        for(;j % 16; j++)
            print("   ");

        print("   ");
        for(j = i; j < (i + 16) && j < size; j++)
        {
            if(buffer[j] >= 0x20 && buffer[j] <= 0x7E)
                print("%c", buffer[j]);
            else
                print(".");
        }

        print("\n");
    }
}


//
// standard CRC-32
//

unsigned int crc32_table[0x100];

void crc32_init(void)
{
    unsigned int    i, j;
    unsigned int    h = 1;
    
    
    crc32_table[0] = 0;
    for(i = 0x80; i != 0; i >>= 1)
    {
        h = (h >> 1) ^ ((h & 1) ? 0xEDB88320: 0);

        for (j = 0; j < 0x100; j += 2*i)
        {
            crc32_table[i+j] = crc32_table[j] ^ h;
        }
    }
}


unsigned int crc32_buffer(unsigned char* buf, unsigned int len)
{
    unsigned int    crc = 0xFFFFFFFF;

    while(len--)
    {
        crc = (crc >> 8) ^ crc32_table[(crc ^ *buf++) & 0xFF];
    }

    return (crc ^ 0xFFFFFFFF);
}

