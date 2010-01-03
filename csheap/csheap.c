//
// CSHEAP -- customizable dynamic memory allocator in C
// Nicolas Falliere
//


#include "csheap.h"
#include <assert.h>


#ifdef WIN32

#   include <windows.h>

#   define  GRANULARITY     8
#   define  CHUNKHDR_SIZE   8
#   define  EMPTYHDR_SIZE   0x10
#   define  PAGESIZE        0x1000
#   define  ROUND(s)        (((s) + 0xFFF) & 0xFFFFF000)

#   define  vmem_reserve(address, size, allowexec)  VirtualAlloc(address, size, MEM_RESERVE, allowexec ? PAGE_EXECUTE_READWRITE: PAGE_READWRITE)
#   define  vmem_commit(address, size, allowexec)   VirtualAlloc(address, size, MEM_COMMIT, allowexec ? PAGE_EXECUTE_READWRITE: PAGE_READWRITE)
#   define  vmem_free(address, size)                VirtualFree(address, 0, MEM_RELEASE)
#   define  rand8()                                 (GetTickCount() % 0x100)
#   define  error()                                 RaiseException(-1, EXCEPTION_NONCONTINUABLE, 0, NULL)
#   define  mutex_create()                          (void*)CreateMutex(NULL, FALSE, NULL)
#   define  mutex_destroy(m)                        CloseHandle((HANDLE)(m))
#   define  mutex_acquire(m)                        (WaitForSingleObject((HANDLE)(m), INFINITE) == WAIT_OBJECT_0)
#   define  mutex_release(m)                        ReleaseMutex((HANDLE)(m))

#endif


// private prototypes
void csheap_insert_chunk(CSHEAP* heap, CHUNKHDR* hdr);


// macros
#if CSHEAP_ENABLE_COOKIES
#   define CHECK_COOKIE(hdr)    { if((hdr)->cookie != heap->cookie) error(); }
#else
#   define CHECK_COOKIE(hdr)    __noop
#endif

#if CSHEAP_ENABLE_MULTITHREAD
#   define HEAP_LOCK()          { if(!mutex_acquire(heap->mutex)) error(); }
#   define HEAP_UNLOCK()        { mutex_release(heap->mutex); }
#else
#   define HEAP_LOCK()          __noop
#   define HEAP_UNLOCK()        __noop
#endif

#if !CSHEAP_ENABLE_SAFEUNLINK
#define DLIST_UNLINK(p){\
    (p)->prev->next = (p)->next;\
    (p)->next->prev = (p)->prev;\
}
#else
#define DLIST_UNLINK(p){\
    if((p)->prev->next != p || (p)->next->prev != p)\
        error();\
    (p)->prev->next = (p)->next;\
    (p)->next->prev = (p)->prev;\
}
#endif

#if !CSHEAP_ENABLE_SAFEUNLINK
#define DLIST_INSERT_AFTER(p0, p){\
    (p)->next = (p0)->next;\
    (p)->prev = p0;\
    (p0)->next = p;\
    (p)->next->prev = p;\
}
#else
#define DLIST_INSERT_AFTER(p0, p){\
    if((p0)->next->prev != (p0))\
        error();\
    (p)->next = (p0)->next;\
    (p)->prev = p0;\
    (p0)->next = p;\
    (p)->next->prev = p;\
}
#endif

#if !CSHEAP_ENABLE_SAFEUNLINK
#define DLIST_INSERT_BEFORE(p0, p){\
    (p)->next = p0;\
    (p)->prev = (p0)->prev;\
    (p0)->prev = p;\
    (p)->prev->next = p;\
}
#else
#define DLIST_INSERT_BEFORE(p0, p){\
    if((p0)->prev->next != (p0))\
        error();\
    (p)->next = p0;\
    (p)->prev = (p0)->prev;\
    (p0)->prev = p;\
    (p)->prev->next = p;\
}
#endif



CSHEAP* csheap_create(unsigned int size, unsigned int maxsize)
{
    void*       p;
    CSHEAP*   heap;


    if(size == 0)
        size = PAGESIZE;
    else
        size = ROUND(size);

    if(maxsize == 0)
        maxsize = size;
    else if(maxsize < size)
        goto error;
    else
        maxsize = ROUND(maxsize);

    p = vmem_reserve(NULL, maxsize, CSHEAP_ENABLE_EXECUTE);
    if(!p)
        goto error;

    if(vmem_commit(p, size, CSHEAP_ENABLE_EXECUTE) != p)
    {
        vmem_free(p, 0);
        goto error;
    }

    heap = (CSHEAP*)p;

    heap->size = size;
    heap->maxsize = maxsize;

#if CSHEAP_ENABLE_MULTITHREAD
    heap->mutex = mutex_create();
#endif

    csheap_reset(heap);
    return heap;

error:
#if CSHEAP_ENABLE_RAISEONERROR
    error();
#endif
    return NULL;
}


void csheap_destroy(CSHEAP* heap)
{
#if CSHEAP_ENABLE_MULTITHREAD
    void*   mutex;
#endif

    HEAP_LOCK();

#if CSHEAP_ENABLE_MULTITHREAD
    mutex = heap->mutex;
#endif

    vmem_free(heap, 0);

#if CSHEAP_ENABLE_MULTITHREAD
    mutex_destroy(mutex);
#endif
}


void csheap_reset(CSHEAP* heap)
{
    unsigned int    i;


    HEAP_LOCK();

    heap->tailoffset = sizeof(CSHEAP);
    heap->lastsizeindex = 0;

    for(i = 0; i < CSHEAP_NB_FREELISTS; i++)
    {
        heap->freelists[i].next = &(heap->freelists[i]);
        heap->freelists[i].prev = &(heap->freelists[i]);
    }

#if CSHEAP_ENABLE_STATS
    heap->metasize = sizeof(CSHEAP);
    heap->allocsize = 0;
    heap->freesize = 0;
    heap->alloccount = 0;
    heap->freecount = 0;
#endif

#if CSHEAP_ENABLE_COOKIES
    heap->cookie = rand8();
#endif

    HEAP_UNLOCK();
}


void* csheap_alloc(CSHEAP* heap, unsigned int size, int zerofill)
{
    unsigned int    i;    
    unsigned int    sizeindex;
    unsigned int    realsize;
    unsigned int    availsize;
    void*           p;
    DLIST*          plist;
    CHUNKHDR*       hdr;


    HEAP_LOCK();

    if(size == 0)
        goto error;

    sizeindex = (size + GRANULARITY - 1) / GRANULARITY;
    size = sizeindex * GRANULARITY;
    realsize = size + CHUNKHDR_SIZE;

    // chunk in regular lists
    for(i = sizeindex; i < CSHEAP_NB_FREELISTS; i++)
    {
        if(heap->freelists[i].next != &(heap->freelists[i]))
        {
            plist = heap->freelists[i].next;
            hdr = (CHUNKHDR*)plist - 1;

            goto unlink_chunk;
        }
    }

    // check in special list 0
    if(heap->freelists[0].next != &(heap->freelists[0]))
    {
        plist = heap->freelists[0].next;

        while(plist != &(heap->freelists[0]))
        {
            hdr = (CHUNKHDR*)plist - 1;

            if(hdr->sizeindex >= sizeindex)
                goto unlink_chunk;

            plist = plist->next;
        }
    }

    // nothing in list 0 either, create a chunk
    goto create_chunk;
        
unlink_chunk:
    p = (void*)plist;

    CHECK_COOKIE(hdr);
    
    DLIST_UNLINK(plist);

    // split the chunk 'hdr'
    if(((hdr->sizeindex - sizeindex) * GRANULARITY) >= EMPTYHDR_SIZE)
    {
        unsigned int sizeindex1 = hdr->sizeindex - sizeindex - (CHUNKHDR_SIZE / GRANULARITY);

        CHUNKHDR* hdr1 = (CHUNKHDR*)((unsigned char*)p + size);
        DLIST* plist1 = (DLIST*)(hdr1 + 1);

        hdr->sizeindex = sizeindex;

        hdr1->prevsizeindex = sizeindex;
        hdr1->sizeindex = sizeindex1;
        hdr1->busy = 0;
#if CSHEAP_ENABLE_COOKIES
        hdr1->cookie = heap->cookie;
#endif

        if(hdr->lastchunk)
        {
            hdr->lastchunk = 0;
            heap->lastsizeindex = sizeindex1;
            
            hdr1->lastchunk = 1;
        }
        else
        {
            CHUNKHDR* hdr2 = (CHUNKHDR*)plist1 + sizeindex1;
            hdr2->prevsizeindex = sizeindex1;

            hdr1->lastchunk = 0;
        }

        // insert the chunk in a freelist
        csheap_insert_chunk(heap, hdr1);

#if CSHEAP_ENABLE_STATS
        heap->metasize += CHUNKHDR_SIZE;
        heap->freesize -= CHUNKHDR_SIZE;
        heap->freecount++;
#endif
    }

#if CSHEAP_ENABLE_STATS
    heap->freesize -= hdr->sizeindex * GRANULARITY;
    heap->freecount--;
#endif

    assert(!hdr->busy);
    goto done;

create_chunk:

    // freelist checks failed: try to create a new chunk
    availsize = heap->size - heap->tailoffset;

    // not enough free space, we need to extend the heap
    if(availsize < realsize)
    {
        unsigned int maxavailsize = heap->maxsize - heap->tailoffset;
        unsigned int s;
        void* p2;

        // the heap cannot be extended further, fail
        if(maxavailsize < realsize)
            goto error;

        // allocate some space
        s = realsize - availsize;
        s = ROUND(s);
        p2 = vmem_commit((unsigned char*)heap + heap->size, s, CSHEAP_ENABLE_EXECUTE);
        if(!p2)
            goto error;

        // the heap was extended successfully
        heap->size += s;

        // update the available size
        availsize = heap->size - heap->tailoffset;
    }

    // create a new chunk
    hdr = (CHUNKHDR*)((unsigned char*)heap + heap->tailoffset);
    hdr->sizeindex = sizeindex;
    hdr->prevsizeindex = heap->lastsizeindex;
    hdr->lastchunk = 1;
#if CSHEAP_ENABLE_COOKIES
    hdr->cookie = heap->cookie;
#endif
    if(hdr->prevsizeindex > 0)
    {
        CHUNKHDR* hdr0 = (CHUNKHDR*)((unsigned char*)hdr - (hdr->prevsizeindex * GRANULARITY) - CHUNKHDR_SIZE);

        assert(hdr0->lastchunk);
        assert(hdr0->sizeindex == hdr->prevsizeindex);

        hdr0->lastchunk = 0;
    }

    heap->lastsizeindex = sizeindex;
    heap->tailoffset += realsize;

    p = (CHUNKHDR*)hdr + 1;

#if CSHEAP_ENABLE_STATS
    heap->metasize += CHUNKHDR_SIZE;
#endif

done:
    assert(!hdr->lastchunk || hdr->sizeindex == heap->lastsizeindex);
    hdr->busy = 1;

#if CSHEAP_ENABLE_STATS
    heap->allocsize += hdr->sizeindex * GRANULARITY;
    heap->alloccount++;
#endif

    if(zerofill)
        memset(p, 0, size);

    HEAP_UNLOCK();
    return p;

error:
    HEAP_UNLOCK();
#if CSHEAP_ENABLE_RAISEONERROR
    error();
#endif
    return NULL;
}


void csheap_free(CSHEAP* heap, void* p)
{
    CHUNKHDR*       hdr;
    CHUNKHDR*       hdr1;
    unsigned int    freesize;


    HEAP_LOCK();

    hdr = (CHUNKHDR*)p - 1;
    CHECK_COOKIE(hdr);

    assert(hdr->busy);

    hdr->busy = 0;
    freesize = hdr->sizeindex * GRANULARITY;

    // coalesce adjacent free chunks
    hdr1 = (CHUNKHDR*)((unsigned char*)p + (hdr->sizeindex * GRANULARITY)); // potential next adjacent chunk

    // coalesce with the previous free chunk, if any
    if(hdr->prevsizeindex > 0)
    {
        CHUNKHDR* hdr0 = (CHUNKHDR*)((unsigned char*)hdr - (hdr->prevsizeindex * GRANULARITY) - CHUNKHDR_SIZE);

        assert(hdr0->sizeindex == hdr->prevsizeindex);
        assert(!hdr0->lastchunk);

        // if the previous chunk is free, we can coalesce
        if(!(hdr0->busy))
        {
            DLIST* plist0 = (DLIST*)(hdr0 + 1);

            CHECK_COOKIE(hdr0);

            DLIST_UNLINK(plist0);

            // update the previous chunk size and flags
            hdr0->lastchunk = hdr->lastchunk;
            hdr0->sizeindex += hdr->sizeindex + (CHUNKHDR_SIZE / GRANULARITY);

            // update the next chunk prevsize
            if(!(hdr->lastchunk))
            {
                assert(hdr1->prevsizeindex == hdr->sizeindex);

                hdr1->prevsizeindex = hdr0->sizeindex;
            }
            else
            {
                heap->lastsizeindex = hdr0->sizeindex;
            }

            // the previous chunk is now the current chunk
            hdr = hdr0;

#if CSHEAP_ENABLE_STATS
            heap->metasize -= CHUNKHDR_SIZE;
            heap->freesize += CHUNKHDR_SIZE;
            heap->freecount--;
#endif
        }
    }

    // coalesce with the next free chunk, if any
    if(!(hdr->lastchunk))
    {
        // if the next chunk is busy, we can't coalesce
        if(!(hdr1->busy))
        {
            DLIST* plist1 = (DLIST*)(hdr1 + 1);

            CHECK_COOKIE(hdr1);

            DLIST_UNLINK(plist1);

            hdr->sizeindex += hdr1->sizeindex + (CHUNKHDR_SIZE / GRANULARITY);

            if(hdr1->lastchunk)
            {
                hdr->lastchunk = 1;

                heap->lastsizeindex = hdr->sizeindex;
            }
            else
            {
                CHUNKHDR* hdr2 = (CHUNKHDR*)((unsigned char*)hdr1 + (hdr1->sizeindex * GRANULARITY) + CHUNKHDR_SIZE);
                hdr2->prevsizeindex = hdr->sizeindex;
            }

#if CSHEAP_ENABLE_STATS
            heap->metasize -= CHUNKHDR_SIZE;
            heap->freesize += CHUNKHDR_SIZE;
            heap->freecount--;
#endif
        }
    }

    // insert the free chunk in its appropriate freelist
    csheap_insert_chunk(heap, hdr);

#if CSHEAP_ENABLE_STATS
    heap->allocsize -= freesize;
    heap->alloccount--;
    heap->freesize += freesize;
    heap->freecount++;
#endif

    HEAP_UNLOCK();
}


unsigned int csheap_size(CSHEAP* heap, void* p)
{
    CHUNKHDR*       hdr0;
    unsigned int    size0;


    HEAP_LOCK();

    hdr0 = (CHUNKHDR*)p - 1;
    size0 = hdr0->sizeindex * GRANULARITY;

    HEAP_UNLOCK();

    return size0;
}


void* csheap_realloc(CSHEAP* heap, void* p, unsigned int size)
{
    CHUNKHDR*       hdr0;
    unsigned int    size0;
    void*           p1;


    HEAP_LOCK();

    hdr0 = (CHUNKHDR*)p - 1;
    size0 = hdr0->sizeindex * GRANULARITY;

    if(size < size0)
    {
#if CSHEAP_ENABLE_RAISEONERROR
        error();
#endif
        return NULL;
    }
    else if(size == size0)
        return p;

    p1 = csheap_alloc(heap, size, 0);

    if(p1)
    {
        memcpy(p1, p, size0);
        csheap_free(heap, p);
    }

    HEAP_UNLOCK();
    return p1;
}


void csheap_validate(CSHEAP* heap)
{
    CHUNKHDR*       hdr;
    unsigned int    prevsizeindex = 0;
    unsigned int    offset = sizeof(CSHEAP);
    int             lastchunkfound = 0;
#if CSHEAP_ENABLE_STATS
    unsigned int    metasize = sizeof(CSHEAP);
    unsigned int    allocsize = 0;
    unsigned int    freesize = 0;
    unsigned int    alloccount = 0;
    unsigned int    freecount = 0;
#endif


    HEAP_LOCK();

    hdr = (CHUNKHDR*)((unsigned char*)heap + offset);

    while(offset != heap->tailoffset)
    {
        if(lastchunkfound
        || hdr->prevsizeindex != prevsizeindex)
            error();

        if(hdr->lastchunk)
            lastchunkfound = 1;

        prevsizeindex = hdr->sizeindex;

#if CSHEAP_ENABLE_STATS
        if(hdr->busy)
        {
            allocsize += prevsizeindex * GRANULARITY;
            alloccount++;
        }
        else
        {
            freesize += prevsizeindex * GRANULARITY;
            freecount++;
        }
        metasize += CHUNKHDR_SIZE;
#endif

        offset += CHUNKHDR_SIZE + prevsizeindex * GRANULARITY;
        hdr = (CHUNKHDR*)((unsigned char*)heap + offset);
    }

    if(!lastchunkfound)
        error();

#if CSHEAP_ENABLE_STATS
    if(metasize != heap->metasize
    || allocsize != heap->allocsize
    || freesize != heap->freesize
    || alloccount != heap->alloccount
    || freecount != heap->freecount)
        error();
#endif

    HEAP_UNLOCK();
}



//
// PRIVATE
//

void csheap_insert_chunk(CSHEAP* heap, CHUNKHDR* hdr)
{
    DLIST* plist = (DLIST*)(hdr + 1);

    // inserted into the regular lists
    if(hdr->sizeindex < CSHEAP_NB_FREELISTS)
    {
        DLIST_INSERT_AFTER(&heap->freelists[hdr->sizeindex], plist);
    }
    else
    {
        // big chunk, goes in list 0, sorted by ascending size
        DLIST* plist0 = heap->freelists[0].next;
        CHUNKHDR* hdr0;

        while(plist0 != &(heap->freelists[0]))
        {
            hdr0 = (CHUNKHDR*)(plist0 - 1);
            
            if(hdr0->sizeindex >= hdr->sizeindex)
                break;

            plist0 = plist0->next;
        }

        DLIST_INSERT_BEFORE(plist0, plist);
    }
}
