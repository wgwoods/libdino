/* Header for (generic) compression interfaces. */
#ifndef _COMPRESSION_H
#define _COMPRESSION_H 1

#include <stddef.h>
#include "../dino.h"

#define COMPRESSNAME_LEN 5
const char *CompressName[DINO_COMPRESSNUM+1];

Dino_CompressID compress_id(const char *name);
const char *compress_name(Dino_CompressID id);
int compress_avail(Dino_CompressID id);

typedef struct Buf {
    void *buf;
    size_t size;
    size_t pos;
} Buf;
typedef struct Buf outBuf;

typedef struct inBuf {
    const void *buf;
    size_t size;
    size_t pos;
} inBuf;

/* NOTE: interesting compression options for each algorithm:
 * zlib: int level, method, windowBits, memLevel, strategy;
 * zstd: int level; enum strategy, format; int windowBits;
 *       bool write_size, write_check, write_dictid, rsyncable;
 *       void *dict; ...
 * lzma: int level; uint64_t memlimit; enum check; ...
 *
 * squashfs stores non-default options as follows:
 *   gzip: u32 level, u16 window_size, u16 strategies_bitfield;
 *   xz: u32 dict_size, u32 filters_bitfield;
 *   lz4: u32 version=1, u32 flags;
 *   lzo: u32 level, u32 algo;
 *   zstd: u32 level;
 *
 * deltarpm stores only int level, from the RPM's 'PAYLOADFLAGS' tag
 * (which is generally a string, like '2')
 */

/* NOTE: we need different structs for:
 * 1. runtime-only options that don't need to be written to the DINO object:
 *    - memlimit, write_size, write_check, ...
 * 2. options that we need in the object for decompression:
 *    - dictsize, dictdata, ...
 * 3. options that we might want in the object for updates & reproduceability:
 *    - level, rsyncable
 */

/* A struct to hold advanced compression/decompression options. */
typedef struct Dino_COpts {
    int level;          /* simple compression level */
    size_t dictsize;    /* dictionary data size */
    uint8_t *dictdata;  /* dictionary data */
    void *params;       /* other algo-specific parameter data */
} Dino_COpts;
typedef struct Dino_COpts Dino_DOpts;

/* Forward declarations for [CD]Stream; needed here because we've got a
 * reference loop where CStream contains a (*CCFuncs) and the function
 * signatures in CCFuncs refer to CStream. */
typedef struct Dino_CStream Dino_CStream;
typedef struct Dino_DStream Dino_DStream;

/* Function declarations for the 'generic' cstream/dstream interface.
 * These are the functions that we should be using in the rest of the
 * library; none of the internal machinery is exposed, which is
 * nice.*/


/* Buffer management functions */
Buf *buf_init(size_t size);
#define inbuf_init(size) ((inBuf *)buf_init(size))
#define outbuf_init(size) ((outBuf *)buf_init(size))
size_t buf_realloc(Buf *buf, size_t size);
void buf_clear(Buf *buf);
void buf_free(Buf *buf);
#define inbuf_free(buf) buf_free((Buf *)buf)
#define outbuf_free(buf) buf_free((Buf *)buf)

/* Stream management */
Dino_CStream *cstream_create(Dino_CompressID id);
int cstream_setlevel(Dino_CStream *cstream, int level);
int cstream_setopts(Dino_CStream *cstream, Dino_COpts *copts);
void cstream_free(Dino_CStream *cstream);

Dino_DStream *dstream_create(Dino_CompressID id);
int dstream_setopts(Dino_DStream *dstream, Dino_DOpts *dopts);
void dstream_free(Dino_DStream *dstream);

/* And here's the actual generic compress/decompress API! */

/* cstream_compress_start: give a hint to the encoder that a new stream
 * is starting. size is the total size of expected input data.
 * returns a suggested size for the first read() operation. */
size_t cstream_compress_start(Dino_CStream *cstream, size_t size);

/* cstream_compress: compress inbuf to outbuf.
 * updates the `pos` fields of both buffers.
 * returns a suggested size for the next read() operation. */
size_t cstream_compress(Dino_CStream *cstream, inBuf *in, outBuf *out);

/* cstream_compress_end: flush internal buffers, finalize stream.
 * returns 0 if the flush completed successfully.
 * if outbuf is full, returns a minimum number of bytes left to write. */
size_t cstream_compress_end(Dino_CStream *cstream, outBuf *out);

/* cstream_compress1: compress inbuf to outbuf in one operation, e.g.:
 *   cstream_compress_start(cstream, (in->size - in->pos));
 *   cstream_compress(cstream, in, out);
 *   return cstream_compress_end(cstream, out);
 * return value is same as cstream_compress_end(). */
size_t cstream_compress1(Dino_CStream *cstream, inBuf *in, outBuf *out);

/* dstream_decompress: decompress inbuf to outbuf.
 * updates the `pos` fields of both buffers.
 * As with cstream_compress, returns:
 *   0 if we finished decompressing this complete frame,
 *   an error code (TODO!!), or
 *   any other positive value, which is the suggested number of bytes to read.
 * (zstd says that the suggested size will never be larger than the rest of
 * the frame. Not sure about other decoders, though. */
size_t dstream_decompress(Dino_DStream *dstream, inBuf *in, outBuf *out);


/* Below here we start getting into the internals of how the generic
 * compression stuff works. Unless you're adding a new compression
 * algorithm or modifying the internals, you probably don't need to
 * worry about any of this. */

/* Opaque pointers for compression/decompression contexts. */
typedef void* Dino_CCtx;
typedef void* Dino_DCtx;

/* function interface for a compliant compression algorithm. */
typedef struct Dino_CCFuncs {
    Dino_CompressID id;
    Dino_CCtx (*create_ctx)(void);
    void (*free_ctx)(Dino_CCtx);
    int (*setup)(Dino_CStream*, Dino_COpts*);
    size_t (*compress)(Dino_CStream*, inBuf*, outBuf*);
    size_t (*flush)(Dino_CStream*, outBuf *);
    size_t (*end)(Dino_CStream*, outBuf *);
} Dino_CCFuncs;

/* function interface for a compliant decompression algorithm. */
typedef struct Dino_DCFuncs {
    Dino_CompressID id;
    Dino_DCtx (*create_ctx)(void);
    void (*free_ctx)(Dino_DCtx);
    int (*setup)(Dino_DStream*, Dino_DOpts*);
    size_t (*decompress)(Dino_DStream*, inBuf*, outBuf*);
} Dino_DCFuncs;

/* getters for looking up [CD]CFuncs by id */
const Dino_CCFuncs *_get_ccfuncs(Dino_CompressID id);
const Dino_DCFuncs *_get_dcfuncs(Dino_CompressID id);

/* The actual internals of the CStream/DStream structs. */
typedef struct Dino_CStream {
    size_t rec_inbuf_size;
    size_t rec_outbuf_size;
    Dino_CCtx cctx;
    const Dino_CCFuncs *funcs;
} Dino_CStream;

typedef struct Dino_DStream {
    size_t rec_inbuf_size;
    size_t rec_outbuf_size;
    Dino_DCtx *dctx;
    const Dino_DCFuncs *funcs;
} Dino_DStream;

#endif /* _COMPRESSION_H */
