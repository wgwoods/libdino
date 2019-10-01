#include "buf.h"
#include "fileio.h"

ssize_t buf_fill(int fd, Buf *b) {
    ssize_t nr = read_retry(fd, b->buf+b->pos, b->size-b->pos);
    if (nr >= 0)
        b->pos += nr;
    return nr;
}

ssize_t buf_refill(int fd, Buf *b) {
    ssize_t nr = read_retry(fd, b->buf, b->size);
    if (nr >= 0)
        b->pos = nr;
    return nr;
}
