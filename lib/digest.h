#ifndef _DIGEST_H
#define _DIGEST_H

#include "dino.h"

typedef struct Hasher Hasher;

Hasher *hasher_create(Dino_DigestID d);
void hasher_free(Hasher *h);

/* NOTE: none of these functions check if *d / *out are NULL, so if you pass
 * a NULL pointer you will cause a segmentation fault. */

/* TODO:
 * It might be smart to have these all check for NULL and return errors, and
 * have _unsafe variants that skip the NULL check.. if that turns out to have
 * any noticable performance difference. If guarding against NULL is trivial,
 * then we should do that by default rather than handing users a loaded gun
 * and suggesting they aim it away from their own feet. */
int hasher_start(Hasher *h);
int hasher_update(Hasher *h, const void *d, size_t len);
int hasher_getdigest(Hasher *h, uint8_t *out);
int hasher_finish(Hasher *h, uint8_t *out);
#define hasher_oneshot(h, d, len, out) \
    ({ hasher_start(h) && hasher_update(h, d, len) && hasher_finish(h, out); })
/* Get the digest from the hasher and compare it to exp_digest.
 * returns 1 if they're equal, 0 otherwise.
 * NOTE: allocates & frees temporary space for hasher_getdigest() */
int hasher_verify(Hasher *h, const uint8_t *exp_digest);

int hasher_size(Hasher *h);
int hasher_blocksize(Hasher *h);

uint8_t digest_size(Dino_DigestID d);
const char *digest_name(Dino_DigestID d);
Dino_DigestID digest_id(const char *name);

#endif /* _DIGEST_H */
