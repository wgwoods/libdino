/* libdino internals */

#ifndef _LIBDINO_INTERNAL_H
#define _LIBDINO_INTERNAL_H 1

#include "dino.h"
#include "libdino.h"

/* A linked list of Dino_Data descriptors makes up section contents. */
typedef struct Dino_Data_List {
    Dino_Data d;
    struct Dino_Data_List *next;
} Dino_Data_List;

/* DINO nametable structure */
struct Dino_Namtab {
    Dino_NameOffset size;   /* size of nametable */
    char *data;             /* contents of nametable */
};

/* DINO Section descriptor. */
struct Dino_Sec {
    Dino_Data_List data;
    Dino_Secidx index;
    struct Dino *dino;
    union {
        Dino_Shdr   *s32;
        Dino_Shdr64 *s64;
    } shdr;
};

/* DINO section table structure */
struct Dino_Sectab {
    Dino_Secidx cnt;                        /* count of sections in this table */
    struct Dino_Sec sec[DINO_SEC_MAXCNT];   /* section descriptors */
};

/* DINO descriptor. */
struct Dino {
    /* The file descriptor, or -1 if we don't have it anymore */
    int fd;

    /* File size, if known; ~0 otherwise */
    size_t filesize;

    /* DINO header. */
    Dino_Dhdr dhdr;

    /* Section table. */
    Dino_Sectab sectab;

    /* Name table. */
    Dino_Namtab namtab;
};

#endif /* _LIBDINO_INTERNAL_H */
