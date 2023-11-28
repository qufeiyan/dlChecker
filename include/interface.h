#ifndef __INTERFACE_H_
#define __INTERFACE_H_

/**
 * @brief init the dlchecker.
 * @param set log level [1:error 2:warn 3:info: 4:debug] 
 */ 
void initDeadlockChecker(int level);

/**
 * @brief create dlc filter.
 * @param list  a set of mutex lock to be filter.
 * @param size  size of the list.
 * @return none.
 */ 
void dlcFilterCreate(void **list, int size);

/**
 * @brief determine whether current mutex needs to be filtered.
 * @param arg. current mutex.
 * @return true: filter, false: not.
 */ 
int isFilter(void *arg);

/**
 * @brief destroy dlc filter.
 */ 
void dlcFilterDestroy(void);


#endif


