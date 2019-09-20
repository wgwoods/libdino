#include "munit.h"
#include "../lib/varint.h"

static MunitResult test_hello(const MunitParameter params[], void *data) {
    char hello[] = "hello, unit tests";
    munit_assert_string_equal(hello, "hello, unit tests");
    return MUNIT_OK;
}

#define VARINT_ARRAY_SIZE (1<<20)
#define UINTMAX_NUM_U32 (sizeof(uintmax_t)/sizeof(uint32_t))

uintmax_t rand_uintmax(void) {
    munit_assert_size(sizeof(uintmax_t) & 3, ==, 0);
    uintmax_t val = munit_rand_uint32();
    for (int i=1; i<UINTMAX_NUM_U32; i++)
        val = (val << 32) + munit_rand_uint32();
    return val;
}

void *make_uintmax_array(const MunitParameter params[], void *userdata) {
    uintmax_t num_vals = VARINT_ARRAY_SIZE;
    void *data = munit_malloc((num_vals+1) * sizeof(uintmax_t));
    uintmax_t *valuep = data;
    *valuep++ = num_vals;
    for (unsigned i=0; i < num_vals; i++)
        valuep[i] = rand_uintmax();
    return data;
}

void *make_varint_stream(const MunitParameter params[], void *userdata) {
    uintmax_t num_vals = VARINT_ARRAY_SIZE;
    void *data = munit_malloc((num_vals+1) * VARINT_MAXLEN);
    void *varintp = data;
    int len = dino_encode_varint(varintp, VARINT_MAXLEN, num_vals);
    for (unsigned i=0; i < num_vals; i++) {
        munit_assert_int(len, >, 0);
        varintp += len;
        len = dino_encode_varint(varintp, VARINT_MAXLEN, rand_uintmax());
    }
    return realloc(data, (varintp-data)+len);
}

void free_fixture(void *fixture) {
    free(fixture);
}

#define UINTMAX_MAX_VARINT "\x80\xfe\xfe\xfe\xfe\xfe\xfe\xfe\xfe\x7f"

static uintmax_t varint_maxval[VARINT_MAXLEN] = {
    0,
    VARINT_MAXVAL_WIDTH(1),
    VARINT_MAXVAL_WIDTH(2),
    VARINT_MAXVAL_WIDTH(3),
    VARINT_MAXVAL_WIDTH(4),
    VARINT_MAXVAL_WIDTH(5),
    VARINT_MAXVAL_WIDTH(6),
    VARINT_MAXVAL_WIDTH(7),
    VARINT_MAXVAL_WIDTH(8),
    VARINT_MAXVAL_WIDTH(9),
};

static MunitResult test_varint_encode(const MunitParameter params[], void *data) {
    int result;
    uint8_t varint[VARINT_MAXLEN];

    result = dino_encode_varint(varint, sizeof(varint), 0);
    munit_assert_int(result, ==, 1);
    munit_assert_uchar(varint[0], ==, 0);

    result = dino_encode_varint(varint, sizeof(varint), 1);
    munit_assert_int(result, ==, 1);
    munit_assert_uchar(varint[0], ==, 1);

    result = dino_encode_varint(varint, sizeof(varint), 42);
    munit_assert_int(result, ==, 1);
    munit_assert_uchar(varint[0], ==, 42);

    result = dino_encode_varint(varint, sizeof(varint), 127);
    munit_assert_int(result, ==, 1);
    munit_assert_uchar(varint[0], ==, 127);

    result = dino_encode_varint(varint, sizeof(varint), 128);
    munit_assert_int(result, ==, 2);
    munit_assert_uchar(varint[0], ==, 128);
    munit_assert_uchar(varint[1], ==, 0);

    result = dino_encode_varint(varint, sizeof(varint), 129);
    munit_assert_int(result, ==, 2);
    munit_assert_uchar(varint[0], ==, 128);
    munit_assert_uchar(varint[1], ==, 1);

    result = dino_encode_varint(varint, sizeof(varint), 255);
    munit_assert_int(result, ==, 2);
    munit_assert_uchar(varint[0], ==, 128);
    munit_assert_uchar(varint[1], ==, 127);

    result = dino_encode_varint(varint, sizeof(varint), 256);
    munit_assert_int(result, ==, 2);
    munit_assert_uchar(varint[0], ==, 129);
    munit_assert_uchar(varint[1], ==, 0);

    for (int i=1; i<VARINT_MAXLEN; i++) {
        uintmax_t val = varint_maxval[i];
        result = dino_encode_varint(varint, sizeof(varint), val);
        munit_assert_int(result, ==, i);
        result = dino_encode_varint(varint, sizeof(varint), val+1);
        munit_assert_int(result, ==, i+1);
    }

    result = dino_encode_varint(varint, sizeof(varint), UINTMAX_MAX);
    munit_assert_int(result, ==, VARINT_MAXLEN);
    munit_assert_memory_equal(VARINT_MAXLEN, varint, UINTMAX_MAX_VARINT);
    return MUNIT_OK;
}

static MunitResult test_varint_decode(const MunitParameter params[], void *data) {
    size_t len;
    uint8_t varint[VARINT_MAXLEN];

    varint[0] = '\0';
    munit_assert_uintmax(dino_decode_varint(varint, &len), ==, 0);
    munit_assert_size(len, ==, 1);

    /* check if we overflow without crashing */
    memset(varint, 0xff, sizeof(varint));
    munit_assert_uintmax(dino_decode_varint(varint, &len), ==, 0);
    munit_assert_size(len, ==, 0);

    varint[0] = 0x80;
    varint[1] = 0x00;
    munit_assert_uintmax(dino_decode_varint(varint, &len), ==, 128);
    munit_assert_size(len, ==, 2);

    memcpy(varint, UINTMAX_MAX_VARINT, VARINT_MAXLEN);
    munit_assert_uintmax(dino_decode_varint(varint, &len), ==, UINTMAX_MAX);
    munit_assert_size(len, ==, VARINT_MAXLEN);

    return MUNIT_OK;
}

static MunitResult test_varint_encdec_array(const MunitParameter params[], void *data) {
    uintmax_t *valuep = data;
    uintmax_t num_vals = *valuep++;
    uint8_t varint[VARINT_MAXLEN];
    for (unsigned i=0; i < num_vals; i++) {
        uintmax_t orig_val = valuep[i];
        int result = dino_encode_varint(varint, sizeof(varint), orig_val);
        munit_assert_int(result, >, 0);
        munit_assert_int(result, <=, VARINT_MAXLEN);

        size_t len;
        uintmax_t decoded_val = dino_decode_varint(varint, &len);
        munit_assert_size(len, ==, result);
        munit_assert_uintmax(decoded_val, ==, orig_val);
    }
    return MUNIT_OK;
}

static MunitResult test_varint_encode_array(const MunitParameter params[], void *data) {
    uintmax_t *valuep = data;
    uintmax_t num_vals = *valuep++;
    uint8_t varint[VARINT_MAXLEN];
    for (unsigned i=0; i < num_vals; i++) {
        int result = dino_encode_varint(varint, VARINT_MAXLEN, valuep[i]);
        munit_assert_int(result, >, 0);
        munit_assert_int(result, <=, VARINT_MAXLEN);
    }
    return MUNIT_OK;
}

static MunitResult test_varint_decode_stream(const MunitParameter params[], void *data) {
    uint8_t *varintp = data;
    size_t len;
    uintmax_t val;
    uintmax_t num_vals = dino_decode_varint(varintp, &len);
    for (unsigned i=0; i < num_vals; i++) {
        munit_assert_size(len, >, 0);
        varintp += len;
        val = dino_decode_varint(varintp, &len);
    }
    return (val || len) ? MUNIT_OK : MUNIT_ERROR;
}

static MunitTest varint_bench[] = {
    { "/encode", test_varint_encode_array, make_uintmax_array, free_fixture, MUNIT_TEST_OPTION_NONE, NULL },
    { "/decode", test_varint_decode_stream, make_varint_stream, free_fixture, MUNIT_TEST_OPTION_NONE, NULL },
    { "/encdec", test_varint_encdec_array, make_uintmax_array, free_fixture, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static const MunitSuite misc_suite = {
    "/libdino",
    (MunitTest []) {
        { "/munit/hello", test_hello, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { "/varint/encode", test_varint_encode, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { "/varint/decode", test_varint_decode, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    },
    (MunitSuite[]) {
        { "/varint/bench", varint_bench, NULL, 100, MUNIT_SUITE_OPTION_NONE },
        // { "/i/bench", bsearchi_benchtests, NULL, 3, MUNIT_SUITE_OPTION_NONE },
        { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE },
    },
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc+1)]) {
    return munit_suite_main(&misc_suite, NULL, argc, argv);
}
