#ifndef __APPLE__
#define _GNU_SOURCE
#include <pthread.h>
#include <stddef.h>
#include <sys/prctl.h>
#endif
#include <dlfcn.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "common.h"
#include "interface.h"
#include "dlcDef.h"
#include "internal.h"


extern __thread dispatcher_t dispatcher;

int log_ctrl_level = 0; //! indicates log level.

extern bool isEnabledFilter; //! indicates whether to enable filter.

extern void eventLoopEnter();
extern void strongConnectedComponent();

void dlcSetTaskName(char *name) {
#ifdef __APPLE__
    pthread_setname_np(name);
#else
    prctl(PR_SET_NAME, name);
#endif
}

typedef int (*pthread_mutex_lock_t)(pthread_mutex_t *);
typedef int (*pthread_mutex_unlock_t)(pthread_mutex_t *);
pthread_mutex_lock_t pthread_mutex_lock_f;
pthread_mutex_unlock_t pthread_mutex_unlock_f;

static void init_hook() {
    pthread_mutex_lock_f = dlsym(RTLD_NEXT, "pthread_mutex_lock");
    pthread_mutex_unlock_f = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
}

void generateWaitEvent(void *arg);
void generateHoldEvent(void *arg);
void generateReleaseEvent(void *arg);

#if !IS_USER_OVERWRITE_BACKTRACE
#include <execinfo.h>
#else
extern void *__libc_stack_end;
static int backtrace(void **array, int size) {
    void *top_frame_p = NULL;
    void *current_frame_p = NULL;
    int frame_count = 0;

    top_frame_p = __builtin_frame_address(0);
    current_frame_p = top_frame_p;
    frame_count = 0;

    fprintf(stderr, LOG_COLOR_ERROR "%s %d  %p %p \n" LOG_COLOR_NONE,
            __FUNCTION__, __LINE__, __builtin_return_address(0), current_frame_p);

    if (__builtin_return_address(0) != (void *)*(long *)current_frame_p) {
        fprintf(stderr, LOG_COLOR_ERROR "%s %d backtrace error\n" LOG_COLOR_NONE,
                __FUNCTION__, __LINE__);
        return frame_count;
    }

    void *sp = NULL;
    __asm__("mov %0, sp\n" : "=r"(sp));

    dlc_dbg("%s sp %p fp %p %p\n", __FUNCTION__, sp, current_frame_p,
            &frame_count);

    if (current_frame_p != NULL &&
        current_frame_p > sp //! 当前栈底在栈中局部变量地址上面
    #ifndef __APPLE__
        && current_frame_p < __libc_stack_end
    #endif
        ) //！当前栈底比栈顶大
    {
        while (frame_count < size && current_frame_p != NULL &&
            current_frame_p > (void *)&frame_count
    #ifndef __APPLE__
            && current_frame_p < __libc_stack_end
    #endif
        ) {
        array[frame_count++] = (void *)*(long *)current_frame_p;
        current_frame_p = (void *)*(long *)(current_frame_p - 4);

        dlc_dbg("%s, %d current_frame_p %p\n", __FUNCTION__, __LINE__,
                current_frame_p);
        }
    }

    return frame_count - 1;
}
#endif

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    generateWaitEvent((void *)mutex);
    int ret = pthread_mutex_lock_f(mutex);
    generateHoldEvent((void *)mutex);
    return ret;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    int ret = pthread_mutex_unlock_f(mutex);
    generateReleaseEvent((void *)mutex);
    return ret;
}
extern long long timeInMilliseconds();
void *checker(void *arg) {
    long long now, pre, diff;
    pre = timeInMilliseconds();

    dlcSetTaskName("checker");

    usleep(100 * 1000);
    while (1) {
        //! process all event.
        eventLoopEnter();

        now = timeInMilliseconds();
        diff = now - pre;
        dlc_dbg("diff %lld\n", diff);
        if (diff >= PERIOD_OF_DLCHECKER) {
            strongConnectedComponent();
            pre = now;
        } else {
            usleep((PERIOD_OF_DLCHECKER - diff) * 1000);
        }
    }
    return NULL;
}

/**
 * @brief initialise the dlchecker...
 * @param int level[in]  Set log level. [1:error 2:warn 3:info: 4:debug]
 */
void initDeadlockChecker(int level) {
    log_ctrl_level = level;
    #if !IS_USE_MEM_LIBC_MALLOC
    memInit();
    #endif
    init_hook();
    mapAllInit();
    memPoolAllInit();

    pthread_t tid;
    pthread_create(&tid, NULL, checker, NULL);
}

void generateWaitEvent(void *arg) {
    if (dispatcher.threadCount == -1) {
        dispatcherInit(&dispatcher);
    }

    pthread_mutex_t *mutex = (pthread_mutex_t *)arg;
    //! filter logic.
    if (isEnabledFilter && isFilter(arg)) {
        dlc_err("arg %p\n", arg);
        return;
    }

    event_t *ev = &dispatcher.ev;
    ev->type = EVENT_WAITLOCK;
    ev->mutexInfo.mid = (size_t)mutex;

    //! tracker logic.
    void **bts = ev->threadInfo.backtrace;
    memset(bts, 0, DEPTH_BACKTRACE * sizeof(void *));
    __unused int n;
    n = backtrace(bts, DEPTH_BACKTRACE);

    //! the system call {@code gettid} is called only once for each thread.
    if (ev->threadInfo.tid == 0) {
    #ifdef __APPLE__
        pthread_t selfid = pthread_self();
    #else
        pid_t selfid = syscall(SYS_gettid);
    #endif

        ev->threadInfo.tid = (size_t)selfid;
    }
    dlc_dbg("%s [%p]\n", __FUNCTION__, (void *)pthread_mutex_lock);

    #if !IS_USER_OVERWRITE_BACKTRACE
    // int i;
    // char **strs = backtrace_symbols(bts, n);
    // for (i = 0; i < n; i++)
    // {
    //     dlc_dbg("[%s]\n", strs[i]);
    // }
    // extern void free(void *);
    // free(strs);
    #endif

    //! the system call {@code prctl} is called only once for each thread.
    if (ev->threadInfo.name[0] == 0) {
    #ifdef __APPLE__
        pthread_getname_np((pthread_t)ev->tid, ev->tName, sizeof(ev->tName));
    #else
        prctl(PR_GET_NAME, (unsigned long)ev->threadInfo.name);
    #endif
    }
    dlc_info("[%s %ld]tid: %ld waits mid: %p\n", ev->threadInfo.name, dispatcher.threadCount,
            ev->threadInfo.tid, (void *)ev->mutexInfo.mid);

    dispatcher.invoke(&dispatcher);
}

void generateHoldEvent(void *arg) {
    pthread_mutex_t *mutex = (pthread_mutex_t *)arg;
    //! filter logic.
    if (isEnabledFilter && isFilter(arg)) {
        dlc_err("arg %p\n", arg);
        return;
    }

    assert(dispatcher.invoke);

    event_t *ev = &dispatcher.ev;
    ev->type = EVENT_HOLDLOCK;
    assert((size_t)mutex == ev->mutexInfo.mid);

    dlc_info("[%s %ld]tid: %ld holds mid: %p\n", ev->threadInfo.name, dispatcher.threadCount,
            ev->threadInfo.tid, (void *)ev->mutexInfo.mid);

    dispatcher.invoke(&dispatcher);
}

void generateReleaseEvent(void *arg) {
    assert(dispatcher.invoke);

    pthread_mutex_t *mutex = (pthread_mutex_t *)arg;
    //! filter logic.
    if (isEnabledFilter && isFilter(arg)) {
        dlc_err("arg %p\n", arg);
        return;
    }

    event_t *ev = &dispatcher.ev;
    ev->type = EVENT_RELEASELOCK;
    ev->mutexInfo.mid = (size_t)mutex;

    void **bts = ev->threadInfo.backtrace;
    __unused int n;
    n = backtrace(bts, DEPTH_BACKTRACE);

    //! the system call {@code gettid} is called only once for each thread.
    if (ev->threadInfo.tid == 0) {
    #ifdef __APPLE__
        pthread_t selfid = pthread_self();
    #else
        pid_t selfid = syscall(SYS_gettid);
    #endif
        ev->threadInfo.tid = (size_t)selfid;
    }

    if (ev->threadInfo.name[0] == 0) {
    #ifdef __APPLE__
        pthread_getname_np((pthread_t)ev->tid, ev->tName, sizeof(ev->tName));
    #else
        prctl(PR_GET_NAME, (unsigned long)ev->threadInfo.name);
    #endif
    }
    dlc_info("[%s %ld]tid: %ld release mid: %p\n", ev->threadInfo.name, dispatcher.threadCount,
        ev->threadInfo.tid, (void *)ev->mutexInfo.mid);

    dispatcher.invoke(&dispatcher);
}
