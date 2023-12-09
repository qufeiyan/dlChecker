#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "hashMap.h"
#include "internal.h"
/**
 * @note A simple hash table that does not have the ability to dynamically change its size.
 * 
 * @attention 1. The usage scenario must be explicit when using it because we fixed the capacity 
 *            of table.
 *            2. All the operation is not thread-safed, we should be careful in the scenario of muilt-thread.
 */ 

#define TABLE_DEFAULT_CAPACITY          (1 << 7)   //! aka 128

#define TABLE_DEFAULT_MIN_CAPACITY      (1 << 4)   //! aka 128

// #define KEY_VALUE_INVAILID_INTEGER  (const void *)(-(1 << (sizeof(void *) - 1)) - 1)


/* ------------------------------- Macros ------------------------------------*/
#define mapSetEntryKey(map, entry, _key_)  do{                   \
    if((map)->type->keyDup)                                      \
        (entry)->key = (map)->type->keyDup((map), _key_);        \
    else                                                         \
        (entry)->key = (_key_);                                  \
}while(0)

#define mapSetEntryVal(map, entry, _val_)  do{                   \
    if((map)->type->valDup)                                      \
        (entry)->value = (map)->type->valDup((map), _val_);      \
    else                                                         \
        (entry)->value = (_val_);                                \
}while(0)

#define mapFreeEntryKey(map, entry)  do{                         \
    if ((map)->type->keyDestructor)                              \
        (map)->type->keyDestructor((map), (entry)->key);         \
}while(0)

#define mapFreeEntryVal(map, entry)  do{                         \
    if ((map)->type->valDestructor)                              \
        (map)->type->valDestructor((map), (entry)->value);       \
}while(0)

#define mapCompareKeys(map, key1, key2)                          \
    (((map)->type->keyCompare) ?                                 \
        (map)->type->keyCompare((map), key1, key2) :             \
        (key1) == (key2))

#define mapHashKey(map, key) (map)->type->hashFunction(key)
#define mapGetEntryKey(entry) ((entry)->key)
#define mapGetEntryVal(entry) ((entry)->value)
#define mapSize(map) ((map)->size)


/* global sigleton in this module. */
// static HASH_MAP hashMap[2] = {};

/**
 * @brief a method for obataining the hashcode of integer key to be insert to map.
 */
size_t _hashCodeOfInteger(size_t key){
    size_t h = (sizeof(size_t) == 4 ? key : (key ^ (key >> 32)));
    return (h ^ (h >> 16));
}

size_t _hashCodeOfString(const char* str){
    return 0;
}


/* Our hash table capability is a power of two */
static unsigned long _mapNextPower(unsigned long size)
{
    unsigned long i = TABLE_DEFAULT_MIN_CAPACITY;

    if (size >= LONG_MAX) return LONG_MAX;
    while(1) {
        if (i >= size)
            return i;
        i *= 2;
    }
}

/* Returns the index of a free slot that can be populated with
 * a hash entry for the given 'key'.
 * If the key already exists, -1 is returned
 * and the optional output parameter may be filled.
 *
 * Note that if we are in the process of rehashing the hash table, the
 * index is always returned in the context of the second (new) hash table. */
static long _mapKeyIndex(HASH_MAP *map, const void *key, uint64_t hash, ENTRY_INFO **existing)
{
    unsigned long idx;
    ENTRY_INFO *he;
    if (existing) *existing = NULL;

    idx = hash & (map->capacity - 1);
    // dlc_dbg("key %lu, hash %llu, idx %ld\n", (size_t)key, hash, idx);
    /* Search if this slot does not already contain the given key */
    he = map->table[idx];
    while(he) {
        if (key == he->key || mapCompareKeys(map, key, he->key)) {
            if (existing) *existing = he;
            return -1;
        }
        he = he->next;
    }
    return idx;
}

/* Low level add or find:
 * This function adds the entry but instead of setting a value returns the
 * dictEntry structure to the user, that will make sure to fill the value
 * field as they wish.
 *
 * This function is also directly exposed to the user API to be called
 * mainly in order to store non-pointers inside the hash value, example:
 *
 * entry = dictAddRaw(dict,mykey,NULL);
 * if (entry != NULL) dictSetSignedIntegerVal(entry,1000);
 *
 * Return values:
 *
 * If key already exists NULL is returned, and "*existing" is populated
 * with the existing entry if existing is not NULL.
 *
 * If key was added, the hash entry is returned to be manipulated by the caller.
 */
ENTRY_INFO *hashMapAddRaw(HASH_MAP *map, void *key, ENTRY_INFO **existing)
{
    long index;
    ENTRY_INFO *entry;
    /* Get the index of the new element, or -1 if
     * the element already exists. */
    if ((index = _mapKeyIndex(map, key, mapHashKey(map, key), existing)) == -1)
        return NULL;
    // dlc_info("key %ld, index %lu\n", (long)key, index);
    entry = zmalloc(sizeof(*entry));

    entry->next = map->table[index];
    map->table[index] = entry;
    map->size++;

    /* Set the hash entry fields. */
    mapSetEntryKey(map, entry, key);
    return entry;
}

/* Add or Overwrite:
 * Add an element, discarding the old value if the key already exists.
 * Return 1 if the key was added from scratch, 0 if there was already an
 * element with such key and hashMapPut() just performed a value update
 * operation. 
 */
int hashMapPut(HASH_MAP *map, void *key, void *val)
{
    ENTRY_INFO *entry, *existing, auxentry;

    /* Try to add the element. If the key
     * does not exists Add will succeed. */
    entry = hashMapAddRaw(map, key, &existing);
    if (entry) {
        mapSetEntryVal(map, entry, val);
        return 1;
    }

    /* Set the new value and free the old one. Note that it is important
     * to do that in this order, as the value may just be exactly the same
     * as the previous one. In this context, think to reference counting,
     * you want to increment (set), and then decrement (free), and not the
     * reverse. */
    auxentry = *existing;
    mapSetEntryVal(map, existing, val);
    mapFreeEntryVal(map, &auxentry);
    return 0;
}

/**
 * @brief initialise the map.
 * @param map  [in] hash map to be initialised.
 * @param type [in] type of key-value pair. 
 * @param table [in] the bucket that will be used to store key-value data.
 * @param capacity [in] the size of bucket.
 */
void* hashMapCreate(HASH_MAP_OPS* type, size_t capacity){
    dlc_isInvalid(type == NULL);

    HASH_MAP* map = zmalloc(sizeof(HASH_MAP));

    map->type = type; 

    capacity = _mapNextPower(capacity);
    map->table = zmalloc(sizeof(ENTRY_INFO *) * capacity);
    memset(map->table, 0, sizeof(ENTRY_INFO *) * capacity);
    map->capacity = capacity;
    map->size = 0;
    return map;
}

#if 0
/**
 * @brief put key-val into map. add or replace.
 * @param type  [in] type of key to be insert.
 * @param key   [in]
 * @param value [in]
 */
void hashMapPut(HASH_MAP* map, const void* key, const void* value){
    dlc_isInvalid(map == NULL || key == NULL || value == NULL);    
    
    int index = map->hashCode(type,  key) & (TABLE_DEFAULT_CAPACITY - 1);
    ENTRY_INFO *table = map->table;
    if(type == TYPE_ENTRY_INTERGER && key == KEY_VALUE_INVAILID_INTEGER){ 
        table[index].key = key;
        table[index].value = value;    
        table[index].next = NULL;
        map->size++;
    }else{
        ENTRY_INFO *head = map->table[index];

        while (head){
            if(map->equal(type, head->key, key)){
                head->value = value;
                return;
            }
            head = head->next;
        }

        ENTRY_INFO *cur = allocFromPool(TYPE_ENTRY_INFO);
        assert(cur == NULL);
        cur->key = key;
        cur->value = value;
        /* head-insert */
        cur->next = map->table[index].next;
        map->table[index].next = cur;
        map->size++;
    }
}
#endif 

ENTRY_INFO *hashMapFind(HASH_MAP *map, const void *key)
{
    ENTRY_INFO *entry;
    uint64_t idx;

    if (mapSize(map) == 0) return NULL; /* dict is empty */

    idx = mapHashKey(map, key) & (map->capacity - 1);
    entry = map->table[idx];
    while(entry) {
        if (key == entry->key || mapCompareKeys(map, key, entry->key))
            return entry;
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief get k-v pair from the map.
 * @param type [in] type of key to be get.
 * @param key  [in]
 * @return val.
 */
void* hashMapGet(HASH_MAP *map, const void* key){
    dlc_isInvalid(map == NULL);

    ENTRY_INFO *entry = NULL;

    entry = hashMapFind(map, key);
    return entry ? mapGetEntryVal(entry) : NULL;
}

/**
 * @brief remove the k-v pair from the map.
 * @param type [in] type of key.
 * @param key  [in]
 * @return 0 on success or -1 if the element was not found. 
 */ 
int hashMapRemove(HASH_MAP *map, const void* key){
    dlc_isInvalid(map == NULL);
    int index = mapHashKey(map,  key) & (map->capacity - 1);
    ENTRY_INFO *cur = map->table[index];
    ENTRY_INFO *pre = NULL;
    while(cur){
        if(key == cur->key || mapCompareKeys(map, key, cur->key)){
            /* Unlink the element from the list */
            if (pre)
                pre->next = cur->next;
            else
                map->table[index] = cur->next;
            mapFreeEntryKey(map, cur);
            mapFreeEntryVal(map, cur);
            zfree(cur);
            map->size--;
            return 0;
        }
        pre = cur;
        cur = cur->next;
    }
    return -1;
}

/**
 * @brief current size of the map. 
 * @return size
 */
inline int hashMapSize(HASH_MAP *map){
    return map->size;
}

void hashMapDestroy(HASH_MAP* map){
    dlc_isInvalid(map == NULL);
    if (mapSize(map) == 0) return ; /* dict is empty */

    int i;
    /* Free all the elements */
    for (i = 0; i < map->capacity && map->size > 0; i++) {
        ENTRY_INFO *he, *nextHe;
        if ((he = map->table[i]) == NULL) continue;
        while(he) {
            nextHe = he->next;
            mapFreeEntryKey(map, he);
            mapFreeEntryVal(map, he);
            zfree(he);
            map->size--;
            he = nextHe;
        }
    }
    /* Free the table and the allocated cache structure */
    zfree(map->table);
    zfree(map);
}

/*---------- Iterator for map -------------*/
/**
 * @brief get an iterator for a map.
 * @param map the map which will be traversed.
 * @return iterator.
 */ 
void *hashMapGetIterator(HASH_MAP *map)
{
    HASH_MAP_ITERATOR *iter = zmalloc(sizeof(*iter));

    iter->map = map;
    iter->index = -1;
    iter->entry = NULL;
    iter->nextEntry = NULL;
    return iter;
}

ENTRY_INFO *hashMapNext(HASH_MAP_ITERATOR *iter)
{
    while (1) {
        if (iter->entry == NULL) {
            iter->index++;
            if (iter->index >= iter->map->capacity) {
                break;
            }
            iter->entry = iter->map->table[iter->index];
        } else {
            iter->entry = iter->nextEntry;
        }
        if (iter->entry) {
            /* We need to save the 'next' here, the iterator user
             * may delete the entry we are returning. */
            iter->nextEntry = iter->entry->next;
            return iter->entry;
        }
    }
    return NULL;
}

void hashMapReleaseIterator(HASH_MAP_ITERATOR *iter)
{
    zfree(iter);
}

/*------ test -------*/
/* ------------------------------- Benchmark ---------------------------------*/

#ifdef DLC_TEST
#include "testhelp.h"

#define UNUSED(V) ((void) V)

uint64_t hashCallback(const void *key) {
    return _hashCodeOfInteger((size_t)key);
}

int compareCallback(HASH_MAP *map, const void *key1, const void *key2) {
    int l1,l2;
    UNUSED(map);

    l1 = strlen((char*)key1);
    l2 = strlen((char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

void freeCallback(HASH_MAP *map, void *val) {
    UNUSED(map);

    zfree(val);
}

char *stringFromLongLong(long long value) {
    char buf[32];
    int len;
    char *s;

    len = snprintf(buf,sizeof(buf),"%lld",value);
    s = zmalloc(len+1);
    memcpy(s, buf, len);
    s[len] = '\0';
    return s;
}

HASH_MAP_OPS BenchmarkDictType = {
    hashCallback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

#define start_benchmark() start = timeInMilliseconds()
#define end_benchmark(msg) do { \
    elapsed = timeInMilliseconds() - start; \
    printf(msg ": %ld items in %lld ms, memory used %lu\n", count, elapsed, zmalloc_used_memory()); \
} while(0)

/* ./demo test dict [<count> | --accurate] */
int hashMapTest(int argc, char **argv, int flags) {
    long j;
    long long start, elapsed;
    HASH_MAP *map;
    map = hashMapCreate(&BenchmarkDictType, TABLE_DEFAULT_CAPACITY);
    long count = 0;
    int accurate = (flags & DLC_TEST_ACCURATE);

    if (argc == 4) {
        if (accurate) {
            count = 5000000;
        } else {
            count = strtol(argv[3],NULL,10);
        }
    } else {
        count = 5000;
    }

    dlc_dbg("count %ld\n", count);

    start_benchmark();
    for (j = 0; j < count; j++) {
        hashMapPut(map, (void *)j, (void*)j);
    }
    end_benchmark("Inserting");
    assert((long)mapSize(map) == count);

    start_benchmark();
    for (j = 0; j < count; j++) {
        ENTRY_INFO *de = hashMapFind(map, (void *)j);
        assert(de != NULL);
    }
    end_benchmark("Linear access of existing elements");

    start_benchmark();
    HASH_MAP_ITERATOR *iter = hashMapGetIterator(map);
    int iters = mapSize(map);
    while(hashMapNext(iter)){
        iters--;
    }
    assert(iters == 0);
    hashMapReleaseIterator(iter);
    end_benchmark("Iterating");

    start_benchmark();
    for (j = 0; j < count; j++) {
        ENTRY_INFO *de = hashMapFind(map, (void *)j);
        assert(de != NULL);
    }
    end_benchmark("Linear access of existing elements (2nd round)");

    start_benchmark();
    for (j = 0; j < count; j++) {
        ENTRY_INFO *de = hashMapFind(map, (void *)(rand() % count));
        assert(de != NULL);
    }
    end_benchmark("Random access of existing elements");

    start_benchmark();
    for (j = 0; j < count; j++) {
        void *key = (void *)j;
        int retval = hashMapRemove(map, key);
        assert(retval == DLC_OK);
        key += 17; /* Change first number to letter. */
        hashMapPut(map, key, (void*)j);
    }
    end_benchmark("Removing and adding");

    start_benchmark();
    hashMapDestroy(map);
    end_benchmark("Destroy map");
    return 0;
}
#endif


