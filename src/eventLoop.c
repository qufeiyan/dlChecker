/**
 * @file    eventLoop.c
 * @author  qufeiyan
 * @brief   Simple implementation of the event loop.
 * @version 1.0.0
 * @date    2023/05/13 12:06:39
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "dlcDef.h"
#include "internal.h"
#include "vertex.h"

typedef int eventError_t;

struct sscImpl{
    vertex_t **ssc;
    int step;
};
typedef struct sscImpl sscImpl_t;
struct sscCountImpl{
    int *sscCount;
    int step;
};
typedef struct sscCountImpl sscCountImpl_t;
struct stackImpl{
    vertex_t **stack;
    int top;
};
typedef struct stackImpl stackImpl_t;


static __attribute__ ((unused))  eventError_t eventError = 0;

void displayInfo(vertex_t **ssc, int *sscCount, int num);
void clearTarjanStatus(hashMapIterator_t *iter);


#if IS_USE_ASSERT
#undef assert
#define assert(expression)                            \
do{                                                   \
    if(!(expression)){                                \
        dlc_err(#expression " is expected...\n");     \
        eventQueuesInfo();                            \
        abort();                                      \
    }                                                 \
}while(0)
#else
#define assert(expression)  (void)(expression)
#endif


/*
enum eventType{
    EVENT_WAITLOCK,
    EVENT_HOLDLOCK,
    EVENT_RELEASELOCK,
    EVENT_BUTT
}; */

/**
 * @brief   event handler for waiting lock.
 *
 * @param   ev is pointer to event.
 * @return  
 * @note    
 * @see     
 */
static void waitLockHandler(event_t *ev){
    vertex_t *tv, *mv;
    threadInfo_t *threadInfo;
    mutexInfo_t *mutexInfo;
    
    assert(ev->type == EVENT_WAITLOCK);
    assert(vertexThreadMap != NULL);
    assert(vertexMutexMap != NULL);
    
    threadInfo = &ev->threadInfo;
    mutexInfo = &ev->mutexInfo;
    //! find or create tv and mv from ev.tid and ev.mid.
    tv = hashMapGet(vertexThreadMap, (void *)threadInfo->tid);
    if(tv == NULL){
        //! create a vertex for thread.
        assert(threadVertexMemPool != NULL);
        tv = vertexCreate(VERTEX_THREAD, &ops);
        assert(tv);
        hashMapPut(vertexThreadMap, (void *)threadInfo->tid, tv);
    }

    assert(tv->type == VERTEX_THREAD);
    //! set tv's status.
    vertexSetInfo(tv, threadInfo);

    mv = hashMapGet(vertexMutexMap, (void *)mutexInfo->mid);
    if(mv == NULL){
        //! create a vertex for mutex.
        assert(mutexVertexMemPool != NULL);
        mv = vertexCreate(VERTEX_MUTEX, &ops);
        assert(mv);
        hashMapPut(vertexMutexMap, (void *)mutexInfo->mid, mv);
    }

    assert(mv->type == VERTEX_MUTEX);
    //! set mv's status.
    vertexSetInfo(mv, mutexInfo);

    //! add edge from tv to mv.
    tv->ops->addEdge(tv, mv);

    //! record thread to request map.    
    assert(requestThreadMap != NULL);
    __unused int ret = hashMapPut(requestThreadMap, (void *)tv, (void *)1);
    assert(ret == 1);
}

/**
 * @brief   event handler for holding lock.
 * @param   ev is pointer to event.
 * @return  
 * @note    
 * @see     
 */
static void holdLockHandler(event_t *ev){
    vertex_t *tv, *mv;
    threadInfo_t *threadInfo;
    mutexInfo_t *mutexInfo;

    assert(ev->type == EVENT_HOLDLOCK);
    assert(vertexThreadMap != NULL);
    assert(vertexMutexMap != NULL);

    threadInfo = &ev->threadInfo;
    mutexInfo = &ev->mutexInfo;
    //! find tv and mv from ev.tid and ev.mid.
    tv = hashMapGet(vertexThreadMap, (void *)threadInfo->tid);
    if(tv == NULL){
        //! record error.
        abort();
        return;
    }

    mv = hashMapGet(vertexMutexMap, (void *)mutexInfo->mid);
    if(mv == NULL){
        //! record error.
        abort();
        return;
    }

    assert(tv->type == VERTEX_THREAD);
    //! set tv's status.
    vertexSetInfo(tv, threadInfo);

    assert(mv->type == VERTEX_MUTEX);
    //! set mv's status.
    vertexSetInfo(mv, mutexInfo);

    //! delete tv --> mv. 
    tv->ops->deleteEdge(tv, mv);

    //! add mv --> tv.
    mv->ops->addEdge(mv, tv);
    
    dlc_warn("mv :%#lx holds by tv :%ld\n", mutexInfo->mid, threadInfo->tid);
    //! remove tv from the request map.
    int ret = hashMapRemove(requestThreadMap, tv);
    assert(ret == 0);
}

/**
 * @brief   event handler for releasing lock.
 * @param   ev is pointer to event.
 * @return  
 * @note    
 * @see     
 */
static void releaseLockHandler(event_t *ev){
    assert(ev->type == EVENT_RELEASELOCK);
    assert(vertexThreadMap != NULL);
    assert(vertexMutexMap != NULL);

    vertex_t *tv, *mv;
    threadInfo_t *threadInfo;
    mutexInfo_t *mutexInfo;

    assert(vertexThreadMap != NULL);
    assert(vertexMutexMap != NULL);

    threadInfo = &ev->threadInfo;
    mutexInfo = &ev->mutexInfo;
    //! find tv and mv from ev.tid and ev.mid.
    tv = hashMapGet(vertexThreadMap, (void *)threadInfo->tid);
    if(tv == NULL){
        //! record error.
        abort();
        return;
    }

    mv = hashMapGet(vertexMutexMap, (void *)mutexInfo->mid);
    if(mv == NULL){
        abort();
        return;
    }

    assert(tv->type == VERTEX_THREAD);
    //! set tv's status.
    vertexSetInfo(tv, threadInfo);

    assert(mv->type == VERTEX_MUTEX);
    //! set mv's status.
    vertexSetInfo(mv, mutexInfo);

    //! delete mv --> tv.
    mv->ops->deleteEdge(mv, tv);
}

static void (*handler[])(event_t *ev) = {
    waitLockHandler,
    holdLockHandler,
    releaseLockHandler
};

void eventHandler(event_t *ev){
    assert(ev != NULL && ev->type < EVENT_BUTT);
    handler[ev->type](ev);
};

void eventLoopEnter(){
    hashMapIterator_t iter;
    entry_t *entry;
    eventQueue_t *eq;
    long loops = atomicThreadCounts;
    int i = 0;
    // dlc_warn("loops %ld\n", loops);
    hashMapIteratorInit(&iter, eventQueueMap);
    while((entry = hashMapNext(&iter)) != NULL){ 
        eq = (eventQueue_t *)hashMapGet(eventQueueMap, (void *)entry->key);
        if(eq == NULL) continue;
        int num = eventQueueUsed(eq);
        dlc_dbg("count %ld i %d, num %d\n", loops, i, num);
        while (num > 0) {
            event_t ev;
        
            assert(1 == eventQueueGet(eq, &ev));
            eventHandler(&ev);
            num--;
        }
    }
}

/**
 * @brief   tarjan algorithm to find ssc.
 *
 * @param   u is current vertex.  
 * @param   stack is stack used for tarjan.
 * @param   top is top of stack.
 * @param   ssc is tripule 
 */
void tarjan(vertex_t *u, short *time, 
            stackImpl_t *stackImpl, sscImpl_t *sscImpl, sscCountImpl_t *sscCountImpl){ 
    vertex_t *v = NULL;
    u->dfn = u->low = ++(*time);
    assert(u->dfn == u->low);
    vertex_t **stack = stackImpl->stack;
    int *top = &stackImpl->top;
    stack[++(*top)] = u;
    u->inStack = true;

    arc_t *head = u->arcList;
    if(head == NULL){
        dlc_warn("u.type %d, top %d\n", u->type, *top);
        goto skip;
    }

    while(head){
        v = head->tail;
        if(v->dfn == 0){
            tarjan(v, time, stackImpl, sscImpl, sscCountImpl);
            u->low = DLC_MIN(u->low, v->low);
        }else if(v->inStack){
            u->low = DLC_MIN(u->low, v->dfn);
        }
        head = head->next;
    }

skip:
    if(u->dfn == u->low){
        vertex_t *w;
        int count = 0;
        vertex_t **ssc;
        int *sscCount;
        do{
            w = stack[(*top)--];
            w->inStack = false;
            ssc = sscImpl->ssc + sscImpl->step;
            sscCount = sscCountImpl->sscCount + sscCountImpl->step;
            *ssc = w;
            sscImpl->step++;
            count++;
        }while(w != u);
        *sscCount = count;
        sscCountImpl->step++;
    }
}

void strongConnectedComponent(){
    hashMapIterator_t iter;
    entry_t *entry;
    vertex_t *u;

    assert(requestThreadMap != NULL);

    int size = hashMapSize(requestThreadMap);
    if(size == 0) {
        dlc_dbg("size == 0\n");
        return;  //! return if there is no thread requesting lock.
    }
    static vertex_t *stack[NUMBER_OF_VERTEX];
    static vertex_t *ssc[NUMBER_OF_VERTEX];
    static int sscCount[NUMBER_OF_VERTEX];
    static short time = 0;
    //! initialise all variables used by the tarjan.
    memset(sscCount, 0, sizeof(sscCount));
    memset(stack, 0, sizeof(stack));
    memset(ssc, 0, sizeof(ssc));
    time = 0;

    sscImpl_t sscImpl = {
        .ssc = ssc,
        .step = 0
    };
    sscCountImpl_t sscCountImpl = {
        .sscCount = sscCount,
        .step = 0
    };
    stackImpl_t stackImpl = {
        .stack = stack,
        .top = -1
    };

    dlc_warn("the number of request thread is %d\n", size);
    hashMapIteratorInit(&iter, requestThreadMap);
    while((entry = hashMapNext(&iter)) != NULL){  
        u = entry->key;
        threadInfo_t *ti = (threadInfo_t *)&u->private[0];
        dlc_dbg("ti.name %s, ti.tid %ld\n", ti->name, ti->tid);
        if(u->dfn == 0){
            tarjan(u, &time, &stackImpl, &sscImpl, &sscCountImpl);
        }
    }

    dlc_warn("sscCount :%p sscCount[0] %d\n", sscCount, sscCount[0]);
    if(sscCount[0] > 1){
        displayInfo(ssc, sscCount, sizeof(sscCount) / sizeof(sscCount[0]));
        // extern long long start;
        // dlc_err("start %lld, resume %lld ms\n", start, (timeInMilliseconds() - start));
        // abort();
    }
    clearTarjanStatus(&iter);
}

int getSSCCount(int *sscCount, int num){
    int ret;
    assert(sscCount != NULL && num > 0);

    ret = 0;
    for (int i = 0; i < num; ++i) {
        if(sscCount[i] == 0){
            break;
        }
        ret++;
    }
    return ret;
}
void reportDeadLock(vertex_t **ssc, int num);

void displayInfo(vertex_t **ssc, int *sscCount, int num){
    assert(ssc != NULL);
    assert(sscCount != NULL && num > 0);

    for (int i = 0; i < num; ++i) {
        if(sscCount[i] != 0){
            if(sscCount[i] > 1){
                printf("----------------find ssc %d: %d vertexs...----------------\n", 
                    i, sscCount[i]);
                reportDeadLock(ssc, sscCount[i]); 
            } 
            ssc += sscCount[i]; 
        }
        else break; //! avoid ineffective iterations.
    }
}

void clearTarjanStatus(hashMapIterator_t *iter){
    entry_t *entry;
    vertex_t *u;
    arc_t *head;
    assert(iter);
    hashMapIteratorReset(iter);
    while((entry = hashMapNext(iter)) != NULL){  
        u = entry->key;
        u->dfn = 0;
        u->low = 0;
        u->inStack = false;

        head = u->arcList;
        while(head){
            head->tail->dfn = 0;
            head->tail->low = 0;
            head->tail->inStack = false;
            head = head->next;
        }
    }
}


/*--------------test for tarjan algorithm */


