/**
 * @file    mempool.c
 * @author  qufeiyan
 * @brief   
 * @version 1.0.0
 * @date    2023/05/16 18:54:40
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mempool.h"
#include "dlcDef.h"
#include "internal.h"


struct memPool{
    char name[SIZE_OF_NAME];          //! name of memory pool.
    void* start;            //! start address for the memory pool.
    size_t size;            //! size of memory pool.
    
    uint8_t* block_list;    //! memory block list.
    size_t block_size;      //! size of memory block.

    size_t free_blocks;
    size_t total_blocks;

    int32_t err;
    int32_t avail;
    int32_t used;
    int32_t max;

    // spinlock_t *lock;       //! lock for memory pool.
};

extern unsigned long __DLC_MEMPOOL_START, __DLC_MEMPOOL_END;
static size_t mp_current_start = (size_t)(&__DLC_MEMPOOL_START);

#define MEMPOOL_UPDATE_CURRENT_START(size)  do{\
    mp_current_start += (MEM_ALIGN_UP(size, MEM_ALIGNMENT));\
}while(0)

/**
 * @brief  define a memPool object.
 
 * @param  name the name of the memPool.    
 * @param  block_count the count of memory block of the memPool.    
 * @param  block_size  the size of memory block of the memPool.  
 * @return  the memPool object which is created.
 * @note   the memPool object is a static memPool.
 * @see     
 */
struct memPool *memPoolDefine(char* name, size_t block_count, size_t block_size){
    struct memPool *mp;
    size_t offset;
    uint8_t *block_ptr;
    void *start;

    assert(name != NULL);
    assert(block_size > 0 && block_count > 0);

    start = (void *)mp_current_start;
    assert(start != NULL);

    mp = (struct memPool *)zmalloc(sizeof(struct memPool));
    if(mp == NULL) return NULL;

    strncpy(mp->name, name, DLC_NAME_SIZE);
    mp->total_blocks = block_count;
    mp->block_size = MEM_ALIGN_UP(block_size, MEM_ALIGNMENT);
    mp->free_blocks = block_count;

    mp->start = start;
    mp->size = (mp->block_size + sizeof(uint8_t*)) * mp->total_blocks;

    block_ptr = (uint8_t *)mp->start;
    for (offset = 0; offset < mp->total_blocks; ++offset) {
        *(uint8_t **)(block_ptr + offset * (mp->block_size + sizeof(uint8_t *))) =  \
        block_ptr + (offset + 1) * (mp->block_size + sizeof(uint8_t *));
    }

    //! remove last invalid block.
    *(uint8_t **)(block_ptr + (offset - 1) * (mp->block_size + sizeof(uint8_t *))) = NULL; 

    mp->block_list = block_ptr;
    MEMPOOL_UPDATE_CURRENT_START(mp->size);

    mp->err = 0;
    mp->max = 0;
    mp->used = 0;
    mp->avail = mp->total_blocks * mp->block_size;
    return mp;
}


/**
 * @brief   allocate a memory block from memory pool.
 
 * @param   mp is the memory pool object.
 * @return  the memory block.
 * @note    
 * @see     
 */
void *memPoolAlloc(struct memPool *mp){
    uint8_t *block_ptr;

    assert(mp != NULL);
    assert(mp->free_blocks <= mp->total_blocks);
    // assert(mp->block_list != NULL);

    if (mp->free_blocks <= 0 || mp->block_list == NULL) {
        mp->err++;
        return NULL;
    }

    //！get the head of block list.
    block_ptr = mp->block_list;

    //! the head of block list points to next block.
    mp->block_list = *(uint8_t **)block_ptr;

    //! unlinked block points to the memPool.
    *(uint8_t **)block_ptr = (uint8_t *)mp;
    mp->free_blocks--;

    //! record the memory usage.
    mp->avail = mp->free_blocks * mp->block_size;
    mp->used = mp->size - mp->avail;
    mp->max = mp->used < mp->max ? mp->max : mp->used;

    return (uint8_t *)(block_ptr + sizeof(uint8_t *));
}

/**
 * @brief   free a memory block 
 
 * @param   mp is the memory pool object.
 * @param   prt is pointer to a memory block.
 * @return  void
 * @note    
 * @see     
 */
void memPoolFree(struct memPool *mp, void *ptr){
    uint8_t *block_ptr;
    assert(mp);
    assert(ptr);

    block_ptr = (uint8_t *)(((uint8_t *)ptr) - sizeof(uint8_t *));
    if((uint8_t *)mp != *(uint8_t **)block_ptr){
        mp->err++;
        return;
    }

    assert(mp->free_blocks < mp->total_blocks);

    //! link the block into the block list. 
    *(uint8_t **)block_ptr = mp->block_list;
    mp->block_list = block_ptr;

    mp->free_blocks++;

    //! record the memory usage.
    mp->avail = mp->free_blocks * mp->block_size;
    mp->used = mp->size - mp->avail;
}

struct memPool *threadVertexMemPool = NULL, *mutexVertexMemPool = NULL;

void memPoolPrint(struct memPool *mp){
    if(mp == NULL) return;
    fprintf(stderr, "\n---------------[%s] memPool info------------------\n",\
        mp->name);\
    fprintf(stderr, "err \t avail \t used \t max\t\n");\
    fprintf(stderr, "%d \t %d \t %d\t %d\t\n", mp->err, \
        mp->avail, mp->used, mp->max);\
    fflush(stderr);\
}

/*------------------------------test-----------------------*/
//! test program
// #define DLC_TEST
#ifdef DLC_TEST
#include "testhelp.h"
#include "vertex.h"

static size_t mpool_used_memory(){
    return threadVertexMemPool->used + mutexVertexMemPool->used;
}

#define UNUSED(V) ((void) V)

#define start_benchmark() start = timeInMilliseconds()
#define end_benchmark(msg) do { \
    elapsed = timeInMilliseconds() - start; \
    printf(msg ": %ld items in %lld ms, memory used %lu, mpool used %lu\n",  \
        count, elapsed, zmalloc_used_memory(), mpool_used_memory()); \
} while(0)

#define report_benchmark(msg) do { \
    printf(msg ": %ld items , size %lu, block_size %lu, free_blocks %lu, err %d avail %d, max %d\n",  \
        count, threadVertexMemPool->size, threadVertexMemPool->block_size, threadVertexMemPool->free_blocks, \
        threadVertexMemPool->err, threadVertexMemPool->avail, threadVertexMemPool->max); \
} while(0)

/* ./demo test mpool [<count> | --accurate] */
int memPoolTest(int argc, char **argv, int flags) {
    long j;
    long long start, elapsed;
    memInit();
    threadVertexMemPool = memPoolDefine("mpool", NUMBER_OF_THREAD, 
        sizeof(vertex_t) + sizeof(struct threadInfo));

    mutexVertexMemPool = memPoolDefine("mpool", NUMBER_OF_THREAD, 
        sizeof(vertex_t) + sizeof(struct mutexInfo));

    long count = 0;
    int accurate = (flags & DLC_TEST_ACCURATE);

    if (argc == 4) {
        if (accurate) {
            count = 5000000;
        } else {
            count = strtol(argv[3],NULL,10);
        }
    } else {
        count = NUMBER_OF_THREAD;
    }

    report_benchmark("initial");
    dlc_dbg("count %ld\n", count);
    vertex_t *vs[NUMBER_OF_THREAD] = {0};
    start_benchmark();

    for (j = 0; j < count; j++) {
        vs[j] = memPoolAlloc(threadVertexMemPool);
        assert(vs[j]);
    }
    end_benchmark("Allocating");

    report_benchmark("Report");

    start_benchmark();
    for (j = 0; j < count; j++) {

    }
    end_benchmark("Validity of allocated memory address");

    start_benchmark();
    for (j = 0; j < count; j++) {
        // fprintf(stderr, "vs[%ld]: %p\n", j, vs[j]);
        memPoolFree(threadVertexMemPool, vs[j]);
    }
    end_benchmark("Free of allocated memory address");

    return 0;
}
#endif

