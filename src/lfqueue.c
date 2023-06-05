/**
 * @file    lfqueue.c
 * @author  qufeiyan
 * @brief   A generic lockfree queue implementation. 
 *          only one concurrent reader and one concurrent writer, 
 *          you don't need extra locking to use put/get functions. 
 * @version 1.0.0
 * @date    2023/05/21 18:57:32
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include "lfqueue.h"
#include "dlcDef.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include <stddef.h>
#include <unistd.h>
#include "common.h"


#define lfqueueAssert(expression) do{                 \
    if(!(expression)){                                \
        dlc_err(#expression " is expected...\n");     \
        abort();                                      \
    }                                                 \
}while(0)

/**
 * @brief   Initialise a lockfree quque.
 
 * @param   queue is pointer to the lfqueue.
 * @param   buffer is pointer to the element buffer.
 * @param   size is the count of elements.
 * @param   esize is the size of element in bytes.
 * @note    size is the count of elements, not the size. 
 * @see     
 */
void lfqueueInit(struct lfqueue* queue, void *buffer, uint32_t size, uint32_t esize){
    assert(queue != NULL);
    assert(buffer != NULL);
    assert(esize > 0);
    assert(size >= 2);

    queue->in = 0;
    queue->out = 0;
    queue->esize = esize;
    if(!is_power_of_2(size))
        size = __rounddown_pow_of_two(size);
    queue->size = size;
    queue->buffer = buffer;

    assert(queue->buffer);
}

/**
 * @brief   Put blocks of element into the queue. If the capacity of queue 
 *          is insufficient, it will discard out-of-range data.
 * 
 * @param   queue is the lockfree queue.
 * @param   src is pointer to the element buffer.
 * @param   size is count of elements.
 * @return  Return the data size we put into the queue.
 * @note    size is {@code (the size of src) / esize}, unit is not one byte.
 * @see     
 */
uint32_t lfqueuePut(struct lfqueue *queue, const void *src, uint32_t size){
    uint32_t avail;
    uint32_t offset;

    assert(queue && src);

    avail = lfqueueAvail(queue);

    if (avail == 0) {
        dlc_info("avail %d\n", avail);
        return 0;
    }

    //! drop some data.
    if(avail < size){
        size = avail;
    }

    offset = queue->in & (queue->size - 1);
    if(size <= queue->size - offset){
        memcpy((queue->buffer + offset * queue->esize), src, size * queue->esize);
    }else{
        memcpy(queue->buffer + offset * queue->esize, src, 
            (queue->size - offset) * queue->esize);
        memcpy(queue->buffer, (const uint8_t *)src + (queue->size - offset) * queue->esize, 
            (size - (queue->size - offset)) * queue->esize);
    }

    // atomic_thread_fence(memory_order_release);

    queue->in += size;
    return size;
}

/**
 * @brief   Get data frome the queue.
 
 * @param   queue is lockfree queue.
 * @param   dst is pointer to the element buffer.
 * @param   size is the count of elements we want to read from the queue.
 * @return  Return the element count we read from the queue.
 * @note    size is {@code (the size of src) / esize}, unit is not one byte.
 * @see     
 */
uint32_t lfqueueGet(struct lfqueue *queue, void *dst, uint32_t size){
    uint32_t used;
    uint32_t offset;

    assert(queue);

    used = lfqueueUsed(queue);
    if(used == 0){
        return 0;
    }

     /* less data */
    if (used < size){
        size = used;
    }

    offset = queue->out & (queue->size - 1);
    if(size <= queue->size - offset){
        memcpy(dst, (queue->buffer + offset * queue->esize), size * queue->esize);
    }else{
        memcpy(dst, queue->buffer + offset * queue->esize, 
            (queue->size - offset) * queue->esize);
        memcpy(dst + (queue->size - offset) * queue->esize , queue->buffer, 
            (size - (queue->size - offset)) * queue->esize);
    }

    // atomic_thread_fence(memory_order_release);

    queue->out += size;
    return size;
}


#if 0
#include <stdio.h>
struct ele{
    char name[5];
    int  age;
};

lfqueue_t queue;

#undef assert
#define assert(exp) do{ \
    if(!(exp)){ \
        printf("%s %d: Assertion "#exp" failed\n", __FILE__, __LINE__); \
    }\
}while(0)

void *producer(void *arg){
    while(1){
        struct ele zhang = {
            .age = 14
        };
        strncpy(zhang.name, "zhang", strlen("zhang"));

        struct ele li = {
            .age = 15
        };
        strncpy(li.name, "li", strlen("li"));

        struct ele ss[2] = {
            zhang, li 
        };

        int ret = lfqueuePut(&queue, &zhang, 1);
        assert(ret == 1);
        printf("avail is %u\n", lfqueueAvail(&queue));

        ret = lfqueuePut(&queue, &li, 1);
        assert(ret == 1);
        printf("avail is %u\n", lfqueueAvail(&queue));

        ret = lfqueuePut(&queue, &ss, 2);
        assert(ret == 2);
        printf("avail is %u\n", lfqueueAvail(&queue));

        ret = lfqueuePut(&queue, &ss, 2);
        assert(ret == 2);
        printf("avail is %u\n", lfqueueAvail(&queue));

        ret = lfqueuePut(&queue, &ss, 2);
        assert(ret == 2);
        printf("avail is %u\n", lfqueueAvail(&queue));

        ret = lfqueuePut(&queue, &ss, 2);
      //  assert(ret == 2);
        printf("avail is %u\n", lfqueueAvail(&queue));
        sleep(1);
    }

    return NULL;
}

void *consumer(void *arg){
    int ret;
    while(1){
        struct ele get[8];
        int used;
        if((used = lfqueueUsed(&queue)) > 0){
            ret = lfqueueGet(&queue, &get, used);
            printf("ret = %d, used %d\n", ret, used);
        }

        for (int i = 0; i < used; ++i) {
            printf("get %d %s %d \n", i, get[i].name, get[i].age);
        }

        printf("avail is %u\n", lfqueueAvail(&queue));
        sleep(1);
    }
    return NULL;
}

#include <pthread.h>
int main(){
    int ret;
    struct ele buffer[10];

    lfqueueInit(&queue, buffer, 10, sizeof(struct ele));

    pthread_t tid[2];
    pthread_create(&tid[0], NULL, producer, NULL);
    pthread_create(&tid[1], NULL, consumer, NULL);

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    return 0;
}
#endif