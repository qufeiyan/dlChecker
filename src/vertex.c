/**
 * @file    vertex.c
 * @author  qufeiyan
 * @brief   defination of the vertex.
 * @version 1.0.0
 * @date    2023/05/15 22:16:05
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include <string.h>
#include "mempool.h"
#include "vertex.h"
#include "common.h"
#include "internal.h"


/*--------------------internal methods --------------------*/
static vertexType_t getType(struct vertex* ver){
    assert(ver);
    return ver->type;
}

static int getIndegree(struct vertex* ver){
    assert(ver);
    return ver->indegree;
}

static int getOutdegree(struct vertex* ver){
    assert(ver);
    return ver->outdegree;
}

static void addEdge(vertex_t *u, vertex_t *v){
    assert(u && v);
    
    arc_t *head = u->arcList;
    while(head){
        assert(head->tail != v);
        head = head->next;
    }

    arc_t *newArc = arcCreate(v, NULL);
    newArc->next = u->arcList;
    u->arcList = newArc;

    u->outdegree++;
    v->indegree++;
}

static void deleteEdge(vertex_t *u, vertex_t *v){
    arc_t *head, dummy = {0};
    assert(u && v);

    //! A thread can request only one lock at any time, 
    //！and a lock can only be occupied by one thread at any time.
    //! so, from a vertex, there is only one edge pointing to a fixed 
    //! vertex at any time. 
    
    dummy.next = u->arcList;
    head = &dummy;

    //! find the arc whose tail is v.
    while(head->next){
        if(head->next->tail == v){
            break;
        }
        head = head->next;
    }

    assert(head->next != NULL);

    //! remove the edge. 
    arc_t *target = head->next;
    head->next = head->next->next;
    target->next = NULL;
    target->tail = NULL;
    arcDestroy(target);
    target = NULL;

    u->arcList = dummy.next;

    //! update the outdegree and indegree. 
    u->outdegree--;
    v->indegree--;
}

vertexOperation_t ops = {
    .getType = getType,
    .getIndegree = getIndegree,
    .getOutdegree = getOutdegree,
    .addEdge = addEdge,
    .deleteEdge = deleteEdge

};

static vertex_t *threadVertexConstructor(vertexOperation_t *ops){
    vertex_t *tv;
    assert(ops != NULL);
    assert(threadVertexMemPool != NULL);

    tv = (vertex_t *)memPoolAlloc(threadVertexMemPool);
    if(tv == NULL){
        memPoolInfo(threadVertexMemPool);
    }
    memset(tv, 0, sizeof(vertex_t) + sizeof(struct threadInfo));

    tv->type = VERTEX_THREAD;
    tv->ops = ops;

    return tv;
}

static vertex_t *mutexVertexConstructor(vertexOperation_t *ops){
    vertex_t *mv;
    assert(ops != NULL);
    assert(mutexVertexMemPool != NULL);

    mv = (vertex_t *)memPoolAlloc(mutexVertexMemPool);
    if(mv == NULL){
        memPoolInfo(mutexVertexMemPool);
    }
    memset(mv, 0, sizeof(vertex_t) + sizeof(struct mutexInfo));

    mv->type = VERTEX_MUTEX;
    // mv->arcList = NULL;
    mv->ops = ops;

    return mv;
}

static void threadVertexDestructor(vertex_t *tv){
    assert(tv != NULL);
    assert(threadVertexMemPool != NULL);

    memPoolFree(threadVertexMemPool, tv);
}

static void mutexVertexDestructor(vertex_t *mv){
    assert(mv != NULL);
    assert(mutexVertexMemPool != NULL);

    memPoolFree(mutexVertexMemPool, mv);
}

static void threadVertexSetInfoImpl(vertex_t *tv, void *info){
    threadInfo_t *res;
    assert(tv != NULL && info != NULL);

    res = (threadInfo_t *)&tv->private[0];
    *res = *(threadInfo_t *)info;
}

static void mutexVertexSetInfoImpl(vertex_t *mv, void *info){
    mutexInfo_t *res;
    assert(mv != NULL && info != NULL);

    res = (mutexInfo_t *)&mv->private[0];
    *res = *(mutexInfo_t *)info;
}

//! global constructor array for all vertex.
static vertex_t *(*vertexConstructor[])(vertexOperation_t *) = {
    threadVertexConstructor, 
    mutexVertexConstructor
};

//! global destructor array for all vertex.
static void (*vertexDestructor[])(vertex_t *) = {
    threadVertexDestructor, 
    mutexVertexDestructor
};

//! global vertex's setInfo.  
static void (*vertexSetInfoImpl[])(vertex_t *, void *) = {
    threadVertexSetInfoImpl,
    mutexVertexSetInfoImpl
};

/**
 * @brief   Create a vertex object based on the vertex type.
 
 * @param   type is the type of vertex.
 * @param   ops is pointer to the opretion sets of a vertex.
 * @return  the concrete vertex object.
 * @note    
 * @see     
 */
vertex_t *vertexCreate(vertexType_t type, vertexOperation_t *ops){
    vertex_t *ret;
    
    assert(ops);
    assert(vertexConstructor != NULL);
    ret = vertexConstructor[type](ops);
    return ret;
}

/**
 * @brief   Destroy a vertex object based on the vertex type.
 
 * @param   type is the type of vertex.
 * @param   vertex is pointer to the concrete vertex object.
 * @note    The method is rarely called because a vertex object is rarely destroyed.
 */
void vertexDestroy(vertexType_t type, vertex_t *vertex){
    assert(vertex != NULL);
    assert(vertexDestructor != NULL);

    vertexDestructor[type](vertex);
}

void vertexSetInfo(vertex_t *vertex, void *info){
    assert(info != NULL);
    assert(vertexSetInfoImpl != NULL);

    vertexSetInfoImpl[vertex->type](vertex, info);
}

arc_t *arcCreate(vertex_t *tail, arc_t *next){
    arc_t *ret;

    assert(tail);
    ret = memPoolAlloc(arcMemPool);
    if(ret == NULL){
        memPoolInfo(arcMemPool);
    }

    assert(ret != NULL);
    memset(ret, 0, sizeof(arc_t));

    ret->tail = tail;
    ret->next = next;

    return ret;
}

void arcDestroy(arc_t *arc){
    assert(arc);

    memPoolFree(arcMemPool, arc);
}