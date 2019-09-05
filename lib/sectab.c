#include "libdino_internal.h"
#include "system.h"

Dino_Sectab *new_sectab(Dino_Secidx alloc_count) {
    Dino_Sectab *sectab = calloc(1, sizeof(Dino_Sectab));
    return sectab;
}

Dino_Secidx realloc_sectab(Dino_Sectab *sectab, Dino_Secidx alloc_count) {
    if (alloc_count < sectab->count) {
        alloc_count = sectab->count;
    }
    if (alloc_count != sectab->allocated) {
        /* FIXME: error checking... */
        sectab->shdr = reallocarray(sectab->shdr, alloc_count, sizeof(Dino_Shdr));
        sectab->sec = reallocarray(sectab->sec, alloc_count, sizeof(Dino_Sec));
        sectab->allocated = alloc_count;
    }
    return alloc_count;
}

void clear_sectab(Dino_Sectab *sectab) {
    void *shdr = sectab->shdr;
    void *sec = sectab->sec;
    sectab->shdr = NULL;
    sectab->sec = NULL;
    sectab->count = 0;
    sectab->allocated = 0;
    free(shdr);
    free(sec);
}

void free_sectab(Dino_Sectab *sectab) {
    clear_sectab(sectab);
    free(sectab);
}

Dino_Sec *get_sec(Dino *dino, Dino_Secidx idx) {
    if (idx < dino->sectab.count) {
        return &dino->sectab.sec[idx];
    } else {
        return NULL;
    }
}

Dino_Shdr *get_shdr(Dino *dino, Dino_Secidx idx) {
    return get_sec(dino, idx)->shdr;
}
