/**
 * @file    spinlock.c
 * @author  qufeiyan
 * @brief   Generic implementation of spinlock based on atomic operation.
 * @version 1.0.0
 * @date    2023/05/24 22:27:13
 * @version Copyright (c) 2023
 */

/* Includes --------------------------------------------------------------------------------*/
#include <sched.h>
#include <stdint.h>
#include "spinlock.h"


__weak void __lock(struct spinlock *spinlock){
    assert(spinlock);
    __prefetchw(&spinlock->lock);
    for(;;){
        if(!atomic_flag_test_and_set_explicit(&spinlock->lock, memory_order_acq_rel)){
            return;
        }
        
        //! backoff
        for(int n = 1; n < spinlock->spin; n <<= 1){
            __nop;
        }

        if(!atomic_flag_test_and_set_explicit(&spinlock->lock, memory_order_relaxed)){
            return;
        }
    
        sched_yield();
    }  
}

__weak void __unlock(struct spinlock *spinlock){
    assert(spinlock);
    atomic_flag_clear_explicit(&spinlock->lock, memory_order_relaxed);
}

void spinlockInit(struct spinlock *spinlock, int32_t spin){
    assert(spinlock);
    assert(spin >= 2);

    spinlock->spin = spin;
    spinlock->acquire = __lock;
    spinlock->release = __unlock;
}




