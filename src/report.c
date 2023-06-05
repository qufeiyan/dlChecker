/**
 * @file    report.c
 * @author  qufeiyan
 * @brief   Report the result of Checker.
 * @version 1.0.0
 * @date    2023/06/05 00:14:00
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include "internal.h"
#include "vertex.h"
#include <assert.h>

static void reportInfo(const char *prefix, vertex_t *v);

/**
 * @brief handler for unlocking the mutex possibly held by a different thread.
 * @param ssc [in] is the dead-lock cycle. 
 * @param num is the number of vertex of ssc. 
 */ 
void reportDeadLock(vertex_t **ssc, int num){
    vertex_t *v;
    const char *info, *prefix;
    assert(num >= 2);
    if(num == 2){
        info = "==1001== [!!!Warnning!!!] Possible self-lock detected...";
        prefix = "==1001==";
    }else{
        info = "==1231== [!!!Warnning!!!] Unlocked mutex possibly held by other thread...";
        prefix = "==1231==";
    }
    fprintf(stderr, "%s\n", info);

    // for(int i = 0; i < num; ++i){
    //     v = ssc[i];
    //     reportInfo(prefix, v);
    // }
    //! find a vertex whose type is thread.
    v = ssc[0];
    while(v){
        if(v->type == VERTEX_THREAD) break;
        v = v->arcList.tail;
    }

    while(num--){
        reportInfo(prefix, v);
        v = v->arcList.tail;
    }
}

static void reportInfo(const char *prefix, vertex_t *v){
    threadInfo_t *ti;
    mutexInfo_t *mi;
    assert(v);

    if(v->type == VERTEX_THREAD){
        ti = (threadInfo_t *)v->private;
        fprintf(stderr, "%s Thread # [%ld %s]:\n", prefix, ti->tid, ti->name);
        assert(v->arcList.tail);
        mi = (mutexInfo_t *)v->arcList.tail->private;
        fprintf(stderr, "%s  \t waits the lock #%p [%p %p %p %p %p]\n",
            prefix, (void *)mi->mid,ti->backtrace[0], ti->backtrace[1], 
            ti->backtrace[2], ti->backtrace[3], ti->backtrace[4]);
    }else if(v->type == VERTEX_MUTEX){
        mi = (mutexInfo_t *)v->private;
        assert(v->arcList.tail);
        ti = (threadInfo_t *)v->arcList.tail->private;
        fprintf(stderr, "%s  \t holds the lock #%p [%p %p %p %p %p]\n", 
            prefix, (void *)mi->mid, ti->backtrace[0], ti->backtrace[1], 
            ti->backtrace[2], ti->backtrace[3], ti->backtrace[4]);
    }
}





