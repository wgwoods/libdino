/* zstd compression support
 * TODO: ZSTD_f_zstd1_magicless?
 */

#include <zstd.h>
#include "compression.h"

void zstd_free_cctx(Dino_CCtx c) { ZSTD_freeCCtx(c); }
void zstd_free_dctx(Dino_DCtx d) { ZSTD_freeDCtx(d); }
Dino_CCtx zstd_create_cctx(void) { return (Dino_CCtx) ZSTD_createCCtx(); }
Dino_DCtx zstd_create_dctx(void) { return (Dino_DCtx) ZSTD_createDCtx(); }

/* FIXME: better error codes */
int zstd_setup_cstream(Dino_CStream *c, Dino_COpts copts) {
    if (cstream_realloc(c, ZSTD_CStreamInSize(), ZSTD_CStreamOutSize()) < 0)
        return -1;
    return 0;
}

int zstd_setup_dstream(Dino_DStream *d, Dino_DOpts dopts) {
    if (dstream_realloc(d, ZSTD_DStreamInSize(), ZSTD_DStreamOutSize()) < 0)
        return -1;
    return 0;
}

size_t zstd_decompress(Dino_DStream *d) {
    ZSTD_inBuffer zib = { d->inbuf->buf, d->inbuf->size, d->inbuf->pos };
    ZSTD_outBuffer zob = { d->outbuf->buf, d->outbuf->size, d->outbuf->pos };
    size_t r = ZSTD_decompressStream((ZSTD_DStream *)d->dctx, &zob, &zib);
    d->inbuf->pos = zib.pos;
    d->outbuf->pos = zob.pos;
    /* FIXME convert errors */
    return r;
}

size_t zstd_compress(Dino_CStream *c) {
    ZSTD_inBuffer zib = { c->inbuf->buf, c->inbuf->size, c->inbuf->pos };
    ZSTD_outBuffer zob = { c->outbuf->buf, c->outbuf->size, c->outbuf->pos };
    size_t r = ZSTD_compressStream((ZSTD_CStream *)c->cctx, &zob, &zib);
    c->inbuf->pos = zib.pos;
    c->outbuf->pos = zob.pos;
    /* FIXME convert errors */
    return r;
}

size_t zstd_flush(Dino_CStream *c) {
    ZSTD_outBuffer zob = { c->outbuf->buf, c->outbuf->size, c->outbuf->pos };
    size_t r = ZSTD_flushStream((ZSTD_CStream *)c->cctx, &zob);
    c->outbuf->pos = zob.pos;
    /* FIXME convert errors */
    return r;
}

size_t zstd_end(Dino_CStream *c) {
    ZSTD_outBuffer zob = { c->outbuf->buf, c->outbuf->size, c->outbuf->pos };
    size_t r = ZSTD_endStream((ZSTD_CStream *)c->cctx, &zob);
    c->outbuf->pos = zob.pos;
    /* FIXME convert errors */
    return r;
}
