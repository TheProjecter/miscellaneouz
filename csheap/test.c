//
// CSHEAP -- customizable dynamic memory allocator in C
// Nicolas Falliere
//


#include "csheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



void**  addrarray       = NULL;
int     addrcount       = 0;
int     addrmaxcount    = 0;


int address_add(void* address)
{
    if(!addrarray)
    {
        addrarray = calloc(0x100, sizeof(void*));
        if(!addrarray)
            return 0;
        addrmaxcount = 0x100;
    }
    else if(addrcount == addrmaxcount)
    {
        addrarray = realloc(addrarray, (addrmaxcount + 0x100) * sizeof(void*));
        if(!addrarray)
            return 0;
        addrmaxcount += 0x100;
    }

    addrarray[addrcount] = address;
    addrcount++;
    return 1;
}


void address_remove(int i)
{
    addrarray[i] = addrarray[addrcount - 1];
    addrcount--;
}


int main(void)
{
    CSHEAP*         heap;
    int             opcount;
    unsigned int    seed;
    int             i;
    unsigned int    size;
    void*           p;

    
    heap = csheap_create(0x1000, 0x10000);
    if(!heap)
        return -1;

    // test the cookies
#if 0
    {
    void* p1;
    void* p2;
    p1 = csheap_alloc(heap, 0x10, 0);
    p2 = csheap_alloc(heap, 0x20, 0);
    memset(p1, 0x90, 0x20);
    csheap_free(heap, p2);
    }
#endif

    seed = (unsigned int)time(NULL);
    //seed = 0x499018A3;
    srand(seed);

    for(opcount = 0; opcount < 10000; opcount++)
    {
        printf("%d: ", opcount);

        //if(opcount == 0)
        //    opcount += 1 - 1;

        switch(rand() % 3)
        {
        case 0:
            size = 1 + (rand() % 0x1FF);
            printf("Alloc\n", size);
            p = csheap_alloc(heap, size, 0);
            if(!p)
            {
                printf("Error!\n");
                return -1;
            }
            if(!address_add(p))
            {
                printf("Cannot add address!\n");
                return -3;
            }
            break;

        case 1:
            printf("Free\n");
            if(addrcount == 0)
                break;
            i = rand() % addrcount;
            p = addrarray[i];
            csheap_free(heap, p);
            address_remove(i);
            break;

        case 2:
            printf("Realloc\n");
            if(addrcount == 0)
                break;
            i = rand() % addrcount;
            p = addrarray[i];
            size = csheap_size(heap, p) + (rand() % 0xFF);
            address_remove(i);
            p = csheap_realloc(heap, p, size);
            if(!p)
            {
                printf("Error!\n");
                return -2;
            }
            if(!address_add(p))
            {
                printf("Cannot add address!\n");
                return -3;
            }
            break;

        default:
            break;
        }

        printf("allocsize=%X alloccnt=%X freesize=%X freecnt=%X metasize=%X tailoff=%X\n",
            heap->allocsize, heap->alloccount, heap->freesize, heap->freecount, heap->metasize, heap->tailoffset);
    }

    csheap_validate(heap);
    csheap_destroy(heap);

    printf("\n(Seed used: 0x%08X)\n", seed);
    printf("Done!\n");
    return 0;
}