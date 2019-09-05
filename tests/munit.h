#ifndef _TEST_MUNIT_H
#define _TEST_MUNIT_H 1

#include "../subprojects/munit/munit.h"

/* Forward declaration of a test function */
#define MUNIT_TESTFN(name) MunitResult name(const MunitParameter params[], void *data)

/* Declare items within a test suite */
#define MUNIT_TEST(name, fn) { (char*) "/"#name, fn, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
#define MUNIT_TEST_PARAM(name, fn, param) { (char*) "/"#name, fn, NULL, NULL, MUNIT_TEST_OPTION_NONE, param },

/* Declare a test suite */
#define MUNIT_SUITE(name, ...) \
    MunitTest name##_tests[] = { \
        __VA_ARGS__ \
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL } \
    }; \
    const MunitSuite name##_test_suite = { \
        (char*) "/"#name, name##_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE \
    }

#endif /* _TEST_MUNIT_H */
