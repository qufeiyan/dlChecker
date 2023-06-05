/**
 * @file    internal.h
 * @author  qufeiyan
 * @brief   declare all global variables in this file.
 * @version 1.0.0
 * @date    2023/05/17 00:31:09
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef INTERNAL_H
#define INTERNAL_H
/* Include ---------------------------------------------------------------------------------*/
#include "common.h"
#include "lfqueue.h"
#include "dlcDef.h"
#include "mempool.h"
#include "spinlock.h"
#include "vertex.h"
#include "hashMap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUMBER_OF_THREAD                (512)

#define SIZE_OF_EVENT                   sizeof(event_t)
#define SIZE_OF_EVENTQUEUE              sizeof(eventQueue_t)
#define SIZE_OF_EVENTQUEUE_BUFFER       (SIZE_OF_EVENT * NUMBER_OF_EVENT)
#define NUMBER_OF_EVENT                 (1 << 8)
#define NUMBER_OF_EVENTQUEUE            NUMBER_OF_THREAD
#define NUMBER_OF_EVENTQUEUE_BUFFER     NUMBER_OF_EVENTQUEUE

#define SIZE_OF_NAME                    (16)

#define SIZE_OF_VERTEX_THREAD           (sizeof(vertex_t) + sizeof(threadInfo_t))       
#define SIZE_OF_VERTEX_MUTEX            (sizeof(vertex_t) + sizeof(mutexInfo_t))
#define NUMBER_OF_VERTEX_THREAD         NUMBER_OF_THREAD
#define NUMBER_OF_VERTEX_MUTEX          NUMBER_OF_THREAD
#define NUMBER_OF_VERTEX                (NUMBER_OF_VERTEX_THREAD + NUMBER_OF_VERTEX_MUTEX)


enum eventType{
    EVENT_WAITLOCK,
    EVENT_HOLDLOCK,
    EVENT_RELEASELOCK,
    EVENT_BUTT
};
typedef enum eventType eventType_t;

struct event{
    eventType_t     type;
    threadInfo_t    threadInfo;
    mutexInfo_t     mutexInfo;
};
typedef struct event event_t;

typedef struct lfqueue eventQueue_t;

struct dispatcher{
    // /**  holds the mapping relationship of 
    //  *   a thread and message queue that it owns.
    //  */
    long threadCount;     //! record current thread count. 
    eventQueue_t* eq;     //! messageQueue object for a thread.
    int (*invoke)(struct dispatcher *);     //! dispatch a thread's event to specific event queue.  
    event_t ev;           //! event to be dispatched.
};

typedef struct HASH_MAP hashMap_t;
typedef struct ENTRY_INFO entry_t;
typedef struct HASH_MAP_ITERATOR hashMapIterator_t;

typedef struct dispatcher dispatcher_t;

extern __thread dispatcher_t dispatcher; //! define thread local dispatcher for each thread.

extern hashMap_t *eventQueueMap;  //! record all eventqueue for each thread.

extern hashMap_t *vertexThreadMap, *vertexMutexMap;
extern hashMap_t *requestThreadMap;


static inline void hashMapIteratorInit(hashMapIterator_t *iter, hashMap_t *map){
    assert(iter != NULL && map != NULL);
    iter->map = map;
    iter->index = -1;
    iter->entry = NULL;
    iter->nextEntry = NULL;
}

#define hashMapIteratorReset(iter) do{\
    hashMapIterator_t *_iter = (typeof(iter)) iter;\
    assert(iter != NULL);\
    _iter->index = -1;\
    _iter->entry = NULL;\
    _iter->nextEntry = NULL;\
}while(0)

#define eventQueueInit(eq, buffer)({\
    assert(eq && buffer);\
    lfqueueInit(eq, buffer, NUMBER_OF_EVENT, SIZE_OF_EVENT);\
})

#define eventQueuePut(eq, ev) ({\
    uint32_t ret; \
    assert(eq != NULL && ev != NULL);\
    assert(sizeof(*eq) == sizeof(eventQueue_t)); \
    ret = lfqueuePut(eq, ev, 1);\
    ret;\
})

#define eventQueueGet(eq, ev) ({\
    uint32_t ret;\
    assert(eq != NULL && ev != NULL);\
    assert(sizeof(*eq) == sizeof(eventQueue_t)); \
    ret = lfqueueGet(eq, ev, 1);\
    ret;\
})

#define eventQueueGets(eq, ev, num) ({\
    uint32_t ret;\
    assert(eq != NULL && ev != NULL);\
    assert(sizeof(*eq) == sizeof(eventQueue_t)); \
    ret = lfqueueGet(eq, ev, num);\
    ret;\
})

#define eventQueueUsed(eq) ({\
    uint32_t ret;\
    assert(sizeof(*eq) == sizeof(eventQueue_t)); \
    ret = lfqueueUsed(eq);\
    ret;\
})

static inline eventQueue_t *eventQueueCurrent(){
    return dispatcher.eq;
}

#define hashMapInitLocked(type, capacity, lock) ({\
    spinlock_t *_lock = (typeof(lock)) lock;\
    hashMap_t *map;\
    map = hashMapCreate(type, capacity);\
    spinlockInit(_lock, 2048); \
    (void *)map;\
})

#define hashMapPutLocked(map, key, val, lock) ({\
    int ret;\
    spinlock_t *_lock = (typeof(lock))lock;\
    assert(_lock != NULL);  \
    _lock->acquire(_lock);  \
    ret = hashMapPut(map, key, val); \
    _lock->release(_lock);  \
    ret;\
})



//! get eventqueue memory frome pool.
extern memPool_t *eventQueueMemPool, *eventQueueBufferMemPool;
extern spinlock_t eventQueueMemPoolLock, eventQueueBufferMemPoolLock;
extern atomic_long atomicThreadCounts;
extern spinlock_t eventQueueMapLock;


//! initial function.
void mapAllInit();
void memPoolAllInit();
void dispatcherInit(dispatcher_t *dispatch);
long long timeInMilliseconds(void);


#ifdef __cplusplus
}
#endif

#endif	//  INTERNAL_H