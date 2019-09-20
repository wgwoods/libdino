#include "munit.h"
#include "../lib/bsearchn.h"

const int intdata[] = {
    0,5,9,42,2903,31337,77777
};
#define INTDATA_SIZE sizeof(int)
#define INTDATA_NUM (sizeof(intdata)/sizeof(int))

const char bindata[] = {
    0x00, 0x00, 0x00, 0x00, 0xff,
    0x00, 0x00, 0x00, 0x29, 0x03,
    0x00, 0x00, 0x01, 0xde, 0xad,
    0x00, 0x00, 0x01, 0xde, 0xad,
    0x00, 0x00, 0x02, 0x01, 0x00,
    0x00, 0x00, 0x03, 0x07, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00,
};
#define BINDATA_SIZE 5
#define BINDATA_NUM (sizeof(bindata)/BINDATA_SIZE)

MunitResult test_bsearch_selftest(const MunitParameter params[], void *data) {
    int i;
    for (i=0; i<INTDATA_NUM-1; i++) {
        munit_assert_int(intdata[i], <, intdata[i+1]);
    }
    for (i=0; i<BINDATA_NUM-1; i++) {
        const char *a = bindata+(i*BINDATA_SIZE);
        const char *b = bindata+((i+1)*BINDATA_SIZE);
        munit_assert_int(memcmp(a,b,BINDATA_SIZE), <=, 0);
    }
    return MUNIT_OK;
}

MunitResult test_bsearchi_trivial(const MunitParameter params[], void *data) {
    munit_assert_int(bsearchi(&bindata[BINDATA_SIZE], &bindata, BINDATA_NUM, BINDATA_SIZE), ==, 1);
    return MUNIT_OK;
}

MunitResult test_bsearchi_first(const MunitParameter params[], void *data) {
    munit_assert_int(bsearchi(&intdata[0], &intdata, INTDATA_NUM, INTDATA_SIZE), ==, 0);
    return MUNIT_OK;
}

MunitResult test_bsearchi_last(const MunitParameter params[], void *data) {
    munit_assert_int(bsearchi(&intdata[INTDATA_NUM-1], &intdata, INTDATA_NUM, INTDATA_SIZE), ==, INTDATA_NUM-1);
    return MUNIT_OK;
}

MunitResult test_bsearchi_notfound_gt(const MunitParameter params[], void *data) {
    const int key = intdata[4]+1;
    ssize_t idx = bsearchi(&key, &intdata, INTDATA_NUM, INTDATA_SIZE);
    munit_assert_int(idx, <, 0);
    munit_assert_int(~idx, ==, 5);
    return MUNIT_OK;
}

MunitResult test_bsearchi_notfound_lt(const MunitParameter params[], void *data) {
    const int key = intdata[4]-1;
    ssize_t idx = bsearchi(&key, &intdata, INTDATA_NUM, INTDATA_SIZE);
    munit_assert_int(idx, <, 0);
    munit_assert_int(~idx, ==, 4);
    return MUNIT_OK;
}

MunitResult test_bsearchn_notfound(const MunitParameter params[], void *data) {
    const int key = 666;
    munit_assert_null(bsearchn(&key, &intdata, INTDATA_NUM, INTDATA_SIZE));
    return MUNIT_OK;
}

MunitResult test_bsearchn_match(const MunitParameter params[], void *data) {
    const char *key = &bindata[4*BINDATA_SIZE];
    munit_assert_memory_equal(BINDATA_SIZE, key,
            bsearchn(key, &bindata, BINDATA_NUM, BINDATA_SIZE));
    return MUNIT_OK;
}

static MunitTest bsearchi_tests[] = {
    { "/trivial", test_bsearchi_trivial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/first", test_bsearchi_first, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/last", test_bsearchi_last, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/notfound_lt", test_bsearchi_notfound_lt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/notfound_gt", test_bsearchi_notfound_gt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    /* End-of-array marker */
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static MunitTest bsearchn_tests[] = {
    { "/notfound", test_bsearchn_notfound, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/match", test_bsearchn_match, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    /* End-of-array marker */
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static const MunitSuite bsearch_suite = {
    "/libdino/bsearch",
    (MunitTest []) {
        { "/selftest", test_bsearch_selftest, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    },
    (MunitSuite[]) {
        { "/i", bsearchi_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE },
        { "/n", bsearchn_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE },
        // { "/i/bench", bsearchi_benchtests, NULL, 3, MUNIT_SUITE_OPTION_NONE },
        { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE },
    },
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    return munit_suite_main(&bsearch_suite, NULL, argc, argv);
};
