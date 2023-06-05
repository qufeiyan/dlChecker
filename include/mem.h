/**
 * @file    mem.h
 * @author  qufeiyan
 * @brief   
 * @version 1.0.0
 * @date    2023/06/02 22:16:41
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef MEM_H
#define MEM_H
/* Include ---------------------------------------------------------------------------------*/

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

//! small malloc.
/*------------- mem --------------*/
void memInit(void);
void* memAlloc(size_t size);
void * memCalloc(size_t count, size_t size);
void memFree(void* ptr);
size_t memAllocSize(const void *ptr);
//! extern interface
void *zmalloc(size_t size);
void *ztrymalloc(size_t size);
void *zcalloc(size_t size);
void *ztrycalloc(size_t size);
void zfree(void *ptr);
size_t zmalloc_used_memory(void);

#ifdef __cplusplus
}
#endif

#endif	//  MEM_H