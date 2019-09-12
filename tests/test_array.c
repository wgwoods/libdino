#include "munit.h"
#include "../lib/array.h"

MunitResult test_array_new(const MunitParameter params[], void* user_data) {
    size_t isize = 32; /* TODO: params */
    munit_assert_null(array_new(0));
    Array *a = array_new(isize);
    if (!a)
        return MUNIT_ERROR;
    munit_assert_null(a->data);
    munit_assert_size(a->isize, ==, isize);
    munit_assert_size(a->count, ==, 0);
    munit_assert_size(a->allocated, ==, 0);
    array_free(a);
    return MUNIT_OK;
}

MunitResult test_array_init(const MunitParameter params[], void* user_data) {
    size_t isize = 32; /* TODO: params */
    munit_assert_null(array_init(0));
    Array *a = array_init(isize);
    if (!a)
        return MUNIT_ERROR;
    munit_assert_not_null(a->data);
    munit_assert_size(a->isize, ==, isize);
    munit_assert_size(a->count, ==, 0);
    munit_assert_size(a->allocated, >, 0);
    array_free(a);
    return MUNIT_OK;
}

MunitResult test_array_with_capacity(const MunitParameter params[], void* user_data) {
    size_t isize = 32; /* TODO: params */
    size_t alloc = 42;
    munit_assert_null(array_with_capacity(0, 0));
    munit_assert_null(array_with_capacity(0, alloc));

    Array *a = array_with_capacity(isize, 0);
    munit_assert_not_null(a);
    munit_assert_size(a->isize, ==, isize);
    munit_assert_size(a->count, ==, 0);
    munit_assert_size(a->allocated, ==, 0);
    array_free(a);

    a = array_with_capacity(isize, alloc);
    munit_assert_not_null(a);
    munit_assert_not_null(a->data);
    munit_assert_size(a->isize, ==, isize);
    munit_assert_size(a->count, ==, 0);
    munit_assert_size(a->allocated, ==, alloc);
    array_free(a);

    return MUNIT_OK;
}

int array_is_sorted(const Array *a) {
    if (!a || !a->data)
        return 1;
    int r = 1;
    void *item, *prev;
    for (size_t i=1; (r && (i < a->count)); i++) {
        item = array_get(a, i);
        prev = array_get(a, i-1);
        r = (memcmp(item, prev, a->isize) >= 0);
    }
    return r;
}

static const uint32_t uintdata[] = {8,6,7,5,3,0,9};
#define UINTDATA_ISIZE sizeof(uintdata[0])
#define UINTDATA_COUNT (sizeof(uintdata)/UINTDATA_ISIZE)

MunitResult test_array_append(const MunitParameter params[], void* user_data) {
    Array *a = array_init(UINTDATA_ISIZE);
    for (int i=0; i<UINTDATA_COUNT; i++)
        array_append(a, &uintdata[i]);
    munit_assert_memory_equal(sizeof(uintdata), uintdata, a->data);
    array_free(a);
    return MUNIT_OK;
}

MunitResult test_array_insert(const MunitParameter params[], void* user_data) {
    Array *a = array_init(UINTDATA_ISIZE);
    for (int i=UINTDATA_COUNT-1; i >= 0; i--)
        array_insert(a, &uintdata[i], 0);
    munit_assert_memory_equal(sizeof(uintdata), uintdata, a->data);
    array_free(a);
    return MUNIT_OK;
}

MunitResult test_array_insort(const MunitParameter params[], void* fixture) {
    Array *a = array_init(UINTDATA_ISIZE);
    ssize_t idx;
    ssize_t exp_idx[] = {0, 0, 1, 0, 0, 0, 6};
    uint32_t exp_data[] = {0, 3, 5, 6, 7, 8, 9};
    for (int i=0; i<UINTDATA_COUNT; i++) {
        idx = array_insort(a, &uintdata[i]);
        munit_assert_size(idx, ==, exp_idx[i]);
        munit_assert(array_is_sorted(a));
    };
    munit_assert_memory_equal(sizeof(exp_data), exp_data, a->data);
    array_free(a);
    return MUNIT_OK;
}

MunitTest arraytests[] = {
    { "/new", test_array_new, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/init", test_array_init, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/append", test_array_append, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/insert", test_array_insert, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/insort", test_array_insort, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/with_capacity", test_array_with_capacity, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    /* End-of-array marker */
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static Array *make_random_array(size_t isize, size_t num) {
    size_t size = isize * num;
    Array *a = array_with_capacity(isize, num);
    munit_assert_not_null(a);
    if (num)
        munit_assert_not_null(a->data);
    munit_rand_memory(size, a->data);
    a->count = num;
    return a;
}

static Array *make_unsorted_array(size_t isize, size_t num) {
    Array *a = make_random_array(isize, num);
    if (a->count <= 1) /* Consider empty/1-element arrays "unsorted" */
        return a;
    size_t size = isize * num;
    while (array_is_sorted(a))
        munit_rand_memory(size, a->data);
    return a;
}

static Array *make_sorted_array(size_t isize, size_t num) {
    Array *a = make_random_array(isize, num);
    array_sort(a);
    return a;
}

#define INTPARAM(name) atoi(munit_parameters_get(params, name))
#define munit_assert_array_item_equal(a, idx, item) \
    munit_assert_memory_equal(a->isize, array_get(a, idx), item)


static void *randarray_setup(const MunitParameter params[], void* user_data) {
    return make_unsorted_array(INTPARAM("isize"), INTPARAM("count"));
}

static void *sortarray_setup(const MunitParameter params[], void* user_data) {
    return make_sorted_array(INTPARAM("isize"), INTPARAM("count"));
}

static void array_teardown(void *a) {
    array_free(a);
}

MunitResult test_randarray_sort(const MunitParameter params[], void* fixture) {
    Array *a = fixture;
    array_sort(a);
    munit_assert(array_is_sorted(a));
    return MUNIT_OK;
}

#define munit_assert_array_item_equal(a, idx, item) \
    munit_assert_memory_equal(a->isize, array_get(a, idx), item)
#define array_item_acopy(a, idx) memcpy(munit_malloc(a->isize), array_get(a, idx), a->isize)

MunitResult test_randidx_insert(const MunitParameter params[], void* fixture) {
    Array *a = fixture;
    int inserts = INTPARAM("inserts");
    void *olditem, *lastitem;
    size_t oldcount;
    void *item = munit_malloc(a->isize);
    for (int i=0; i<inserts; i++) {
        munit_rand_memory(a->isize, item);
        uint32_t idx = (a->count > 1) ? munit_rand_int_range(0, a->count-1) : 0;

        oldcount = a->count;
        olditem = array_item_acopy(a, idx);
        lastitem = array_item_acopy(a, a->count-1);

        array_insert(a, item, idx);

        munit_assert_size(a->count, ==, oldcount+1);
        munit_assert_array_item_equal(a, idx, item);
        munit_assert_array_item_equal(a, idx+1, olditem);
        munit_assert_array_item_equal(a, oldcount, lastitem);
    }
    free(item);
    free(olditem);
    free(lastitem);
    return MUNIT_OK;
}

MunitResult test_append_and_sort(const MunitParameter params[], void* fixture) {
    Array *a = fixture;
    int inserts = INTPARAM("inserts");
    void *item = munit_malloc(a->isize);
    size_t idx;
    for (int i=0; i<inserts; i++) {
        munit_rand_memory(a->isize, item);
        idx = array_append(a, item);
        munit_assert_array_item_equal(a, idx, item);
    }
    array_sort(a);
    munit_assert(array_is_sorted(a));
    free(item);
    return MUNIT_OK;
}

MunitResult test_insort(const MunitParameter params[], void* fixture) {
    Array *a = fixture;
    int inserts = INTPARAM("inserts");
    void *item = munit_malloc(a->isize);
    size_t idx;
    for (int i=0; i<inserts; i++) {
        munit_rand_memory(a->isize, item);
        idx = array_insort(a, item);
        munit_assert_array_item_equal(a, idx, item);
    }
    array_sort(a);
    munit_assert(array_is_sorted(a));
    free(item);
    return MUNIT_OK;
}

static MunitParameterEnum randarray_params[] = {
    { (char*) "inserts", (char*[]) { "1", NULL } } ,
    { (char*) "count", (char*[]) { "1", "10", "100", "1000", "10000", NULL } },
    { (char*) "isize", (char*[]) { "1", "2", "4", "8", "20", "32", NULL } },
    { NULL, NULL },
};

static MunitParameterEnum bench_params[] = {
    { (char*) "inserts", (char*[]) { "100", NULL } } ,
    { (char*) "count", (char*[]) { "100", "10000", "100000", NULL } },
    { (char*) "isize", (char*[]) { "20", "32", NULL } },
    { NULL, NULL },
};

MunitTest arrayrandtests[] = {
    { "/insert", test_randidx_insert, randarray_setup, array_teardown, MUNIT_TEST_OPTION_NONE, randarray_params },
    { "/sort", test_randarray_sort, randarray_setup, array_teardown, MUNIT_TEST_OPTION_NONE, randarray_params },
    { "/insort", test_insort, sortarray_setup, array_teardown, MUNIT_TEST_OPTION_NONE, randarray_params },
    { "/append-and-sort", test_append_and_sort, sortarray_setup, array_teardown, MUNIT_TEST_OPTION_NONE, randarray_params },
    /* End-of-array marker */
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

MunitTest arraybenchtests[] = {
    { "/insert", test_randidx_insert, randarray_setup, array_teardown, MUNIT_TEST_OPTION_NONE, bench_params },
    { "/insort", test_insort, sortarray_setup, array_teardown, MUNIT_TEST_OPTION_NONE, bench_params },
    { "/append-and-sort", test_append_and_sort, sortarray_setup, array_teardown, MUNIT_TEST_OPTION_NONE, bench_params },
    /* End-of-array marker */
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static const MunitSuite arraysuite = {
    "/libdino/array",
    arraytests,
    (MunitSuite[]) {
        { "/rand", arrayrandtests, NULL, 1, MUNIT_SUITE_OPTION_NONE },
        { "/bench", arraybenchtests, NULL, 3, MUNIT_SUITE_OPTION_NONE },
        { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE },
    },
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    return munit_suite_main(&arraysuite, (void*) "libdino", argc, argv);
};
