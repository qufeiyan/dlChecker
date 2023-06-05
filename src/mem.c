/**
 * @file    mem.c
 * @author  qufeiyan (2491411913@qq.com)
 * @brief   simple implementation of small memory manage algorithm based on first fit.
 * @version 0.1
 * @date    2022-10-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "common.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define MEMDBG   (0)
#define mem_dbg(format, ...)                                                      \
do{                                                                               \
    if(MEMDBG)                                                                    \
    {                                                                             \
        printf(LOG_COLOR_DEBUG "[MEM_DBG %s:%d](#%s) " format LOG_COLOR_NONE ,    \
            __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);                     \
    }                                                                             \
}while(0) 

#define MEM_ALIGNMENT    (4)

/**
 * @ingroup BasicDef
 *
 * @def MEM_ALIGN_UP(size, align)
 * Return the most contiguous size aligned at specified width. MEM_ALIGN_UP(13, 4)
 * would return 16.
 */
#define MEM_ALIGN_UP(size, align)        (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def MEM_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. MEM_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define MEM_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

typedef struct{
    size_t prev;
    size_t next;
    uint8_t used;
}SMALL_MEM_INFO;

typedef struct{
    uint8_t *heap;            /**< pointer to the heap */
    SMALL_MEM_INFO *heapFree; /**< pointer to the lowest free block, this is used for faster search */
    SMALL_MEM_INFO *heapEnd;  /**< the last entry, always unused! */
    size_t memSizeAligned;    /**< aligned memory size */

    const char* name;         /**< the name of memory block. */
    size_t err;
    size_t avail;
    size_t used;
    size_t max;
}MEM_MANAGER;


#define MIN_SIZE 12  //! 32-bit cpu
#define MIN_SIZE_ALIGNED   MEM_ALIGN_UP(MIN_SIZE, MEM_ALIGNMENT)
#define SIZEOF_STRUCT_MEM  MEM_ALIGN_UP(sizeof(SMALL_MEM_INFO), MEM_ALIGNMENT)

#if USE_STATIC_HEAP
#define MEM_SIZE           (64 * 1024)    //！set heap size.
#define MEM_SIZE_ALIGNED   MEM_ALIGN_UP(MEM_SIZE, MEM_ALIGNMENT)

/** the heap. we need one struct mem at the end and some room for alignment */
uint8_t ram_heap[MEM_SIZE_ALIGNED + (2U * SIZEOF_STRUCT_MEM) + MEM_ALIGNMENT - 1U];
#else
extern unsigned long __DLC_HEAP_START, __DLC_HEAP_END;
#define ram_heap (size_t)(&__DLC_HEAP_START)
#define MEM_SIZE_ALIGNED   MEM_ALIGN_DOWN(((size_t)(&__DLC_HEAP_END) - (size_t)(&__DLC_HEAP_START)) \
                                - 2 * SIZEOF_STRUCT_MEM, MEM_ALIGNMENT)
#endif

/** the small memory management object. */
static MEM_MANAGER manager = {0};


static void memInfo(MEM_MANAGER* pMem);

/**
 * @brief initialize small memory management algorithm. 
 * 
 */
void memInit(void){
    MEM_MANAGER *pMemManager = &manager;
    pMemManager->name = "small mem";

    SMALL_MEM_INFO *mem;
    
    // mem_dbg("ram_heap %p, %p, %#lx\n", &__DLC_HEAP_START, &__DLC_HEAP_END, MEM_SIZE_ALIGNED);
   
    pMemManager->heap = (uint8_t *)MEM_ALIGN_DOWN((size_t)(ram_heap), MEM_ALIGNMENT);

    /* initialize the start of the heap */
    mem = (SMALL_MEM_INFO *)(void *)pMemManager->heap;
    mem->prev = 0;
    mem->next = MEM_SIZE_ALIGNED + SIZEOF_STRUCT_MEM;
    mem->used = 0;
    pMemManager->heapFree = mem;
    pMemManager->heapEnd = (SMALL_MEM_INFO *)&pMemManager->heap[mem->next];
    pMemManager->heapEnd->prev = pMemManager->heapEnd->next = MEM_SIZE_ALIGNED + SIZEOF_STRUCT_MEM;
    pMemManager->memSizeAligned = MEM_SIZE_ALIGNED;

    pMemManager->avail = pMemManager->memSizeAligned;
}

/**
 * @brief Allocate a block of memory with a minimum of 'size' bytes.
 * 
 * @param size is the minimum size of the requested block in bytes.
 * @return the pointer to allocated memory or NULL if no free memory was found.
 */
void* memAlloc(size_t size){
    MEM_MANAGER *pMemManager = &manager;

    size_t ptr, ptr2;
    SMALL_MEM_INFO *mem, *mem2;

    assert(pMemManager->memSizeAligned > 0); //! if memSizeAligned == 0 means memInit() is not called.

    if(size == 0) return NULL;

    size = MEM_ALIGN_UP(size, MEM_ALIGNMENT);

    /* every data block must be at least MIN_SIZE_ALIGNED long */
    if(size < MIN_SIZE_ALIGNED){
        size = MIN_SIZE_ALIGNED;
    }
    if(size > pMemManager->memSizeAligned){
        mem_dbg("Sorry, out of memory trying to allocate %lu bytes.\n", size);
        return NULL;
    }
    
    /** Scan through the heap searching for a free block that is big enough,
     *  beginning with the lowest free block.
     */
    for(ptr = (uint8_t *)pMemManager->heapFree - pMemManager->heap; 
        ptr <= pMemManager->memSizeAligned - size;
        ptr = ((SMALL_MEM_INFO *)&pMemManager->heap[ptr])->next){
        
        /** mem is the current free block in the small memory list. */
        mem = (SMALL_MEM_INFO *)&pMemManager->heap[ptr];

        if(!mem->used && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size){
            /** mem is not used and at least perfect fit is possible:
             *  mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem 
             */
            
            if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED)) {
                /** (in addition to the above, we test if another struct SMALL_MEM_INFO (SIZEOF_STRUCT_MEM) containing
                 * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
                 * -> split large block, create empty remainder,
                 * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
                 * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
                 * struct SMALL_MEM_INFO would fit in but no data between mem2 and mem2->next
                 * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
                 *       region that couldn't hold data, but when mem->next gets freed,
                 *       the 2 regions would be combined, resulting in more free memory
                 */
                //! ptr2 
                ptr2 = (size_t)(ptr + SIZEOF_STRUCT_MEM + size);
                /* create mem2 struct */
                mem2 = (SMALL_MEM_INFO *)&pMemManager->heap[ptr2];
                mem2->used = 0;
                mem2->prev = ptr;
                mem2->next = mem->next;
                /* and insert it between mem and mem->next */
                mem->next = ptr2;
                mem->used = 1;
                
                if(mem2->next != MEM_SIZE_ALIGNED + SIZEOF_STRUCT_MEM){
                    ((SMALL_MEM_INFO *)&pMemManager->heap[mem2->next])->prev = ptr2;
                }
                //! increase used..
                pMemManager->used += (size + SIZEOF_STRUCT_MEM);
                if (pMemManager->max < pMemManager->used){
                    pMemManager->max = pMemManager->used;
                }
            }else{
                mem->used = 1;  
                pMemManager->used += mem->next - ((uint8_t *)mem - pMemManager->heap);
                if (pMemManager->max < pMemManager->used){
                    pMemManager->max = pMemManager->used;
                }
            }

            if(mem == pMemManager->heapFree){
                /* Find next free block after mem and update lowest free pointer */
                SMALL_MEM_INFO *cur = pMemManager->heapFree;
                while (cur->used && cur != pMemManager->heapEnd) {
                    cur = (SMALL_MEM_INFO *)&pMemManager->heap[cur->next];
                }
                pMemManager->heapFree = cur;
                assert((pMemManager->heapFree == pMemManager->heapEnd || !pMemManager->heapFree->used));
            }

            assert(((size_t)mem + SIZEOF_STRUCT_MEM + size) <= (size_t)pMemManager->heapEnd);
            assert(((size_t)mem & (MEM_ALIGNMENT - 1)) == 0);

            mem_dbg("allocate memory at %p, size: %lu\n",
                ((uint8_t *)mem + SIZEOF_STRUCT_MEM),
                (size_t)(mem->next - ((uint8_t *)mem - pMemManager->heap)));
            /* return the memory data except mem struct */
            return (uint8_t *)mem + SIZEOF_STRUCT_MEM;
        }
    }
    pMemManager->err++;  /**< record the number of failure to malloc.*/

    mem_dbg("mem_malloc: could not allocate %lu bytes\n", size);
    memInfo(pMemManager);
    return NULL;
}

/**
 * @brief obtain the size of address pointed to by the pointer ptr,
 *        which is assigned by the memAlloc function.
 *        
 * @param ptr the address of memory which allocted by memAlloc function.
 * @return the size of address pointed to by the pointer ptr.
 */
size_t memAllocSize(const void *ptr){
    MEM_MANAGER *pMemManager = &manager;
    SMALL_MEM_INFO *mem;

    if (NULL == ptr) return 0;
    assert((((size_t)ptr) & (MEM_ALIGNMENT - 1)) == 0);

    /* Get the corresponding struct SMALL_MEM_INFO ... */
    mem = (SMALL_MEM_INFO *)((uint8_t *)ptr - SIZEOF_STRUCT_MEM);
    assert(mem->used);
    assert((uint8_t *)ptr >= (uint8_t *)pMemManager->heap &&
            (uint8_t *)ptr < (uint8_t *)pMemManager->heapEnd);

    return (mem->next - ((uint8_t *)mem - pMemManager->heap) - SIZEOF_STRUCT_MEM);
}

/**
 * @brief Contiguously allocates enough space for count objects that are size bytes
 *        of memory each and returns a pointer to the allocated memory.
 *        The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void *memCalloc(size_t count, size_t size)
{
    void *p;
    size_t alloc_size = (size_t)count * (size_t)size;

    if ((size_t)(size_t)alloc_size != alloc_size) {
        mem_dbg("mem_calloc: could not allocate %lu bytes\n", alloc_size);
        return NULL;
    }

    /* allocate 'count' objects of size 'size' */
    p = memAlloc((size_t)alloc_size);
    if (p) {
        /* zero the memory */
        memset(p, 0, alloc_size);
    }
    return p;
}

static void memPlugHoles(SMALL_MEM_INFO *mem);

/**
 * @brief This function will release the previously allocated memory block by
 *        memAlloc. The released memory block is taken back to system heap.
 *
 * @param ptr the address of memory which will be released.
 */
void memFree(void* ptr){
    MEM_MANAGER *pMemManager = &manager;
    SMALL_MEM_INFO *mem;

    if (NULL == ptr) return;
    assert((((size_t)ptr) & (MEM_ALIGNMENT - 1)) == 0);

    /* Get the corresponding struct SMALL_MEM_INFO ... */
    mem = (SMALL_MEM_INFO *)((uint8_t *)ptr - SIZEOF_STRUCT_MEM);
    assert(mem->used);
    assert((uint8_t *)ptr >= (uint8_t *)pMemManager->heap &&
            (uint8_t *)ptr < (uint8_t *)pMemManager->heapEnd);
    
    /* ... and is now unused. */
    mem->used = 0;

    if (mem < pMemManager->heapFree) {
    /* the newly freed struct is now the lowest */
        pMemManager->heapFree = mem;
    }

    pMemManager->used -= (mem->next - ((uint8_t *)mem - pMemManager->heap));
    
    /* finally, see if prev or next are free also */
    memPlugHoles(mem);
}

/**
 * @brief "Plug holes" by combining adjacent empty struct mems.
 *        After this function is through, there should not exist
 *        one empty struct mem pointing to another empty struct mem.
 *
 * @param mem this points to a struct mem which just has been freed
 * @internal this function is only called by memFree()
 *
 */
static void memPlugHoles(SMALL_MEM_INFO *mem){
    MEM_MANAGER *pMemManager = &manager;
    SMALL_MEM_INFO *nmem, *pmem;

    assert((uint8_t *)mem >= pMemManager->heap);
    assert((uint8_t *)mem < (uint8_t *)pMemManager->heapEnd);

    /* plug hole forward */
    nmem = (SMALL_MEM_INFO *)&pMemManager->heap[mem->next];
    if (mem != nmem && !nmem->used &&
        (uint8_t *)nmem != (uint8_t *)pMemManager->heapEnd){
        /* if mem->next is unused and not end of m->heap_ptr,
         * combine mem and mem->next
         */
        if (pMemManager->heapFree == nmem)
        {
            pMemManager->heapFree = mem;
        }
        mem->next = nmem->next;
        if(nmem->next != MEM_SIZE_ALIGNED + SIZEOF_STRUCT_MEM){
            ((SMALL_MEM_INFO *)&pMemManager->heap[nmem->next])->prev = (uint8_t *)mem - pMemManager->heap;
        }
    }

    /* plug hole backward */
    pmem = (SMALL_MEM_INFO *)&pMemManager->heap[mem->prev];
    if (pmem != mem && !pmem->used){
        /* if mem->prev is unused, combine mem and mem->prev */
        if (pMemManager->heapFree == mem){
            pMemManager->heapFree = pmem;
        }
        pmem->next = mem->next;
        if(mem->next != MEM_SIZE_ALIGNED + SIZEOF_STRUCT_MEM){
            ((SMALL_MEM_INFO *)&pMemManager->heap[mem->next])->prev = (uint8_t *)pmem - pMemManager->heap;
        }
    }
}

static void memInfo(MEM_MANAGER* pMem)
{
    mem_dbg("\nmem name: %s\n\t", pMem->name);
    mem_dbg("avail: %lu\n\t", pMem->avail);
    mem_dbg("used: %lu\n\t", pMem->used);
    mem_dbg("max: %lu\n\t", pMem->max);
    mem_dbg("err: %lu\n", pMem->err);
}

/*------ test -------*/
/* ------------------------------- Benchmark ---------------------------------*/

#ifdef DLC_TEST
#include "testhelp.h"

#define UNUSED(V) ((void) V)
#define MAX(x, y)  ((x) > (y) ? (x) : (y))

#define start_benchmark() start = timeInMilliseconds()
#define end_benchmark(msg) do { \
    elapsed = timeInMilliseconds() - start; \
    printf(msg ": %ld items in %lld ms\n", count, elapsed); \
    memInfo(&manager); \
} while(0)

/* ./demo test dict [<count> | --accurate] */
int memTest(int argc, char *argv[], int flags) {
    long j;
    long long start, elapsed;

    long count = 0;
    int accurate = (flags & DLC_TEST_ACCURATE);

    if (argc == 4) {
        if (accurate) {
            count = 5000000;
        } else {
            count = strtol(argv[3],NULL,10);
        }
    }

    dlc_dbg("count %ld\n", count);
    
    memInit();
    void* mems[count];
    start_benchmark();
    for (j = 0; j < count; j++) {
        mems[j] = memAlloc(4);
        assert(memAllocSize(mems[j]) == MAX(MIN_SIZE_ALIGNED, 4));
    }
    end_benchmark("Malloc fixed size");

    start_benchmark();
    for (j = 0; j < count; j++) {
        memFree(mems[j]);
    }
    end_benchmark("Free fixed size");

    start_benchmark();
    for (j = 0; j < count; j++) {
        mems[j] = memAlloc(4 * j + 1);
        assert(memAllocSize(mems[j]) == MAX(MEM_ALIGN_UP(4 * j + 1, MEM_ALIGNMENT), MIN_SIZE_ALIGNED));
    }
    end_benchmark("Malloc variable size");

    start_benchmark();
    for (j = 0; j < count; j++) {
        memFree(mems[j]);
    }
    end_benchmark("Free variable size");

    start_benchmark();
    for (j = 0; j < count; j++) {
        mems[j] = memAlloc(8 * j + 1);
        assert(memAllocSize(mems[j]) == MAX(MEM_ALIGN_UP(8 * j + 1, MEM_ALIGNMENT), MIN_SIZE_ALIGNED));
        memFree(mems[j]);
        assert((&manager)->err == 0);
    }
    end_benchmark("Malloc && Free variable size");
    return 0;
}

#endif