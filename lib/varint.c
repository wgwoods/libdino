/* Variable-length integer encoding/decoding.
 *
 * dino_encode_varint and dino_decode_varint adapted from libgit2:
 *   https://github.com/libgit2/libgit2/blob/v0.28.3/src/varint.c
 */

#include "common.h"
#include "memory.h"
#include "varint.h"

int dino_encode_varint(uint8_t *dst, size_t dstsize, uintmax_t val) {
    uint8_t varint[16];
    unsigned pos = sizeof(varint)-1;
    varint[pos] = val & 127;
    while (val >>= 7)
        varint[--pos] = 128 | (--val & 127);
    int len = sizeof(varint)-pos;
    if (dst) {
        if (dstsize < len)
            return -1;
        memcpy(dst, varint+pos, len);
    }
    return len;
}

uintmax_t dino_decode_varint(const uint8_t *varint, size_t *len) {
    const uint8_t *bptr = varint;
    uint8_t b = *bptr++;
    uintmax_t val = b & 127;
    while (b & 128) {
        val += 1;
        if (!val || MSB(val, 7)) {
            /* If val is 0 we just overflowed; if there's anything in the top
             * 7 bits of val, we're about to overflow. */
            if (!len)
                return VARINT_MAXVAL;
            *len = 0;
            return 0;
        }
        b = *bptr++;
        val = (val << 7) + (b & 127);
    }
    if (len)
        *len = bptr - varint;
    return val;
}
