/**
 * @file    internal.c
 * @author  qufeiyan
 * @brief   
 * @version 1.0.0
 * @date    2023/05/17 22:06:44
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include "internal.h"
#include "common.h"
#include "mempool.h"
#include <stdatomic.h>


hashMap_t *eventQueueMap = NULL;
hashMap_t *requestThreadMap = NULL;
hashMap_t *vertexThreadMap = NULL;
hashMap_t *vertexMutexMap = NULL;
memPool_t *eventQueueMemPool = NULL, *eventQueueBufferMemPool = NULL;
spinlock_t eventQueueMemPoolLock, eventQueueBufferMemPoolLock;
spinlock_t eventQueueMapLock;

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
}



// /**
//  * @brief   initialise all the message queue.
 
//  * @param   mqs is the pointer of array of message queues.
//  * @param   nums is the size of array.
//  * @return  
//  * @note    
//  * @see     
//  */
// void initALLMessageQueues(MessageQueue **mqs, size_t nums){
//     memPool_t *messageQueueMemPool;
//     int i;

//     assert(mqs != NULL);
//     assert(nums <= NUMS_MAX_THREAD);

//     messageQueueMemPool = memPoolDefine("messagequeue", 
//         nums, sizeof(MessageQueue));

//     for (i = 0; i < nums; ++i) {
//         mqs[i] = (MessageQueue *)memPoolAlloc(messageQueueMemPool);
//     }
// }

// MessageQueue *getMessageQueue(MessageQueue **mqs){
//     int idx;
    
//     assert(mqs != NULL);

//     do{
//         idx = idxMsgQueue; //! old index.
//     }while(CAS(idxMsgQueue, idx, idx + 1) != TRUE);

//     assert(idxMsgQueue < NUMS_MAX_THREAD);

//     return mqs[idx + 1];
// }

// #include <sys/sysinfo.h>
// int ncoresGet(){
//     if(ncores == 0){
//         ncores = get_nprocs();
//     }
//     return ncores;
// }