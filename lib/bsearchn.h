#ifndef _BSEARCHN_H
#define _BSEARCHN_H 1

#include "memory.h"

/* bsearchn_cmp() is basically identical to bsearch(), except the comparison
 * function gets `size` as a third argument so it doesn't walk off into
 * invalid memory.
 */
void *bsearchn_cmp(const void *key,
                   const void *array, size_t num, size_t size,
                   int (*cmp)(const void *, const void *, size_t));

/* bsearchn() is a convenience macro that calls bsearchn_cmp with memcmp() as
 * the comparison function, which is probably what you're gonna do like
 * 99% of the time anyway. */
#define bsearchn(k, a, n, s) bsearchn_cmp(k, a, n, s, memcmp)

/* bsearchir_cmp() is roughly the same as bsearchn_cmp(), except you can
 * provide a base index to start at, and the return value is an index where an
 * item matching `key` was found, or a negative number if no matching item was
 * found.
 *
 * If the return value is >= 0, the array element at that index matches key.
 * If the array contains multiple sequential copies of the same element,
 * there's no guarantee about which element in that sequence we've chosen.
 *
 * If the return value is negative, taking the bitwise negation of that value
 * will give the (positive) index where the key could be inserted while
 * maintaining the sort order of the array. So, given nidx = ~idx:
 * - nidx is in the range [0,num]
 * - if (nidx > 0): array[nidx-1] < key
 * - if (nidx < num): array[nidx] > key
 */
ssize_t bsearchir_cmp(const void *key,
                      const void *array, size_t baseidx, size_t num, size_t size,
                      int (*cmp)(const void *, const void *, size_t));

/* bsearchir() is bsearchir_cmp() with memcmp() */
#define bsearchir(k, a, b, n, s) bsearchir_cmp(k, a, b, n, s, memcmp)

/* bsearchi() is bsearchir() with baseidx=0 */
#define bsearchi(k, a, n, s) bsearchir_cmp(k, a, 0, n, s, memcmp)

/* bisect_cmp is like Python's bisect.bisect() - a sorted array bisection
 * algorithm that returns an index where `key` could be inserted while keeping
 * `array` sorted. */
size_t bisect_cmp(const void *key,
                  const void *array, size_t lo, size_t hi, size_t size,
                  int (*cmp)(const void *, const void *, size_t));
#define bisect(k, a, l, h, s) bisect_cmp(k, a, l, h, s, memcmp)

/* A struct to return a range of indexes */
typedef struct idx_range {
    size_t lo;
    size_t hi;
} idx_range;

/* Check to see if a range indicates no match */
#define idx_range_nomatch(i) (i.lo > i.hi)

/* bsearchpkr_cmp() ("binary search, partial key range") is a search algorithm
 * for sorted arrays that returns a range of indexes that match the prefix
 * given in `pkey`, with size `pkeysize`.
 * If the returned range `r` has (r.lo == r.hi) then there was only one match;
 * if (r.lo > r.hi) then there are no keys that match the prefix, but
 * `r.hi` is the index where matching keys would go, as with bisect(). */
idx_range bsearchpkr_cmp(const void *pkey, size_t pkeysize,
                         const void *array, size_t baseidx, size_t num, size_t size,
                         int (*cmp)(const void *, const void *, size_t));
#define bsearchpkr(pk, pks, a, b, n, s) bsearchpkr_cmp(pk, pks, a, b, n, s, memcmp)

#endif /* _BSEARCHN_H */
