/* buf.c - Simple buffer API */

#include "common.h" /* MIN, MAX */
#include "fileio.h" /* read, write, etc. */
#include "buf.h"

Buf *buf_init(size_t size) {
    Buf *buf = calloc(1, sizeof(Buf));
    if (buf && !buf_realloc(buf, size))
        return NULL;
    return buf;
}

size_t buf_realloc(Buf *buf, size_t size) {
    void *newbuf = realloc(buf->buf, size);
    if (!newbuf)
        return 0;
    buf->buf = newbuf;
    buf->size = size;
    buf->pos = MIN(buf->pos, buf->size);
    return size;
}

void buf_free(Buf *buf) {
    free(buf->buf);
    free(buf);
}

Slice *slice_buf(Buf *b, size_t pos, size_t len) {
    Slice *s = calloc(1, sizeof(Slice));
    s->buf = b->buf + pos;
    s->size = len;
    return s;
}

void slice_free(Slice *s) {
    free(s);
}
