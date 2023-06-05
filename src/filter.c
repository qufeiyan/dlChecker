#include "internal.h"
#include "interface.h"

static hashMap_t *filters;
bool isEnabledFilter = false;

//! hash function for long.
static uint64_t hashCallback(const void *key) {
    extern size_t _hashCodeOfInteger(size_t key);
    return _hashCodeOfInteger((size_t)key);
}

static HASH_MAP_OPS IntegerDictType = {
    hashCallback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/**
 * @brief create dlc filter.
 * @param list  a set of mutex lock to be filter.
 * @param size  size of the list.
 * @return none.
 */ 
void dlcFilterCreate(void **list, int size){
    assert(list && size > 0);
    filters = hashMapCreate(&IntegerDictType, 1 >> 4);
    size_t i;
    for ( i = 0; i < size; i++)
    {   
        hashMapPut(filters, list[i], (void *)i);
    }

    isEnabledFilter = true;
}

/**
 * @brief push a mutex into filters.
 * @param arg.  the mutex to be filter.
 * @return none.
 */ 
void setFilter(void *arg){
    assert(filters && arg);
    size_t val = hashMapSize(filters);
    hashMapPut(filters, arg, (void *)val);
}

/**
 * @brief determine whether current mutex needs to be filtered.
 * @param arg. current mutex.
 * @return true: filter, false: not.
 */ 
BOOL isFilter(void *arg){
    assert(arg); 
    return hashMapFind(filters, arg) ? TRUE : FALSE;
}

/**
 * @brief destroy dlc filter.
 */ 
void dlcFilterDestroy(){
    hashMapDestroy(filters);
}


