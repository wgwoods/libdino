#include "compression.h"
#include "../common.h"
#include "../memory.h"

int compress_avail(Dino_CompressID id) {
    return (_get_ccfuncs(id) != NULL);
}

typedef struct Memcpy_CCtx {
    size_t srcsize;
    size_t bytes_in;
    size_t bytes_out;
} Memcpy_CCtx;
typedef struct Memcpy_CCtx Memcpy_DCtx;

/* passthru functions for testing / DINO_COMPRESS_NONE */
Dino_DCtx memcpy_create_dctx() {
    Memcpy_DCtx *dctx = calloc(1, sizeof(Memcpy_DCtx));
    dctx->srcsize = UNCOMPRESS_SIZE_UNKNOWN;
    return dctx;
}
#define memcpy_create_cctx ((Dino_CCtx (*)(void))memcpy_create_dctx)

void memcpy_free_dctx(Dino_DCtx dctx) { free(dctx); }
#define memcpy_free_cctx   ((void (*)(Dino_CCtx))memcpy_free_dctx)

int memcpy_setup_dstream(Dino_DStream *ds, Dino_DOpts *dopts) {
    ds->rec_inbuf_size = PAGESIZE;
    ds->rec_outbuf_size = PAGESIZE*2;
    return 1;
}
int memcpy_setup_cstream(Dino_CStream *cs, Dino_COpts *copts) {
    cs->rec_inbuf_size = PAGESIZE*2;
    cs->rec_outbuf_size = PAGESIZE;
    return 1;
}
size_t memcpy_setsize(Dino_CStream *cs, size_t srcsize) {
    Memcpy_CCtx *ctx = cs->cctx;
    if (ctx->bytes_out)
        return COMPRESS_ERR_STG;
    if (srcsize >= UNCOMPRESS_SIZE_MAX)
        return COMPRESS_ERR_BIG;
    return (ctx->srcsize = srcsize);
}

static uint32_t MEMCPY_MAGIC = 0xbedabeef;
#define MEMCPY_HDRSIZE (sizeof(MEMCPY_MAGIC)+sizeof(size_t))

size_t memcpy_compress(Dino_CStream *cs, inBuf *in, outBuf *out) {
    Memcpy_CCtx *ctx = cs->cctx;
    /* Write the header if this is the first write */
    if (!ctx->bytes_out) {
        if ((out->pos+MEMCPY_HDRSIZE) > out->size)
            return COMPRESS_ERR_BUF;
        uint32_t *outptr = (out->buf + out->pos);
        *outptr++ = MEMCPY_MAGIC;
        *(size_t*)outptr = ctx->srcsize;
        ctx->bytes_out += MEMCPY_HDRSIZE;
        out->pos += MEMCPY_HDRSIZE;
    }
    size_t max_in = in->size - in->pos;
    size_t max_out = out->size - out->pos;
    size_t outsize = MIN(max_in, max_out);
    memcpy(out->buf+out->pos, in->buf+in->pos, outsize);
    ctx->bytes_in += outsize;
    ctx->bytes_out += outsize;
    in->pos += outsize;
    out->pos += outsize;
    return 0;
}

size_t memcpy_flush(Dino_CStream *cs, outBuf *outbuf) { return 0; }
size_t memcpy_end(Dino_CStream *cs, outBuf *outbuf) {
    Memcpy_CCtx *ctx = cs->cctx;
    ctx->bytes_in = 0;
    ctx->bytes_out = 0;
    ctx->srcsize = UNCOMPRESS_SIZE_UNKNOWN;
    return 0;
}

size_t memcpy_getsize(Dino_DStream *ds, inBuf *in) {
    if (in->pos + MEMCPY_HDRSIZE > in->size)
        return UNCOMPRESS_SIZE_ERROR;
    uint32_t const *maybe_magic = (in->buf + in->pos);
    size_t const *maybe_srcsize = (in->buf + in->pos + sizeof(uint32_t));
    if (*maybe_magic == MEMCPY_MAGIC)
        return *maybe_srcsize;
    /* TODO: byteswap and check again */
    return UNCOMPRESS_SIZE_ERROR;
}

size_t memcpy_decompress(Dino_DStream *ds, inBuf *in, outBuf *out) {
    Memcpy_DCtx *ctx = ds->dctx;
    /* If this is the first read, read the header */
    if (!ctx->bytes_in) {
        if (in->pos + MEMCPY_HDRSIZE > in->size)
            return COMPRESS_ERR_BUF;
        size_t srcsize = memcpy_getsize(ds, in);
        if (srcsize == UNCOMPRESS_SIZE_ERROR)
            return COMPRESS_ERR_HDR;
        ctx->srcsize = srcsize;
        in->pos += MEMCPY_HDRSIZE;
        ctx->bytes_in += MEMCPY_HDRSIZE;
    }
    size_t max_in = in->size - in->pos;
    size_t max_out = out->size - out->pos;
    if (ctx->srcsize != UNCOMPRESS_SIZE_UNKNOWN)
        max_in = MIN(max_in, ctx->srcsize-ctx->bytes_out);
    size_t outsize = MIN(max_in, max_out);
    memcpy(out->buf+out->pos, in->buf+in->pos, outsize);
    ctx->bytes_in += outsize;
    ctx->bytes_out += outsize;
    in->pos += outsize;
    out->pos += outsize;
    if (ctx->srcsize == UNCOMPRESS_SIZE_UNKNOWN)
        return 0;
    if (ctx->bytes_out == ctx->srcsize) {
        ctx->bytes_in = 0;
        ctx->bytes_out = 0;
        ctx->srcsize = UNCOMPRESS_SIZE_UNKNOWN;
        return 0;
    }
    return MIN(ds->rec_inbuf_size, ctx->srcsize-ctx->bytes_in);
}

/* Here's some no-op placeholder functions for optional stuff */
size_t noop_getsize(Dino_DStream *d, inBuf *inbuf) {
    return UNCOMPRESS_SIZE_UNKNOWN;
}

size_t noop_setsize(Dino_CStream *c, size_t srcsize) {
    return srcsize;
}

/* Do the #includes for all the algorithms we're including */
#define ALGO_DOINCLUDES 1
#include "algo.inc"

/* Here's our struct that holds decompression interfaces... */
const Dino_DCFuncs dcfuncs[] = {
#if LIBDINO_ZSTD
    { DINO_COMPRESS_ZSTD,
        zstd_create_dctx, zstd_free_dctx,
        zstd_setup_dstream, zstd_getsize,
        zstd_decompress },
#endif
#if LIBDINO_XZ
    { DINO_COMPRESS_XZ,
        xz_create_dctx, xz_free_dctx,
        xz_setup_dstream, noop_getsize,
        xz_decompress },
#endif
    { DINO_COMPRESS_NONE,
        memcpy_create_dctx, memcpy_free_dctx,
        memcpy_setup_dstream, memcpy_getsize,
        memcpy_decompress },
    { DINO_COMPRESS_INVALID,
        NULL, NULL,
        NULL,
        NULL, },
};

/* And here's the compression interfaces */
const Dino_CCFuncs ccfuncs[] = {
#if LIBDINO_ZSTD
    {
        DINO_COMPRESS_ZSTD,
        zstd_create_cctx, zstd_free_cctx,
        zstd_setup_cstream, zstd_setsize,
        zstd_compress, zstd_flush, zstd_end
    },
#endif
#if LIBDINO_XZ
    { DINO_COMPRESS_XZ,
        xz_create_cctx, xz_free_cctx,
        xz_setup_cstream, noop_setsize,
        xz_compress, xz_flush, xz_end },
#endif
    { DINO_COMPRESS_NONE,
        memcpy_create_cctx, memcpy_free_cctx,
        memcpy_setup_cstream, memcpy_setsize,
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

