#include "digest.h"
#include "dino.h"

typedef struct digestInfo_s {
    Dino_DigestID id;
    const char *name;
    const EVP_MD *(*mdfn)(void);
    uint8_t size;
    uint8_t deprecated;
} digestInfo_t;

static const digestInfo_t digestInfo[] = {
    { DINO_DIGEST_UNKNOWN,   "unknown",   EVP_md_null,    0, 0},
    { DINO_DIGEST_MD5,       "md5",       EVP_md5,       16, 1},
    { DINO_DIGEST_SHA1,      "sha1",      EVP_sha1,      20, 1},
    { DINO_DIGEST_RIPEMD160, "ripemd160", EVP_ripemd160, 20, 0},
    { DINO_DIGEST_UNKNOWN,   "invalid",   EVP_md_null,    0, 1},
    { DINO_DIGEST_UNKNOWN,   "invalid",   EVP_md_null,    0, 1},
    { DINO_DIGEST_UNKNOWN,   "invalid",   EVP_md_null,    0, 1},
    { DINO_DIGEST_UNKNOWN,   "invalid",   EVP_md_null,    0, 1},
    { DINO_DIGEST_SHA256,    "sha256",    EVP_sha256,    32, 0},
    { DINO_DIGEST_SHA384,    "sha384",    EVP_sha384,    48, 0},
    { DINO_DIGEST_SHA512,    "sha512",    EVP_sha512,    64, 0},
    { DINO_DIGEST_SHA224,    "sha224",    EVP_sha224,    28, 0},
};

static inline const digestInfo_t get_digestinfo(Dino_DigestID d) {
    return digestInfo[d < DINO_DIGESTNUM ? d : 0];
}

uint8_t digest_size(Dino_DigestID d) {
    return get_digestinfo(d).size;
}

const char *digest_name(Dino_DigestID d) {
    return get_digestinfo(d).name;
}
