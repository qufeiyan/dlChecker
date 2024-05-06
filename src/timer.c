/**
 * @file    timer.c
 * @author  qufeiyan
 * @brief   define a generic timer.
 * @version 1.0.0
 * @date    2023/12/09 18:54:45
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/

#include <assert.h>
#include <internal.h>
#include <unistd.h>
#include "mem.h"
#include "timer.h"

static dlcTimer_t timerDummy;

long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

dlcTimer_t *dlcTimerCreate(dlcTimerConfig_t *config){
    dlcTimer_t *ret, *head, *prev;
    assert(config != NULL);
    assert(config->timerFunc != NULL);
    assert(config->whenMs != 0);

    ret = (dlcTimer_t *)zmalloc(sizeof(dlcTimer_t));
    assert(ret != NULL);

    ret->period = config->period;
    ret->whenMs = config->whenMs;
    ret->timerFunc = config->timerFunc;
    ret->args = config->args;
    ret->cycle = config->cycle;
    ret->next = NULL;

    head = timerDummy.next;
    prev = &timerDummy;
    while(head){
        head = head->next;
        prev = prev->next;
    }
    //! insert timer to the tail of timer list.
    prev->next = ret;
    dlc_dbg("ret %p, prev %p\n", ret, prev);
    return ret;
}

void dlcTimerDestroy(dlcTimer_t *timer){
    dlcTimer_t *head, *prev;
    assert(timer != NULL);

    head = timerDummy.next;
    prev = &timerDummy;
    assert(head != NULL);

    //! remove timer from timer list.
    while (head) {
        if(head == timer){
            prev->next = head->next;
            break;
        }
        prev = prev->next;
        head = head->next;
    }

    //! free memory.
    zfree(timer); 
}

static void dlcTimerUpdate(dlcTimer_t *timer){
    assert(timer);
    assert(timer->cycle == TIMER_CYCLE);

    timer->whenMs += timer->period;
}

/**
 * @brief  find the first timer will fire. 
 *
 * @param   void  
 * @return  the timer will fire.    
 */
static dlcTimer_t *dlcTimerNearest(void){
    dlcTimer_t *timer = timerDummy.next;
    dlcTimer_t *nearest = NULL;
    
    while(timer){
        if(nearest == NULL || timer->whenMs < nearest->whenMs){
            nearest = timer;
        }
        timer = timer->next;
    }

    return nearest;
}

void dlcTimerProc(void){
    dlcTimer_t *timer = timerDummy.next;
    long long now;

    now = timeInMilliseconds();
    while(timer){
        if(timer->whenMs <= now){
            timer->timerFunc(timer->args);
            // dlc_warn("timer period %lu\n", timer->period);

            if(timer->cycle == TIMER_CYCLE){
                dlcTimerUpdate(timer);
            } else{
                dlcTimerDestroy(timer);
            }
        }
        timer = timer->next;
    }

    dlcTimer_t *nearest = dlcTimerNearest();
    assert(nearest != NULL);
    now = timeInMilliseconds();
    usleep((nearest->whenMs <= now ? 0 : nearest->whenMs - now));
}
