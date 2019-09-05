/* libdino internals */

#ifndef _LIBDINO_INTERNAL_H
#define _LIBDINO_INTERNAL_H 1

#include "dino.h"
#include "libdino.h"

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     (_a > _b) ? _a : _b; })
#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     (_a < _b) ? _a : _b; })


/* A linked list of Dino_Data descriptors makes up section contents. */
typedef struct Dino_Data_List {
    Dino_Data d;
    struct Dino_Data_List *next;
} Dino_Data_List;

#define EMPTY_DATA_LIST ((Dino_Data_List) { EMPTY_DINO_DATA , NULL })

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
    Dino_Shdr *shdr;
    Dino_Size64 size;
    Dino_Size64 count;
    Dino_Size64 offset;
};

/* DINO section table structure */
struct Dino_Sectab {
    Dino_Secidx count;      /* count of sections in this table */
    Dino_Secidx allocated;  /* how many sections we've allocated memory for */
    Dino_Shdr *shdr;        /* buffer for raw shdr entries */
    struct Dino_Sec *sec;   /* section descriptors */
};

/* Internal sectab functions */
void clear_sectab(Dino_Sectab *sectab);
Dino_Secidx realloc_sectab(Dino_Sectab *sectab, Dino_Secidx alloc_count);

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
