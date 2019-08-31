#ifndef _COMMON_H
#define _COMMON_H 1

#include <stdlib.h>

#include "libdino.h"
#include "libdino_internal.h"

static inline Dino *allocate_dino(int fd, size_t filesize) {
    Dino *d = (Dino *) calloc(1, sizeof(Dino));
    if (d != NULL) {
        d->fd = fd;
        d->filesize = filesize;
    }
    return d;
}

static inline Dino *new_dino(void) {
    return allocate_dino(-1, ~0);
}

#endif /* _COMMON_H */
