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

    hasher_free(h);
    return MUNIT_OK;
}

MunitResult test_hasher_empty(const MunitParameter params[], void* user_data) {
    Hasher *h = hasher_create(DINO_DIGEST_SHA256);
    unsigned size = digest_size(DINO_DIGEST_SHA256);
    uint8_t digest[size];
    hasher_start(h);
    hasher_finish(h, digest);
    munit_assert_memory_equal(size, sha256_empty, digest);
    hasher_free(h);
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
    hasher_free(h);
    return MUNIT_OK;
}

MunitResult test_hasher_oneshot(const MunitParameter params[], void* user_data) {
    Hasher *h = hasher_create(DINO_DIGEST_SHA256);
    unsigned size = digest_size(DINO_DIGEST_SHA256);
    uint8_t digest[size];
    uint8_t beefy[] = "beefy miracle";
    hasher_oneshot(h, NULL, 0, digest);
    munit_assert_memory_equal(size, sha256_empty, digest);
    hasher_oneshot(h, beefy, sizeof(beefy)-1, digest);
    munit_assert_memory_equal(size, sha256_beefy, digest);
    hasher_free(h);
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
    hasher_free(h);
    return MUNIT_OK;
}

#define DIGESTID_PARAM(p) digest_id(munit_parameters_get(params, p))

MunitResult test_hasher_verify(const MunitParameter params[], void* user_data) {
    Hasher *h = hasher_create(DIGESTID_PARAM("algo"));
    int hashsize = hasher_size(h);
    uint8_t *empty_digest = munit_malloc(hashsize);
    /* grab the empty digest result for this algo */
    munit_assert(hasher_oneshot(h, NULL, 0, empty_digest));

    /* okay, start over */
    hasher_start(h);
    /* should still be empty */
    munit_assert(hasher_verify(h, empty_digest));
    /* even if we do it twice */
    munit_assert(hasher_verify(h, empty_digest));
    /* also a null update shouldn't change that */
    munit_assert(hasher_update(h, "", 0));
    munit_assert(hasher_verify(h, empty_digest));
    munit_assert(hasher_update(h, NULL, 0));
    munit_assert(hasher_verify(h, empty_digest));

    /* clean up and exit */
    free(empty_digest);
    hasher_free(h);
    return MUNIT_OK;
}


/* This test is unused right now because they all crash. Weird that openssl
 * doesn't bother guarding against NULL pointers. Whose job is that, anyway?
 * TODO: either safe variant functions, or fix these and maybe have unsafe
 * variant functions... you know, so we can skip an unnecessary branch for
 * callers that actually guard against NULLs.. */
#if 0
MunitResult test_hasher_nullptrs(const MunitParameter params[], void* user_data) {
    Hasher *h = hasher_create(DIGESTID_PARAM("algo"));
    int datasize = 42;
    uint8_t *datablock = munit_malloc(datasize);
    hasher_start(h);

    //munit_assert(hasher_update(h, NULL, datasize) == 0);
    //munit_assert(hasher_getdigest(h, NULL) == 0);
    //munit_assert(hasher_finish(h, NULL) == 0);
    //munit_assert(hasher_oneshot(h, NULL, datasize, NULL) == 0);
    //munit_assert(hasher_oneshot(h, datablock, datasize, NULL) == 0);
    //munit_assert(hasher_verify(h, NULL) == 0);

    free(datablock);
    hasher_free(h);
    return MUNIT_OK;
}
#endif

MunitResult test_digest_multipart(const MunitParameter params[], void* user_data) {
    Hasher *h = hasher_create(DIGESTID_PARAM("algo"));
    int digestsize = hasher_size(h);
    uint8_t *digest_part1 = malloc(digestsize);
    uint8_t *digest_full1 = malloc(digestsize);
    uint8_t *digest_part2 = malloc(digestsize);
    uint8_t *digest_full2 = malloc(digestsize);
    size_t datasize = 1<<20;
    /* make a 1MB block of random data */
    void *databuf = munit_malloc(datasize);
    /* pick a point at random somewhere in the middle */
    munit_assert_size(datasize, >, 1024);
    uint32_t pivot = (datasize/2) + munit_rand_int_range(0, datasize/4);
    /* make sure there's random data on both sides of the pivot */
    munit_rand_memory(64, databuf+(pivot-32));
    /* read everything up to there */
    hasher_start(h);
    hasher_update(h, databuf, pivot);
    /* get digest */
    hasher_getdigest(h, digest_part1);
    /* read the rest */
    hasher_update(h, databuf+pivot, datasize-pivot);
    /* get full digest */
    hasher_getdigest(h, digest_full1);

    /* do it again, should get the same result */
    hasher_getdigest(h, digest_full2);
    munit_assert_memory_equal(digestsize, digest_full1, digest_full2);

    /* do hasher_finish, should be the same result */
    hasher_finish(h, digest_full2);
    munit_assert_memory_equal(digestsize, digest_full1, digest_full2);

    /* now oneshot each block and check that we get the same results */
    hasher_oneshot(h, databuf, pivot, digest_part2);
    hasher_oneshot(h, databuf, datasize, digest_full2);
    munit_assert_memory_equal(digestsize, digest_part1, digest_part2);
    munit_assert_memory_equal(digestsize, digest_full1, digest_full2);

    /* woo, cleanup and exit */
    free(digest_part1);
    free(digest_part2);
    free(digest_full1);
    free(digest_full2);
    hasher_free(h);
    return MUNIT_OK;
}


static char *digest_algos[] = {
    "md5", "sha1", "ripemd160", "sha256", "sha384", "sha512", "sha224"
};

static MunitParameterEnum digest_params[] = {
    { (char*) "algo", digest_algos },
    { NULL, NULL },
};

MunitTest hasher_tests[] = {
    { "/create", test_hasher_create, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/empty", test_hasher_empty, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/beefy", test_hasher_beefy, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/oneshot", test_hasher_oneshot, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/verify", test_hasher_verify, NULL, NULL, MUNIT_TEST_OPTION_NONE, digest_params },
    //{ "/nullptr", test_hasher_nullptrs, NULL, NULL, MUNIT_TEST_OPTION_NONE, digest_params },
    { "/info", test_digest_info, NULL, NULL, MUNIT_TEST_OPTION_NONE, digest_params },
    { "/multipart", test_digest_multipart, NULL, NULL, MUNIT_TEST_OPTION_NONE, digest_params },
    /* End-of-array marker */
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

/* TODO: might be nice to benchmark the algos so we can have data about
 * performance impact and/or watch for weird spikes.. */

static const MunitSuite hasher_suite = {
    "/libdino/digest",
    hasher_tests,
    (MunitSuite[]) {
        // { "/bench", hasher_benchtests, NULL, 3, MUNIT_SUITE_OPTION_NONE },
        { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE },
    },
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    return munit_suite_main(&hasher_suite, (void*) "libdino", argc, argv);
};
