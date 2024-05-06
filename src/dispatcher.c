/**
 * @file    dispatcher.c
 * @author  qufeiyan
 * @brief   Simple implementation of the event dispatcher.
 * @version 1.0.0
 * @date    2023/05/11 22:51:23
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include "common.h"
#include "internal.h"
#include "mempool.h"


static int dispatcherInvoke(dispatcher_t *dispatcher){
    int ret = 0;
    assert(NULL != dispatcher && NULL != dispatcher->eq);
    assert(-1 != dispatcher->threadCount);

    ret = eventQueuePut(dispatcher->eq, &dispatcher->ev);
    assert(ret == 1); //! ret = 1 means put opretion is successful.
    return ret;
}


__thread dispatcher_t dispatcher = {
    .threadCount = -1, //! -1 means current thread have not been scheduled yet. 
    .tid = 0,
    .eq = NULL,
    .ev = {0},
    .invoke = NULL
}; //! define thread local dispatcher for each thread.


/**
 * @brief   Initialise the dispatcher for a thread, and record the 
 *          eq in the global map. 
 * @param   
 * @note    This function should be called only in the time a thread is first dispatched.
 * @see     
 */
void dispatcherInit(dispatcher_t *dispatch){
    int ret;
    uint8_t *buffer;
    //! eventQueueMap must be initialised before enter this function.
    assert(eventQueueMap != NULL);
    assert(dispatch != NULL);

    if(dispatch->eq == NULL){
        dispatch->eq = (eventQueue_t *)memPoolAllocLocked(eventQueueMemPool, &eventQueueMemPoolLock);

        //! initialise eq for the dispatcher.
        buffer = (uint8_t *)memPoolAllocLocked(eventQueueBufferMemPool, &eventQueueBufferMemPoolLock);
        eventQueueInit(dispatch->eq, buffer);
        
        /** @brief shared variable {@code atomicThreadCounts} is in danger of concurrency.
        atomicThreadCounts++;
        dispatch->threadCount = atomicThreadCounts;
        
        ret = hashMapPutLocked(eventQueueMap, (void *)dispatch->threadCount, 
            dispatch->eq, &eventQueueMapLock);
        */

        if(dispatch->tid == 0){
            dispatch->tid = dlcGetThreadId();
        }

        //! record the eventqueue. 
        ret = eventQueueMapPutLocked(dispatcher, &eventQueueMapLock); 

        //! ret == 1 means that there is no such key in the map before we put it.
        if(ret != 1){
            dlc_err("tc %ld\n", dispatch->threadCount);
        } 
        assert(ret == 1);
    }

    if(dispatch->invoke == NULL){
        dispatch->invoke = dispatcherInvoke;
    }
}
