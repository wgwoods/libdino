#include <stdlib.h>
#include <string.h>
#include "compression.h"

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

Dino_CStream *cstream_new(Dino_CompressID id) {
    const Dino_CCFuncs *funcs;
    if (!(funcs = _get_ccfuncs(id)))
        return NULL;

    Dino_CStream *cs;
    if (!(cs = calloc(1, sizeof(Dino_CStream))))
        return NULL;

    cs->funcs = funcs;
    cs->inbuf = calloc(1, sizeof(inBuf));
    cs->outbuf = calloc(1, sizeof(outBuf));
    if (!(cs->funcs && cs->inbuf && cs->outbuf) || \
        !(cs->cctx = cs->funcs->create_ctx())) {
        cstream_free(cs);
        return NULL;
    }
    return cs;
}

/* FIXME: better error codes */
int _stream_realloc(inBuf *inbuf, outBuf *outbuf, size_t insize, size_t outsize) {
    void *newin = realloc((void*)inbuf->buf, insize);
    if (insize && !newin)
        return -1;
    inbuf->size = insize;
    inbuf->buf = insize ? newin : NULL;

    void *newout = realloc(outbuf->buf, outsize);
    if (outsize && !newout)
        return -2;
    outbuf->size = outsize;
    outbuf->buf = outsize ? newout : NULL;
    return 0;
}

int cstream_setup(Dino_CStream *cs, Dino_COpts copts) {
    return cs->funcs->setup(cs, copts);
}

int cstream_realloc(Dino_CStream *cs, size_t insize, size_t outsize) {
    return _stream_realloc(cs->inbuf, cs->outbuf, insize, outsize);
}

void cstream_free(Dino_CStream *cs) {
    if (!cs)
        return;
    if (cs->inbuf)
        free(cs->inbuf);
    if (cs->outbuf)
        free(cs->outbuf);
    if (cs->funcs && cs->funcs->free_ctx && cs->cctx)
        cs->funcs->free_ctx(cs->cctx);
    free(cs);
}

Dino_DStream *dstream_new(Dino_CompressID id) {
    const Dino_DCFuncs *funcs;
    if (!(funcs = _get_dcfuncs(id)))
        return NULL;

    Dino_DStream *ds;
    if (!(ds = calloc(1, sizeof(Dino_DStream))))
        return NULL;

    ds->funcs = funcs;
    ds->inbuf = calloc(1, sizeof(inBuf));
    ds->outbuf = calloc(1, sizeof(outBuf));
    if (!(ds->funcs && ds->inbuf && ds->outbuf) || \
        !(ds->dctx = ds->funcs->create_ctx())) {
        dstream_free(ds);
        return NULL;
    }
    return ds;
}

int dstream_setup(Dino_DStream *ds, Dino_DOpts dopts) {
    return ds->funcs->setup(ds, dopts);
}

int dstream_realloc(Dino_DStream *ds, size_t insize, size_t outsize) {
    return _stream_realloc(ds->inbuf, ds->outbuf, insize, outsize);
}

void dstream_free(Dino_DStream *ds) {
    if (!ds)
        return;
    if (ds->inbuf)
        free(ds->inbuf);
    if (ds->outbuf)
        free(ds->outbuf);
    if (ds->funcs && ds->funcs->free_ctx && ds->dctx)
        ds->funcs->free_ctx(ds->dctx);
    free(ds);
}
