#include "munit.h"
#include "test_bsearch.h"

static MunitResult test_hello(const MunitParameter params[], void *data) {
    char hello[] = "hello, unit tests";
    munit_assert_string_equal(hello, "hello, unit tests");
    return MUNIT_OK;
}

MUNIT_SUITE(munit,
    MUNIT_TEST(hello, test_hello)
);

static MunitSuite other_test_suites[] = {
    munit_test_suite,
    bsearchi_test_suite,
    bsearchn_test_suite,
};

static const MunitSuite main_test_suite = {
    (char*) "/libdino",     /* prefix for test names */
    NULL,                   /* array of test cases */
    other_test_suites,      /* array of other test suites */
    1,                      /* how many times to run this suite */
    MUNIT_SUITE_OPTION_NONE /* suite options */
};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc+1)]) {
    return munit_suite_main(&main_test_suite, NULL, argc, argv);
}
