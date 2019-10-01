#include "libdino_internal.h"
#include "bsearchn.h"
#include "fileio.h"
#include "array.h"

/* TODO: fanout could probably be optional? I mean, it's only 1Kb of data,
 * so it's not a huge problem, but it also probably doesn't help much for
 * small indexes. My gut sense is that if the keys fit into a single memory
 * page, we really don't need a full fanout table.
 * We could probably get Very Clever and use smaller fanout value types for
 * smaller indexes - for instance, if the counts all fit in a byte each, then
 * we could fit the fanout table in 256 bytes. But is the added complexity
 * really worth saving 768 bytes of RAM? Doubtful. */

typedef void *getval_t(Dino_Index *idx, Dino_Idx_Cnt i);

/* An in-memory Index object. */
typedef struct Dino_Index {
    /* How many items are in the index? */
    Dino_Idx_Cnt count;

    /* Which section's data is this an index of? */
    Dino_Secidx othersec;

    /* Section-specific flags */
    Dino_Idx_Flags flags;

    /* Fanout table - not resizeable, so not an Array */
    Dino_Idx_Cnt *fanout;

    /* Resizeable Array objects for keys and vals */
    Array *keys;
    Array *vals;
} Dino_Index;

/* TODO: everything above should probably be in the headers.. */

void index_free(Dino_Index *idx);

inline Dino_Sec *dino_get_index_othersec(Dino *dino, Dino_Index *idx) {
    return dino_getsec(dino, idx->othersec);
}

Dino_Index *index_new(Dino_Idx_Keysize keysize, uint8_t valsize) {
    if (!keysize)
        return NULL;
    Dino_Index *idx = calloc(1, sizeof(Dino_Index));
    if (idx == NULL)
        return NULL;
    idx->keys = array_new(keysize);
    idx->vals = array_new(valsize);
    if ((idx->keys == NULL) || (idx->vals == NULL)) {
        index_free(idx);
        return NULL;
    }
    return idx;
}

Dino_Index *index_init(Dino_Idx_Keysize keysize, uint8_t valsize) {
    Dino_Index *idx = index_new(keysize, valsize);
    if (idx == NULL)
        return NULL;
    idx->fanout = calloc(256, sizeof(Dino_Idx_Cnt));
    array_grow(idx->keys);
    array_grow(idx->vals);
    return idx;
}

ssize_t index_realloc(Dino_Index *idx, size_t alloc_count) {
    if (idx->fanout == NULL)
        idx->fanout = calloc(256, sizeof(Dino_Idx_Cnt));
    if (array_realloc(idx->keys, alloc_count) < alloc_count)
        return -1;
    if (array_realloc(idx->vals, alloc_count) < alloc_count)
        return -1;
    return alloc_count;
}

Dino_Index *index_with_capacity(Dino_Idx_Keysize keysize, uint8_t valsize, size_t alloc_count) {
    Dino_Index *idx = index_new(keysize, valsize);
    if (idx == NULL)
        return NULL;
    if (alloc_count == 0)
        return idx;
    if (index_realloc(idx, alloc_count) > 0)
        return idx;
    index_free(idx);
    return NULL;
}

void index_clear(Dino_Index *idx) {
    free(idx->fanout);
    idx->fanout = NULL;
    array_clear(idx->keys);
    array_clear(idx->vals);
    idx->count = 0;
}

void index_free(Dino_Index *idx) {
    index_clear(idx);
    array_free(idx->keys);
    array_free(idx->vals);
    free(idx);
}

Dino_Index *index_from_shdr(Dino_Shdr *shdr) {
    Dino_Idx_Flags flags = DINO_SECINFO_IDX_FLAGS(shdr->info);
    uint8_t valsize;
    switch (flags & (DINO_IDX_FLAG_64BIT|DINO_IDX_FLAG_UNC_SIZE)) {
        case DINO_IDX_FLAG_64BIT|DINO_IDX_FLAG_UNC_SIZE:
            valsize = sizeof(Dino_Idx_Val_Unc64);
            break;
        case DINO_IDX_FLAG_UNC_SIZE:
            valsize = sizeof(Dino_Idx_Val_Unc32);
            break;
        case DINO_IDX_FLAG_64BIT:
            valsize = sizeof(Dino_Idx_Val64);
            break;
        case 0:
            valsize = sizeof(Dino_Idx_Val32);
    }
    Dino_Idx_Keysize keysize = DINO_SECINFO_IDX_KEYSIZE(shdr->info);
    Dino_Index *idx = index_new(keysize, valsize);
    if (idx) {
        idx->othersec = DINO_SECINFO_IDX_OTHERSEC(shdr->info);
        idx->flags = flags;
    }
    return idx;
}

#define FANOUT_SIZE (sizeof(Dino_Idx_Cnt) << 8)

ssize_t load_index_data(Dino_Sec *sec) {
    ssize_t r;
    off_t off;
    Dino_Index *idx;

    if (!(idx = index_from_shdr(sec->shdr)))
        return -ENOMEM;

    off = sec->offset;
    idx->count = sec->count;

    /* FIXME: decode varints... */
    idx->fanout = malloc(FANOUT_SIZE);
    r = pread_retry(sec->dino->fd, idx->fanout, FANOUT_SIZE, sec->offset);
    if (r < FANOUT_SIZE) {
        free(idx->fanout);
        return -EIO;
    }
    off += r;

    if (!array_load(idx->keys, sec->dino->fd, off, idx->count))
        return -EIO;

    off += array_size(idx->keys);

    if (!array_load(idx->vals, sec->dino->fd, off, idx->count))
        return -EIO;

    sec->data.d.off = 0;
    sec->data.d.data = idx;
    sec->data.d.size = sec->size;
    return r;
}

int load_indexes(Dino *dino) {
    ssize_t r;
    int cnt;
    /* TODO: section iterator would be nice... */
    for (int i=0; i<dino->dhdr.section_count; i++) {
        Dino_Sec *sec = &dino->sectab.sec[i];
        if (sec->shdr->type == DINO_SEC_INDEX) {
            r = load_index_data(sec);
            if (r < 0)
                return -EIO;
            cnt++;
        }
    }
    return cnt;
}

ssize_t index_find(Dino_Index *idx, const Dino_Idx_Key *key) {
    size_t baseidx, num;
    uint8_t b = key[0];
    baseidx = (b==0) ? 0 : idx->fanout[b-1];
    num = idx->fanout[b] - baseidx;
    return bsearchir(key, idx->keys->data, baseidx, num, idx->keys->isize);
}

Dino_Idx_Val *index_search(Dino_Index *idx, const Dino_Idx_Key *key) {
    ssize_t i = index_find(idx, key);
    return (i < 0) ? NULL : array_get(idx->vals, i);
}

Dino_Idx_Range index_key_match(Dino_Index *idx, const Dino_Idx_Key *key, size_t matchlen) {
    size_t baseidx, num;
    uint8_t b = key[0];
    baseidx = (b==0) ? 0 : idx->fanout[b-1];
    num = idx->fanout[b] - baseidx;
    idx_range r = bsearchpkr(key, matchlen, idx->keys->data, baseidx, num, idx->keys->isize);
    /* TODO: this is goofy. These should be the same type... */
    return (Dino_Idx_Range) { r.lo, r.hi };
}

Dino_Index *get_index(Dino *dino, Dino_Secidx idx) {
    Dino_Sec *sec = dino_getsec(dino, idx);
    if (sec && (sec->shdr->type == DINO_SEC_INDEX))
        return sec->data.d.data;
    return NULL;
}

Dino_Index *get_index_byname(Dino *dino, const char *name) {
    Dino_Secidx idx = get_secidx_byname(dino, name);
    if (idx >= 0)
        return get_index(dino, idx);
    return NULL;
}

uint8_t index_get_keysize(Dino_Index *idx) {
    return idx->keys->isize;
}

Dino_Idx_Key *index_get_key(Dino_Index *idx, Dino_Idx_Cnt i) {
    return array_get(idx->keys, i);
}

Dino_Idx_Val *index_get_val(Dino_Index *idx, Dino_Idx_Cnt i) {
    return array_get(idx->vals, i);
}

Dino_Idx_Cnt index_get_cnt(Dino_Index *idx) {
    return idx->count;
}

ssize_t index_add(Dino_Index *idx, const Dino_Idx_Key *key, const Dino_Idx_Val *val) {
    ssize_t i = index_find(idx, key);
    if (i >= 0) {
        array_set(idx->vals, val, i);
    } else {
        i = ~i;
        array_insert(idx->keys, key, i);
        array_insert(idx->vals, val, i);
    }
    return i;
}

/* TODO: index_del? */
