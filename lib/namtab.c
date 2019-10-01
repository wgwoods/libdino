#include "libdino_internal.h"

inline const char *dino_getname(Dino *dino, Dino_NameOffset off) {
    if (_namtab_hasstr(dino->namtab, off))
        return _namtab_getstr(dino->namtab, off);
    return NULL;
}
