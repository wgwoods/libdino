#include "munit.h"
#include "../lib/compression.h"
#include "../lib/config.h"

#define INTPARAM(name) atoi(munit_parameters_get(params, name))

MunitResult test_stream_new(const MunitParameter params[], void* user_data) {
    /* Make sure we get NULL on invalid id */
    munit_assert_null(cstream_new(DINO_COMPRESS_INVALID));
    munit_assert_null(dstream_new(DINO_COMPRESS_INVALID));
    munit_assert_uint(compress_id("FUNK"), ==, DINO_COMPRESS_INVALID);

    Dino_CompressID id = compress_id(munit_parameters_get(params, "algo"));
    Dino_CStream *cs = cstream_new(id);
    Dino_DStream *ds = dstream_new(id);
    if (!cs || !ds)
        return MUNIT_ERROR;

    munit_assert_not_null(cs);
    munit_assert_not_null(cs->funcs);
    munit_assert_not_null(cs->inbuf);
    munit_assert_not_null(cs->outbuf);
    if (id != DINO_COMPRESS_NONE)
        munit_assert_not_null(cs->cctx);
    munit_assert_uint8(cs->funcs->id, ==, id);
    munit_assert_null(cs->inbuf->buf);
    munit_assert_size(cs->inbuf->size, ==, 0);
    munit_assert_null(cs->outbuf->buf);
    munit_assert_size(cs->outbuf->size, ==, 0);

    munit_assert_not_null(ds);
    munit_assert_not_null(ds->funcs);
    munit_assert_not_null(ds->inbuf);
    munit_assert_not_null(ds->outbuf);
    if (id != DINO_COMPRESS_NONE)
        munit_assert_not_null(ds->dctx);
    munit_assert_uint8(ds->funcs->id, ==, id);
    munit_assert_null(ds->inbuf->buf);
    munit_assert_null(ds->outbuf->buf);
    munit_assert_size(ds->inbuf->size, ==, 0);
    munit_assert_size(ds->outbuf->size, ==, 0);
    /* cleanup! */
    cstream_free(cs);
    dstream_free(ds);
    /* also this shouldn't crash */
    cstream_free(NULL);
    dstream_free(NULL);
    return MUNIT_OK;
}

MunitResult test_stream_setup(const MunitParameter params[], void* user_data) {
    Dino_CompressID id = compress_id(munit_parameters_get(params, "algo"));
    Dino_CStream *cs = cstream_new(id);
    Dino_DStream *ds = dstream_new(id);
    if (!cs || !ds)
        return MUNIT_ERROR;
    /* default options should be fine */
    munit_assert_int(cstream_setup(cs, NULL), ==, 0);
    munit_assert_int(dstream_setup(ds, NULL), ==, 0);
    munit_assert_not_null(cs->inbuf->buf);
    munit_assert_size(cs->inbuf->size, >, 0);
    munit_assert_not_null(cs->outbuf->buf);
    munit_assert_size(cs->outbuf->size, >, 0);
    cstream_free(cs);
    dstream_free(ds);
    return MUNIT_OK;
}

#define CHUNKSIZE 64
#define TESTBLOCK (CHUNKSIZE*16)

MunitResult test_compress_one(const MunitParameter params[], void* user_data) {
    Dino_CompressID id = compress_id(munit_parameters_get(params, "algo"));
    Dino_CStream *cs = cstream_new(id);

    /* make sure the buffers are bigger than the little test block */
    if (cstream_setup(cs, NULL))
        return MUNIT_ERROR;
    munit_assert_size(cs->inbuf->size, >=, TESTBLOCK);
    munit_assert_size(cs->outbuf->size, >=, TESTBLOCK);

    /* make a test block of the same data repeated 16 times */
    uint8_t *randchunk = munit_malloc(CHUNKSIZE);
    munit_rand_memory(CHUNKSIZE, randchunk);
    for (int c=0; c<(TESTBLOCK/CHUNKSIZE); c++)
        memcpy((void *)cs->inbuf->buf+(c*CHUNKSIZE), randchunk, CHUNKSIZE);
    cs->inbuf->size = TESTBLOCK;

    /* compress it! */
    /* FIXME: cstream_compress(cs, inbuf, outbuf) */
    cs->funcs->compress(cs);
    size_t c_left = cs->funcs->end(cs);
    munit_assert_size(c_left, ==, 0);
    munit_assert_size(cs->inbuf->pos, ==, cs->inbuf->size);
    if (id == DINO_COMPRESS_NONE) {
        munit_assert_size(cs->outbuf->pos, ==, cs->inbuf->size);
        munit_assert_memory_equal(cs->inbuf->size, cs->inbuf->buf, cs->outbuf->buf);
    } else {
        munit_assert_size(cs->outbuf->pos, >, 0);
        munit_assert_memory_not_equal(cs->outbuf->pos, cs->inbuf->buf, cs->outbuf->buf);
    }

    /* now see if we can uncompress it */
    Dino_DStream *ds = dstream_new(id);
    ds->inbuf->buf = cs->outbuf->buf;
    ds->inbuf->size = cs->outbuf->pos;
    ds->outbuf->buf = (void*)cs->inbuf->buf;
    ds->outbuf->size = cs->inbuf->size;
    /* clear it, just to be sure we're really writing the original data */
    if (id) {
        memset(ds->outbuf->buf, 0, ds->outbuf->size);
        munit_assert_memory_not_equal(CHUNKSIZE, randchunk, ds->outbuf->buf);
    }
    /* FIXME: dstream_decompress(ds, inbuf, outbuf) */
    size_t d_left = ds->funcs->decompress(ds);
    munit_assert_size(d_left, ==, 0);
    munit_assert_size(ds->outbuf->pos, ==, TESTBLOCK);
    for (int c=0; c<(TESTBLOCK/CHUNKSIZE); c++)
        munit_assert_memory_equal(CHUNKSIZE, randchunk, ds->outbuf->buf+(c*CHUNKSIZE));
    dstream_free(ds);
    cstream_free(cs);
    return MUNIT_OK;
}


/* TODO: this is kind of dumb */
static char *avail_algos[] = {
    "none",
#if LIBDINO_ZSTD
    "zstd",
#endif
    NULL,
};

static MunitParameterEnum compr_params[] = {
    { (char*) "algo", avail_algos }, /* to be filled in at startup */
    { NULL, NULL },
};

MunitTest compr_tests[] = {
    { "/new", test_stream_new, NULL, NULL, MUNIT_TEST_OPTION_NONE, compr_params },
    { "/setup", test_stream_setup, NULL, NULL, MUNIT_TEST_OPTION_NONE, compr_params },
    { "/compress1", test_compress_one, NULL, NULL, MUNIT_TEST_OPTION_NONE, compr_params },
    /* End-of-array marker */
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static const MunitSuite compr_suite = {
    "/libdino/compress",
    compr_tests,
    (MunitSuite[]) {
        // { "/bench", compr_benchtests, NULL, 3, MUNIT_SUITE_OPTION_NONE },
        { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE },
    },
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    return munit_suite_main(&compr_suite, (void*) "libdino", argc, argv);
};
