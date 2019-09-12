#include "compression.h"
#include "common.h"
#include "config.h"

#if LIBDINO_ZSTD
#include "d_zstd.h"
#endif

int compress_avail(Dino_CompressID id) {
    return (_get_ccfuncs(id) != NULL);
}

/* no-op functions for DINO_COMPRESS_NONE */
/* TODO: these are really passthru, not noop */
Dino_DCtx noop_create_dctx() { return (void*)(0xbeef1e57); }
void noop_free_dctx(Dino_DCtx dctx) { dctx==(void*)0xbeef1e57; }
int noop_setup_dstream(Dino_DStream *ds, Dino_DOpts dopts) {
    dstream_realloc(ds, 4096, 4096); return 0;
}

size_t noop_decompress(Dino_DStream *ds) {
    size_t insize = ds->inbuf->size - ds->inbuf->pos;
    size_t max_out = ds->outbuf->size - ds->outbuf->pos;
    size_t outsize = MIN(insize, max_out);
    ds->inbuf->pos += outsize;
    ds->outbuf->buf = (void *)ds->inbuf->buf;
    ds->outbuf->pos = ds->inbuf->pos;
    return insize - outsize;
}
#define noop_create_cctx ((Dino_CCtx (*)(void))noop_create_dctx)
#define noop_free_cctx   ((void (*)(Dino_CCtx))noop_free_dctx)
int noop_setup_cstream(Dino_CStream *cs, Dino_COpts copts) {
    cstream_realloc(cs, 4096, 4096); return 0;
}
size_t noop_compress(Dino_CStream *cs) {
    size_t insize = cs->inbuf->size - cs->inbuf->pos;
    size_t max_out = cs->outbuf->size - cs->outbuf->pos;
    size_t outsize = MIN(insize, max_out);
    cs->inbuf->pos += outsize;
    cs->outbuf->buf = (void *)cs->inbuf->buf;
    cs->outbuf->pos = cs->inbuf->pos;
    return insize - outsize;
}

const Dino_DCFuncs dcfuncs[] = {
#if LIBDINO_ZSTD
    { DINO_COMPRESS_ZSTD,
        zstd_create_dctx, zstd_free_dctx,
        zstd_setup_dstream,
        zstd_decompress },
#endif
#if LIBDINO_LZMA
    { DINO_COMPRESS_XZ,
        xz_create_dctx, xz_free_dctx,
        xz_setup_dstream,
        xz_decompress  },
#endif
    { DINO_COMPRESS_NONE,
        noop_create_dctx, noop_free_dctx,
        noop_setup_dstream,
        noop_decompress },
    { DINO_COMPRESS_INVALID,
        NULL, NULL,
        NULL,
        NULL, },
};

const Dino_CCFuncs ccfuncs[] = {
#ifdef LIBDINO_ZSTD
    {
        DINO_COMPRESS_ZSTD,
        zstd_create_cctx, zstd_free_cctx,
        zstd_setup_cstream,
        zstd_compress, zstd_flush, zstd_end
    },
#endif
#if LIBDINO_LZMA
    { DINO_COMPRESS_XZ,
        xz_create_cctx, xz_free_cctx,
        xz_setup_dstream,
        xz_compress, xz_flush, xz_end },
#endif
    { DINO_COMPRESS_NONE,
        noop_create_cctx, noop_free_cctx,
        noop_setup_cstream,
        noop_compress, noop_compress, noop_compress },
    { DINO_COMPRESS_INVALID,
        NULL, NULL,
        NULL,
        NULL, NULL, NULL },
};

const Dino_DCFuncs *_get_dcfuncs(Dino_CompressID id) {
    for (const Dino_DCFuncs *d=dcfuncs; d->decompress; d++)
        if (d->id == id)
            return d;
    return NULL;
}

const Dino_CCFuncs *_get_ccfuncs(Dino_CompressID id) {
    for (const Dino_CCFuncs *c=ccfuncs; c->compress; c++)
        if ((c->id != DINO_COMPRESS_INVALID) && (c->id == id))
            return c;
    return NULL;
}

