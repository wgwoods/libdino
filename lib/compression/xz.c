/* xz compression support.
 *
 * For reference, check the xz example programs, e.g.:
 * https://git.tukaani.org/?p=xz.git;a=blob;f=doc/examples/01_compress_easy.c;tag=v5.2.4
 */

#include <assert.h>

#include "../common.h"
#include "../memory.h"
#include "xz.h"

/* Values taken from xz/src/liblzma/lzma2_encoder.h. */
/* Maximum compressed chunk size */
#define CHUNK_MAX (1<<16)
/* Maximum chunk header size */
#define HEADER_MAX 6
/* FIXME: what about footer size, stream header, etc? */
/* #define REC_COMPRESS_BUFSIZE (CHUNK_MAX+HEADER_MAX+FOOTER_MAX) */


void *xz_create_ctx(void) { return calloc(1, sizeof(lzma_stream)); }
void xz_free_ctx(void *ctx) { free(ctx); }

int xz_setup_cstream(Dino_CStream *c, Dino_COpts *copts) {
    lzma_ret ret;
    if (!copts) {
        c->rec_inbuf_size = DEFAULT_BUFSIZE;
        c->rec_outbuf_size = DEFAULT_BUFSIZE;
        /* TODO: we could use LZMA_CHECK_NONE when we're calculating our
         * own checksums... */
        ret = lzma_easy_encoder((lzma_stream *)c->cctx,
                                XZ_LEVEL_DEFAULT,
                                LZMA_CHECK_CRC32);
        if (ret == LZMA_OK)
            return 1;
        /* TODO: better error codes */
    }
    return 0;
}

/* TODO: implement these...
 * size_t xz_getsize(Dino_DStream *d, inBuf *inbuf) { return UNCOMPRESS_SIZE_UNKNOWN; }
 * size_t xz_setsize(Dino_CStream *c, size_t srcsize) { return srcsize; }
 *
 * UNFORTUNATELY since xz puts the uncompressed content size in the stream
 * _footer_, this really isn't going to work out for simple stream I/O.
 * It's still feasible - you'd just have to pass in the _end_ of the stream
 * rather than the start.
 */

size_t xz_compress(Dino_CStream *c, inBuf *inbuf, outBuf *outbuf) {
    lzma_stream *strm = c->cctx;
    lzma_ret ret;
    size_t in_len = inbuf->size - inbuf->pos;
    size_t out_len = outbuf->size - outbuf->pos;

    strm->next_in = inbuf->buf + inbuf->pos;
    strm->avail_in = in_len;

    strm->next_out = outbuf->buf + outbuf->pos;
    strm->avail_out = out_len;

    ret = lzma_code(strm, LZMA_RUN);

    /* update inbuf->pos and outbuf->pos */
    inbuf->pos += in_len - strm->avail_in;
    outbuf->pos += out_len - strm->avail_out;

    /* Naive hint for next read: just refill inbuf */
    /* TODO: need a better upper limit if inbuf->size is huge...
     * TODO: could also use strm->total_in and total_out */
    if (ret == LZMA_OK)
        return MIN(c->rec_inbuf_size, inbuf->size);
    /* TODO: better errors */
    return COMPRESS_ERR_UNK;
}


size_t xz_flush(Dino_CStream *c, outBuf *outbuf) {
    lzma_stream *strm = c->cctx;
    lzma_ret ret;
    size_t out_len = outbuf->size - outbuf->pos;

    assert(strm->avail_in == 0);
    strm->next_in = NULL;
    strm->avail_in = 0;

    strm->next_out = outbuf->buf + outbuf->pos;
    strm->avail_out = out_len;

    ret = lzma_code(strm, LZMA_FULL_FLUSH);

    outbuf->pos += out_len - strm->avail_out;

    /* Return number of bytes left to flush, or an error */
    switch (ret) {
        case LZMA_STREAM_END:
            return 0;
        case LZMA_BUF_ERROR:  /* not enough room in outbuf */
        case LZMA_OK:         /* XXX: is this a possible return value? */
            /* TODO: need a better estimate of remaining size than this */
            return MAX(1, strm->avail_in);
        default:
            return COMPRESS_ERR_UNK;
    }
}

size_t xz_end(Dino_CStream *c, outBuf *outbuf) {
    lzma_stream *strm = c->cctx;
    lzma_ret ret;
    size_t out_len = outbuf->size - outbuf->pos;

    assert(strm->avail_in == 0);
    strm->next_in = NULL;
    strm->avail_in = 0;

    strm->next_out = outbuf->buf + outbuf->pos;
    strm->avail_out = out_len;

    ret = lzma_code(strm, LZMA_FINISH);
    outbuf->pos += out_len - strm->avail_out;

    /* Return number of bytes left to flush, or an error */
    switch (ret) {
        case LZMA_STREAM_END:
            return 0;
        case LZMA_BUF_ERROR:  /* not enough room in outbuf */
        case LZMA_OK:         /* XXX: is this a possible return value? */
            /* TODO: need a better estimate of remaining size than this */
            return MAX(1, strm->avail_in);
        default:
            return COMPRESS_ERR_UNK;
    }
}

int xz_setup_dstream(Dino_DStream *d, Dino_DOpts *dopts) {
    lzma_ret ret;
    if (!dopts) {
        d->rec_inbuf_size = DEFAULT_BUFSIZE;
        d->rec_outbuf_size = DEFAULT_BUFSIZE;
        /* TODO: we could set the LZMA_IGNORE_CHECK flag if we're doing
         * our own integrity check of the decompressed data... */
        ret = lzma_stream_decoder((lzma_stream *)d->dctx,
                                  UINT64_MAX, /* memlimit */
                                  0);         /* checksum flags */
        if (ret == LZMA_OK)
            return 1;
        /* FIXME: better return/error codes */
    }
    return 0;
}

size_t xz_decompress(Dino_DStream *d, inBuf *inbuf, outBuf *outbuf) {
    lzma_stream *strm = d->dctx;
    lzma_ret ret;
    size_t in_len = inbuf->size - inbuf->pos;
    size_t out_len = outbuf->size - outbuf->pos;

    strm->next_in = inbuf->buf + inbuf->pos;
    strm->avail_in = in_len;

    strm->next_out = outbuf->buf + outbuf->pos;
    strm->avail_out = out_len;

    ret = lzma_code(strm, LZMA_RUN);
    inbuf->pos += in_len - strm->avail_in;
    outbuf->pos += out_len - strm->avail_out;

    switch (ret) {
        case LZMA_STREAM_END:
            /* Decoding finished - nothing more to read! */
            return 0;
        case LZMA_OK:
            /* More to decode - return a hint about the next read */
            /* TODO: hint should be (framesize-total_bytes_read), if we know those.. */
            return MIN(d->rec_inbuf_size, inbuf->size);
        /* TODO: better error codes */
        default:
            return COMPRESS_ERR_UNK;
    }
}
