#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

//! define the atomic operation.
#define dlcAtomic
#if (__i386 || __amd64 || __powerpc__) && __GNUC__
#define GNUC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if defined(__clang__)
#define HAVE_ATOMIC
#endif
#if (defined(__GLIBC__) && defined(__GLIBC_PREREQ))
#if (GNUC_VERSION >= 40100 && __GLIBC_PREREQ(2, 6))
#define HAVE_ATOMIC
#endif
#endif
#endif

#if defined(dlcAtomic)
#define HAVE_ATOMIC
#endif


#if defined(HAVE_ATOMIC)
/* Implementation using __sync macros. */

#define atomicIncr(var,count) __sync_add_and_fetch(&var,(count))
#define atomicGetIncr(var,oldvalue_var,count) do { \
    oldvalue_var = __sync_fetch_and_add(&var,(count)); \
} while(0)
#define atomicDecr(var,count) __sync_sub_and_fetch(&var,(count))
#define atomicGet(var,dstvar) do { \
    dstvar = __sync_sub_and_fetch(&var,0); \
} while(0)
#define atomicSet(var,value) do { \
    while(!__sync_bool_compare_and_swap(&var,var,value)); \
} while(0)
/* Actually the builtin issues a full memory barrier by default. */
#define atomicGetWithSync(var,dstvar) do { \
    dstvar = __sync_sub_and_fetch(&var,0,__sync_synchronize); \
} while(0)
#define atomicSetWithSync(var,value) do { \
    while(!__sync_bool_compare_and_swap(&var,var,value,__sync_synchronize)); \
} while(0)
#define DLC_ATOMIC_API "sync-builtin"
#endif

#if defined(__APPLE__) && (IS_USE_MEM_LIBC_MALLOC)
#include <malloc/malloc.h>
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) malloc_size(p)
#elif !IS_USE_MEM_LIBC_MALLOC
#define HAVE_MALLOC_SIZE 1
#include "mem.h"
#define zmalloc_size(p) memAllocSize(p)
#define malloc memAlloc
#define free memFree
#define calloc memCalloc
#endif

#ifdef HAVE_MALLOC_SIZE
#define PREFIX_SIZE (0)
#else
#if defined(__sun) || defined(__sparc) || defined(__sparc__)
#define PREFIX_SIZE (sizeof(long long))
#else
#define PREFIX_SIZE (sizeof(size_t))
#endif
#endif

#ifdef HAVE_ATOMIC
#define update_zmalloc_stat_alloc(__n) atomicIncr(used_memory,(__n))
#define update_zmalloc_stat_free(__n) atomicDecr(used_memory,(__n))

#define dlcAtomic 
#endif

#define MALLOC_MIN_SIZE(x) ((x) > 0 ? (x) : sizeof(long))

static dlcAtomic size_t used_memory = 0;

size_t zmalloc_used_memory(void);

static void zmalloc_default_oom(size_t size) {
    fprintf(stderr, "zmalloc: Out of memory trying to allocate %zu bytes, used %lu\n",
        size, zmalloc_used_memory());
    fflush(stderr);
    abort();
}

static void (*zmalloc_oom_handler)(size_t) = zmalloc_default_oom;

/* Try allocating memory, and return NULL if failed.
 * '*usable' is set to the usable size if non NULL. */
void *ztrymalloc_usable(size_t size, size_t *usable) {
    /* Possible overflow, return NULL, so that the caller can panic or handle a failed allocation. */
    if (size >= SIZE_MAX / 2) return NULL;
    void *ptr = malloc(MALLOC_MIN_SIZE(size) + PREFIX_SIZE);

    if (!ptr) return NULL;
#ifdef HAVE_MALLOC_SIZE
    size = zmalloc_size(ptr);
    update_zmalloc_stat_alloc(size);
    if (usable) *usable = size;
    return ptr;
#else
    *((size_t*)ptr) = size;
    update_zmalloc_stat_alloc(size + PREFIX_SIZE);
    if (usable) *usable = size;
    return (char*)ptr+PREFIX_SIZE;
#endif
}

/* Allocate memory or panic */
void *zmalloc(size_t size) {
    void *ptr = ztrymalloc_usable(size, NULL);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}

/* Try allocating memory, and return NULL if failed. */
void *ztrymalloc(size_t size) {
    void *ptr = ztrymalloc_usable(size, NULL);
    return ptr;
}


/* Try allocating memory and zero it, and return NULL if failed.
 * '*usable' is set to the usable size if non NULL. */
void *ztrycalloc_usable(size_t size, size_t *usable) {
    /* Possible overflow, return NULL, so that the caller can panic or handle a failed allocation. */
    if (size >= SIZE_MAX / 2) return NULL;
    void *ptr = calloc(1, MALLOC_MIN_SIZE(size)+PREFIX_SIZE);
    if (ptr == NULL) return NULL;

#ifdef HAVE_MALLOC_SIZE
    size = zmalloc_size(ptr);
    update_zmalloc_stat_alloc(size);
    if (usable) *usable = size;
    return ptr;
#else
    *((size_t*)ptr) = size;
    update_zmalloc_stat_alloc(size+PREFIX_SIZE);
    if (usable) *usable = size;
    return (char*)ptr+PREFIX_SIZE;
#endif
}

/* Allocate memory and zero it or panic */
void *zcalloc(size_t size) {
    void *ptr = ztrycalloc_usable(size, NULL);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}

/* Try allocating memory, and return NULL if failed. */
void *ztrycalloc(size_t size) {
    void *ptr = ztrycalloc_usable(size, NULL);
    return ptr;
}

void zfree(void *ptr) {
#ifndef HAVE_MALLOC_SIZE
    void *realptr;
    size_t oldsize;
#endif

    if (ptr == NULL) return;
#ifdef HAVE_MALLOC_SIZE
    update_zmalloc_stat_free(zmalloc_size(ptr));
    free(ptr);
#else
    realptr = (char*)ptr-PREFIX_SIZE;
    oldsize = *((size_t*)realptr);
    update_zmalloc_stat_free(oldsize+PREFIX_SIZE);
    free(realptr);
#endif
    ptr = NULL;
}

size_t zmalloc_used_memory(void) {
    size_t um;
    atomicGet(used_memory,um);
    return um;
}

void zmalloc_set_oom_handler(void (*oom_handler)(size_t)) {
    zmalloc_oom_handler = oom_handler;
}

// int log_ctrl_level = 3;
// void main() {
  
//     zmalloc(sizeof(int));
    
// }