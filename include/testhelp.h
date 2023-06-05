#ifndef __TESTHELP_H
#define __TESTHELP_H

#define DLC_TEST_ACCURATE     (1 << 0)
#define DLC_TEST_LARGE_MEMORY (1 << 1)

extern int __failed_tests;
extern int __test_num;

#define test_cond(descr,_c) do { \
    __test_num++; printf("%d - %s: ", __test_num, descr); \
    if(_c) printf("PASSED\n"); else {printf("FAILED\n"); __failed_tests++;} \
} while(0)
#define test_report() do { \
    printf("%d tests, %d passed, %d failed\n", __test_num, \
                    __test_num-__failed_tests, __failed_tests); \
    if (__failed_tests) { \
        printf("=== WARNING === We have failed tests here...\n"); \
        exit(1); \
    } \
} while(0)

#endif
