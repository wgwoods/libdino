#ifndef _TEST_MUNIT_H
#define _TEST_MUNIT_H 1

#include "../subprojects/munit/munit.h"

#define munit_uintmax_t uintmax_t
#define munit_assert_uintmax(a, op, b) \
  munit_assert_type(munit_uintmax_t, PRIuMAX, a, op, b)

#endif /* _TEST_MUNIT_H */
