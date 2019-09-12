#ifndef _ZSTD_H
#define _ZSTD_H 1
#include "compression.h"
#include <zstd.h>

//#define zstd_create_cctx ((Dino_CCtx (*)(void))ZSTD_createCCtx)
Dino_CCtx zstd_create_cctx(void);
void zstd_free_cctx(Dino_CCtx c);
int zstd_setup_cstream(Dino_CStream *c, Dino_COpts copts);
size_t zstd_compress(Dino_CStream *c);
size_t zstd_flush(Dino_CStream *c);
size_t zstd_end(Dino_CStream *c);

//#define zstd_create_dctx ((Dino_DCtx (*)(void))ZSTD_createDCtx)
Dino_DCtx zstd_create_dctx(void);
void zstd_free_dctx(Dino_DCtx d);
int zstd_setup_dstream(Dino_DStream *d, Dino_DOpts dopts);
size_t zstd_decompress(Dino_DStream *d);

#endif /* _ZSTD_H */
