/**
 * @file    report.c
 * @author  qufeiyan
 * @brief   Report the result of Checker.
 * @version 1.0.0
 * @date    2023/06/05 00:14:00
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include "hashMap.h"
#include "internal.h"
#include "vertex.h"
#include <stddef.h>

static void reportInfo(const char *prefix, vertex_t *u, vertex_t *v);

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
 * @brief handler for unlocking the mutex possibly held by a different thread.
 * @param ssc [in] is the dead-lock cycle. 
 * @param num is the number of vertex of ssc. 
 */ 
void reportDeadLock(vertex_t **ssc, int num){
    vertex_t *v;
    const char *info, *prefix;
    assert(num >= 2);
    hashMap_t *sscMap = hashMapCreate(&IntegerMapType, num);

    if(num == 2){
        info = "==1001== [!!!Warnning!!!] Possible self-lock detected...";
        prefix = "==1001==";
    }else{
        info = "==1231== [!!!Warnning!!!] Unlocked mutex possibly held by other thread...";
        prefix = "==1231==";
    }
    fprintf(stderr, "%s\n", info);

    for(size_t i = 0; i < num; ++i){
        hashMapPut(sscMap, ssc[i], (void *)i);

        //! find a vertex whose type is thread.
        if(ssc[i]->type == VERTEX_THREAD){
            v = ssc[i];
            continue;
        }
    }

    arc_t *head = v->arcList;

    //! the outer loop traverses every vertex in the ssc.
    while (num--) {
        //! traverse v's arc list to find a vertex in the sscMap. 
        while(head){
            if(hashMapFind(sscMap, head->tail)){
                reportInfo(prefix, v, head->tail);
                //! head->tail is the next vertex in the circle.
                v = head->tail;
                head = v->arcList;
                break;
            }
            head = head->next;
        }
    }

    hashMapDestroy(sscMap);
}

static void reportInfo(const char *prefix, vertex_t *u, vertex_t *v){
    threadInfo_t *ti;
    mutexInfo_t *mi;
    assert(v);

    if(u->type == VERTEX_THREAD && v->type == VERTEX_MUTEX){
        ti = (threadInfo_t *)u->private;
        mi = (mutexInfo_t *)v->private;

        fprintf(stderr, "%s Thread # [%ld %s]:\n", prefix, ti->tid, ti->name);
        fprintf(stderr, "%s  \t holds the lock #%p [%p %p %p %p %p]\n", 
            prefix, (void *)mi->mid, ti->backtrace[0], ti->backtrace[1], 
            ti->backtrace[2], ti->backtrace[3], ti->backtrace[4]);
    }else if(u->type == VERTEX_MUTEX && v->type == VERTEX_THREAD){
        ti = (threadInfo_t *)v->private;
        mi = (mutexInfo_t *)u->private;
        
        fprintf(stderr, "%s Thread # [%ld %s]:\n", prefix, ti->tid, ti->name);
        fprintf(stderr, "%s  \t waits the lock #%p [%p %p %p %p %p]\n",
            prefix, (void *)mi->mid,ti->backtrace[0], ti->backtrace[1], 
            ti->backtrace[2], ti->backtrace[3], ti->backtrace[4]);
    }
}





