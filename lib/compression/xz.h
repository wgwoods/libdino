#ifndef _XZ_H
#define _XZ_H 1

#include "compression.h"
#include <lzma.h>

/* This is what RPM is currently using, so.. */
#define XZ_LEVEL_DEFAULT 2

/* This part is the same for xz/lzma dctx/cctx */
void *xz_create_ctx(void);
void xz_free_ctx(void *ctx);

#define xz_create_cctx ((Dino_CCtx (*)(void))xz_create_ctx)
#define xz_free_cctx ((void (*)(Dino_CCtx))xz_free_ctx)
int xz_setup_cstream(Dino_CStream *c, Dino_COpts *copts);
//size_t xz_setsize(Dino_CStream *c, size_t srcsize);

size_t xz_compress(Dino_CStream *c, inBuf *inbuf, outBuf *outbuf);
size_t xz_flush(Dino_CStream *c, outBuf *outbuf);
size_t xz_end(Dino_CStream *c, outBuf *outbuf);

#define xz_create_dctx ((Dino_DCtx (*)(void))xz_create_ctx)
#define xz_free_dctx ((void (*)(Dino_DCtx))xz_free_ctx)
int xz_setup_dstream(Dino_DStream *d, Dino_DOpts *dopts);
//size_t xz_getsize(Dino_DStream *d, inBuf *inbuf);
size_t xz_decompress(Dino_DStream *d, inBuf *inbuf, outBuf *outbuf);

#endif /* _XZ_H */
