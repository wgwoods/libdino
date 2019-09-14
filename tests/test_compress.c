#include "munit.h"
#include "../lib/compression/compression.h"
#include "../lib/common.h"

#define INTPARAM(name) atoi(munit_parameters_get(params, name))

MunitResult test_stream_new(const MunitParameter params[], void* user_data) {
    /* Make sure unknown compression names give us DINO_COMPRESS_INVALID */
    munit_assert_uint(compress_id("FUNK"), ==, DINO_COMPRESS_INVALID);

    /* Make sure we get NULL on invalid id */
    munit_assert_null(cstream_create(DINO_COMPRESS_INVALID));
    munit_assert_null(dstream_create(DINO_COMPRESS_INVALID));

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
    /* also make sure this doesn't crash */
    cstream_free(NULL);
    dstream_free(NULL);
    return MUNIT_OK;
}

size_t memcpy_repeat(void *dest, const void *src, size_t size, size_t count) {
    size_t totalsize = size*count;
    for (off_t off=0; off<totalsize; off+=size)
        memcpy(dest+off, src, size);
    return totalsize;
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
    inbuf->size = memcpy_repeat((void *)inbuf->buf, randchunk, CHUNKSIZE, 16);

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

MunitResult test_compress_buf_pos(const MunitParameter params[], void* user_data) {
    /* set up cstream */
    Dino_CompressID id = compress_id(munit_parameters_get(params, "algo"));
    Dino_CStream *cs = cstream_create(id);
    /* make sure bufsize is a multiple of 8 */
    size_t bufsize = MAX(cs->rec_inbuf_size, cs->rec_outbuf_size) & (~7);
    Buf *a = buf_init(bufsize);
    Buf *b = buf_init(bufsize);
    if (!a || !b)
        return MUNIT_ERROR;

    /* zero both buffers */
    memset((void *)a->buf, 0, a->size);
    memset((void *)b->buf, 0, b->size);

    /* fill half of Buf a with a random int */
    a->size >>= 1;
    uint32_t randint = munit_rand_uint32();
    for (size_t i=0; i<a->size; i+=sizeof(uint32_t))
        *(uint32_t*)(a->buf+i) = randint;

    /* Compress it to somewhere in Buf b */
    b->pos = 8;
    cstream_compress1(cs, (inBuf *)a, b);
    /* Check that the bytes before b->pos are still 0 */
    munit_assert_size(b->pos, >, 8);
    munit_assert_size(a->pos, ==, a->size);
    munit_assert_memory_equal(8, b->buf, a->buf+(bufsize-8));
    /* decompress the output to fill the second half of Buf a */
    Dino_DStream *ds = dstream_create(id);
    a->size = bufsize;
    b->size = bufsize;
    a->pos = bufsize >> 1;
    b->pos = 8;
    dstream_decompress(ds, (inBuf *)b, a);
    /* Check that Buf a is full and that its left and right half are equal */
    munit_assert_size(a->pos, ==, a->size);
    munit_assert_memory_equal(bufsize>>1, a->buf, a->buf+(bufsize>>1));
    return MUNIT_OK;
}

static MunitParameterEnum compr_params[] = {
    { (char*) "algo", (char **)libdino_compression_available },
    { NULL, NULL },
};

MunitTest compr_tests[] = {
    { "/new", test_stream_new, NULL, NULL, MUNIT_TEST_OPTION_NONE, compr_params },
    { "/buf_pos", test_compress_buf_pos, NULL, NULL, MUNIT_TEST_OPTION_NONE, compr_params },
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
