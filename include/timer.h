/**
 * @file    timer.h
 * @author  qufeiyan
 * @brief   define a generic timer.
 * @version 1.0.0
 * @date    2023/12/09 19:25:49
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef __TIMER_H
#define __TIMER_H
/* Include ---------------------------------------------------------------------------------*/

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    TIMER_ONCE,
    TIMER_CYCLE
}timerCycle_t;

typedef void (*timerCallback_t)(void *);

typedef struct dlcTimer{
    uint64_t period;
    uint64_t whenMs;
    timerCallback_t timerFunc;
    void *args;
    timerCycle_t cycle;
    struct dlcTimer *next; 
}dlcTimer_t;

typedef struct dlcTimerConfig{
    uint64_t period;
    uint64_t whenMs;
    timerCallback_t timerFunc;
    void *args;
    timerCycle_t cycle;
}dlcTimerConfig_t;

dlcTimer_t *dlcTimerCreate(dlcTimerConfig_t *config);
void dlcTimerDestroy(dlcTimer_t *timer);
void dlcTimerProc(void);



#ifdef __cplusplus
}
#endif

#endif	//  __TIMER_H