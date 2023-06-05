/**
 * @file    mempool.h
 * @author  qufeiyan
 * @brief   simple implementation of a memory pool.
 * @version 1.0.0
 * @date    2023/05/16 00:09:45
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef MEMPOOL_H
#define MEMPOOL_H
/* Include ---------------------------------------------------------------------------------*/
#include "dlcDef.h"
#include "spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct memPool memPool_t;

memPool_t *memPoolDefine(char* name, size_t block_count, size_t block_size);
void *memPoolAlloc(memPool_t *mp);
void memPoolFree(memPool_t *mp, void *ptr);
void memPoolPrint(memPool_t *mp);
extern memPool_t *threadVertexMemPool, *mutexVertexMemPool;


/**
 * @brief  memPoolLockedDefine - macro to define and initialize a locked 
 *         memory pool
 * @param  name the name of the memPool.    
 * @param  block_count the count of memory block of the memPool.    
 * @param  block_size  the size of memory block of the memPool.  
 * @param  lock pointer to the spinlock.
 * @return  the memPool object which is created.
 * @note   the memPool object is a static memPool.
 *
 * Note: the macro can be used for global and local fifo data type variables.
 */
#define memPoolLockedDefine(name, block_count, block_size, lock) ({ \
    spinlock_t *_lock = (typeof(lock)) lock;\
    memPool_t *mp;\
    mp = memPoolDefine(name, block_count, block_size);\
    spinlockInit(_lock, 2048); \
    mp;\
})

/**
 * @brief   memPoolAllocLocked - macro to allocate a block of memory from a
 *          locked memory pool.
 * @param   mp is pointer to a memory pool.
 * @param   lock is pointer to a spinlock. 
 * @return  pointer to a block of memory.
 * @note    
 * @see     
 */
#define memPoolAllocLocked(mp, lock) ({ \
    void *res;\
    spinlock_t *_lock = (typeof(lock)) lock;\
    assert(_lock != NULL);  \
    _lock->acquire(_lock);  \
    res = memPoolAlloc(mp); \
    _lock->release(_lock);  \
    if(!res) memPoolInfo(mp);\
    res;\
})

/**
 * @brief   memPoolFreeLocked - macro to free a block of memory to a 
 *          memory pool.          
 * @param   mp is pointer to a memory pool.
 * @param   lock is pointer to a spinlock.
 */
#define memPoolFreeLocked(mp, lock) ({  \
    spinlock_t *_lock = (typeof(lock)) lock;\
    assert(_lock != NULL);  \
    _lock->acquire(_lock);  \
    memPoolFree(mp);        \
    _lock->release(_lock);  \
})


#define memPoolInfo(mp) ({\
    assert(mp != NULL);\
    memPoolPrint(mp);\
})

#ifdef __cplusplus
}
#endif

#endif	//  MEMPOOL_H