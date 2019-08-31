/* libdino's public interface. */

#ifndef _LIBDINO_H
#define _LIBDINO_H 1

#include <stddef.h>

#include "dino.h"

/* Opaque structs; internals are in libdino_internal.h */
typedef struct Dino_Sectab Dino_Sectab;
typedef struct Dino_Namtab Dino_Namtab;
typedef struct Dino Dino;

/* Descriptor for a chunk of data to be converted to/from disk format */
typedef struct Dino_Data {
    void *data;         /* Pointer to actual data */
    size_t size;        /* Size (in bytes) of data */
    Dino_Offset off;    /* Offset into the owning section */
    /* TODO: type, version, alignment? */
    /* NOTE: libelfP.h extends Elf_Data to add a pointer to the owning
     * section with the comment:
     *   The visible `Elf_Data' type is not sufficent for some operations due
     *   to a misdesigned interface.  Extend it for internal purposes.
     * TODO: what was the misdesigned interface? how do we avoid that? */
} Dino_Data;

Dino *read_dino(int fd);
Dino_Dhdr *get_dhdr(Dino *dino);
Dino_Shdr *get_shdr(Dino *dino, Dino_Secidx idx);
char *get_name(Dino *dino, Dino_NameOffset off);

#endif /* _LIBDINO_H */
