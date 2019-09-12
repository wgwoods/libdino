#include "libdino_internal.h"
#include "bsearchn.h"
#include "array.h"
#include "system.h"

/* TODO: fanout could probably be optional? I mean, it's only 1Kb of data,
 * so it's not a huge problem, but it also probably doesn't help much for
 * small indexes. My gut sense is that if the keys fit into a single memory
 * page, we really don't need a full fanout table.
 * We could probably get Very Clever and use smaller fanout value types for
 * smaller indexes - for instance, if the counts all fit in a byte each, then
 * we could fit the fanout table in 256 bytes. But is the added complexity
 * really worth saving 768 bytes of RAM? Doubtful. */

/* An in-memory Index object. */
typedef struct Dino_Index {
    /* How many items are in the index? */
    Dino_Idx_Cnt count;

    /* Which section's data is this an index of? */
    Dino_Secidx othersec;

    /* How big are the keys? */
    uint8_t keysize;

    /* Fanout table - not resizeable, so not an Array */
    Dino_Idx_Cnt *fanout;

    /* Resizeable Array objects for keys and vals */
    Array *keys;
    Array *vals;
} Dino_Index;

/* TODO: everything above should probably be in the headers.. */

void index_free(Dino_Index *idx);

Dino_Index *index_new(uint8_t keysize) {
    if (!keysize)
        return NULL;
    Dino_Index *idx = calloc(1, sizeof(Dino_Index));
    if (idx == NULL)
        return NULL;
    idx->keysize = keysize;
    idx->keys = array_new(keysize);
    idx->vals = array_new(sizeof(Dino_Idx_Val));
    if ((idx->keys == NULL) || (idx->vals == NULL)) {
        index_free(idx);
        return NULL;
    }
    return idx;
}

Dino_Index *index_init(uint8_t keysize) {
    Dino_Index *idx = index_new(keysize);
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

Dino_Index *index_with_capacity(uint8_t keysize, size_t alloc_count) {
    Dino_Index *idx = index_new(keysize);
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
    Dino_Index *idx = index_new(shdr->info & 0xff);
    idx->othersec = (shdr->info >> 8) & 0xff;
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

    idx->fanout = malloc(FANOUT_SIZE);
    r = pread_retry(sec->dino->fd, idx->fanout, FANOUT_SIZE, sec->offset);
    if (r < FANOUT_SIZE) {
        free(idx->fanout);
        return -EIO;
    }
    off += r;

    /* FIXME: error checking... */
    idx->keys = array_read(sec->dino->fd, off, idx->keysize, idx->count);
    off += (idx->keysize * sec->count);

    idx->vals = array_read(sec->dino->fd, off, sizeof(Dino_Idx_Val), idx->count);

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
    return bsearchir(key, idx->keys->data, baseidx, num, idx->keysize);
}

Dino_Idx_Val *index_search(Dino_Index *idx, const Dino_Idx_Key *key) {
    ssize_t i = index_find(idx, key);
    return (i < 0) ? NULL : array_get(idx->vals, i);
}

Dino_Idx_Key *index_key_match(Dino_Index *idx, const Dino_Idx_Key *key, size_t matchlen) {
    size_t baseidx, num;
    uint8_t b = key[0];
    baseidx = (b==0) ? 0 : idx->fanout[b-1];
    num = idx->fanout[b] - baseidx;
    idx_range r = bsearchpkr(key, matchlen, idx->keys->data, baseidx, num, idx->keysize);
    if (r.hi == r.lo)
        return array_get(idx->keys, r.lo);
    return NULL;
}

Dino_Index *get_index(Dino *dino, Dino_Secidx idx) {
    Dino_Sec *sec = get_sec(dino, idx);
    if (sec->shdr->type == DINO_SEC_INDEX)
        return sec->data.d.data;
    return NULL;
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
