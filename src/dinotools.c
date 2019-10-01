/* dinotools.c - utility data/functions common to all DINO tools */

#include <stdlib.h>
#include "dinotools.h"

const char *objtype_name[DINO_OBJTYPENUM+1] = {
    [DINO_TYPE_INVALID]     = "Invalid",
    [DINO_TYPE_ARCHIVE]     = "Archive",
    [DINO_TYPE_SYSIMG]      = "Sysimg",
    [DINO_TYPE_COMPONENT]   = "Component",
    [DINO_TYPE_DYNIMG]      = "Dynimg",
    [DINO_TYPE_APPLICATION] = "Application",
    [DINO_TYPE_DUMP]        = "Dump",
    [DINO_OBJTYPENUM]       = "Unknown",
};

const char *compressid_name[DINO_COMPRESSNUM+1] = {
    [DINO_COMPRESS_NONE] = "none",
    [DINO_COMPRESS_ZLIB] = "zlib",
    [DINO_COMPRESS_LZMA] = "lzma",
    [DINO_COMPRESS_LZO]  = "lzo",
    [DINO_COMPRESS_XZ]   = "xz",
    [DINO_COMPRESS_LZ4]  = "lz4",
    [DINO_COMPRESS_ZSTD] = "zstd",
    [DINO_COMPRESSNUM]   = "unknown",
};

/* Some hex handling helpers */

static char hexchr[16] = "0123456789abcdef";
static signed char hexval[256] = {
/* 0x00 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x10 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x20 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x30 */  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
/* 0x40 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x50 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x60 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x70 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x80 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0x90 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0xa0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0xb0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0xc0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0xd0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0xe0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 0xf0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

inline int hexchrval(const char ch) {
    return hexval[(unsigned char)ch];
}

int canonicalize_hexstr(char *hex, unsigned *len) {
    signed char v;
    char *h = hex;
    while (*h) {
        v = hexval[(unsigned char)*h];
        if (v < 0) return 0;
        *h++ = hexchr[v];
    }
    *len = h - hex;
    return 1;
}

int key2hex(const Dino_Idx_Key *key, const size_t keysize, char *hex) {
    for (int i=0; i<keysize; i++) {
        hex[i<<1]     = hexchr[key[i] >> 4];
        hex[(i<<1)+1] = hexchr[key[i] & 0x0f];
    }
    hex[keysize<<1] = '\0';
    return keysize<<1;
}

char *key2hex_a(const Dino_Idx_Key *key, const size_t keysize) {
    if (key == NULL || keysize == 0)
        return NULL;
    char *out = malloc((keysize<<1)+1);
    if (out == NULL)
        return NULL;
    key2hex(key, keysize, out);
    return out;
}

int hex2key(const char *hex, const size_t hexsize, Dino_Idx_Key *key) {
    size_t keylen = (hexsize >> 1);
    for (int i=0; i<keylen; i++)
        key[i] = (hexval[(unsigned char)hex[i<<1]] << 4) | hexval[(unsigned char)hex[(i<<1)+1]];
    if (hexsize & 1)
        key[keylen++] = hexval[(unsigned char)hex[hexsize-1]] << 4;
    return keylen;
}

Dino_Idx_Key *hex2key_a(const char *hex, const size_t hexsize) {
    if (!(hex && *hex && hexsize))
        return NULL;
    Dino_Idx_Key *out = malloc((hexsize+1)>>1);
    if (!out)
        return NULL;
    hex2key(hex, hexsize, out);
    return out;
}
