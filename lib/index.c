#include "libdino_internal.h"
#include "bsearchn.h"
#include "system.h"

/* TODO: fanout could probably be optional? I mean, it's only 1Kb of data,
 * so it's not a huge problem, but it also probably doesn't help much for
 * small indexes. My gut sense is that if the keys fit into a single memory
 * page, we really don't need a full fanout table.
 * We could probably get Very Clever and use smaller fanout value types for
 * smaller indexes - for instance, if the counts all fit in a byte each, then
 * we could fit the fanout table in 256 bytes. But is the added complexity
 * really worth saving 768 bytes of RAM? Doubtful. */

#if 0
/* A key is really just an array of bytes... */
typedef uint8_t *Dino_Idx_Key;

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
#endif

/* An in-memory Index object. */
typedef struct Dino_Index {
    /* How many items are in the index, and how many items we've allocated
     * memory for */
    Dino_Idx_Cnt count;
    Dino_Idx_Cnt allocated;

    /* Which section's data is this an index of? */
    Dino_Secidx othersec;

    /* How big are the keys? */
    uint8_t keysize;

    /* Index data */
    void *data;
    Dino_Idx_Cnt *fanout;
    Dino_Idx_Key *keys;
    Dino_Idx_Val *vals;
} Dino_Index;

Dino_Index *new_index(uint8_t keysize);
Dino_Idx_Cnt realloc_index(Dino_Index *idx, Dino_Idx_Cnt count);

/* TODO: everything above should probably be in the headers.. */

Dino_Index *new_index(uint8_t keysize) {
    if (!keysize)
        return NULL;
    Dino_Index *idx = calloc(1, sizeof(Dino_Index));
    idx->keysize = keysize;
    return idx;
}

static inline void _init_idx_ptrs(Dino_Index *idx) {
    idx->fanout = (Dino_Idx_Cnt *) idx->data;
    /* are you ready for FUN WITH C POINTER MATH???
     *
     * WRONG: (Dino_Idx_Key *) idx->fanout + (sizeof(Dino_Idx_Cnt) * 256);
     *
     *   Since idx->fanout is type (Dino_Idx_Cnt *), pointer addition is
     *   already multiplied by sizeof(value-type), so this ends up being
     *   0x1000 instead of 0x400.
     *
     * WRONG: (Dino_Idx_Key *) idx->fanout+256;
     *
     *   The cast has higher precedence than the addition, so this ends up
     *   using sizeof(Dino_Idx_Key), which is sizeof(void*), which is
     *   0x800 (on 64-bit platforms) instead of 0x400.
     */
    idx->keys   = (Dino_Idx_Key *) &idx->fanout[256];
    idx->vals   = (Dino_Idx_Val *) idx->keys;
}

static inline void _upd_idx_ptrs(Dino_Index *idx) {
    idx->vals = (Dino_Idx_Val *) (idx->keys + (idx->keysize * idx->count));
}

Dino_Idx_Cnt realloc_index(Dino_Index *idx, Dino_Idx_Cnt alloc_count) {
    /* We can't shrink it smaller than the data we're using... */
    if (alloc_count < idx->count) {
        alloc_count = idx->count;
    }
    if (alloc_count != idx->allocated) {
        int alloc_size = ((sizeof(Dino_Idx_Cnt) << 8) + \
                         ((idx->keysize+sizeof(Dino_Idx_Val)) * alloc_count));
        idx->data = realloc(idx->data, alloc_size);
        idx->allocated = alloc_count;
        /* TODO: we could keep tail pointers for keys/vals so we don't have to
         * recalculate the offset to the empty space; if we did that, we'd have
         * to adjust those here. */
    }
    if (idx->fanout == NULL) {
        _init_idx_ptrs(idx);
    }
    return alloc_count;
}

void clear_index(Dino_Index *idx) {
    void *datap = idx->data;
    idx->data = NULL;
    idx->fanout = NULL;
    idx->keys = NULL;
    idx->vals = NULL;
    idx->count = 0;
    idx->allocated = 0;
    free(datap);
}

void free_index(Dino_Index *idx) {
    clear_index(idx);
    free(idx);
}

Dino_Index *index_from_shdr(Dino_Shdr *shdr) {
    Dino_Index *idx = new_index(shdr->info & 0xff);
    idx->othersec = (shdr->info >> 8) & 0xff;
    return idx;
}

ssize_t load_index_data(Dino_Sec *sec) {
    ssize_t r;
    Dino_Index *idx = index_from_shdr(sec->shdr);
    if (realloc_index(idx, sec->count) != sec->count) {
        free_index(idx);
        return -ENOMEM;
    }
    r = pread_retry(sec->dino->fd, idx->data, sec->size, sec->offset);
    if (r < sec->size) {
        free_index(idx);
        return -EIO;
    }
    idx->count = sec->count;
    _upd_idx_ptrs(idx);
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

#define INDEX_KEY_PTR(i, n) (i->keys + (i->keysize * n))
#define INDEX_VAL_PTR(i, n) (i->vals + (sizeof(Dino_Idx_Val) * n))

ssize_t index_find(Dino_Index *idx, const Dino_Idx_Key *key) {
    size_t baseidx, num;
    /* TODO: if fanout == NULL: baseidx=0, num=idx->count */
    uint8_t b = key[0];
    baseidx = (b==0) ? 0 : idx->fanout[b-1];
    num = idx->fanout[b] - baseidx;
    return bsearchir(key, idx->keys, baseidx, num, idx->keysize);
}

Dino_Idx_Val *index_search(Dino_Index *idx, const Dino_Idx_Key *key) {
    ssize_t i = index_find(idx, key);
    return (i < 0) ? NULL : INDEX_VAL_PTR(idx, i);
}

/* TODO: return key(s) that match partial key */
Dino_Idx_Key *index_key_match(Dino_Index *idx, const Dino_Idx_Key *key, size_t matchlen) {
    /* FIXME: use fanout to get baseidx/num */
    idx_range r = bsearchpkr(key, matchlen, idx->keys, 0, idx->count, idx->keysize);
    if (r.hi == r.lo)
        return INDEX_KEY_PTR(idx, r.lo);
    return NULL;
}

Dino_Index *get_index(Dino *dino, Dino_Secidx idx) {
    Dino_Sec *sec = get_sec(dino, idx);
    if (sec->shdr->type == DINO_SEC_INDEX)
        return sec->data.d.data;
    return NULL;
}

Dino_Idx_Key *index_get_key(Dino_Index *idx, Dino_Idx_Cnt i) {
    return idx->keys + (i*idx->keysize);
}

Dino_Idx_Val index_get_val(Dino_Index *idx, Dino_Idx_Cnt i) {
    return idx->vals[i];
}

Dino_Idx_Cnt index_get_cnt(Dino_Index *idx) {
    return idx->count;
}

/* TODO: index_add(Dino_Idx_Key key, Dino_Idx_Val *val)
 */
