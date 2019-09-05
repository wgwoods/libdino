/* libdino's public interface. */

#ifndef _LIBDINO_H
#define _LIBDINO_H 1

#include <stddef.h>

#include "dino.h"

/* Opaque structs; internals are in libdino_internal.h */
typedef struct Dino_Namtab Dino_Namtab;
typedef struct Dino_Sectab Dino_Sectab;
typedef struct Dino_Sec Dino_Sec;
typedef struct Dino Dino;

/* Section data structs */
typedef struct Dino_Index Dino_Index;

/* Descriptor for a chunk of data to be converted to/from disk format */
/* TODO: hrm, why is this public in the elfuitls API? */
typedef struct Dino_Data {
    void *data;         /* Pointer to actual data */
    size_t size;        /* Size (in bytes) of data */
    Dino_Offset off;    /* Offset into the owning section */
    /* TODO: use Dino_Off64 / off_t */
    /* TODO: type, version, alignment? */
    /* NOTE: libelfP.h extends Elf_Data to add a pointer to the owning
     * section with the comment:
     *   The visible `Elf_Data' type is not sufficent for some operations due
     *   to a misdesigned interface.  Extend it for internal purposes.
     * TODO: what was the misdesigned interface? how do we avoid that? */
} Dino_Data;
#define EMPTY_DINO_DATA ((Dino_Data) { NULL, 0, 0 })

Dino *read_dino(int fd);
Dino_Dhdr *get_dhdr(Dino *dino);
Dino_Shdr *get_shdr(Dino *dino, Dino_Secidx idx);
Dino_Sec *get_sec(Dino *dino, Dino_Secidx idx);
char *get_name(Dino *dino, Dino_NameOffset off);
uint8_t digest_size(Dino_DigestID d);

/*
 * NOTE: the Index stuff here is a work-in-progress, especially handling the
 * Dino_Idx_Key stuff..
 */
int load_indexes(Dino *dino);
Dino_Index *get_index(Dino *dino, Dino_Secidx idx);

/* A key is really just an array of bytes, but the size of the array is
 * determined at runtime. We could represent it with a struct like
 * { uint8_t size; uint8_t *val; } but we already have keysize in the
 * Dino_Index itself - plus this doesn't help make any of the pointer
 * arithmetic any easier. So for now it's just uint8_t and we only use
 * pointers. */
typedef uint8_t Dino_Idx_Key;

/* 32-bit count of keys, for the fanout table.
 * TODO: 64-bit extension... */
typedef uint32_t Dino_Idx_Cnt;

/* Index section values are an offset/size pair that points into
 * another associated section.
 * TODO: 64-bit extension... */
typedef struct Dino_Idx_Val {
    Dino_Offset offset;
    Dino_Size size;
} Dino_Idx_Val;

Dino_Idx_Cnt index_get_cnt(Dino_Index *idx);
Dino_Idx_Key *index_get_key(Dino_Index *idx, Dino_Idx_Cnt);
Dino_Idx_Val index_get_val(Dino_Index *idx, Dino_Idx_Cnt);
Dino_Idx_Key *index_key_match(Dino_Index *idx, const Dino_Idx_Key *key, size_t matchlen);

#endif /* _LIBDINO_H */
