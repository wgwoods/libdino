#define _GNU_SOURCE
#include "libdino_internal.h"
#include "bsearchn.h"
#include "system.h"
#include "array.h"

static long PAGESIZE = 4096;
static long CACHELINE = 64;

void __attribute__ ((constructor)) _array_init_static_vars(void) {
    long sys_pagesize = sysconf(_SC_PAGESIZE);
    long sys_cacheline = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (sys_pagesize > 0) PAGESIZE = sys_pagesize;
    if (sys_cacheline > 0) CACHELINE = sys_cacheline;
}

ssize_t array_realloc(Array *a, size_t alloc_count) {
    void *data;
    if (alloc_count == 0)
        return 0;
    alloc_count = MAX(alloc_count, a->count);
    data = reallocarray(a->data, alloc_count, a->isize);
    if (data == NULL)
        return -1;
    a->allocated = alloc_count;
    a->data = data;
    return alloc_count;
}

ssize_t array_grow(Array *a) {
    size_t size = a->allocated * a->isize;
    /* Small/empty array: allocate enough space to fill a page */
    if (size < PAGESIZE)
        return array_realloc(a, MAX(PAGESIZE/a->isize,1));
    /* Medium-sized array: just double it */
    size_t chonk = PAGESIZE<<10;
    if (size < chonk)
        return array_realloc(a, a->allocated << 1);
    /* Large(ish) array: allocate another chonk's worth */
    return array_realloc(a, a->allocated + (chonk / a->isize));
}

Array *array_new(size_t isize) {
    Array *a = calloc(1, sizeof(Array));
    if (a)
        a->isize = isize;
    return a;
}

Array *array_init(size_t isize) {
    Array *a = array_new(isize);
    array_grow(a);
    return a;
}

Array *array_with_capacity(size_t isize, size_t alloc_count) {
    Array *a = array_new(isize);
    if (array_realloc(a, alloc_count) < alloc_count)
        return NULL;
    return a;
}

/* NOTE: if buf can't be realloc/free'd you're gonna have a bad time */
Array *array_from_buf(void *buf, size_t isize, size_t count) {
    Array *a = array_new(isize);
    a->count = count;
    a->allocated = ~0; /* TODO: other functions should check this */
    a->data = buf;
    return a;
}

Array *array_read(int fd, off_t offset, size_t isize, size_t count) {
    ssize_t r;
    Array *a = array_with_capacity(isize, count);
    if (a == NULL)
        return NULL;
    r = pread_retry(fd, a->data, isize*count, offset);
    if (r < isize*count) {
        array_free(a);
        return NULL;
    }
    a->count = count;
    return a;
}

ssize_t array_shrink(Array *a) {
    return array_realloc(a, a->count);
}

void array_clear(Array *a) {
    free(a->data);
    a->data = NULL;
    a->count = 0;
    a->allocated = 0;
}

void array_free(Array *a) {
    array_clear(a);
    free(a);
}

#define ARRAY_IDX_PTR(a, i) (a->data + (a->isize*i))
#define ARRAY_ENSURE_SPACE(a) ((a->allocated < a->count) || (array_grow(a) > 0))

ssize_t array_append(Array *a, const void *item) {
    if (!ARRAY_ENSURE_SPACE(a)) return -1;
    memcpy(ARRAY_IDX_PTR(a, a->count), item, a->isize);
    a->count++;
    return a->count;
}

ssize_t array_insert(Array *a, const void *item, size_t idx) {
    if (idx == a->count)
        return array_append(a, item);
    if (idx > a->count)
        return -1;
    if (!ARRAY_ENSURE_SPACE(a)) return -1;
    void *iptr = ARRAY_IDX_PTR(a, idx);
    /* FIXME error checking here? */
    memmove(iptr, iptr+a->isize, a->isize*(a->count-idx));
    memcpy(iptr, item, a->isize);
    a->count++;
    return a->count;
}

void *array_set(Array *a, const void *item, size_t idx) {
    if (idx >= a->count)
        return NULL;
    return memcpy(ARRAY_IDX_PTR(a, idx), item, a->isize);
}

/* TODO: array_insort_range_nr (no-replace) */

ssize_t array_insort_range(SortArray *a, const void *item, size_t baseidx, size_t num) {
    if (!ARRAY_ENSURE_SPACE(a))
        return -1;
    if (baseidx > a->count)
        return -1;
    size_t idx;
    if (num)
        idx = bisect(item, a->data, baseidx, baseidx+num, a->isize);
    else
        idx = baseidx;
    array_insert(a, item, idx);
    return idx;
}

int memcmp_p(const void *a, const void *b, void *sizep) {
    return memcmp(a, b, *(size_t *)sizep);
}

SortArray *array_sort(Array *a) {
    qsort_r(a->data, a->count, a->isize, memcmp_p, &a->isize);
    return (SortArray *)a;
}
