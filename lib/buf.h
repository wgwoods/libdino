/* Header for our simple sized buffer interface.
 * This gets used in the compression subsys and elsewhere. */
#ifndef _BUF_H
#define _BUF_H 1
#include "memory.h"

/* A buffer containing some amount of data.
 * size is the buffer's current maximum (i.e. allocated) size.
 * pos is used by callers to tell functions where to start read/writing data,
 * and is usually adjusted by the called function to indicated how much data
 * was read or written. */
typedef struct Buf {
    void *buf;
    size_t size;
    size_t pos;
} Buf;

/* outBuf is an explicitly writable buf. */
typedef struct Buf outBuf;

/* inBuf is a read-only Buf; functions won't modify its contents. */
typedef struct inBuf {
    const void *buf;
    size_t size;
    size_t pos;
} inBuf;

/* Function declarations */
Buf *buf_init(size_t size);
#define inbuf_init(size) ((inBuf *)buf_init(size))
#define outbuf_init(size) ((outBuf *)buf_init(size))
size_t buf_realloc(Buf *buf, size_t size);
void buf_clear(Buf *buf);
void buf_free(Buf *buf);
#define inbuf_free(buf) buf_free((Buf *)buf)
#define outbuf_free(buf) buf_free((Buf *)buf)

#define buf_clear(b)    (b->pos = 0)

/* Slices are read-only references to parts of a Buf */
typedef inBuf Slice;
Slice *slice_buf(Buf *buf, size_t pos, size_t len);
void slice_free(Slice *s);

#endif /* _BUF_H */
