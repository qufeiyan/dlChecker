/**
 * @file    lfqueue.h
 * @author  qufeiyan
 * @brief   A simple lockfree queue implementation
 * @version 1.0.0
 * @date    2023/05/21 18:58:59
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef LFQUEUE_H
#define LFQUEUE_H
/* Include ---------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "dlcDef.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct lfqueue{
    uint32_t in;
    uint32_t out;
    uint32_t size;
    uint32_t esize;

    void *buffer; //！ buffer for elements in queue.
};

typedef struct lfqueue lfqueue_t;

static inline bool lfqueueIsFull(lfqueue_t *queue){
    assert(queue != NULL);
    return ((queue->in - queue->out) >= queue->size);    
}

static inline bool lfqueueIsEmpty(lfqueue_t *queue){
    assert(queue != NULL);
    return (queue->in == queue->out);    
}

static inline uint32_t lfqueueUsed(lfqueue_t *queue){
    assert(queue != NULL);
    return queue->in - queue->out;
}

static inline uint32_t lfqueueAvail(lfqueue_t *queue){
    assert(queue != NULL);
    return queue->size - lfqueueUsed(queue);
}

static inline void lfqueuePrint(lfqueue_t *queue){
    fprintf(stderr, "\n-----------------lfqueue info------------------\n");
    fprintf(stderr, "buffer \t size \t esize \t in \t out\t\n");
    fprintf(stderr, "%p \t %d \t %d \t %d\t %d\t\n", queue->buffer, queue->size, 
        queue->esize, queue->in, queue->out);
}

#define lfqueueInfo(queue) ({\
    lfqueue_t *_queue = (typeof(queue))queue;\
    lfqueuePrint(_queue);\
})

void lfqueueInit(struct lfqueue* queue, void *buffer, uint32_t size, uint32_t esize);
uint32_t lfqueuePut(struct lfqueue *queue, const void *src, uint32_t size);
uint32_t lfqueueGet(struct lfqueue *queue, void *dst, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif	//  LFQUEUE_H