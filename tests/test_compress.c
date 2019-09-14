#include "munit.h"
#include "../lib/compression/compression.h"
#include "../lib/config.h"

#define INTPARAM(name) atoi(munit_parameters_get(params, name))

MunitResult test_stream_new(const MunitParameter params[], void* user_data) {
    /* Make sure we get NULL on invalid id */
    munit_assert_null(cstream_create(DINO_COMPRESS_INVALID));
    munit_assert_null(dstream_create(DINO_COMPRESS_INVALID));
    munit_assert_uint(compress_id("FUNK"), ==, DINO_COMPRESS_INVALID);

    Dino_CompressID id = compress_id(munit_parameters_get(params, "algo"));
    Dino_CStream *cs = cstream_create(id);
    Dino_DStream *ds = dstream_create(id);
    if (!cs || !ds)
        return MUNIT_ERROR;

    munit_assert_not_null(cs);
    munit_assert_not_null(cs->funcs);
    munit_assert_not_null(cs->cctx);
    munit_assert_uint8(cs->funcs->id, ==, id);
    munit_assert_size(cs->rec_inbuf_size, >, 0);
    munit_assert_size(cs->rec_outbuf_size, >, 0);

    munit_assert_not_null(ds);
    munit_assert_not_null(ds->funcs);
    munit_assert_not_null(ds->dctx);
    munit_assert_uint8(ds->funcs->id, ==, id);
    munit_assert_size(ds->rec_inbuf_size, >, 0);
    munit_assert_size(ds->rec_outbuf_size, >, 0);
    /* cleanup! */
    cstream_free(cs);
    dstream_free(ds);
    /* also this shouldn't crash */
    cstream_free(NULL);
    dstream_free(NULL);
    return MUNIT_OK;
}

#define CHUNKSIZE 64
#define TESTBLOCK (CHUNKSIZE*16)

MunitResult test_compress1(const MunitParameter params[], void* user_data) {
    Dino_CompressID id = compress_id(munit_parameters_get(params, "algo"));
    Dino_CStream *cs = cstream_create(id);
    /* make buffers */
    inBuf *inbuf = inbuf_init(cs->rec_inbuf_size);
    outBuf *outbuf = outbuf_init(cs->rec_outbuf_size);
    if (!inbuf || !outbuf)
        return MUNIT_ERROR;

    /* make sure the buffers are bigger than the little test block */
    munit_assert_size(inbuf->size, >=, TESTBLOCK);
    munit_assert_size(outbuf->size, >=, TESTBLOCK);

    /* make a test block of the same data repeated 16 times */
    uint8_t *randchunk = munit_malloc(CHUNKSIZE);
    munit_rand_memory(CHUNKSIZE, randchunk);
    for (int c=0; c<(TESTBLOCK/CHUNKSIZE); c++)
        memcpy((void *)inbuf->buf+(c*CHUNKSIZE), randchunk, CHUNKSIZE);
    inbuf->size = TESTBLOCK;

    /* compress it! */
    cstream_compress1(cs, inbuf, outbuf);
    munit_assert_size(inbuf->pos, ==, inbuf->size);
    if (id == DINO_COMPRESS_NONE) {
        munit_assert_size(outbuf->pos, ==, inbuf->size);
        munit_assert_memory_equal(inbuf->size, inbuf->buf, outbuf->buf);
    } else {
        munit_assert_size(outbuf->pos, >, 0);
        munit_assert_memory_not_equal(outbuf->pos, inbuf->buf, outbuf->buf);
    }

    /* now see if we can uncompress it */
    Dino_DStream *ds = dstream_create(id);

    /* reset inbuf to its original size and clear it out */
    inbuf->size = cs->rec_inbuf_size;
    inbuf->pos = 0;
    if (id) {
        memset((void *)inbuf->buf, 0, inbuf->size);
        munit_assert_memory_not_equal(CHUNKSIZE, randchunk, outbuf->buf);
    }
    /* make outbuf the size of the output data */
    outbuf->size = outbuf->pos;
    outbuf->pos = 0;
    /* decompress output back to inbuf */
    dstream_decompress(ds, (inBuf *)outbuf, (outBuf *)inbuf);
    /* NOTE inbuf/outbuf are still switched here */
    munit_assert_size(outbuf->pos, ==, outbuf->size); /* we read everything? */
    munit_assert_size(inbuf->pos, ==, TESTBLOCK);     /* got expected size? */
    /* do we have the original data? */
    for (int c=0; c<(TESTBLOCK/CHUNKSIZE); c++)
        munit_assert_memory_equal(CHUNKSIZE, randchunk, inbuf->buf+(c*CHUNKSIZE));
    inbuf_free(inbuf);
    outbuf_free(outbuf);
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
    { "/compress1", test_compress1, NULL, NULL, MUNIT_TEST_OPTION_NONE, compr_params },
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
