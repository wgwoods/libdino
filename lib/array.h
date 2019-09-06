#ifndef _ARRAY_H
#define _ARRAY_H 1

#define _GNU_SOURCE

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct Array {
    size_t isize;
    size_t count;
    size_t allocated;
    void *data;
} Array;


#define array_len(a) (a->count)
#define array_size(a) (a->isize*a->count)
#define array_get(a, i) (a->data + ((i)*a->isize))
#define array_icpy(dest, a, i) memcpy(dest, array_get(a,i), a->isize)

Array *array_new(size_t isize);
Array *array_init(size_t isize);
Array *array_with_capacity(size_t isize, size_t alloc_count);
Array *array_from_buf(void *buf, size_t isize, size_t count);
Array *array_read(int fd, off_t offset, size_t isize, size_t count);

ssize_t array_realloc(Array *a, size_t alloc_count);
ssize_t array_grow(Array *a);
ssize_t array_shrink(Array *a);
void array_clear(Array *a);
void array_free(Array *a);
ssize_t array_append(Array *a, const void *item);
ssize_t array_insert(Array *a, const void *item, size_t idx);
void *array_set(Array *a, const void *item, size_t idx);
int array_cmp(const Array *a, const Array *b);

/* Sorted array functions / macros */
typedef struct Array SortArray;
SortArray *array_sort(Array *a);
ssize_t array_insort_range(SortArray *a, const void *item, size_t baseidx, size_t num);
#define array_bisect_range(a, i, b, n) bisect(i, a->data, b, (b)+(n), a->isize)
#define array_insort(a, item) array_insort_range(a, item, 0, a->count)


#endif /* _ARRAY_H */
