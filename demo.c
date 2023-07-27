#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "common.h"
#include "internal.h"
#include "interface.h"

volatile long long start;

extern void dlcSetTaskName(char* name);

pthread_mutex_t mid[4 + NUMBER_OF_THREAD - 4] = {
    PTHREAD_MUTEX_INITIALIZER, 
    PTHREAD_MUTEX_INITIALIZER, 
    PTHREAD_MUTEX_INITIALIZER, 
    PTHREAD_MUTEX_INITIALIZER

};

void* func0(void *arg){
    dlcSetTaskName("func0");
    // while(1)
    {
        dlc_info("thread_routine 1 : %lu\n", (size_t)pthread_self());
        pthread_mutex_lock(&mid[0]);
        sleep(2);
        pthread_mutex_lock(&mid[1]);
        sleep(1);
        pthread_mutex_lock(&mid[0]);
        sleep(1);
        pthread_mutex_unlock(&mid[0]);
    }
    return NULL;
}

void* func1(void *arg){
    dlcSetTaskName("func1");
    // while(1)
    {
	    dlc_info("thread_routine 2 : %lu\n", (size_t)pthread_self());

        pthread_mutex_lock(&mid[1]);
        sleep(2);
        pthread_mutex_lock(&mid[2]);
        sleep(1);
        pthread_mutex_unlock(&mid[2]);
        sleep(1);
        pthread_mutex_unlock(&mid[1]);
    }
    return NULL;
}

void* func2(void *arg){
    dlcSetTaskName("func2");
    // while(1)
    {
        dlc_info("thread_routine 3 : %lu\n", (size_t)pthread_self());
        pthread_mutex_lock(&mid[2]);
        sleep(3);
        pthread_mutex_lock(&mid[0]);
        sleep(1);
        pthread_mutex_unlock(&mid[0]);
        sleep(1);
        pthread_mutex_unlock(&mid[2]);
    }
    return NULL;
}


void* func3(void *arg){
    dlcSetTaskName("func3");
    // while(1)
    {
        dlc_info("thread_routine 4 : %lu\n", (size_t)pthread_self());
        pthread_mutex_lock(&mid[3]);
        sleep(4);
        pthread_mutex_lock(&mid[3]);
        pthread_mutex_unlock(&mid[3]);
    }
    return NULL;
}

void* func(void *arg){
    size_t i = (size_t)arg;
    char name[16] = "func";
    sprintf(name, "func%ld", i);
    dlcSetTaskName(name);
    while(1)
    {
        dlc_info("thread_routine %ld : %lu\n", i, (size_t)pthread_self());
        pthread_mutex_lock(&mid[i]);
        int count = 10000;
        while(count--);
        sleep(1);
        while(count++ < 10000);
        pthread_mutex_unlock(&mid[i]);
    }
    return NULL;
}


#ifdef DLC_TEST
#include "testhelp.h"

int __failed_tests = 0;
int __test_num = 0;

/* The flags are the following:
* --accurate:     Runs tests with more iterations.
* --large-memory: Enables tests that consume more than 100mb. */
typedef int dlcTestProc(int argc, char **argv, int flags);
struct dlcTest {
    char *name;
    dlcTestProc *proc;
    int failed;
} dlcTests[] = {
    {"map", hashMapTest},
    {"mpool", mPoolTest},
    {"list", dclistTest},
    {"mem", memTest}
};
dlcTestProc *getTestProcByName(const char *name) {
    int numtests = sizeof(dlcTests) / sizeof(struct dlcTest);
    int j;
    for (j = 0; j < numtests; j++) {
        if (!strcasecmp(name, dlcTests[j].name)) {
            return dlcTests[j].proc;
        }
    }
    return NULL;
}
#endif

int main(int argc, char** argv){
#ifdef DLC_TEST
    log_ctrl_level = 4;
    int j;
    if (argc >= 3 && !strcasecmp(argv[1], "test")) {
        int flags = 0;
        for (j = 3; j < argc; j++) {
            char *arg = argv[j];
            if (!strcasecmp(arg, "--accurate")) flags |= DLC_TEST_ACCURATE;
            else if (!strcasecmp(arg, "--large-memory")) flags |= DLC_TEST_LARGE_MEMORY;
        }

        if (!strcasecmp(argv[2], "all")) {
            int numtests = sizeof(dlcTests) / sizeof(struct dlcTest);
            for (j = 0; j < numtests; j++) {
                dlcTests[j].failed = (dlcTests[j].proc(argc,argv,flags) != 0);
            }

            /* Report tests result */
            int failed_num = 0;
            for (j = 0; j < numtests; j++) {
                if (dlcTests[j].failed) {
                    failed_num++;
                    printf("[failed] Test - %s\n", dlcTests[j].name);
                } else {
                    printf("[ok] Test - %s\n", dlcTests[j].name);
                }
            }

            printf("%d tests, %d passed, %d failed\n", numtests,
                   numtests-failed_num, failed_num);

            return failed_num == 0 ? 0 : 1;
        } else {
            dlcTestProc *proc = getTestProcByName(argv[2]);
            if (!proc) return -1; /* test not found */
            return proc(argc, argv, flags);
        }

        return 0;
    }
#endif
    start = timeInMilliseconds();
 
    // void* list[] = {&mid[3]};
    // dlcFilterCreate(list, 1);
    initDeadlockChecker(3);

    dlc_err("mid[3] %p\n", &mid[3]);

    pthread_t tid[4 + NUMBER_OF_THREAD - 4];

    // pthread_attr_t attr; 
    int policy;
    // pthread_attr_init(&attr);
    // pthread_attr_getschedpolicy(&attr, &policy);
    // dlc_info("policy %d\n", policy);
    
    pthread_create(&tid[0], NULL, func0, NULL);
    pthread_create(&tid[1], NULL, func1, NULL);
    pthread_create(&tid[2], NULL, func2, NULL);
    pthread_create(&tid[3], NULL, func3, NULL);

    size_t i;
    for (i = 4; i < NUMBER_OF_THREAD; i++)
    {   
        pthread_mutex_init(&mid[i], NULL);
        pthread_create(&tid[i], NULL, func, (void *)i);
    }

    struct sched_param param;   
    param.sched_priority = 10;
    policy = SCHED_FIFO;
    dlc_info("%d \n", pthread_setschedparam(tid[0], policy, &param));
    param.sched_priority = 5;
    pthread_setschedparam(tid[1], policy, &param);
    param.sched_priority = 5;
    pthread_setschedparam(tid[2], policy, &param);
    param.sched_priority = 5;
    pthread_setschedparam(tid[3], policy, &param);

    for (i = 0; i < 4 + NUMBER_OF_THREAD - 4; i++) {
        pthread_join(tid[i], NULL);
    }   

    dlcFilterDestroy();
    return 0;
}