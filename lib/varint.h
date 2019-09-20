#ifndef _VARINT_H
#define _VARINT_H 1

#include <stdint.h>

/* Encode val into dst, with a maximum length of dstsize.
 * Returns the length of the encoded value, or -1 if the encoded value length
 * is greater than dstsize.
 * If dst is NULL, no data is copied, but you still get the length. */
int dino_encode_varint(uint8_t *dst, size_t dstsize, uintmax_t val);
#define dino_varint_length(val) dino_encode_varint(NULL, 0, val)

/* Decode and return a varint value.
 * Sets len (if non-NULL) to the length of the encoded value.
 * If an overflow occurs, returns 0 and sets len to 0.
 * If len is NULL, returns VARINT_MAXVAL on overflow. */
uintmax_t dino_decode_varint(const uint8_t *varint, size_t *len);

#define VARINT_MAXVAL UINTMAX_MAX
/* 10, assuming uintmax_t == uint64_t */
#define VARINT_MAXLEN (((sizeof(uintmax_t)*8)/7)+1)


/* For reference / efficiency checks here's the largest values you can fit in
 * a varint of a given byte width */
#define VARINT_MAXVAL_1 (              0x80-1) /* 127                 */
#define VARINT_MAXVAL_2 (            0x4080-1) /* 16511               */
#define VARINT_MAXVAL_3 (          0x204080-1) /* 2113663             */
#define VARINT_MAXVAL_4 (        0x10204080-1) /* 270549119           */
#define VARINT_MAXVAL_5 (      0x0810204080-1) /* 34630287487         */
#define VARINT_MAXVAL_6 (    0x040810204080-1) /* 4432676798591       */
#define VARINT_MAXVAL_7 (  0x02040810204080-1) /* 567382630219903     */
#define VARINT_MAXVAL_8 (0x0102040810204080-1) /* 72624976668147839   */
#define VARINT_MAXVAL_9 (0x8102040810204080-1) /* 9295997013522923647 */
#define VARINT_MAXVAL_WIDTH(x) VARINT_MAXVAL_##x



#endif /* _VARINT_H */
