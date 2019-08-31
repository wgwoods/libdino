#include "libdino_internal.h"

static char *get_str(Dino_Namtab namtab, Dino_NameOffset off) {
    if (off < namtab.size) {
        return &namtab.data[off];
    } else {
        return NULL;
    }
}

inline char *get_name(Dino *dino, Dino_NameOffset off) {
    return get_str(dino->namtab, off);
}

inline char *get_secname(Dino *dino, Dino_Secidx idx) {
    return get_name(dino, get_shdr(dino, idx)->name);
}
