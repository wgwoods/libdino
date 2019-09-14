#include <string.h>
#include "compression.h"
#include "common.h"
#include "config.h"

#if LIBDINO_ZSTD
#include "d_zstd.h"
#endif

int compress_avail(Dino_CompressID id) {
    return (_get_ccfuncs(id) != NULL);
}

/* passthru functions for testing / DINO_COMPRESS_NONE */
Dino_DCtx memcpy_create_dctx() { return (void*)(0xbeef1e57); }
void memcpy_free_dctx(Dino_DCtx dctx) {  }
#define memcpy_create_cctx ((Dino_CCtx (*)(void))memcpy_create_dctx)
#define memcpy_free_cctx   ((void (*)(Dino_CCtx))memcpy_free_dctx)
int memcpy_setup_dstream(Dino_DStream *ds, Dino_DOpts *dopts) {
    ds->rec_inbuf_size = ds->rec_outbuf_size = 4096;
    return 1;
}
int memcpy_setup_cstream(Dino_CStream *cs, Dino_COpts *copts) {
    cs->rec_inbuf_size = cs->rec_outbuf_size = 4096;
    return 1;
}
size_t memcpy_compress(Dino_CStream *ds, inBuf *in, outBuf *out) {
    size_t insize = in->size - in->pos;
    size_t max_out = out->size - out->pos;
    size_t outsize = MIN(insize, max_out);
    memcpy(out->buf+out->pos, in->buf+in->pos, outsize);
    in->pos += outsize;
    out->pos += outsize;
    return insize - outsize;
}
#define memcpy_decompress ((size_t (*)(Dino_DStream*, inBuf *, outBuf *))memcpy_compress)
size_t memcpy_flush(Dino_CStream *cs, outBuf *outbuf) { return 0; }


/* Here's our struct that holds decompression interfaces... */
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
        memcpy_create_dctx, memcpy_free_dctx,
        memcpy_setup_dstream,
        memcpy_decompress },
    { DINO_COMPRESS_INVALID,
        NULL, NULL,
        NULL,
        NULL, },
};

/* And here's the compression interfaces */
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
        memcpy_create_cctx, memcpy_free_cctx,
        memcpy_setup_cstream,
        memcpy_compress, memcpy_flush, memcpy_flush },
    { DINO_COMPRESS_INVALID,
        NULL, NULL,
        NULL,
        NULL, NULL, NULL },
};

/* lookup functions for dcfuncs/ccfuncs */
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

