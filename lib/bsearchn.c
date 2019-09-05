#include "bsearchn.h"
#include <stdlib.h>

/* Fucking C, man. The only binary search algorithm in the standard library
 * is bsearch(), which calls a comparison function:
 *   int cmp(const void *a, const void *b)
 * Since this function doesn't get the key length, it can't really
 * determine whether two keys are equal or not, which makes it unsafe
 * and kind of useless for comparing keys in our indexes.
 *
 * C11 Annex K introduces bsearch_s (and various other safe string handling
 * functions) but... it's not in glibc, so ¯\_(ツ)_/¯
 *
 * So guess who wrote his own binary search algorithm like some kind of a
 * sucker who's in the middle of a shitty tech interview? YA BOY WILLRAD.
 *
 * ...remind me why I'm not writing this in Rust or something? sheesh.
 *
 * Anyway, bsearchn_cmp() is basically just bsearch() with a limit on the size
 * of the objects being compared. It returns a matching item or NULL.
 */
void *bsearchn_cmp(const void *key,
                   const void *array, size_t num, size_t size,
                   int (*cmp)(const void *, const void *, size_t))
{
    const char *item;
    int result;

    while (num) {
        item = array + (num >> 1) * size;
        result = cmp(key, item, size);
        if (result == 0)
            return (void *)item;
        if (result > 0) {
            array = item + size;
            num--;
        }
        num >>= 1;
    }
    return NULL;
}

/* bsearchir_cmp() is roughly the same, except it returns the index of
 * a matching item, or a negative number if no match is found.
 * If the return value `idx` is negative, then `~idx` will be a
 * (positive) index where `key` could be inserted while maintaining
 * the sort ordering of `array`. */
ssize_t bsearchir_cmp(const void *key,
                      const void *array, size_t baseidx, size_t num, size_t size,
                      int (*cmp)(const void *, const void *, size_t))
{
    const char *item;
    ssize_t idx;
    int res;

    while (num) {
        idx = baseidx + (num>>1);       /* midpoint of [baseidx,baseidx+num] */
        item = array + (idx*size);      /* get pointer to midpoint item */
        res = cmp(key, item, size);     /* compare with key */
        if (res == 0)                   /* it's a match - return this index */
            return idx;
        if (res > 0) {                  /* key > item? */
            baseidx = idx+1;            /* - new baseidx is the next item */
            num--;                      /* - one less item to compare */
        }                               /* key < item: no change to baseidx */
        num >>= 1;                      /* divide search space in half */
    }                                   /* ...and try again */
    return ~baseidx;                    /* no match; return the final index */
}

/*
 * We encode the result into a negative number by doing bitwise negation.
 *
 * We *could* encode it into a negative by negating it and subtracting 1
 * (because we need a "negative zero" to indicate "not found, idx=0"),
 * but, hey, turns out that's the same result as bitwise negation.. at
 * least for two's complement integers. Interestingly, the representation
 * of signed integers is still undefined as of C11 and C++17, so according
 * to the standards, this is technically undefined behavior.
 *
 * HOWEVER, in all three binary representations of integers listed in C11
 * (sign+magnitude, one's complement, two's complement), it's true that
 * for any signed integer i >= 0, (i!=~i) && (~i<0) && (~~i==i).
 * And unlike unary negation and subtration, none of those operations can
 * overflow.
 *
 * So: mathematically, returning -1-baseidx is the safest option, but
 * technically, returning ~baseidx is more standards-compliant, less
 * risky, and is also a little cleaner to "decode" back to the idx value.
 *
 * So that's why I'm returning ~lo instead of -1-lo: Because C is dumb.
 */


/*
 * bisect_cmp() is an array bisection algorithm like Python's bisect.bisect():
 * it returns an index where the given `key` could be inserted while keeping
 * `array` sorted.
 */

size_t bisect_cmp(const void *key,
                  const void *array, size_t lo, size_t hi, size_t size,
                  int (*cmp)(const void *, const void *, size_t))
{
    const char *item;
    size_t mid;
    int res;

    while (lo<hi) {
        mid = lo + ((hi-lo)>>1);        /* midpoint of [lo,hi] */
        item = array + (mid*size);      /* get pointer to midpoint item */
        res = cmp(key, item, size);     /* compare with key */
        if (res > 0) {                  /* key > item? */
            lo = mid+1;                 /* - new baseidx is the next item */
        } else {
            hi = mid;
        }                               /* - one less item to compare */
    }                                   /* ...and try again */
    return lo;                          /* no match; return the final index */
}

/* bsearchpkr_cmp() ("binary search, partial key range") - find a range of
 * indexes that match the prefix `pkey`, which is `pkeysize` bytes long. */
idx_range bsearchpkr_cmp(const void *pkey, size_t pkeysize,
                         const void *array, size_t baseidx, size_t num, size_t size,
                         int (*cmp)(const void *, const void *, size_t))
{
    const char *item;
    ssize_t idx;
    int lr=0, hr=0;
    idx_range rv = { 0, 0 };

    char *lokey = malloc(size);
    memset(lokey, 0x00, size);
    memcpy(lokey, pkey, pkeysize);

    char *hikey = malloc(size);
    memset(hikey, 0xff, size);
    memcpy(hikey, pkey, pkeysize);

    while (num) {
        idx = baseidx + (num>>1);       /* midpoint of [baseidx,baseidx+num] */
        item = array + (idx*size);      /* get pointer to midpoint item */
        lr = cmp(lokey, item, size);    /* compare with lokey */
        hr = cmp(hikey, item, size);    /* and compare with hikey */
        if (lr != hr)                   /* lokey <= item <= hikey! */
            break;
        if (lr > 0) {                   /* - key > item? */
            baseidx = idx+1;            /* - new baseidx is the next item */
            num--;                      /* - one less item to compare */
        }                               /* - key < item: no change to baseidx */
        num >>= 1;                      /* - divide search space in half */
    }
    if (num == 0) {                     /* nothing matched */
        rv.lo = baseidx+1;
        rv.hi = baseidx;
    } else {
        /* We broke out of the loop because idx falls within the range of
         * matching items! There's three possibilities:
         *
         * 1. We found the leftmost item:  ((lr == 0) && (hr == 1))
         * 2. We found the rightmost item: ((lr == -1) && (hr == 0))
         * 3. We found a middle item:      ((lr == -1) && (hr == 1))
         *
         * So if (lr != 0), baseidx <= rv.lo <  idx, and similarly
         *    if (hr != 0),     idx <  rv.hi <= idx+(num>>1)
         *
         * It's definitely simplest to just do a linear search up/down from
         * `idx`, and probably just as fast for small values of `num` and/or
         * small numbers of likely matches, so that's what we do here.
         *
         * TODO: for large `num` or many matches it might be faster to do a
         * binary search of the remaining space, like:
         * num >>= 1;
         * rv.lo = bisect_cmp(lokey, array, baseidx, idx-1, size, cmp);
         * rv.hi = bisect_cmp(hikey, array, idx+1, idx+num, size, cmp);
         * Without some benchmarks it's hard to say what the right
         * threshold for binary vs. linear search is, though.
         */
        rv.lo = idx;
        rv.hi = idx;
        if (lr)
            while ((rv.lo>baseidx) && (cmp(pkey, array+((rv.lo-1)*size), pkeysize) == 0))
                rv.lo--;
        idx = idx+(num>>1); /* max index */
        if (hr)
            while ((rv.hi<idx) && (cmp(pkey, array+((rv.hi+1)*size), pkeysize) == 0))
                rv.hi++;
    }
    return rv;
}
