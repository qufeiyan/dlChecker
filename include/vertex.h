/**
 * @file    vertex.h
 * @author  qufeiyan
 * @brief   
 * @version 1.0.0
 * @date    2023/05/15 23:50:32
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef VERTEX_H
#define VERTEX_H
/* Include ---------------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef SIZE_OF_NAME 
#define SIZE_OF_NAME (16)

enum vertexType{
    VERTEX_THREAD,
    VERTEX_MUTEX
};

typedef enum vertexType vertexType_t;


struct vertexOperation{
    vertexType_t (*getType)();
    int (*getIndegree)();
    int (*getOutdegree)();

    void (*addEdge)();
    void (*deleteEdge)();

};
typedef struct vertexOperation vertexOperation_t;

struct arc{
    struct vertex *tail;       //! pointer to the tail of a edge.
    struct arc *next;   
};
typedef struct arc arc_t;

/** notes that vertex is abstract class and 
 *  must be implemented as a concrete subclass.
 */
struct vertex{
    vertexType_t type;
    struct {
        short dfn;
        short low;

        bool inStack;
    };
    short indegree;
    short outdegree;
    arc_t *arcList;  

    vertexOperation_t *ops;
    char private[0];
};
typedef struct vertex vertex_t;

/**
 * @brief mutex info.
 */
struct mutexInfo{
    size_t mid; /* mutex id */
};

typedef struct mutexInfo mutexInfo_t;

struct threadInfo{
    char name[SIZE_OF_NAME];
    size_t tid; /* thread id */
    void *backtrace[DEPTH_BACKTRACE];
};

typedef struct threadInfo threadInfo_t;


vertex_t *vertexCreate(vertexType_t type, vertexOperation_t *ops);
void vertexDestroy(vertexType_t type, vertex_t *vertex);
void vertexSetInfo(vertex_t *vertex, void *info);


arc_t *arcCreate(vertex_t *tail, arc_t *next);
void arcDestroy(arc_t *arc);


//! declaration for common operations.
extern vertexOperation_t ops;

#ifdef __cplusplus
}
#endif

#endif	//  VERTEX_H