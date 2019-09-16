#include "munit.h"
#include "../lib/digest.h"
#include "../lib/common.h"

#define INTPARAM(name) atoi(munit_parameters_get(params, name))

uint8_t sha256_empty[32] = {
    0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
    0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
    0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
    0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55 };

uint8_t sha256_beefy[32] = {
    0xfd, 0xef, 0x20, 0x2c, 0x14, 0x20, 0x2a, 0x14,
    0xae, 0x25, 0x76, 0xea, 0xc2, 0x2a, 0xd7, 0x82,
    0x72, 0x74, 0x90, 0x93, 0xaf, 0xa2, 0xbf, 0x50,
    0xaa, 0xd2, 0x7c, 0x96, 0xff, 0x36, 0x27, 0xde };

MunitResult test_hasher_create(const MunitParameter params[], void* user_data) {
    munit_assert_null(hasher_create(0));
    munit_assert_null(hasher_create(DINO_DIGEST_UNKNOWN));
    munit_assert_uint8(digest_size(DINO_DIGEST_UNKNOWN), ==, 0);

    Hasher *h = hasher_create(DINO_DIGEST_SHA256);
    munit_assert_not_null(h);
    munit_assert_uint8(digest_size(DINO_DIGEST_SHA256), ==, 32);

    return MUNIT_OK;
}

MunitResult test_hasher_empty(const MunitParameter params[], void* user_data) {
    Hasher *h = hasher_create(DINO_DIGEST_SHA256);
    unsigned size = digest_size(DINO_DIGEST_SHA256);
    uint8_t digest[size];
    hasher_start(h);
    hasher_finish(h, digest);
    munit_assert_memory_equal(size, sha256_empty, digest);
    return MUNIT_OK;
}

MunitResult test_hasher_beefy(const MunitParameter params[], void* user_data) {
    Hasher *h = hasher_create(DINO_DIGEST_SHA256);
    unsigned size = digest_size(DINO_DIGEST_SHA256);
    uint8_t digest[size];
    uint8_t beefy[] = "beefy miracle";

    hasher_start(h);
    hasher_update(h, beefy, sizeof(beefy)-1);
    hasher_finish(h, digest);
    munit_assert_memory_equal(size, sha256_beefy, digest);
    return MUNIT_OK;
}

MunitResult test_digest_info(const MunitParameter params[], void* user_data) {
    const char *name = munit_parameters_get(params, "algo");
    Dino_DigestID id = digest_id(name);
    munit_assert_uint8(id, >, 0);
    munit_assert_uint8(digest_size(id), >, 0);
    munit_assert_string_equal(name, digest_name(id));
    Hasher *h = hasher_create(id);
    munit_assert_not_null(h);
    return MUNIT_OK;
}


static MunitParameterEnum digest_params[] = {
    { (char*) "algo", (char *[]) { "md5", "sha1", "ripemd160", "sha256", "sha384", "sha512", "sha224" } },
    { NULL, NULL },
};

MunitTest hasher_tests[] = {
    { "/create", test_hasher_create, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/empty", test_hasher_empty, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/beefy", test_hasher_beefy, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/info", test_digest_info, NULL, NULL, MUNIT_TEST_OPTION_NONE, digest_params },
    /* End-of-array marker */
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static const MunitSuite hasher_suite = {
    "/libdino/digest",
    hasher_tests,
    (MunitSuite[]) {
        // { "/bench", compr_benchtests, NULL, 3, MUNIT_SUITE_OPTION_NONE },
        { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE },
    },
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    return munit_suite_main(&hasher_suite, (void*) "libdino", argc, argv);
};
