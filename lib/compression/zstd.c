/* zstd compression support */

#include <zstd.h>
#include "compression.h"

void zstd_free_cctx(Dino_CCtx c) { ZSTD_freeCCtx(c); }
void zstd_free_dctx(Dino_DCtx d) { ZSTD_freeDCtx(d); }
Dino_CCtx zstd_create_cctx(void) { return (Dino_CCtx) ZSTD_createCCtx(); }
Dino_DCtx zstd_create_dctx(void) { return (Dino_DCtx) ZSTD_createDCtx(); }

/* FIXME: better error codes */
int zstd_setup_cstream(Dino_CStream *c, Dino_COpts *copts) {
    if (!copts) {
        c->rec_inbuf_size = ZSTD_CStreamInSize();
        c->rec_outbuf_size = ZSTD_CStreamOutSize();
        return 1;
    }
    /* TODO: handle copts! */
    return 0;
}

int zstd_setup_dstream(Dino_DStream *d, Dino_DOpts *dopts) {
    if (!dopts) {
        d->rec_inbuf_size = ZSTD_DStreamInSize();
        d->rec_outbuf_size = ZSTD_DStreamOutSize();
        return 1;
    }
    /* TODO: handle dopts! */
    return 0;
}

size_t zstd_setsize(Dino_CStream *c, size_t srcsize) {
    size_t r = ZSTD_CCtx_setPledgedSrcSize(c->cctx, srcsize);
    if (ZSTD_isError(r))
        return COMPRESS_ERR_STG;
    return r;
}

size_t zstd_getsize(Dino_DStream *d, inBuf *inbuf) {
    void *src = (void*)inbuf->buf + inbuf->pos;
    /* The zstd (1.4.2) public API docs mention "ZSTD_frameHeaderSize_max",
     * which has been removed from the public API. There is a #define for
     * ZSTD_FRAMEHEADERSIZE_MAX, but it's part of the "Experimental" API and
     * can't be included here. There's also a ZSTD_frameHeaderSize() function,
     * which also isn't part of the public API. So.. */
    size_t header_max_size = inbuf->size - inbuf->pos;

    unsigned long long size = ZSTD_getFrameContentSize(src, header_max_size);
    if (size == ZSTD_CONTENTSIZE_UNKNOWN)
        return UNCOMPRESS_SIZE_UNKNOWN;
    else if (size == ZSTD_CONTENTSIZE_ERROR)
        return UNCOMPRESS_SIZE_ERROR;
    return size;
}

size_t zstd_decompress(Dino_DStream *d, inBuf *inbuf, outBuf *outbuf) {
    ZSTD_inBuffer zib = { inbuf->buf, inbuf->size, inbuf->pos };
    ZSTD_outBuffer zob = { outbuf->buf, outbuf->size, outbuf->pos };
    size_t r = ZSTD_decompressStream((ZSTD_DStream *)d->dctx, &zob, &zib);
    inbuf->pos = zib.pos;
    outbuf->pos = zob.pos;
    /* FIXME convert errors */
    return r;
}

size_t zstd_compress(Dino_CStream *c, inBuf *inbuf, outBuf *outbuf) {
    ZSTD_inBuffer zib = { inbuf->buf, inbuf->size, inbuf->pos };
    ZSTD_outBuffer zob = { outbuf->buf, outbuf->size, outbuf->pos };
    size_t r = ZSTD_compressStream((ZSTD_CStream *)c->cctx, &zob, &zib);
    inbuf->pos = zib.pos;
    outbuf->pos = zob.pos;
    /* FIXME convert errors */
    return r;
}

size_t zstd_flush(Dino_CStream *c, outBuf *outbuf) {
    ZSTD_outBuffer zob = { outbuf->buf, outbuf->size, outbuf->pos };
    size_t r = ZSTD_flushStream((ZSTD_CStream *)c->cctx, &zob);
    outbuf->pos = zob.pos;
    /* FIXME convert errors */
    return r;
}

size_t zstd_end(Dino_CStream *c, outBuf *outbuf) {
    ZSTD_outBuffer zob = { outbuf->buf, outbuf->size, outbuf->pos };
    size_t r = ZSTD_endStream((ZSTD_CStream *)c->cctx, &zob);
    outbuf->pos = zob.pos;
    /* FIXME convert errors */
    return r;
}
