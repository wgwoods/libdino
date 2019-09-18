#include "../memory.h"
#include "compression.h"

/* list of compression algorithms we've built with */
#define DECLARE_ALGONAME(id, name) name,
const char *libdino_compression_available[] = {
    "none",
    #include "algo.inc"
    NULL,
};

const char *CompressName[DINO_COMPRESSNUM+1] = {
    "none",
    "zlib",
    "lzma",
    "lzo",
    "xz",
    "lz4",
    "zstd",
    NULL
};

Dino_CompressID compress_id(const char *name) {
    Dino_CompressID id;
    for (id=0; id < DINO_COMPRESSNUM; id++) {
        if (strncmp(CompressName[id], name, COMPRESSNAME_LEN) == 0) {
            return id;
        }
    }
    return DINO_COMPRESS_INVALID;
}

const char *compress_name(Dino_CompressID id) {
    if (id < DINO_COMPRESSNUM)
        return CompressName[id];
    return NULL;
}

Dino_CStream *cstream_create(Dino_CompressID id) {
    const Dino_CCFuncs *funcs;
    if (!(funcs = _get_ccfuncs(id)))
        return NULL;

    Dino_CStream *cs;
    if (!(cs = calloc(1, sizeof(Dino_CStream))))
        return NULL;

    cs->funcs = funcs;
    cs->cctx = cs->funcs->create_ctx();

    if (!cs->funcs->setup(cs, NULL)) {
        cstream_free(cs);
        return NULL;
    }
    return cs;
}

/* FIXME what are the expected return values etc. here... */

size_t cstream_compress_start(Dino_CStream *cstream, size_t size) {
    return 0; /* FIXME: need a way to pass this to the cctx.. */
}

size_t cstream_compress(Dino_CStream *cstream, inBuf *in, outBuf *out) {
    /* FIXME: error checking... */
    return cstream->funcs->compress(cstream, in, out);
}

size_t cstream_compress_end(Dino_CStream *cstream, outBuf *out) {
    return cstream->funcs->end(cstream, out);
}

size_t cstream_compress1(Dino_CStream *cstream, inBuf *in, outBuf *out) {
    /* FIXME error handling! */
    cstream_compress_start(cstream, in->size);
    cstream_compress(cstream, in, out);
    cstream_compress_end(cstream, out);
    /* FIXME I guess i'll return the bytes written? */
    return out->pos;
}

size_t dstream_decompress(Dino_DStream *dstream, inBuf *in, outBuf *out) {
    return dstream->funcs->decompress(dstream, in, out);
}

void cstream_free(Dino_CStream *cs) {
    if (!cs)
        return;
    if (cs->funcs && cs->funcs->free_ctx && cs->cctx)
        cs->funcs->free_ctx(cs->cctx);
    free(cs);
}

Dino_DStream *dstream_create(Dino_CompressID id) {
    const Dino_DCFuncs *funcs;
    if (!(funcs = _get_dcfuncs(id)))
        return NULL;

    Dino_DStream *ds;
    if (!(ds = calloc(1, sizeof(Dino_DStream))))
        return NULL;

    ds->funcs = funcs;
    ds->dctx = ds->funcs->create_ctx();

    if (!(ds->funcs->setup(ds, NULL))) {
        dstream_free(ds);
        return NULL;
    }
    return ds;
}

void dstream_free(Dino_DStream *ds) {
    if (!ds)
        return;
    if (ds->funcs && ds->funcs->free_ctx && ds->dctx)
        ds->funcs->free_ctx(ds->dctx);
    free(ds);
}
