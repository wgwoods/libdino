#include "libdino_internal.h"

Dino_Shdr *get_shdr(Dino *dino, Dino_Secidx idx) {
    if (idx < dino->sectab.cnt) {
        return dino->sectab.sec[idx].shdr.s32;
    } else {
        return NULL;
    }
}
