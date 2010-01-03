//
// CSHEAP -- customizable dynamic memory allocator in C
// Nicolas Falliere
//


#ifndef __CSHEAP_H__
#define __CSHEAP_H__


// options (0: disabled, 1: enabled)
#define CSHEAP_ENABLE_COOKIES       1   // chunk cookies set and checked before operations (overflows detection)
#define CSHEAP_ENABLE_SAFEUNLINK    1   // extra checks on pointer validity during chunk unlinking (overflows detection)
#define CSHEAP_ENABLE_STATS         1   // extra counters to profile the heap's usage
#define CSHEAP_ENABLE_MULTITHREAD   0   // allows concurrent threads to access a same heap safely
#define CSHEAP_ENABLE_RAISEONERROR  1   // raise an exception on heap creation/alloc/free errors
#define CSHEAP_ENABLE_EXECUTE       0   // heap chunks can contain executable code


#pragma pack (push,1)

typedef struct _DLIST
{
    struct _DLIST*  next;
    struct _DLIST*  prev;
}
DLIST;

#define CSHEAP_NB_FREELISTS 128

typedef struct _CSHEAP
{
    unsigned int    size;
    unsigned int    maxsize;
    unsigned int    tailoffset;
    unsigned int    lastsizeindex;
    DLIST           freelists[CSHEAP_NB_FREELISTS];

#if CSHEAP_ENABLE_STATS
    unsigned int    metasize;
    unsigned int    allocsize;
    unsigned int    freesize;
    unsigned int    alloccount;
    unsigned int    freecount;
#endif

#if CSHEAP_ENABLE_COOKIES
    unsigned int    cookie;
#endif

#if CSHEAP_ENABLE_MULTITHREAD
    void*           mutex;
#endif
}
CSHEAP;

typedef struct _CHUNKHDR
{
    unsigned int    sizeindex:24;
    unsigned int    busy:1;
    unsigned int    lastchunk:1;
    unsigned int    unused:6;
    unsigned int    prevsizeindex:24;
    unsigned int    cookie:8;
}
CHUNKHDR;

#pragma pack (pop)

// public prototypes
CSHEAP* csheap_create(unsigned int size, unsigned int maxsize);
void csheap_reset(CSHEAP* heap);
void csheap_destroy(CSHEAP* heap);
void* csheap_alloc(CSHEAP* heap, unsigned int size, int zerofill);
void csheap_free(CSHEAP* heap, void* p);
unsigned int csheap_size(CSHEAP* heap, void* p);
void* csheap_realloc(CSHEAP* heap, void* p, unsigned int size);
void csheap_validate(CSHEAP* heap);


#endif