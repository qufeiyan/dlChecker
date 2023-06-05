/**
 * @file    spinlock.h
 * @author  qufeiyan
 * @brief   
 * @version 1.0.0
 * @date    2023/05/28 22:55:08
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef SPINLOCK_H
#define SPINLOCK_H
/* Include ---------------------------------------------------------------------------------*/
#include "dlcDef.h"
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __sevl
#define __sevl() __asm__ volatile("sevl");
#endif

#ifndef __wfe
#define __wfe()  __asm__ volatile("wfe":::"memory") 
#endif

#ifndef __prefetchw
#define __prefetchw(addr) __builtin_prefetch(addr, 1) 
#endif

#ifndef __nop
#define __nop __asm__ volatile("nop")
#endif

#ifndef __doSpins
#define __doSpins(times) do{    \
    __asm__ volatile(           \
        ".rept " #times "\n"    \
        "nop\n"                 \
        ".endr\n"               \
    );                          \
}while(0)
#endif

#ifndef __yield
#define __yield()  do{\
    __asm__ volatile( \
        "yield"       \
    );                \
}while(0)
#endif

struct spinlock{
    union{
        atomic_flag lock;
        struct{
            uint16_t current;
            uint16_t next;
        } ticket;  
    };
    
    uint32_t spin;
    void (*acquire)(struct spinlock *);
    void (*release)(struct spinlock *);
};

typedef struct spinlock spinlock_t;
__weak void __lock(spinlock_t *spinlock);
__weak void __unlock(spinlock_t *spinlock);

void spinlockInit(spinlock_t *spinlock, int32_t spin);


#ifdef __cplusplus
}
#endif

#endif	//  SPINLOCK_H