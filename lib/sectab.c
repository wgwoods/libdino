#include "libdino_internal.h"
#include "fileio.h"
#include "memory.h"

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

inline Dino_Sec *dino_getsec(Dino *dino, Dino_Secidx idx) {
    if (_sectab_hassec(dino->sectab, idx))
        return _sectab_getsec(dino->sectab, idx);
    return NULL;
}

inline const char *dino_secname(Dino_Sec *sec) {
    return dino_getname(sec->dino, sec->shdr->name);
}

int get_secidx_byname(Dino *dino, const char *name) {
    for (Dino_Secidx idx=0; idx < dino->sectab.count; idx++) {
        Dino_Sec *sec = _dino_getsec(dino, idx);
        const char *secname = _dino_getname(dino, sec->shdr->name);
        if (secname && (strncmp(secname, name, dino->namtab.size) == 0)) {
            return idx;
        }
    }
    return -1;
}

Dino_Shdr *get_shdr(Dino *dino, Dino_Secidx idx) {
    return _dino_getsec(dino, idx)->shdr;
}
