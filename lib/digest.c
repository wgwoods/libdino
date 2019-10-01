#include <string.h>
#include <openssl/evp.h>

#include "digest.h"
#include "dino.h"

typedef struct Hasher {
    const EVP_MD *type;
    EVP_MD_CTX *ctx;
} Hasher;

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
#define DIGESTNAME_MAX 10

static inline const digestInfo_t get_digestinfo(Dino_DigestID d) {
    return digestInfo[d < DINO_DIGESTNUM ? d : 0];
}
uint8_t digest_size(Dino_DigestID d) { return get_digestinfo(d).size; }
const char *digest_name(Dino_DigestID d) { return get_digestinfo(d).name; }
/* This could probably be more clever. */
Dino_DigestID digest_id(const char *name) {
    for (int d=0; d < DINO_DIGESTNUM; d++)
        if (strncmp(name, digestInfo[d].name, DIGESTNAME_MAX) == 0)
            return digestInfo[d].id;
    return DINO_DIGEST_UNKNOWN;
}

Hasher *hasher_create(Dino_DigestID d) {
    digestInfo_t dig = get_digestinfo(d);
    if (dig.id == DINO_DIGEST_UNKNOWN)
        return NULL;

    Hasher *h;
    if (!(h = calloc(1, sizeof(Hasher))))
        return NULL;

    h->type = dig.mdfn();
    h->ctx = EVP_MD_CTX_new();

    if (!h->type || !h->ctx) {
        hasher_free(h);
        return NULL;
    }

    return h;
}

int hasher_start(Hasher *h) {
    return EVP_DigestInit_ex(h->ctx, h->type, NULL);
}

int hasher_update(Hasher *h, const void *d, size_t len) {
    return EVP_DigestUpdate(h->ctx, d, len);
}

int hasher_finish(Hasher *h, uint8_t *out) {
    return EVP_DigestFinal_ex(h->ctx, out, NULL);
}

int hasher_getdigest(Hasher *h, uint8_t *out) {
    int r = 0;
    EVP_MD_CTX *ctx_copy = EVP_MD_CTX_new();
    if (ctx_copy && EVP_MD_CTX_copy_ex(ctx_copy, h->ctx))
        r = EVP_DigestFinal_ex(ctx_copy, out, NULL);
    EVP_MD_CTX_free(ctx_copy);
    return r;
}

int hasher_verify(Hasher *h, const uint8_t *exp_digest) {
    int digestsize = hasher_size(h);
    uint8_t *digest = malloc(digestsize);
    if (!digest || !hasher_getdigest(h, digest))
        return 0;
    int r = memcmp(digest, exp_digest, digestsize);
    free(digest);
    return (r == 0);
}

int hasher_size(Hasher *h) {
    return EVP_MD_size(h->type);
}

int hasher_blocksize(Hasher *h) {
    return EVP_MD_block_size(h->type);
}

void hasher_free(Hasher *h) {
    if (!h)
        return;
    if (h->ctx)
        EVP_MD_CTX_free(h->ctx);
    free(h);
}
