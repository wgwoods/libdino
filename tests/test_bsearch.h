#ifndef _TEST_BSEARCH_H
#define _TEST_BSEARCH_H 1

#include "munit.h"


MUNIT_TESTFN(test_bsearchi_selftest);
MUNIT_TESTFN(test_bsearchi_trivial);
MUNIT_TESTFN(test_bsearchi_first);
MUNIT_TESTFN(test_bsearchi_last);
MUNIT_TESTFN(test_bsearchi_notfound_gt);
MUNIT_TESTFN(test_bsearchi_notfound_lt);

MUNIT_SUITE(bsearchi,
    MUNIT_TEST(selftest, test_bsearchi_selftest)
    MUNIT_TEST(trivial, test_bsearchi_trivial)
    MUNIT_TEST(first, test_bsearchi_first)
    MUNIT_TEST(last, test_bsearchi_last)
    MUNIT_TEST(notfound_gt, test_bsearchi_notfound_gt)
    MUNIT_TEST(notfound_lt, test_bsearchi_notfound_lt)
);

MUNIT_TESTFN(test_bsearchn_notfound);
MUNIT_TESTFN(test_bsearchn);
MUNIT_TESTFN(test_bsearchn_match);

MUNIT_SUITE(bsearchn,
    MUNIT_TEST(notfound, test_bsearchn_notfound)
    MUNIT_TEST(match, test_bsearchn_match)
);

#endif /* _TEST_BSEARCH_H */
