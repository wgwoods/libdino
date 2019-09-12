/* Header for (generic) compression interfaces. */
#ifndef _COMPRESSION_H
#define _COMPRESSION_H 1

#include <stddef.h>
#include "dino.h"

#define COMPRESSNAME_LEN 5
const char *CompressName[DINO_COMPRESSNUM+1];

Dino_CompressID compress_id(const char *name);
const char *compress_name(Dino_CompressID id);
int compress_avail(Dino_CompressID id);

typedef struct inBuf {
    const void *buf;
    size_t size;
    size_t pos;
} inBuf;

typedef struct outBuf {
    void *buf;
    size_t size;
    size_t pos;
} outBuf;

typedef void* Dino_CCtx;
typedef void* Dino_DCtx;

typedef void* Dino_COpts;
typedef void* Dino_DOpts;

typedef struct Dino_CStream Dino_CStream;
typedef struct Dino_DStream Dino_DStream;

typedef struct Dino_CCFuncs {
    Dino_CompressID id;
    Dino_CCtx (*create_ctx)(void);
    void (*free_ctx)(Dino_CCtx);
    int (*setup)(Dino_CStream*, Dino_COpts);
    size_t (*compress)(Dino_CStream *);
    size_t (*flush)(Dino_CStream *);
    size_t (*end)(Dino_CStream *);
} Dino_CCFuncs;

typedef struct Dino_DCFuncs {
    Dino_CompressID id;
    Dino_DCtx (*create_ctx)(void);
    void (*free_ctx)(Dino_DCtx);
    int (*setup)(Dino_DStream*, Dino_DOpts);
    size_t (*decompress)(Dino_DStream *);
} Dino_DCFuncs;

const Dino_CCFuncs *_get_ccfuncs(Dino_CompressID id);
const Dino_DCFuncs *_get_dcfuncs(Dino_CompressID id);

typedef struct Dino_CStream {
    inBuf *inbuf;
    outBuf *outbuf;
    Dino_CCtx cctx;
    const Dino_CCFuncs *funcs;
} Dino_CStream;

typedef struct Dino_DStream {
    inBuf *inbuf;
    outBuf *outbuf;
    Dino_DCtx *dctx;
    const Dino_DCFuncs *funcs;
} Dino_DStream;


Dino_CStream *cstream_new(Dino_CompressID id);
int cstream_setup(Dino_CStream *cstream, Dino_COpts copts);
int cstream_realloc(Dino_CStream *cstream, size_t insize, size_t outsize);
void cstream_free(Dino_CStream *cstream);

Dino_DStream *dstream_new(Dino_CompressID id);
int dstream_setup(Dino_DStream *dstream, Dino_DOpts dopts);
int dstream_realloc(Dino_DStream *dstream, size_t insize, size_t outsize);
void dstream_free(Dino_DStream *dstream);

#endif /* _COMPRESSION_H */
