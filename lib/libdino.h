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

/* TODO: gonna need more consistent names here... */
/* TODO: symbol visibility! */

Dino *read_dino(int fd);
Dino_Dhdr *get_dhdr(Dino *dino);
Dino_Shdr *get_shdr(Dino *dino, Dino_Secidx idx);

Dino_Sec *dino_getsec(Dino *dino, Dino_Secidx idx);
int get_secidx_byname(Dino *dino, const char *secname);

const char *dino_getname(Dino *dino, Dino_NameOffset off);
const char *dino_secname(Dino_Sec *sec);
uint8_t digest_size(Dino_DigestID d);

/* Index sections contain (surprise!) an index that allows for fast lookup of
 * data that's contained in another section.
 *
 * Indexes have keys (which are the digests of the indexed items) and
 * values (which give the offset and size of the indexed item so it can be
 * fetched from the other section).
 *
 * They work like git packfile indexes, but with some additional features
 * intended to improve performance for clients reading data over a "dumb"
 * network transport like HTTP/1.1:
 * 1. give more data about each indexed item - offset, size, and optionally
 *    uncompressed size - so the client can determine the exact byte ranges
 *    to fetch (and the amount of space required to hold them).
 * 2. allow more compact on-disk/network representation - optionally omit
 *    fanout table or encode values using varints - to save client bandwidth.
 *
 * It's worth noting that the compact index encoding typically results in a
 * blob of data that is dense enough to be nearly uncompressable. So while
 * DINO _allows_ compressed index sections, it's generally a (very minor)
 * waste of time and space to compress them.
 */
int load_indexes(Dino *dino);
Dino_Index *get_index(Dino *dino, Dino_Secidx idx);
Dino_Index *get_index_byname(Dino *dino, const char *name);
Dino_Sec *get_index_othersec(Dino_Sec *idxsec);

/* A key is really just an array of bytes, but the size of the array is
 * determined at runtime. We could represent it with a struct like
 * { uint8_t size; uint8_t *val; } but we already have keysize in the
 * Dino_Index itself - plus this doesn't help make any of the pointer
 * arithmetic any easier. So for now it's just uint8_t and we only use
 * pointers. */
typedef uint8_t Dino_Idx_Key;

/* This will probably be my "640k ought to be enough for anyone" but I'm fairly
 * sure we're not going to want keys larger than 256 bytes anytime soon. */
typedef uint8_t Dino_Idx_Keysize;
#define KEYSIZE_MAX UINT8_MAX;

/* 32-bit count of keys, for the fanout table.
 * Also used for the index of each key in the key list.
 * If you have more than 4.2 billion keys you'll probably need multiple
 * indexes. Sorry! */
typedef uint32_t Dino_Idx_Cnt;

/* Index values. Just like keys, the values can be different sizes
 * based on the section flags. So a generic val is just a blob of bytes. */
typedef uint8_t Dino_Idx_Val;

/* Index values (in-memory), in four different flavors. Which one you should
 * use depends on two flags in the index's "info" field:
 * - DINO_IDX_FLAG_UNC_SIZE: indicates the values include unc_size.
 * - DINO_IDX_FLAG_64BIT: at least one index value is > UINT32_MAX.
 * Applications _can_ use Dino_Idx_Val_Unc64 as a generic value holder
 * at the cost of increased memory usage. */

/* !(flags & (DINO_IDX_FLAG_UNC_SIZE|DINO_IDX_FLAG_64BIT)) */
typedef struct Dino_Idx_Val32 {
    Dino_Offset offset;
    Dino_Size size;
} Dino_Idx_Val32;

/* ((flags & DINO_IDX_FLAG_UNC_SIZE) && !(flags & DINO_IDX_FLAG_64BIT)) */
typedef struct Dino_Idx_Val_Unc32 {
    Dino_Offset offset;
    Dino_Size size;
    Dino_Size unc_size;
} Dino_Idx_Val_Unc32;

/* ((flags & DINO_IDX_FLAG_UNC_SIZE) && !(flags & DINO_IDX_FLAG_64BIT)) */
typedef struct Dino_Idx_Val64 {
    Dino_Off64 offset;
    Dino_Size64 size;
} Dino_Idx_Val64;

/* ((flags & DINO_IDX_FLAG_UNC_SIZE) && (flags & DINO_IDX_FLAG_64BIT)) */
typedef struct Dino_Idx_Val_Unc64 {
    Dino_Off64 offset;
    Dino_Size64 size;
    Dino_Size64 unc_size;
} Dino_Idx_Val_Unc64;

/* For an Index section, the info item is divided up into four bytes:
 * MSB                               LSB
 * +--------+--------+--------+--------+
 * |RESERVED| flags  |othersec|keysize |
 * +--------+--------+--------+--------+
 * keysize: size of keys (in bytes). Must not be 0.
 * othersec: the Dino_Secidx of the section this index refers to.
 * flags: information about the size/layout of the index values.
 * RESERVED: reserved for future use; must be 0.
 *   Applications should treat this like a version field - if it's not 0, this
 *   may be a completely different index format, and you should not assume you
 *   can read/write anything else in this section.
 *
 * (FUTURE WORK: the RESERVED byte could be used to encode valsize, and/or we
 * could use some compact schema encoding like sqlite's to allow generic and
 * self-describing index values.)
 */
typedef enum Dino_Idx_Flags_e {
    DINO_IDX_FLAG_NOFANOUT = 1<<0, /* index omits the fanout table */
    DINO_IDX_FLAG_64BIT    = 1<<1, /* index contains 64-bit size/offsets */
    DINO_IDX_FLAG_UNC_SIZE = 1<<2, /* values are Dino_Idx_Val_Unc{32,64} structs */
    /* The rest are reserved for future use.. */
} Dino_Idx_Flags_e;
typedef uint8_t Dino_Idx_Flags;
/*
#define BITSLICE(uint, start, stop) \
    ( ((uint)>>(start)) & (UINTMAX_MAX >> ((stop)-(start))) )
#define DINO_SECINFO_EXTRACT(type, lobit, nbits, info) ((type) BITSLICE(info, lobit, (lobit+nbits)))
#define DINO_SECINFO_IDX_RESERVED(i) DINO_SECINFO_EXTRACT(uint8_t, 24, 8, i)
*/
#define DINO_SECINFO_IDX_RESERVED(i) ( (uint8_t)        (((i)>>24) & 0xff) )
#define DINO_SECINFO_IDX_FLAGS(i)    ( (Dino_Idx_Flags) (((i)>>16) & 0xff) )
#define DINO_SECINFO_IDX_OTHERSEC(i) ( (Dino_Secidx)    (((i)>>8)  & 0xff) )
#define DINO_SECINFO_IDX_KEYSIZE(i)  ( (uint8_t)         ((i)      & 0xff) )

/* Get the index's keysize */
Dino_Idx_Keysize index_get_keysize(Dino_Index *idx);

/* Get the associated section */
Dino_Sec *dino_get_index_othersec(Dino *dino, Dino_Index *idx);

/* Get count of keys & values */
Dino_Idx_Cnt index_get_cnt(Dino_Index *idx);

/* Get the key at index i */
Dino_Idx_Key *index_get_key(Dino_Index *idx, Dino_Idx_Cnt i);

/* Get the value at index i. Cast it to the right type! */
Dino_Idx_Val *index_get_val(Dino_Index *idx, Dino_Idx_Cnt i);
#define index_get_val32(idx, i) ((Dino_Idx_Val32 *)index_get_val(idx, i))
#define index_get_val64(idx, i) ((Dino_Idx_Val64 *)index_get_val(idx, i))
#define index_get_val_unc32(idx, i) ((Dino_Idx_Val_Unc32 *)index_get_val(idx, i))
#define index_get_val_unc64(idx, i) ((Dino_Idx_Val_Unc64 *)index_get_val(idx, i))

/* Index match ranges, for partial key matching */
typedef struct Dino_Idx_Range {
    size_t lo;
    size_t hi;
} Dino_Idx_Range;

Dino_Idx_Range index_key_match(Dino_Index *idx, const Dino_Idx_Key *key, size_t matchlen);

#endif /* _LIBDINO_H */
