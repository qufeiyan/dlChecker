/**
 * @file    internal.c
 * @author  qufeiyan
 * @brief   
 * @version 1.0.0
 * @date    2023/05/17 22:06:44
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include <stdatomic.h>
#include "hashMap.h"
#include "internal.h"
#include "common.h"
#include "mempool.h"
#include "spinlock.h"
#include "vertex.h"


hashMap_t *eventQueueMap = NULL;
hashMap_t *requestThreadMap = NULL;
hashMap_t *vertexThreadMap = NULL;
hashMap_t *vertexMutexMap = NULL;
hashMap_t *residentThreadMap = NULL;
memPool_t *eventQueueMemPool = NULL, *eventQueueBufferMemPool = NULL;
memPool_t *threadVertexMemPool = NULL, *mutexVertexMemPool = NULL;
memPool_t *arcMemPool = NULL;
spinlock_t eventQueueMemPoolLock = {.lock = ATOMIC_FLAG_INIT};
spinlock_t eventQueueBufferMemPoolLock = {.lock = ATOMIC_FLAG_INIT};
spinlock_t eventQueueMapLock = {.lock = ATOMIC_FLAG_INIT};

atomic_long atomicThreadCounts = 0;

//! hash function for long.
static uint64_t hashCallback(const void *key) {
    extern size_t _hashCodeOfInteger(size_t key);
    return _hashCodeOfInteger((size_t)key);
}

static HASH_MAP_OPS IntegerMapType = {
    hashCallback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/**
 * @brief   1. Initialise a map that will record all threads and its eventqueue.
 *          2. Initialise a map that will record threads requesting mutex and not holding it.
 * @note    this function must be called int the initial phase of program.   
 */
void mapAllInit(){
    if(eventQueueMap == NULL){
        eventQueueMap = (hashMap_t *)hashMapInitLocked(&IntegerMapType, 
            NUMBER_OF_EVENTQUEUE, &eventQueueMapLock);
    }

    if(requestThreadMap == NULL){
        requestThreadMap = (hashMap_t *)hashMapCreate(&IntegerMapType, 
            NUMBER_OF_THREAD);
    }

    if(vertexThreadMap == NULL){
        vertexThreadMap = (hashMap_t *)hashMapCreate(&IntegerMapType, 
            NUMBER_OF_VERTEX_THREAD);
    }

    if(vertexMutexMap == NULL){
        vertexMutexMap = (hashMap_t *)hashMapCreate(&IntegerMapType, 
            NUMBER_OF_VERTEX_MUTEX);
    }

    if(residentThreadMap == NULL){
        residentThreadMap = (hashMap_t *)hashMapCreate(&IntegerMapType, 
            NUMBER_OF_VERTEX_THREAD);
    }
}

/**
 * @brief   Initialise all memory pool to be used.
 
 * @param   
 * @note    this function must be called int the initial phase of program.  
 */
void memPoolAllInit(){
    if(eventQueueMemPool == NULL){
        eventQueueMemPool = memPoolLockedDefine("eq", 
            NUMBER_OF_EVENTQUEUE, SIZE_OF_EVENTQUEUE,
            &eventQueueMemPoolLock);
    }

    if(eventQueueBufferMemPool == NULL){
        eventQueueBufferMemPool = memPoolLockedDefine("eqBuffer",
            NUMBER_OF_EVENTQUEUE_BUFFER, SIZE_OF_EVENTQUEUE_BUFFER,
            &eventQueueBufferMemPoolLock);
    }

    if(threadVertexMemPool == NULL){
        threadVertexMemPool = memPoolDefine("tv", 
            NUMBER_OF_VERTEX_THREAD, SIZE_OF_VERTEX_THREAD);
    }

    if(mutexVertexMemPool == NULL){
        mutexVertexMemPool = memPoolDefine("mv", 
            NUMBER_OF_VERTEX_MUTEX, SIZE_OF_VERTEX_MUTEX);
    }

    if(arcMemPool == NULL){
        arcMemPool = memPoolDefine("arc", 
            NUMBER_OF_ARC, SIZE_OF_ARC);
    }
}

void gcForThread(void *args){
    eventQueue_t *eq;
    vertex_t *vertex;

    assert(args);
    size_t tid = (size_t)args;

    //! destroy event queue.
    eq = hashMapGet(eventQueueMap, (void *)tid);
    if(eq){
        eventQueueDeInit(eq);
    }
     
    //! destroy vertex.  
    vertex = hashMapGet(vertexThreadMap, (void *)tid);
    if(vertex){
        //! remove the vertex first.
        hashMapRemove(vertexThreadMap, (void *)tid);
        
        //！ then destroy it.
        vertexDestroy(VERTEX_THREAD, vertex);
    }
}


