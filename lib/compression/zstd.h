#ifndef _ZSTD_H
#define _ZSTD_H 1
#include "compression.h"
#include <zstd.h>

Dino_CCtx zstd_create_cctx(void);
void zstd_free_cctx(Dino_CCtx c);
int zstd_setup_cstream(Dino_CStream *c, Dino_COpts *copts);
size_t zstd_setsize(Dino_CStream *c, size_t srcsize);
size_t zstd_compress(Dino_CStream *c, inBuf *inbuf, outBuf *outbuf);
size_t zstd_flush(Dino_CStream *c, outBuf *outbuf);
size_t zstd_end(Dino_CStream *c, outBuf *outbuf);

Dino_DCtx zstd_create_dctx(void);
void zstd_free_dctx(Dino_DCtx d);
int zstd_setup_dstream(Dino_DStream *d, Dino_DOpts *dopts);
size_t zstd_getsize(Dino_DStream *d, inBuf *inbuf);
size_t zstd_decompress(Dino_DStream *d, inBuf *inbuf, outBuf *outbuf);

#endif /* _ZSTD_H */
