/**
 * @file    hashMap.h
 * @author  qufeiyan
 * @brief   
 * @version 1.0.0
 * @date    2023/06/02 00:59:35
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef HASHMAP_H
#define HASHMAP_H
/* Include ---------------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HASH_MAP HASH_MAP;

/* define a set of oprations for hash map */
typedef struct{
    uint64_t (*hashFunction)(const void *key);
    void *(*keyDup)(HASH_MAP *map, const void *key);
    void *(*valDup)(HASH_MAP *map, const void *obj);
    int (*keyCompare)(HASH_MAP *map, const void *key1, const void *key2);
    void (*keyDestructor)(HASH_MAP *map, void *key);
    void (*valDestructor)(HASH_MAP *map, void *obj);
}HASH_MAP_OPS;

/* define a element type including key and value. */
typedef struct ENTRY_INFO{
    void*  key;
    void*  value;
    struct ENTRY_INFO *next;
}ENTRY_INFO;

struct HASH_MAP{
    HASH_MAP_OPS *type;
    /* the bucket that will be used to store key-value data. */
    ENTRY_INFO **table;
    int size; /* indicate the size of current stored data. */
    int capacity;  /* capacity of the buckets, must be Nth power of 2. */
};

/* it is a non safe iterator, and only hashMapNext()
 * should be called while iterating. */
typedef struct HASH_MAP_ITERATOR {
    HASH_MAP *map;
    int index;
    ENTRY_INFO *entry, *nextEntry;
} HASH_MAP_ITERATOR;
typedef struct HASH_MAP_ITERATOR HASH_MAP_ITERATOR;


/*--=-=-=-=-=-- hash map interface -=-=-=-=-=-=-=-*/
void* hashMapCreate(HASH_MAP_OPS* type, size_t capacity);
int hashMapPut(HASH_MAP *map, void *key, void *val);
void *hashMapGet(HASH_MAP *map, const void* key);
ENTRY_INFO *hashMapFind(HASH_MAP *map, const void *key);
ENTRY_INFO *hashMapAddRaw(HASH_MAP *map, void *key, ENTRY_INFO **existing);
int hashMapRemove(HASH_MAP *map, const void* key);
int hashMapSize(HASH_MAP* map);
void hashMapDestroy(HASH_MAP* map);
void *hashMapGetIterator(HASH_MAP *map);
ENTRY_INFO *hashMapNext(HASH_MAP_ITERATOR *iter);
void hashMapReleaseIterator(HASH_MAP_ITERATOR *iter);


#ifdef __cplusplus
}
#endif

#endif	//  HASHMAP_H