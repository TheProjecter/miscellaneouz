#ifndef __BASE_H__
#define __BASE_H__


//----------------------------------------------------------------------------
// Types
#include "types.h"
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Macros
#define P8(x, o)                *((UINT8*)((UINT8*)x + (o)))
#define P16(x, o)               *((UINT16*)((UINT8*)x + (o)))
#define P32(x, o)               *((UINT32*)((UINT8*)x + (o)))
#define P64(x, o)               *((UINT64*)((UINT8*)x + (o)))
#define PVAR(x, o, s)           ((s) == 8) ? P8(x, o): ((s) == 32) ? P32(x, o): ((s) == 16) ? P16(x, o): 0

#define OFFSETOF(s, f)          ((UINT32)&(((s*)0)->f))

#define INRANGE(a, min, max)    (a >= (min) && a <= (max))
#define AINRANGE(a, min, max)   (a >= (min) && a < (max))
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Prototypes
void terminate(void);

typedef void FILEHANDLE;
FILEHANDLE* file_open(char* filename);
void file_close(FILEHANDLE* f);
int file_size(FILEHANDLE* f);
int file_read(FILEHANDLE* f, void* buffer, int size);
int file_seek(FILEHANDLE* f, int offset, UINT32 method);
int file_tell(FILEHANDLE* f);
int file_seekread(FILEHANDLE* f, int offset, void* buffer, int size);

int file_get(char* filename, UINT8** ppdata, UINT32* psize);
int file_put(char* filename, UINT8* pdata, UINT32 size);

PVOID memory_alloc(UINT32 size);
PVOID memory_alloc_zero(UINT32 size);
void memory_free(PVOID p);
PVOID memory_realloc(PVOID p, UINT32 size);

void memory_set(void* buffer, unsigned int size, unsigned char value);
void memory_copy(void* dst, void* src, unsigned int size);
int memory_compare(void* p1, void* p2, unsigned int size);
int memory_find(PVOID buffer, UINT32 size, UINT8 c);
void string_copy(char* dst, char* src, int dstmaxsize);
void string_cat(char* dst, char* src, int dstmaxsize);
int string_length(char* s);
int string_length_wide(wchar* s);
int string_length_safe(char* s, int maxsize);
int string_compare(char* s1, char* s2);
int string_icompare(char* s1, char* s2);
int string_ncompare(char* s1, char* s2, UINT32 n);
int string_nicompare(char* s1, char* s2, UINT32 n);
char* string_lower(char* s);
void format(char* dst, int dstmaxsize, char* format, ...);
void formatcat(char* dst, int dstmaxsize, char* format, ...);
void formatbin(char* dst, int dstmaxsize, PVOID data, int datasize);

void print(char* format, ...);
void printbin(unsigned char* buffer, unsigned int size, unsigned int address);

extern unsigned int crc32_table[0x100];
void crc32_init(void);
unsigned int crc32_buffer(unsigned char* buf, unsigned int len);
//unsigned int crc32(unsigned int crc, unsigned char c);
//----------------------------------------------------------------------------


#endif