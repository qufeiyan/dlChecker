#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

#define PERIOD_OF_DLCHECKER         (200)      //! uint:ms

#ifndef USER_BACKTRACE
#define IS_USER_OVERWRITE_BACKTRACE (0)
#else
#define IS_USER_OVERWRITE_BACKTRACE (1)
#endif

#define IS_USE_HASHMAP              (1)
#define IS_USE_ASSERT               (1)
#define IS_USE_HEAP                 (0)


/**
 * IS_USE_MEM_LIBC_MALLOC == 1: Use malloc/free/realloc provided by C-library
 * instead of the internal allocator. 
 */
#define IS_USE_MEM_LIBC_MALLOC      (0)  //! whether use malloc function of libc or not.

//! options
#define DEPTH_BACKTRACE             (5)

#ifndef BOOL
#define BOOL   int
#define FALSE  0
#define TRUE   1
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define DLC_OK   (0)
#define DLC_ERR  (1)

#define LOG_COLOR_NONE       "\x1B[0;m"
#define LOG_COLOR_RED        "\x1B[4;91m"
#define LOG_COLOR_GREEN      "\x1B[2;92m"
#define LOG_COLOR_YELLOW     "\x1B[2;93m"
#define LOG_COLOR_BLUE       "\x1B[0;94m"
#define LOG_COLOR_MAGENTA    "\x1B[4;95m"
#define LOG_COLOR_CYAN       "\x1B[1;96m"

#define LOG_COLOR_DEBUG      LOG_COLOR_GREEN
#define LOG_COLOR_INFO       LOG_COLOR_CYAN
#define LOG_COLOR_WARN       LOG_COLOR_YELLOW
#define LOG_COLOR_ERROR      LOG_COLOR_MAGENTA
#define LOG_COLOR_FATAL      LOG_COLOR_RED

#define LOG_CTRL_LEVEL_ERROR (1)
#define LOG_CTRL_LEVEL_WARN  (2)
#define LOG_CTRL_LEVEL_INFO  (3)
#define LOG_CTRL_LEVEL_DEBUG (4)     

#define DEBUG_GRAPH

#ifdef DEBUG_GRAPH
extern int log_ctrl_level;
#define dlc_dbg(format, ...)                                                      \
do{                                                                               \
    if(log_ctrl_level >= LOG_CTRL_LEVEL_DEBUG){                                   \
        printf(LOG_COLOR_DEBUG "[DLC_DBG %s:%d](#%s) " format LOG_COLOR_NONE ,    \
            __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);                     \
    }                                                                             \
}while(0)

#define dlc_err(format, ...)                                                      \
do{                                                                               \
    if(log_ctrl_level >= LOG_CTRL_LEVEL_ERROR){                                   \
        printf(LOG_COLOR_ERROR "[DLC_ERR %s:%d](#%s) " format LOG_COLOR_NONE ,    \
            __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);                     \
    }                                                                             \
}while(0)

#define dlc_warn(format, ...)                                                     \
do{                                                                               \
    if(log_ctrl_level >= LOG_CTRL_LEVEL_WARN){                                    \
        printf(LOG_COLOR_WARN "[DLC_WARN %s:%d](#%s) " format LOG_COLOR_NONE ,    \
            __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);                     \
    }                                                                             \
}while(0)

#define dlc_info(format, ...)                                                     \
do{                                                                               \
    if(log_ctrl_level >= LOG_CTRL_LEVEL_INFO){                                    \
        printf(LOG_COLOR_INFO "[DLC_INFO %s:%d](#%s) " format LOG_COLOR_NONE ,    \
            __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);                     \
    }                                                                             \
}while(0)


#define dlcPanic(fmt, ...)                                                        \
do {                                                                              \
    fprintf(stderr, LOG_COLOR_FATAL "[%s:%d:](#%s): " fmt LOG_COLOR_NONE,         \
        __FILE__, __LINE__, __func__, ##__VA_ARGS__);                             \
    exit(-1);                                                                     \
} while (0)  

#ifndef assert
#if IS_USE_ASSERT
#undef assert
#define assert(expression)                            \
do{                                                   \
    if(!(expression)){                                \
        dlc_err(#expression " is expected...\n");     \
        abort();                                      \
    }                                                 \
}while(0)
#else
#define assert(expression)  (void)(expression)
#endif
#endif

#endif //! end of define DEBUG_GRAPH

#if IS_USE_ASSERT
#define dlc_isInvalid(expression)                     \
do{                                                   \
    if(!!(expression)){                               \
        dlc_err(#expression " is not expected...\n"); \
        raise(SIGSEGV);                               \
    }                                                 \
}while(0) 
#else
#define dlc_isInvalid(expression)  (void)(expression)
#endif


#endif