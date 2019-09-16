#ifndef _DIGEST_H
#define _DIGEST_H

#include "dino.h"

typedef struct Hasher Hasher;

Hasher *hasher_create(Dino_DigestID d);
int hasher_start(Hasher *h);
int hasher_update(Hasher *h, void *d, size_t len);
int hasher_finish(Hasher *h, uint8_t *out);
int hasher_getdigest(Hasher *h, uint8_t *out);
void hasher_free(Hasher *h);

uint8_t digest_size(Dino_DigestID d);
const char *digest_name(Dino_DigestID d);
Dino_DigestID digest_id(const char *name);

#endif /* _DIGEST_H */
